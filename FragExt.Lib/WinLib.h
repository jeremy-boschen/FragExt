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

/* WinLib.h
 *    Windowing library
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#pragma once

#include "MemUtil.h"
#include "SpinLock.h"

#include <delayimp.h>

/*++!FOR LIBRARY USE ONLY!*/
PVOID
APIENTRY
__CreateWndProcThunk(
   __in PVOID ClassInstance,
   __in PVOID WindowProcedure
);

void
APIENTRY
__DestroyWndProcThunk(
   PVOID WndProc
);

extern DWORD __WindowInstanceTlsIndex;
/*--!FOR LIBRARY USE ONLY!*/

template < class T >
inline
WNDPROC
CreateWndProcThunk(
   __in T* ClassInstance,
   __in LRESULT (CALLBACK* WindowProcedure)(T*, HWND,UINT,WPARAM,LPARAM)
)
{
   PVOID pThunk;
   
   pThunk = __CreateWndProcThunk(ClassInstance,
                                 WindowProcedure);

   return ( reinterpret_cast<WNDPROC>(pThunk) );
}

inline
void
DestroyWndProcThunk(
   __in WNDPROC WndProc
)
{ 
   __DestroyWndProcThunk(WndProc);
}

/*
 */
template < class T >
class
DECLSPEC_NOVTABLE
DynamicWndProcT
{
public:
   DynamicWndProcT(
   ) throw()
   {
      _pStaticWndProcThunk = CreateWndProcThunk<T>(static_cast<T*>(this),
                                                   &T::WindowProcedure);
   }

   ~DynamicWndProcT(
   ) throw()
   {
      DestroyWndProcThunk(_pStaticWndProcThunk);
   }

   WNDPROC 
   GetWindowProcedure(
   ) const
   {
      return ( _pStaticWndProcThunk );
   }

   WNDPROC _pStaticWndProcThunk;
};

/* CStaticWndProcT
 *    WNDPROC thunk for classes that only ever have one instance running
 *    at a time
 */
template < class T >
class 
DECLSPEC_NOVTABLE 
StaticWndProcT
{
public:
   StaticWndProcT(
   ) throw()
   {
      _pT = static_cast<T*>(this);
   }

   WNDPROC 
   GetWindowProcedure(
   ) const
   {
      return ( &T::StaticWndProcThunk );
   }
      
   static
   LRESULT
   CALLBACK
   StaticWndProcThunk(
      __in HWND hWnd,
      __in UINT uMsg,
      __in WPARAM wParam,
      __in LPARAM lParam
   )
   {
      return ( T::WindowProcedure(_pT,
                                  hWnd,
                                  uMsg,
                                  wParam,
                                  lParam) );
   }
      
   static T* _pT;
};

template < class T > __declspec(selectany) T* StaticWndProcT<T>::_pT = NULL;

enum WindowInstanceType
{
   Single   = 0,
   Multiple = 1
};

template < class T, WindowInstanceType InstanceType >
class
DECLSPEC_NOVTABLE
WndProcThunkInstanceT
{
};

template <class T> 
class
WndProcThunkInstanceT<T, Single>
{
public:
   typedef StaticWndProcT<T> __WndProcThunk;
};

template <class T> 
class
WndProcThunkInstanceT<T, Multiple>
{
public:
   typedef DynamicWndProcT<T> __WndProcThunk;
};


/*
Window
   CWindowInstance
      CWindowMessageHandler
         CUserWindowClass - Caller defined class
 */

struct WNDCLASSINFO
{
   LPCWSTR  Name;
   HBRUSH   Background;
   UINT     Style;
   WORD     ClassAtom;
   SPINLOCK CreateLock;
      
   static
   BOOLEAN
   GenerateClassName(
      PVOID UniqueID,
      __out_ecount_z(NameLength) LPWSTR Name,
      __in size_t NameLength
   )
   {
      HRESULT hr;
      hr = StringCchPrintfW(Name,
                            NameLength,
                            L"JWL$%p",
                            UniqueID);

      return ( SUCCEEDED(hr) );
   }
};

#define DECLARE_WNDCLASSINFO( _Name, _Style, _Background ) \
   static WNDCLASSINFO* GetWndClassInfo( ) \
   { \
      static WNDCLASSINFO __WndClassInfo = {_Name, (HBRUSH)(_Background + 1), _Style,  0, SPINLOCK_INIT(4000)}; \
      return ( &__WndClassInfo ); \
   }


class 
DECLSPEC_NOVTABLE 
WindowMessageHandler
{
public:
   virtual 
   BOOLEAN
   ProcessWindowMessage(
      __in HWND hWnd,
      __in UINT uMsg,
      __in WPARAM wParam,
      __in LPARAM lParam,
      __out LRESULT& lRet
   ) = 0;
};

