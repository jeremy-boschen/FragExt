/* FragExt - Shell extension for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2009 Jeremy Boschen. All rights reserved.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software. 
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in
 * a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 * be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 */

/* AtlEx.h
 *    Miscellaneous ATL extentions
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include <comdef.h>

#include <SeUtil.h>

namespace ATLEx
{

/**********************************************************************

    

 **********************************************************************/

inline HRESULT CoStatusFromWin32( DWORD dwError ) throw()
{
   return ( __HRESULT_FROM_WIN32(dwError) );
}

inline HRESULT CoFailureFromWin32( DWORD dwError, DWORD dwFailure ) throw()
{
   return ( NO_ERROR != dwError ? __HRESULT_FROM_WIN32(dwError) : __HRESULT_FROM_WIN32(dwFailure) );
}

inline BOOL IsCmdLineSetting( LPCTSTR pszCmdLine, LPCTSTR pszCmd ) throw()
{
   TCHAR   chToken[] = {_T("-/")};
   LPCTSTR pszToken  = _pModule->FindOneOf(pszCmdLine, chToken);

   while ( pszToken )
   {
      if ( 0 == _pModule->WordCmpI(pszToken, pszCmd) )
      {
         return ( TRUE );
      }

      pszToken = _pModule->FindOneOf(pszToken, chToken);
   }

   return ( FALSE );
}   

template < class T > 
inline HRESULT CreateObjectInstance( REFIID riid, void** ppvObj, CComObject<T>** ppT = NULL ) throw()
{
   HRESULT        hr = E_FAIL;
   CComObject<T>* pT = NULL;

   if ( ppT )
   {
      (*ppT) = NULL;
   }

   __try
   {
      hr = CComObject<T>::CreateInstance(&pT);
      if ( SUCCEEDED(hr) )
      {
         pT->AddRef();
         hr = pT->QueryInterface(riid, ppvObj);
         if ( ppT )
         {
            (*ppT) = pT;
            pT->AddRef();
         }
      }
   }
   __finally
   {
      if ( pT )
      {
         pT->Release();
      }
   }

   return ( hr );
}

template < class T > 
class ATL_NO_VTABLE CoFreeThreadedMarshalerImpl
{
public:
   typedef CoFreeThreadedMarshalerImpl<T> CoFTMImpl;

   virtual ~CoFreeThreadedMarshalerImpl( ) throw()
   {
      _spFTM = NULL;
   }

   /* Add the following entries to T's COM_MAP 
         COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, CoFTMImpl::QueryMarshalInterface)
    */
   static HRESULT WINAPI QueryMarshalInterface( void* pv, REFIID riid, LPVOID* ppv, DWORD /*dw*/ )
   {
      if ( !InlineIsEqualGUID(riid, IID_IMarshal) )
      {
         return ( E_NOINTERFACE );
      }

      HRESULT   hr   = E_NOINTERFACE;
      T*        pT   = (T*)pv;
      /* Use DECLARE_GET_CONTROLLING_UNKNOWN() if necessary */
      IUnknown* pUnk = pT->GetControllingUnknown();

      pT->Lock();

      __try
      {
         if ( !pT->_spFTM )
         {
            hr = CoCreateFreeThreadedMarshaler(pUnk, &pT->_spFTM);
            if ( FAILED(hr) )
            {
               /* Failure */
               __leave;
            }
         }
         
         ATLASSERT(NULL != pT->_spFTM.p);
         hr = pT->_spFTM->QueryInterface(riid, ppv);
      }
      __finally
      {
         pT->Unlock();
      }

      return ( hr );
   }

   CComPtr<IUnknown> _spFTM;
};

template < class Base, const IID* piid, class T, class Copy, class ThreadModel = CComObjectThreadModel >
class ATL_NO_VTABLE CComEnumOnFTM : public CComObjectRootEx<ThreadModel>,
                                    public CComEnumImpl<Base, piid, T, Copy>,
                                    public CoFreeThreadedMarshalerImpl<CComEnumOnFTM<Base, piid, T, Copy, ThreadModel> >
{
public:
   typedef CComEnumOnFTM<Base, piid, T, Copy, ThreadModel> _EnumFTM;
   typedef CComEnumImpl<Base, piid, T, Copy>               _EnumImpl;

   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(_EnumFTM)
	   COM_INTERFACE_ENTRY_IID(*piid, _EnumImpl)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, CoFTMImpl::QueryMarshalInterface)
   END_COM_MAP()
};

