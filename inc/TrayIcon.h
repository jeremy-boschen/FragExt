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

/* TrayIcon.h
 *    Taskbar tray icon wrapper
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef _countof
   #define _countof(rg) (sizeof(rg) / sizeof(rg[0]))
#endif

template < class T > 
class ATL_NO_VTABLE CTrayIcon
{
public:
   typedef CTrayIcon<T> _TrayIcon;
   enum
   {
      IconHeight = 16,
      IconWidth  = 16
   };

   CTrayIcon( ) throw() : _bRawMessages(FALSE), _bInitialized(FALSE), _bIconAdded(FALSE), _uTrayCallbackMsg(0), _dwVersion(0)
   {
      ZeroMemory(&_nid,
                 sizeof(NOTIFYICONDATA));
   }
   
   void Initialize( HWND hwndNotify, UINT uID, UINT uCallbackMsg = 0, BOOL bRawMessages = FALSE ) throw()
   {
      _dwVersion            = _GetShell32Version();
      _nid.hWnd             = hwndNotify;
      _nid.uID              = uID;
      _nid.uCallbackMessage = _uTrayCallbackMsg = uCallbackMsg;
      _nid.uFlags           = (0 != uCallbackMsg ? NIF_MESSAGE : 0);
      _bRawMessages         = bRawMessages;
      _bInitialized         = TRUE;
   }

   void Uninitialize( ) throw()
   {
      if ( _bIconAdded )
      {
         Delete();            
      }

      ZeroMemory(&_nid,
                 sizeof(NOTIFYICONDATA));

      _bRawMessages     = FALSE;
      _bInitialized     = FALSE;        
      _bIconAdded       = FALSE;
      _uTrayCallbackMsg = 0;
   }

   BOOL IsInitialized( ) const throw()
   {
      return ( _bInitialized );
   }

   BOOL Add( ) throw()
   {
      ATLASSERT(IsInitialized());
      _bIconAdded = _NotifyShell(NIM_ADD);
      return ( _bIconAdded );
   }

   BOOL IsAdded( ) const throw()
   {
      return ( _bIconAdded );
   }

   BOOL Delete( ) throw()
   {
      ATLASSERT(IsInitialized() && IsAdded());
      _bIconAdded = !_NotifyShell(NIM_DELETE);        
      return ( !_bIconAdded );
   }

   BOOL Update( ) throw()
   {
      ATLASSERT(IsInitialized() && IsAdded());
      return ( _bInitialized && _NotifyShell(NIM_MODIFY) );
   }

   BOOL SetFocus( ) throw()
   {
#if (_WIN32_IE >= 0x0500)
      if ( _dwVersion >= MAKELONG(5,0) )
      {
         return ( _NotifyShell(NIM_SETFOCUS) );
      }
#endif /* _WIN32_IE >= 0x0500 */
      return ( FALSE );
   }

   BOOL SetVersion( UINT uVersion ) throw()
   {
#if (_WIN32_IE >= 0x0500)
      if ( _dwVersion >= MAKELONG(5,0) )
      {
         _nid.uVersion = uVersion;
         return ( _NotifyShell(NIM_SETVERSION) );
      }
#endif /* (_WIN32_IE >= 0x0500) */
      return ( FALSE );
   }

   void SetTrayIcon( HICON hIcon, BOOL bFreeActive = TRUE ) throw()
   {
      _nid.uFlags |= NIF_ICON;
      if ( bFreeActive && _nid.hIcon )
      {
         ::DestroyIcon(_nid.hIcon);
      }
      _nid.hIcon = hIcon;
   }

   void SetMessage( UINT uMsgCallback ) throw()
   {
      _nid.uFlags           |= NIF_MESSAGE;
      _nid.uCallbackMessage  = uMsgCallback;
   }
   
   void SetTip( LPCTSTR pszTip ) throw()
   {
      if ( SUCCEEDED(StringCchCopy(_nid.szTip, 
                                   _countof(_nid.szTip), 
                                   pszTip)) )
      {
         _nid.uFlags |= NIF_TIP;
      }
   }

   void SetInfo( LPCTSTR pszInfo, LPCTSTR pszInfoTitle, DWORD dwInfoFlags, UINT uTimeout ) throw() 
   {
#if (_WIN32_IE >= 0x0500)
      if ( _dwVersion >= MAKELONG(5,0) )
      {
         if ( SUCCEEDED(StringCchCopy(_nid.szInfo, 
                                      _countof(_nid.szInfo), 
                                      pszInfo)) && 
              SUCCEEDED(StringCchCopy(_nid.szInfoTitle, 
                                      _countof(_nid.szInfoTitle), 
                                      pszInfoTitle)) )
         {
            _nid.uFlags     |= NIF_INFO;
            _nid.uTimeout    = uTimeout;
            _nid.dwInfoFlags = dwInfoFlags;
         }
      }
#endif /* (_WIN32_IE >= 0x0500) */
   }

   void SetState( DWORD dwState, DWORD dwMask ) throw()
   {
#if (_WIN32_IE >= 0x0500)
      if ( _dwVersion >= MAKELONG(5,0) )
      {
         _nid.uFlags     |= NIF_STATE;
         _nid.dwState     = dwState;
         _nid.dwStateMask = dwMask;
      }
#endif /* (_WIN32_IE >= 0x0500) */
   }

   void SetTimeout( UINT uTimeout ) throw()
   {
#if (_WIN32_IE >= 0x0500)
      if ( _dwVersion >= MAKELONG(5,0) )
      {
         _nid.uFlags  |= NIF_INFO;
         _nid.uTimeout = uTimeout;
      }
#endif /* (_WIN32_IE >= 0x0500) */
   }

   void SetGuidItem( GUID& guid ) throw()
   {
#if (_WIN32_IE >= 0x0600)
      if ( _dwVersion >= MAKELONG(6,0) )
      {
         _nid.uFlags  |= NIF_GUID;
         _nid.guidItem = guidItem;
      }
#endif /* (_WIN32_IE >= 0x0600) */
   }

   operator NOTIFYICONDATA*( ) throw()
   {
      return ( &_nid );
   }

   BEGIN_MSG_MAP(CTrayIcon<T>)
      MESSAGE_HANDLER(_uTrayCallbackMsg, OnTrayMsgCallback)
   END_MSG_MAP()

   LRESULT OnTrayMsgCallback( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw()
   {
      if ( WM_NULL == uMsg )
      {
         bHandled = FALSE;

         return ( 0 );
      }

      T* pT = static_cast<T*>(this);

      if ( uMsg != _uTrayCallbackMsg )
      {
         bHandled = FALSE;

         return ( 0 );
      }

      if ( _bRawMessages )
      {
         __if_exists(T::OnTrayMessage)
         {                    
               return ( pT->OnTrayMessage(wParam, lParam) );
         }
         /* OnTrayMessage not declared */
         ATLASSERT(FALSE);
      }

      LRESULT lRet = 0;
      POINT   cPos = {0};

      ::GetCursorPos(&cPos);

      switch ( (UINT)lParam )
      {
         case WM_MOUSEMOVE:
               __if_exists(T::OnTrayMouseMove)
               {
                  lRet = pT->OnTrayMouseMove((UINT)wParam, cPos);
               }
               break;
         case WM_LBUTTONDOWN:
               __if_exists(T::OnTrayLButtonDown)
               {
                  lRet = pT->OnTrayLButtonDown((UINT)wParam, cPos);
               }
               break;
         case WM_LBUTTONUP:
               __if_exists(T::OnTrayLButtonUp)
               {
                  lRet = pT->OnTrayLButtonUp((UINT)wParam, cPos);
               }
               break;
         case WM_LBUTTONDBLCLK:
               __if_exists(T::OnTrayLButtonDblClk)
               {
                  lRet = pT->OnTrayLButtonDblClk((UINT)wParam, cPos);
               }
               break;            
         case WM_RBUTTONDOWN:
               __if_exists(T::OnTrayRButtonDown)
               {
                  lRet = pT->OnTrayRButtonDown((UINT)wParam, cPos);
               }
               break;
         case WM_RBUTTONUP:
               __if_exists(T::OnTrayRButtonUp)
               {
                  lRet = pT->OnTrayRButtonUp((UINT)wParam, cPos);
               }
               break;
         case WM_RBUTTONDBLCLK:
               __if_exists(T::OnTrayRButtonDblClk)
               {
                  lRet = pT->OnTrayRButtonDblClk((UINT)wParam, cPos);
               }
               break;
         case WM_CONTEXTMENU:
               __if_exists(T::OnTrayContextMenu)
               {
                  lRet = pT->OnTrayContextMenu((UINT)wParam, cPos);
               }
               break;
#if (_WIN32_IE >= 0x0500)
         case NIN_SELECT:
               __if_exists(T::OnTraySelect)
               {
                  lRet = pT->OnTraySelect((UINT)wParam, cPos);
               }
               break;
         case NIN_KEYSELECT:
               __if_exists(T::OnTrayKeySelect)
               {
                  lRet = pT->OnTrayKeySelect((UINT)wParam, cPos);
               }
               break;
#endif /* (_WIN32_IE >= 0x0500) */
#if (_WIN32_IE >= 0x0501)
         case NIN_BALLOONSHOW:
               __if_exists(T::OnTrayBalloonShow)
               {
                  lRet = pT->OnTrayBalloonShow((UINT)wParam, cPos);
               }
               break;
         case NIN_BALLOONHIDE:
               __if_exists(T::OnTrayBalloonHide)
               {
                  lRet = pT->OnTrayBalloonHide((UINT)wParam, cPos);
               }
               break;
         case NIN_BALLOONTIMEOUT:
               __if_exists(T::OnTrayBalloonTimeout)
               {
                  lRet = pT->OnTrayBalloonTimeout((UINT)wParam, cPos);
               }
               break;
         case NIN_BALLOONUSERCLICK:
               __if_exists(T::OnTrayBalloonUserClick)
               {
                  lRet = pT->OnTrayBalloonUserClick((UINT)wParam, cPos);
               }
               break;
#endif /* (_WIN32_IE >= 0x0501) */
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
         case NIN_POPUPOPEN:
            __if_exists(T::OnTrayPopupOpen)
            {
               lRet = pT->OnTrayPopupOpen(static_cast<UINT>(wParam), cPos);
            }
            break;
         case NIN_POPUPCLOSE:
            __if_exists(T::OnTrayPopupClose)
            {
               lRet = pT->OnTrayPopupClose(static_cast<UINT>(wParam), cPos);
            }
            break;
#endif /* (NTDDI_VERSION >= NTDDI_LONGHORN) */
         default:
               break;
      } 

      return ( TRUE );
   }

   BOOL           _bRawMessages;
   BOOL           _bInitialized;
   BOOL           _bIconAdded;
   /* For caching copy of _nid.uCallbackMsg */
   UINT           _uTrayCallbackMsg;
   NOTIFYICONDATA _nid;
   DWORD          _dwVersion;

   /* Neither of these are supported */
   CTrayIcon( const CTrayIcon& );
   CTrayIcon& operator =( const CTrayIcon& );

   BOOL _NotifyShell( DWORD dwMessage ) throw()
   {
      if ( 0 == _nid.cbSize )
      {
         _nid.cbSize = _GetSizeOfNid();
         ATLASSERT(sizeof(NOTIFYICONDATA) >= _nid.cbSize);
      }      

      return ( Shell_NotifyIcon(dwMessage, 
                                &_nid) );
   }

   DWORD _GetSizeOfNid( ) throw()
   {
      DWORD cbSize;

      if ( _dwVersion < MAKELONG(5,0) )
      {
         cbSize = NOTIFYICONDATA_V1_SIZE;
      }
      else if ( _dwVersion < MAKELONG(6,0) )
      {
         cbSize = NOTIFYICONDATA_V2_SIZE;
      }
      else
      {
         cbSize = sizeof(NOTIFYICONDATA);
      }

      return ( cbSize );
   }

   static DWORD _GetShell32Version( ) throw()
   {
      DWORD             dwVer;
      HMODULE           hModule;
      DLLGETVERSIONPROC pDllGetVersion;
      DLLVERSIONINFO    VersionInfo;

      /* Incase of failure, revert to minimum functionality */
      dwVer = MAKELONG(4,0);

      hModule = GetModuleHandle(_T("SHELL32"));
      if ( hModule )
      {
         pDllGetVersion = reinterpret_cast<DLLGETVERSIONPROC>(GetProcAddress(hModule, 
                                                                             "DllGetVersion"));

         if ( pDllGetVersion )
         {
            ZeroMemory(&VersionInfo,
                       sizeof(DLLVERSIONINFO));

            VersionInfo.cbSize = sizeof(DLLVERSIONINFO);

            if ( SUCCEEDED(pDllGetVersion(&VersionInfo)) )
            {
               dwVer = MAKELONG(VersionInfo.dwMajorVersion, VersionInfo.dwMinorVersion);
            }
         }
      }

      return ( dwVer );
   }
};

/**
 * CreateUniqueWindowMessage
 *
 * Parameters
 *		dwSig
 *       [in] Unique identifier used to help create the message value.
 *
 * Return Value
 *	   Returns non-zero if successful, 0 otherwise.
 *
 * Remarks
 *		None
 */
inline UINT CreateUniqueWindowMessage( DWORD dwSig ) throw()
{
   DWORD dwTick;
   DWORD dwTid;
   TCHAR chMsg[24];

   dwTick = GetTickCount();
   dwTid  = GetCurrentThreadId();

   ZeroMemory(chMsg,
              sizeof(chMsg));


   if ( SUCCEEDED(StringCchPrintf(chMsg, 
                                  _countof(chMsg), 
                                  _T("UM:%08lX:%08lX"), 
                                  dwTick ^ dwSig, 
                                  dwTid)) )
   {
      return ( RegisterWindowMessage(chMsg) );
   }

   return ( 0 );
}