typedef union _CMD_INFO
{
   struct {
      WORD  cmdCode;
      WORD  ctlID;
      HWND  hwndCtl;
   };

   struct {
      WORD  sysCode;
      int   xPos;
      int   yPos;
   };

   WORD  menuID;
   WORD  accelID;
}CMDHDR, *PCMDHDR;

typedef struct _MSG_INFO
{ 
   UINT        uMsg;
   WPARAM      wParam;
   LPARAM      lParam;

   union {
      PCMDHDR  CmdInfo;
      LPNMHDR  NotifyInfo;
   };
}MSG_INFO;

#define BEGIN_MSG_ROUTER( ) \
   BOOLEAN ProcessWindowMessage( __in HWND hWnd, __in UINT uMsg, __in WPARAM wParam, __in LPARAM lParam, __out LRESULT& lRet ) { \
      CMDHDR   CmdInfo; \
      MSG_INFO MsgInfo; \
      BOOLEAN  bProcessed; \
      \
      hWnd; \
      uMsg; \
      wParam; \
      lParam; \
      lRet; \
      \
      MsgInfo.uMsg    = uMsg; \
      MsgInfo.wParam  = wParam; \
      MsgInfo.lParam  = lParam; \
      MsgInfo.CmdInfo = NULL;

#define MESSAGE_ADAPTER( _WM ) \
      if ( _WM == uMsg ) {

#define _COMMAND_ADAPTER_BASE( ) \
      if ( 0 == HIWORD(wParam) ) { \
            CmdInfo.menuID = LOWORD(wParam); \
         } \
         else if ( 1 == HIWORD(wParam) ) { \
            CmdInfo.accelID = LOWORD(wParam); \
         } \
         else { \
            CmdInfo.cmdCode = HIWORD(wParam); \
            CmdInfo.ctlID   = LOWORD(wParam); \
            CmdInfo.hwndCtl = (HWND)lParam; \
         } \
         MsgInfo.CmdInfo = &CmdInfo;

#define COMMAND_ADAPTER( _CtlID, _Code ) \
      if ( (WM_COMMAND == uMsg) && (LOWORD(wParam) == _CtlID) && (HIWORD(wParam) == _Code) ) { \
         _COMMAND_ADAPTER_BASE()

#define COMMAND_ID_ADAPTER( _CtlID ) \
      if ( (WM_COMMAND == uMsg) && (LOWORD(wParam) == _CtlID) ) { \
         _COMMAND_ADAPTER_BASE()

#define COMMAND_CODE_ADAPTER( _Code ) \
      if ( (WM_COMMAND == uMsg) && (HIWORD(wParam) == _Code) ) { \
         _COMMAND_ADAPTER_BASE()

#define SYSCOMMAND_ADAPTER( _SysCode ) \
   if ( (WM_SYSCOMMAND == uMsg) && (GET_SC_WPARAM(wParam) == _SysCode) ) { \
      CmdInfo.sysCode = GET_SC_WPARAM(wParam); \
      CmdInfo.xPos    = (int)(short)LOWORD(lParam); \
      CmdInfo.yPos    = (int)(short)HIWORD(lParam); \
      MsgInfo.CmdInfo = &CmdInfo;

#define NOTIFY_ADAPTER( _CtlID, _Code ) \
      if ( (WM_NOTIFY == uMsg) && (((LPNMHDR)lParam)->idFrom == _CtlID) && (((LPNMHDR)lParam)->code == _Code) ) { \
         MsgInfo.NotifyInfo = (LPNMHDR)lParam;

#define NOTIFY_ID_ADAPTER( _CtlID ) \
      if ( (WM_NOTIFY == uMsg) && (((LPNMHDR)lParam)->idFrom == _CtlID) ) { \
         MsgInfo.NotifyInfo = (LPNMHDR)lParam;

#define NOTIFY_CODE_ADAPTER( _Code ) \
      if ( (WM_NOTIFY == uMsg) && (((LPNMHDR)lParam)->code == _Code) ) { \
         MsgInfo.NotifyInfo = (LPNMHDR)lParam;

#define MENUCOMMAND_ADAPTER(_MenuID) \
	if ( (WM_MENUCOMMAND == uMsg) && (LOWORD(wParam) == _MenuID) ) { 
		
#define MENUCOMMAND_RANGE_ADAPTER(_MenuIDFirst, _MenuIDLast) \
	if ( (WM_MENUCOMMAND == uMsg) &&  ((LOWORD(wParam) >= _MenuIDFirst) && (LOWORD(wParam) <= _MenuIDLast)) ) { 

#define ADAPTER_ENTRY( _Handler, _Adapter ) \
      _Adapter \
         bProcessed = TRUE; \
         lRet = _Handler(MsgInfo, bProcessed); \
         if ( bProcessed ) { \
            return ( TRUE ); \
         } \
      } 

#define END_MSG_ROUTER() \
      return ( FALSE ); \
   }

class Window
{
public:
   HWND _hWnd;
};

class WindowInstanceBase : public Window
{
public:
   WindowInstanceBase(
   ) throw()
   {
      _ChainedWndProc = ::DefWindowProcW;
   }

   LRESULT
   DefWindowProc(
      __in HWND hWnd,
      __in UINT uMsg,
      __in WPARAM wParam,
      __in LPARAM lParam
   )
   {
      return ( ::CallWindowProc(_ChainedWndProc,
                                hWnd,
                                uMsg,
                                wParam,
                                lParam) );
   }
   
   HWND
   Create(    
      __in DWORD dwExStyle,
      __in_opt LPCWSTR lpClassName,
      __in_opt LPCWSTR lpWindowName,
      __in DWORD dwStyle,
      __in int X,
      __in int Y,
      __in int nWidth,
      __in int nHeight,
      __in_opt HWND hWndParent,
      __in_opt HMENU hMenu,
      __in_opt HINSTANCE hInstance,
      __in_opt LPVOID lpParam
   )
   {
      return ( ::CreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam) );
   }

   ATOM
   RegisterWndClass(
      WNDCLASSEX* WndClassEx
   )
   {
      return ( ::RegisterClassEx(WndClassEx) );
   }
   
   WNDPROC _ChainedWndProc;
};

template < class T, WindowInstanceType InstanceType >
class 
DECLSPEC_NOVTABLE
WindowInstanceT : public WindowInstanceBase,
                  public WndProcThunkInstanceT<T, InstanceType>::__WndProcThunk
{
public:
   HINSTANCE
   GetWindowInstance(
   )
   {
      return ( (HINSTANCE)&__ImageBase );
   }

   ATOM
   RegisterWndClassT(
   )
   {
      T*             pT;
      ATOM           WndClassAtom;
      WNDCLASSINFO*  pWndClassInfo;
      WNDCLASSEXW    WndClassEx;
      WCHAR          chClassName[5+(sizeof(WNDCLASSINFO*)*CHAR_BIT)];

      /* Initialize locals */
      pT            = static_cast<T*>(this);
      pWndClassInfo = T::GetWndClassInfo();
      WndClassAtom  = 0;

      if ( 0 == *((volatile WORD*)&(pWndClassInfo->ClassAtom)) ) {
         /*
          * Don't allow the above check to be optomized away
          */
         _ForceMemoryReadCompletion();
                  
         WndClassEx.cbSize        = sizeof(WndClassEx);
         WndClassEx.style         = pWndClassInfo->Style;
         WndClassEx.lpfnWndProc   = &T::__InitialWndProc;
         WndClassEx.cbClsExtra    = 0;
         WndClassEx.cbWndExtra    = 0;
         WndClassEx.hInstance     = pT->GetWindowInstance();
         WndClassEx.hIcon         = NULL;
         WndClassEx.hCursor       = LoadCursor(NULL,
                                               IDC_ARROW);
         WndClassEx.hbrBackground = pWndClassInfo->Background;
         WndClassEx.lpszMenuName  = NULL;
         WndClassEx.lpszClassName = pWndClassInfo->Name;
         WndClassEx.hIconSm       = NULL;

         if ( !pWndClassInfo->Name ) {
            if ( WNDCLASSINFO::GenerateClassName(pWndClassInfo,
                                                 chClassName,
                                                 ARRAYSIZE(chClassName)) ) {
               WndClassEx.lpszClassName = chClassName;
            }
         }

         /*
          * Check again if another thread was able to register the class
          */
         AcquireSpinLock(&(pWndClassInfo->CreateLock)); {
            if ( 0 == *((volatile WORD*)&(pWndClassInfo->ClassAtom)) ) {
               /*
                * Don't allow the above check to be optomized away
                */                        
               _ForceMemoryReadCompletion();

               WndClassAtom = pT->RegisterWndClass(&WndClassEx);
               if ( 0 != WndClassAtom ) {
                   pWndClassInfo->ClassAtom = WndClassAtom;
               }
            }
         }
         ReleaseSpinLock(&(pWndClassInfo->CreateLock));
#ifdef _DEBUG
         if ( 0 == WndClassAtom ) {
            _ASSERTE(GetLastError() == ERROR_CLASS_ALREADY_EXISTS);
         }
#endif /* _DEBUG */
      }

      return ( pWndClassInfo->ClassAtom );
   }

   HWND
   CreateT(
    __in_opt HWND hwndParent,
    __in_opt LPCWSTR pwszWindowName,
    __in DWORD dwStyle,
    __in DWORD dwExStyle,
    __in int x,
    __in int y,
    __in int cx,
    __in int cy,
    __in_opt HMENU hMenu,
    __in_opt LPVOID pParameter
   )
   {
      POINT Position;
      SIZE  Dimensions;

      Position.x    = x;
      Position.y    = y;
      Dimensions.cx = cx;
      Dimensions.cy = cy;

      return ( CreateT(hwndParent,
                       pwszWindowName,
                       dwStyle,
                       dwExStyle,
                       Position,
                       Dimensions,
                       hMenu,
                       pParameter) );
   }
   
   HWND
   CreateT(
    __in_opt HWND hwndParent,
    __in_opt LPCWSTR pwszWindowName,
    __in DWORD dwStyle,
    __in DWORD dwExStyle,
    __in POINT Position,
    __in SIZE Dimensions,
    __in_opt HMENU hMenu,
    __in_opt LPVOID pParameter
   )
   {
      HWND     hWnd;

      T*       pT;
      ATOM     WndClassAtom;

      /* Initialize locals */
      pT = static_cast<T*>(this);
      
      /*
       * Ensure that if the window is using a thunk, it has been created
       */      
      if ( !pT->GetWindowProcedure() ) {
         /* Failure */
         return ( NULL );
      }      

      /*
       * Store the WndProc with the thread so that __InitialWndProc can extract it and pass
       * the messages along once it has done its initialization
       */
      if ( !TlsSetValue(__WindowInstanceTlsIndex,
                        pT) )
      {
         /* Failure */
         return ( NULL );
      }
      
      /*
       * Make sure the window class is registered then create the window. __InitialWndProc will receive
       * the first message, then all future messages will be sent to whatever __WndProcThunk type is
       * being used. The __WndProcThunk will then forward to WindowProcedure which calls the message
       * router
       */
      WndClassAtom = pT->RegisterWndClassT();
      if ( 0 == WndClassAtom ) {
         TlsSetValue(__WindowInstanceTlsIndex,
                     NULL);

         /* Failure */
         return ( NULL );
      }

      hWnd = pT->Create(dwExStyle,
                        MAKEINTATOM(WndClassAtom),
                        pwszWindowName,
                        dwStyle,
                        Position.x,
                        Position.y,
                        Dimensions.cx,
                        Dimensions.cy,
                        hwndParent,
                        hMenu,
                        pT->GetWindowInstance(),
                        pParameter);

      return ( hWnd );
   }

   static
   LRESULT
   CALLBACK
   __InitialWndProc(
      __in HWND hWnd, 
      __in UINT uMsg, 
      __in WPARAM wParam, 
      __in LPARAM lParam
   )
   {
      T*      pT;
      WNDPROC pWndProc;
      WNDPROC pPreviousWndProc;

      pT = reinterpret_cast<T*>(TlsGetValue(__WindowInstanceTlsIndex));
      _ASSERTE(NULL != pT);

      TlsSetValue(__WindowInstanceTlsIndex,
                  NULL);

      /*
       * Get the class' window procedure that will handle all messages after we do our
       * initial setup
       */
      pWndProc = pT->GetWindowProcedure();
      _ASSERTE(NULL != pWndProc);
      
      /*
       * Assign the window handle to the class before we send it any messages
       */
      pT->_hWnd = hWnd;

      /*
       * Now reset the window procedure so all future messages go to the class' window
       * procedure. We'll manually forward this first message
       */
      pPreviousWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hWnd,
                                                                    GWLP_WNDPROC,
                                                                    reinterpret_cast<LONG_PTR>(pWndProc)));
      _ASSERTE(__InitialWndProc == pPreviousWndProc);
      
      return ( pWndProc(hWnd,
                        uMsg,
                        wParam,
                        lParam) );
   }

   static
   LRESULT
   CALLBACK
   WindowProcedure(
      __in T* pT,
      __in HWND hWnd,
      __in UINT uMsg,
      __in WPARAM wParam,
      __in LPARAM lParam
   )
   {
      LRESULT  lRet;
      BOOLEAN  bProcessed;

      /* Initialize locals */
      lRet = 0;
      
      bProcessed = pT->ProcessWindowMessage(hWnd,
                                            uMsg,
                                            wParam,
                                            lParam,
                                            lRet);

      if ( !bProcessed ) {
         lRet = pT->DefWindowProc(hWnd,
                                  uMsg,
                                  wParam,
                                  lParam);
      }

      return ( lRet );
   }
};

/*
 *
 *
 */

#define DECLARE_DIALOG_CLASSNAME( _Name ) \
   static LPCTSTR GetDialogClassName() throw() { \
      return ( _Name ); \
   }

template < class T, class TBase = Window >
class 
DECLSPEC_NOVTABLE
DialogInstanceBase : public ATL::CWindowImplBaseT<TBase, ATL::CNullTraits>
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
         wci.m_wc.lpszClassName = T::GetDialogClassName();
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