template < class T, const IID* piid, class CDV >
class IConnectionPointRefImpl : public IConnectionPointImpl<T, piid, CDV>
{
public:
   ~IConnectionPointRefImpl( ) throw()
   {
	   IUnknown** ppUnk = m_vec.begin();

	   while ( ppUnk < m_vec.end() )
	   {
         if ( (*ppUnk) )
         {
            (*ppUnk)->Release();
            (*ppUnk) = NULL;
         }

         ppUnk++;
	   }
   }
};

template < class CDV >
class CComUnkRefArray
{
public:
   CDV* _pvec;

   CComUnkRefArray( CDV* pvec = NULL ) throw() : _pvec(pvec)
   {
   }

   void Initialize( CDV* pvec )
   {
      ATLASSERT(!_pvec);
      _pvec = pvec;
   }

   DWORD Add( IUnknown* pUnk ) 
   {
      ATLASSERT(NULL != _pvec);
      return ( _pvec->Add(pUnk) );
   }

   BOOL Remove( DWORD dwCookie )
   {
      ATLASSERT(NULL != _pvec);
      return ( _pvec->Remove(dwCookie) );
   }

   DWORD WINAPI GetCookie( IUnknown** ppFind )
   {
      ATLASSERT(NULL != _pvec);
      return ( _pvec->GetCookie(ppFind) );
   }

   IUnknown* WINAPI GetUnknown( DWORD dwCookie )
   {
      ATLASSERT(NULL != _pvec);
      return ( _pvec->GetUnknown(dwCookie) );
   }

   IUnknown** begin( )
   {
      ATLASSERT(NULL != _pvec);
      return ( _pvec->begin() );
   }

   IUnknown** end( )
   {
      ATLASSERT(NULL != _pvec);
      return ( _pvec->end() );
   }
};

#ifdef __ATLWIN_H__

#define MENUCOMMAND_HANDLER(id, func) \
	if ( (uMsg == WM_MENUCOMMAND) && (id == LOWORD(wParam)) ) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam & 0xFFFFFFFF), LOWORD(wParam), (HMENU)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define MENUCOMMAND_RANGE_HANDLER(idFirst, idLast, func) \
	if ( (uMsg == WM_MENUCOMMAND) && ((LOWORD(wParam) >= idFirst) && (LOWORD(wParam) <= idLast)) ) \
	{ \
		bHandled = TRUE; \
		lResult = func(HIWORD(wParam & 0xFFFFFFFF), LOWORD(wParam), (HMENU)lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

/**********************************************************************

   CCustomDialogImplBaseT / CCustomDialogImpl    

 **********************************************************************/

#define DECLARE_DLG_CLASSNAME( sz ) \
   static LPCTSTR GetWndClassName() throw() \
   { \
      return ( sz ); \
   }

template < class T, class TBase = ATL::CWindow >
class ATL_NO_VTABLE CCustomDialogImplBaseT : public ATL::CWindowImplBaseT<TBase, ATL::CNullTraits>
{
public:
   CCustomDialogImplBaseT( ) throw()
   {
      m_pfnSuperWindowProc = (WNDPROC)::DefDlgProc;
   }

   HINSTANCE GetDialogInstance( ) throw()
   {
      return ( _AtlBaseModule.GetResourceInstance() );
   }

   LPCDLGTEMPLATE LoadDialogTemplate( HINSTANCE hInstance, LPCTSTR pszName ) throw()
   {
      HRSRC hResInfo = ::FindResource(hInstance, pszName, RT_DIALOG);

      if ( hResInfo )
      {
         HGLOBAL hResData = ::LoadResource(hInstance, hResInfo);

         if ( hResData )
         {
            return ( (LPCDLGTEMPLATE)::LockResource(hResData) );
         }
      }

      return ( NULL );
   }

   virtual ATOM RegisterDialog( ) throw()
   {
      ATL::CWndClassInfo& wci = T::GetWndClassInfo();

      if ( 0 != wci.m_atom )
      {
         return ( wci.m_atom );
      }

      if ( NULL == wci.m_wc.lpszClassName )
      {
         wci.m_wc.lpszClassName = T::GetWndClassName();
      }

      wci.m_wc.cbWndExtra += DLGWINDOWEXTRA;
      wci.m_wc.lpfnWndProc = T::StartWindowProc;

      return ( wci.Register(&m_pfnSuperWindowProc) );
   }

   BOOL MapDialogRect( LPRECT pRect )
   {
      ATLASSERT(::IsWindow(m_hWnd));
      return ( ::MapDialogRect(m_hWnd, lpRect) );
   }

   BOOL ExecuteDlgInit( int iDlgID )
   {
      return ( ATL::CDialogImplBaseT<TBase>::ExecuteDlgInit(iDlgID) );
   }

   static INT_PTR CALLBACK DefDialogProc( HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/ )
   {
      return ( (INT_PTR)FALSE );
   }
};

template < class T, class TBase = CWindow >
class ATL_NO_VTABLE CCustomDialogImpl : public CCustomDialogImplBaseT<T, TBase>
{
public:
#ifdef _DEBUG
   bool _fModal;

   CCustomDialogImpl( ) throw() : _fModal(false)
   {
   }
#endif

   DECLARE_WND_CLASS_EX(NULL, CS_DBLCLKS|CS_SAVEBITS|CS_BYTEALIGNWINDOW, COLOR_3DFACE);

   INT_PTR DoModal( HWND hWndParent = ::GetActiveWindow(), LPARAM dwInitParam = NULL )
   {
      T* pT = static_cast<T*>(this);

      ATOM atom = pT->RegisterDialog();

      if ( NULL == atom )
      {
         return ( 0 );
      }
      
      LPCDLGTEMPLATE pTemplate = pT->LoadDialogTemplate(pT->GetDialogInstance(), MAKEINTRESOURCE(pT->IDD));

      if ( !pTemplate )
      {
         return ( NULL );
      }

   #ifdef _DEBUG
      _fModal = true;
   #endif
      
      _AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);

      return ( ::DialogBoxIndirectParam(_AtlBaseModule.GetModuleInstance(), pTemplate, hWndParent, T::DefDialogProc, dwInitParam) );
   }

   BOOL EndDialog( int nRetCode )
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(_fModal);

      return ( ::EndDialog(m_hWnd, nRetCode) );
   }

   HWND Create( HWND hWndParent, LPARAM dwInitParam = NULL )
   {
      ATLASSERT(NULL == m_hWnd);
      
      ATOM atom = RegisterDialog();

      if ( NULL == atom )
      {
         return ( NULL );
      }

      T* pT = static_cast<T*>(this);

		LPCDLGTEMPLATE pTemplate = LoadDialogTemplate(pT->GetDialogInstance(), MAKEINTRESOURCE(pT->IDD));

      if ( !pTemplate )
      {
         return ( NULL );
      }

      _AtlWinModule.AddCreateWndData(&m_thunk.cd, (CDialogImplBaseT< TBase >*)this);

   #ifdef _DEBUG
	   _fModal = false;
   #endif

      HWND hWnd = ::CreateDialogIndirectParam(_AtlBaseModule.GetModuleInstance(), pTemplate, hWndParent, T::DefDialogProc, dwInitParam);      
      ATLASSERT(m_hWnd == hWnd);
      return ( hWnd );
   }

   HWND Create( HWND hWndParent, RECT&, LPARAM dwInitParam = NULL )
   {
      return ( Create(hWndParent, dwInitParam) );
   }

   BOOL DestroyWindow( )
   {
      ATLASSERT(::IsWindow(m_hWnd));
      ATLASSERT(!_fModal);

      return ( ::DestroyWindow(m_hWnd) );
   }

   HWND Create( HWND hWndParent, _U_RECT rect, LPCTSTR szWindowName, DWORD dwStyle, DWORD dwExStyle, _U_MENUorID MenuOrID, ATOM atom, LPVOID lpCreateParam = NULL )
   {
      ATLASSERT(FALSE);
      return ( NULL );
   }
};

/* Like WTL::CMenuT<>::_FixTrackMenuPopupX, but dynamically links to MonitorFromPointW & GetMonitorInfoW */
static int __stdcall FixTrackMenuPopupX( int x, int y )
{
   typedef HMONITOR (WINAPI* PMONITORFROMPOINT)(POINT, DWORD);
   typedef BOOL (WINAPI* PGETMONITORINFO)(HMONITOR, LPMONITORINFO);

   static PMONITORFROMPOINT pMonitorFromPoint = NULL;
   static PGETMONITORINFO pGetMonitorInfo = NULL;

   if ( !pMonitorFromPoint )
   {
      InterlockedExchangePointer((PVOID*)&pMonitorFromPoint, ::GetProcAddress(::GetModuleHandle(_T("USER32.DLL")), "MonitorFromPoint"));
   }

   if ( !pGetMonitorInfo )
   {
      InterlockedExchangePointer((PVOID*)&pGetMonitorInfo, ::GetProcAddress(::GetModuleHandle(_T("USER32.DLL")), "GetMonitorInfoW"));
   }

   if ( pMonitorFromPoint && pGetMonitorInfo )
   {
		POINT    pt       = {x, y};
		HMONITOR hMonitor = (*pMonitorFromPoint)(pt, MONITOR_DEFAULTTONULL);

		if( !hMonitor )
		{
			HMONITOR hMonitorNear = (*pMonitorFromPoint)(pt, MONITOR_DEFAULTTONEAREST);
			
         if ( hMonitorNear )
			{
				MONITORINFO mi = {sizeof(MONITORINFO), 0};

				if ( FALSE != (*pGetMonitorInfo)(hMonitorNear, &mi) )
				{
					if( x < mi.rcWork.left )
               {
                  x = mi.rcWork.left;
               }
					else if ( x > mi.rcWork.right )
               {
                  x = mi.rcWork.right;
               }
				}
			}
		}
   }

   return ( x );
}
#endif /* __ATLWIN_H__ */

template <class T>
class ATL_NO_VTABLE CAtlCoExeModuleT : public ATL::CAtlExeModuleT<T>
{
public:
   typedef ATL::CAtlExeModuleT<T> _AtlExeModuleT;
#if _ATL_VER < 10
   LONG Lock( ) throw()
   {
       CoAddRefServerProcess();
       return ( _AtlExeModuleT::Lock() );
   }

   LONG Unlock( ) throw()
   {
      CoReleaseServerProcess();
      return ( _AtlExeModuleT::Unlock() );
   }
#endif /* _ATL_VER < 0x0A00 */
};

template <class T, UINT nServiceNameID, UINT nDisplayNameID, UINT nDescriptionID >
class ATL_NO_VTABLE CAtlServiceModuleExT : public ATL::CAtlServiceModuleT<T, nServiceNameID>
{
   /* ATL */
public:
   typedef CAtlServiceModuleExT<T, nServiceNameID, nDisplayNameID, nDescriptionID> _AtlServiceModuleExT;
   typedef ATL::CAtlServiceModuleT<T, nServiceNameID> _AtlServiceModuleT;
   typedef ATL::CAtlExeModuleT<T> _AtlExeModuleT;
   
   CAtlServiceModuleExT( ) throw() : ATL::CAtlServiceModuleT<T, nServiceNameID>()
   {
   }

   HRESULT RegisterAppId( bool bService = false ) throw()
   {
      HRESULT hr;
      T*      pT;

      pT = static_cast<T*>(this);
      hr = _AtlServiceModuleT::RegisterAppId(bService);
      if ( FAILED(hr) )
      {
         /* Failure */
         return ( hr );
      }

      if ( bService )
      {
         hr = pT->InstallExtra();
      }

      /* Success / Failure */
      return ( hr );
   }

  	HRESULT InstallExtra( ) throw()
	{
      HRESULT              hr;
      BOOL                 bRet;
      int                  iRet;
      TCHAR                chDisplayName[256];
      TCHAR                chDescription[512];      
      SERVICE_DESCRIPTION  ServiceDesc;
      SC_HANDLE            hSCManager;
      SC_HANDLE            hService;

      hSCManager = ::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
      if ( !hSCManager )
      {
         hr = AtlHresultFromWin32(GetLastError());
         /* Failure */
         return ( hr );
      }
      
		hService = ::OpenService(hSCManager, m_szServiceName, SERVICE_ALL_ACCESS);
      if ( !hService )
      {
         hr = AtlHresultFromWin32(GetLastError());
         goto __CLEANUP;
      }

      iRet = LoadString(_AtlBaseModule.GetResourceInstance(), nDisplayNameID, chDisplayName, _countof(chDisplayName));
      ATLASSERT((iRet > 0) && (iRet < _countof(chDisplayName)));
      
      bRet = ::ChangeServiceConfig(hService,
                                   SERVICE_NO_CHANGE,
                                   SERVICE_NO_CHANGE,
                                   SERVICE_NO_CHANGE,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   chDisplayName);

      if ( !bRet )
      {
         hr = AtlHresultFromWin32(GetLastError());
         goto __CLEANUP;
      }

      iRet = LoadString(_AtlBaseModule.GetResourceInstance(), nDescriptionID, chDescription, _countof(chDescription));
      ATLASSERT((iRet > 0) && (iRet < _countof(chDescription)));
      ServiceDesc.lpDescription = chDescription;
      bRet = ::ChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &ServiceDesc);
      ATLASSERT(bRet);

      /* Success */
      hr = S_OK;
__CLEANUP:
      if ( hService )
      {
         ::CloseServiceHandle(hService);
      }

		::CloseServiceHandle(hSCManager);

      /* Success / Failure */
		return ( hr );
	}

   void GetServiceStatusEx( DWORD* pdwCurrentState, DWORD* pdwCheckPoint, DWORD* pdwWaitHint, DWORD* pdwControlsAccepted, DWORD* pdwWin32ExitCode, DWORD* pdwServiceSpecificExitCode ) throw()
   {
      if ( pdwCurrentState )
      {
         (*pdwCurrentState) = m_status.dwCurrentState;
      }

      if ( pdwCheckPoint )
      {
         (*pdwCheckPoint) = m_status.dwCheckPoint;
      }

      if ( pdwWaitHint )
      {
         (*pdwWaitHint) = m_status.dwWaitHint;
      }

      if ( pdwControlsAccepted )
      {
         (*pdwControlsAccepted) = m_status.dwControlsAccepted;
      }

      if ( pdwWin32ExitCode )
      {
         (*pdwWin32ExitCode) = m_status.dwWin32ExitCode;
      }

      if ( pdwServiceSpecificExitCode )
      {
         (*pdwServiceSpecificExitCode) = m_status.dwServiceSpecificExitCode;
      }
   }

   void SetServiceStatusEx( DWORD dwCurrentState, DWORD dwCheckPoint = SERVICE_NO_CHANGE, DWORD dwWaitHint = SERVICE_NO_CHANGE, DWORD dwControlsAccepted = SERVICE_NO_CHANGE, DWORD dwWin32ExitCode = NO_ERROR, DWORD dwServiceSpecificExitCode = 0 ) throw()
   {
      BOOL bRet;

      if ( SERVICE_STOPPED != dwCurrentState )
      {
         dwWin32ExitCode           = NO_ERROR;
         dwServiceSpecificExitCode = 0;
      }

      if ( (SERVICE_STOPPED == dwCurrentState) || (SERVICE_PAUSED == dwCurrentState) || (SERVICE_RUNNING == dwCurrentState) )
      {
         dwWaitHint   = 0;
         dwCheckPoint = 0;
      }
#ifdef _DEBUG
      else
      {
         ATLASSERT((0 != dwWaitHint) && (0 != dwCheckPoint));
      }
#endif /* _DEBUG */

      Lock();
      {
         m_status.dwCurrentState = dwCurrentState;

         if ( SERVICE_NO_CHANGE != dwCheckPoint )
         {
            m_status.dwCheckPoint = dwCheckPoint;
         }

         if ( SERVICE_NO_CHANGE != dwWaitHint )
         {
            m_status.dwWaitHint = dwWaitHint;
         }

         if ( SERVICE_NO_CHANGE != dwControlsAccepted )
         {
            m_status.dwControlsAccepted = dwControlsAccepted;
         }

         m_status.dwWin32ExitCode = dwWin32ExitCode;
         m_status.dwServiceSpecificExitCode = dwServiceSpecificExitCode;

         bRet = ::SetServiceStatus(m_hServiceStatus, &m_status);
      }
      Unlock();

      if ( !bRet )
      {
         LogEventEx(static_cast<DWORD>(GetLastError()), NULL, EVENTLOG_ERROR_TYPE);
      }
   }

   void ServiceMain( DWORD /*dwArgc*/, LPTSTR* /*lpszArgv*/ ) throw()
	{
      T*      pT;
		HRESULT hr;

		m_status.dwCurrentState = SERVICE_START_PENDING;
      m_hServiceStatus = RegisterServiceCtrlHandlerEx(m_szServiceName, _HandlerEx, NULL);
	   if ( !m_hServiceStatus )
	   {
         LogEventEx(GetLastError(), _T("Failed to register control handler"), EVENTLOG_ERROR_TYPE);
         /* Failure */
		   return;
	   }

		SetServiceStatusEx(SERVICE_START_PENDING, 1, 1000);

		pT = static_cast<T*>(this);
#ifndef _ATL_NO_COM_SUPPORT
		hr = T::InitializeCom();
		if ( FAILED(hr) )
		{         
			if ( (RPC_E_CHANGED_MODE != hr) || (NULL == GetModuleHandle(_T("Mscoree.dll"))) )
			{
            LogEventEx(hr, _T("Failed to intialize COM"), EVENTLOG_ERROR_TYPE);
            /* Failure */
				return;
			}
		}
		else
		{
			m_bComInitialized = true;
		}

		m_bDelayShutdown = false;
#ifdef _ATL_FREE_THREADED
      ATLASSERT(!m_hEventShutdown);
      m_hEventShutdown = ::CreateEvent(NULL, FALSE, FALSE, NULL);
      if ( !m_hEventShutdown )
      {
         LogEventEx(GetLastError(), _T("Failed to initialize service"), EVENTLOG_ERROR_TYPE);
         /* Failure */
         return;
      }
#endif /* _ATL_FREE_THREADED */

#endif /* _ATL_NO_COM_SUPPORT */

		m_status.dwWin32ExitCode = pT->Run(SW_HIDE);

#ifndef _ATL_NO_COM_SUPPORT
		if ( m_bService && m_bComInitialized )
      {
         T::UninitializeCom();
      }

#ifdef _ATL_FREE_THREADED
      ::CloseHandle(m_hEventShutdown);		
#endif /* _ATL_FREE_THREADED */
#endif /* _ATL_NO_COM_SUPPORT */

      SetServiceStatusEx(SERVICE_STOPPED);
	}

   HRESULT PreMessageLoop( int nShowCmd ) throw()
   {
      return ( _AtlServiceModuleT::PreMessageLoop(nShowCmd) );
   }

   void RunMessageLoop( ) throw()
   {
#ifdef _ATL_FREE_THREADED
      ATLASSERT(NULL != m_hEventShutdown);
      WaitForSingleObject(m_hEventShutdown, INFINITE);
#else /* _ATL_FREE_THREADED */
      _AtlServiceModuleT::RunMessageLoop();
#endif /* _ATL_FREE_THREADED */
   }

   DWORD HandlerEx( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext ) throw()
	{
		DWORD dwRet = NO_ERROR;
      T*    pT    = static_cast<T*>(this);
      
		switch ( dwControl )
		{
		   case SERVICE_CONTROL_STOP:
			   dwRet = pT->OnStopEx(lpContext);
			   break;
		   case SERVICE_CONTROL_PAUSE:
			   dwRet = pT->OnPauseEx(lpContext);
			   break;
		   case SERVICE_CONTROL_CONTINUE:
			   dwRet = pT->OnContinueEx(lpContext);
			   break;
		   case SERVICE_CONTROL_INTERROGATE:
			   dwRet = pT->OnInterrogateEx(lpContext);
			   break;
		   case SERVICE_CONTROL_SHUTDOWN:
			   dwRet = pT->OnShutdownEx(lpContext);
			   break;
         case SERVICE_CONTROL_PARAMCHANGE:
            dwRet = pT->OnParamChangeEx(lpContext);
            break;
         case SERVICE_CONTROL_DEVICEEVENT:
            dwRet = pT->OnDeviceEventEx(dwEventType, lpEventData, lpContext);
            break;
         case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
            dwRet = pT->OnHardwareProfileChangeEx(dwEventType, lpEventData, lpContext);
            break;
         case SERVICE_CONTROL_POWEREVENT:
            dwRet = pT->OnPowerEvent(dwEventType, lpEventData, lpContext);
            break;
         case SERVICE_CONTROL_SESSIONCHANGE:
            dwRet = pT->OnSessionChange(dwEventType, lpEventData, lpContext);
            break;
         case SERVICE_CONTROL_PRESHUTDOWN:
            dwRet = pT->OnPreShutdown(lpContext);
            break;
		   default:
			   dwRet = pT->OnUnknownRequestEx(dwControl, dwEventType, lpEventData, lpContext);
            break;
		}

      /* Success / Failure */
      return ( dwRet );
	}

   DWORD OnStopEx( LPVOID /*lpContext*/ ) throw()
   {
      SetServiceStatusEx(SERVICE_STOP_PENDING, 1, 5000);
#ifdef _ATL_FREE_THREADED
      SetEvent(m_hEventShutdown);
#else /* _ATL_FREE_THREADED */
      PostThreadMessage(m_dwThreadID, WM_QUIT, 0, 0);
#endif /* _ATL_FREE_THREADED */
      return ( NO_ERROR );
   }

   DWORD OnPauseEx( LPVOID /*lpContext*/ ) throw()
   {
      T* pT = static_cast<T*>(this);
      pT->OnPause();
      return ( NO_ERROR );
   }

   DWORD OnContinueEx( LPVOID /*lpContext*/ ) throw()
   {
      T* pT = static_cast<T*>(this);
      pT->OnContinue();
      return ( NO_ERROR );
   }

   DWORD OnInterrogateEx( LPVOID /*lpContext*/ ) throw()
   {
      T* pT = static_cast<T*>(this);
      pT->OnInterrogate();
      return ( NO_ERROR );
   }

   DWORD OnShutdownEx( LPVOID /*lpContext*/ ) throw()
   {
      T* pT = static_cast<T*>(this);
      pT->OnShutdown();
      return ( NO_ERROR );
   }

   DWORD OnParamChangeEx( LPVOID /*lpContext*/ ) throw()
   {
      return ( NO_ERROR );
   }

   DWORD OnDeviceEventEx( DWORD /*dwEventType*/, LPVOID /*lpEventData*/, LPVOID /*lpContext*/ ) throw()
   {
      return ( NO_ERROR );
   }

   DWORD OnHardwareProfileChangeEx( DWORD /*dwEventType*/, LPVOID /*lpEventData*/, LPVOID /*lpContext*/ ) throw()
   {
      return ( NO_ERROR );
   }

   DWORD OnPowerEvent( DWORD /*dwEventType*/, LPVOID /*lpEventData*/, LPVOID /*lpContext*/ ) throw()
   {
      return ( NO_ERROR );
   }

   DWORD OnSessionChange( DWORD /*dwEventType*/, LPVOID /*lpEventData*/, LPVOID /*lpContext*/ ) throw()
   {
      return ( NO_ERROR );
   }

   DWORD OnPreShutdown( LPVOID /*lpContext*/ ) throw()
   {
      return ( NO_ERROR );
   }

   DWORD OnUnknownRequestEx( DWORD dwControl, DWORD /*dwEventType*/, LPVOID /*lpEventData*/, LPVOID /*lpContext*/ )
   {
      T* pT = static_cast<T*>(this);
      pT->OnUnknownRequest(dwControl);
      return ( ERROR_CALL_NOT_IMPLEMENTED );
   }

protected:
   static DWORD WINAPI _HandlerEx( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext ) throw()
   {      
      return ( static_cast<T*>(_pAtlModule)->HandlerEx(dwControl, dwEventType, lpEventData, lpContext) );
   }
};

} /* namespace ATLEx */