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
 
/* TaskDlg.cpp
 *    CTaskDlg implementation
 * 
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "TaskDlg.h"

/**********************************************************************

    CTaskFileDialog

 **********************************************************************/
CTaskFileDialog::CTaskFileDialog( LPCTSTR lpszFilter, HWND hWndParent ) throw() : _MultiFileDialogImpl(NULL, NULL, CTFD_OPENFILEFLAGS, lpszFilter, hWndParent), IncludeAlternateDataStreams(false)
{
}

BOOL CTaskFileDialog::OnFileOK( LPOFNOTIFY /*lpon*/ ) throw()
{
   IncludeAlternateDataStreams = (BST_CHECKED != GetFileDialogWindow().IsDlgButtonChecked(chx1) ? false : true);

   SetSettingValue(FRAGMGX_SUBKEY,
                   FRAGMGX_INCLUDEALTDATESTREAMS,
                   GSVF_HKEY_CURRENT_USER,
                   static_cast<DWORD>(IncludeAlternateDataStreams));

   return ( TRUE );
}

void CTaskFileDialog::OnInitDone( LPOFNOTIFY /*lpon*/ ) throw()
{
   WCHAR   chBuf[128];
   DWORD   dwValue;
   CWindow cDlg;

   ZeroMemory(chBuf,
              sizeof(chBuf));

   LoadString(_AtlModule.GetMUIInstance(),
              IDS_OPENFILE_ALTERNATE_STREAMS,
              chBuf,
              _countof(chBuf));
   ATLASSERT(L'\0' != (*chBuf));

   /* Reuse the 'Read Only' checkbox */
   SetControlText(chx1, 
                  chBuf);

   /* IncludeAlternateDataStreams - Default is 0 */
   dwValue = 0;
   GetSettingValue(FRAGMGX_SUBKEY,
                   FRAGMGX_INCLUDEALTDATESTREAMS,
                   GSVF_HKEY_CURRENT_USER,
                   &dwValue);
   IncludeAlternateDataStreams = (0 == dwValue ? false : true);

   if ( IncludeAlternateDataStreams )
   {
      cDlg = GetFileDialogWindow();      
      cDlg.CheckDlgButton(chx1, 
                          BST_CHECKED);
   }
}

/**********************************************************************

   CAboutDialog : ATL

 **********************************************************************/

LRESULT CAboutDialog::OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ ) throw()
{
   HICON             hIcon;
   WTL::CStatic      ctlIcon;   
   WTL::CStatic      ctlInfo;
   WCHAR             chBuildDate[512];
   WCHAR             chFormat[256];
   WCHAR             chInfo[512];
   PIMAGE_NT_HEADERS pNtHeaders;

   hIcon = reinterpret_cast<HICON>(LoadImage(_AtlBaseModule.GetModuleInstance(),
                                             MAKEINTRESOURCE(IDI_FRAGMGX),
                                             IMAGE_ICON,
                                             32,
                                             32,
                                             LR_LOADMAP3DCOLORS|LR_LOADTRANSPARENT|LR_SHARED));
   if ( hIcon )
   {
      ctlIcon = GetDlgItem(IDC_ABOUTICON);
      ctlIcon.SetIcon(hIcon);
   }

   pNtHeaders = GetModuleImageNtHeaders(_AtlBaseModule.GetModuleInstance());
   if ( pNtHeaders )
   {
      ZeroMemory(chBuildDate,
                 sizeof(chBuildDate));

      _FormatTimeDateStamp(pNtHeaders->FileHeader.TimeDateStamp,
                           chBuildDate,
                           _countof(chBuildDate));

      ZeroMemory(chFormat,
                 sizeof(chFormat));

      LoadString(_AtlModule.GetMUIInstance(),
                 IDS_ABOUT_INFORMATION,
                 chFormat,
                 _countof(chFormat));
      ATLASSERT(L'\0' != (*chFormat));
      
      ZeroMemory(chInfo,
                 sizeof(chInfo));

      _FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                     chFormat,
                     0,
                     0,
                     chInfo,
                     _countof(chInfo),
                     _T(PRODUCTVERSIONSTRING),
                     chBuildDate);

      ctlInfo = GetDlgItem(IDC_ABOUTINFO);
      ctlInfo.SetWindowText(chInfo);
   }
   
   return ( TRUE );
}

LRESULT CAboutDialog::OnSysCommand( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled ) throw()
{
   switch ( wParam )
   {
      case SC_CLOSE:         
         EndDialog(0);
         break;

      default:
         bHandled = FALSE;
         break;
   }

   return ( TRUE );
}

LRESULT CAboutDialog::OnButtonClicked( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& bHandled ) throw()
{
   switch ( wID )
   {
      case IDOK:
         EndDialog(0);
         break;

      default:
         bHandled = FALSE;
         break;
   }

   return ( TRUE );
}

LRESULT CAboutDialog::OnLinkClicked( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   PNMLINK pNmLink;

   /* Initialize locals.. */
   pNmLink = reinterpret_cast<PNMLINK>(pnmh);

   ShellExecute(m_hWnd,
                L"open",
                pNmLink->item.szUrl,
                NULL,
                NULL,
                SW_SHOWNORMAL);

   return ( TRUE );
}

/**********************************************************************

   CAboutDialog : CAboutDialog

 **********************************************************************/
void CAboutDialog::_FormatTimeDateStamp( DWORD dwTimeDateStamp, LPWSTR pwszBuf, DWORD cchBuf ) throw()
{
   SYSTEMTIME     SysTimeUTC;
   FILETIME       FileTimeUTC;
   ULARGE_INTEGER iFileTimeUTC;
   SYSTEMTIME     SysTimeLocal;
   WCHAR          chDate[256];
   WCHAR          chTime[256];

   /* Convert dwTimeDateStamp to system time by adding it to 01/01/1970 0:0:0:0 */
   SysTimeUTC.wYear         = 1970;
   SysTimeUTC.wMonth        = 1;
   SysTimeUTC.wDayOfWeek    = 0;
   SysTimeUTC.wDay          = 1;
   SysTimeUTC.wHour         = 0;
   SysTimeUTC.wMinute       = 0;
   SysTimeUTC.wSecond       = 0;
   SysTimeUTC.wMilliseconds = 0;

   FileTimeUTC.dwLowDateTime  = 0;
   FileTimeUTC.dwHighDateTime = 0;

   SystemTimeToFileTime(&SysTimeUTC,
                        &FileTimeUTC);

   iFileTimeUTC.LowPart   = FileTimeUTC.dwLowDateTime;
   iFileTimeUTC.HighPart  = FileTimeUTC.dwHighDateTime;
   iFileTimeUTC.QuadPart += (static_cast<ULONGLONG>(dwTimeDateStamp) * 10000000);

   FileTimeUTC.dwLowDateTime  = iFileTimeUTC.LowPart;
   FileTimeUTC.dwHighDateTime = iFileTimeUTC.HighPart;

   FileTimeToSystemTime(&FileTimeUTC,
                        &SysTimeUTC);

   SystemTimeToTzSpecificLocalTime(NULL,
                                   &SysTimeUTC,
                                   &SysTimeLocal);

   ZeroMemory(chDate,
              sizeof(chDate));

   if ( GetDateFormat(LOCALE_USER_DEFAULT,
                      0,
                      &SysTimeLocal,
                      L"ddd',' MMM dd yyyy",
                      chDate,
                      _countof(chDate)) > 0 )
   {
      ZeroMemory(chTime,
                 sizeof(chTime));

      if ( GetTimeFormat(LOCALE_USER_DEFAULT,
                         TIME_NOSECONDS,
                         &SysTimeLocal,
                         NULL,
                         chTime,
                         _countof(chTime)) > 0 )
      {
         _FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                        L"%1 %2%0",
                        0,
                        0,
                        pwszBuf,
                        cchBuf,
                        chDate,
                        chTime);
      }
   }
}

/**********************************************************************

   CTaskDlg : ATL/WTL

 **********************************************************************/

void CTaskDlg::RunMessageLoop( ) throw()
{
   MSG  Msg;
   BOOL bRet;

   while ( 1 )
   {
      do
      {
         /* We want to process queue update messages before anything else, so pull
          * them out of the queue while there are any in there. Some will end up
          * in GetMessage() below, we'll get the majority of them here */
         bRet = PeekMessage(&Msg,
                            m_hWnd,
                            WM_QUEUEUPDATE,
                            WM_QUEUEUPDATE,
                            PM_REMOVE);

         /* We need to handle WM_QUIT here too as PeekMessage() always retrieves it */
         if ( WM_QUIT == Msg.message )
         {
            /* Success */
            return;
         }
      }
      while ( bRet );

      bRet = GetMessage(&Msg,
                        NULL,
                        0,
                        0);

      if ( 0 == bRet )
      {
         /* Success - WM_QUIT was received */
         return;
      }
      else if ( bRet > 0 )
      {
         if ( !IsDialogMessage(&Msg) )
         {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
         }
      }
#ifdef _DEBUG
      else
      {
         _dTrace(eTraceError, L"GetMessage() failed!%#08lx, %s\n", GetLastError(), (LPCWSTR)SysErrorMsg(GetLastError(), NULL));
      }
#endif /* _DEBUG */
   }
}

void CTaskDlg::OnFinalMessage( HWND /*hWnd*/ )
{
   _SaveSettings();

   /* Shutdown the message pump.. */
   PostQuitMessage(0);

   /* Lock() called in OnCreate() */
   _pAtlModule->Unlock();
}

LRESULT CTaskDlg::OnCreate( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
   /* Unlock() called in OnFinalMessage() */
   _pAtlModule->Lock();

   if ( !AtlInitCommonControls(ICC_BAR_CLASSES|ICC_PROGRESS_CLASS) )
   {
      _dTrace(eTraceError, L"Failed to initialize common controls library. Possible error:%08lx\n", GetLastError());
      
      /* Failure */
      return ( -1 );
   }

   /* Message broadcast by the system when the taskbar is created, such as when
    * Explorer crashes and restarts. We use it to re-add our icon to the system tray */
   _WmTaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");
   
   /* Load the settings so code in OnInitDialog() can reference the settings */
   _LoadSettings();

   /* Success */   
   return ( 0 );
}

LRESULT CTaskDlg::OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
   HICON hIcon;

   /* Assign the window icons.. */
   hIcon = LoadIcon(_pModule->GetModuleInstance(),
                    MAKEINTRESOURCE(IDI_FRAGMGX));
   ATLASSERT(NULL != hIcon);

   CWindow::SetIcon(hIcon,
                    FALSE);
   CWindow::SetIcon(hIcon,
                    TRUE);

   /* Setup the auto-resize stuff */
   DlgResize_Init(true, 
                  true, 
                  0);

   _ctlLog = GetDlgItem(IDC_DEFRAGLOG);

   /* Setup the dialog controls.. */
   if ( !_InitializeDefragLog() || !_InitializeProgressBar() || !_InitializePushPin() )
   {
      _ShutdownDialog();
      /* Failure */
      return ( FALSE );
   }
   
   /* Initialize the TaskManager. This won't start posting updates until CModule::Run() registers
    * the class object with COM and clients start sending files */
   if ( !_InitializeTaskCallbacks() )
   {
      _dTrace(eTraceError, L"Failed to initialize task callbaks:%08lx\n", GetLastError());
      
      /* Failure */
      return ( -1 );
   }
   
   /* Now that everything is good to go, load the tray icon. If this fails, oh well */
   _InitializeTrayIcon();

   /* Signal that we're running.. */
   _SetStatus(TSF_RUNNING);
   
   /* Allow the dialog manager to set the default focus */
   return ( TRUE );
}

LRESULT CTaskDlg::OnSysCommand( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled )
{
   switch ( wParam )
   {
      case SC_CLOSE:         
         _ShutdownDialog();
         break;

      case SC_MINIMIZE:
      {         
         ShowWindow(SW_MINIMIZE);

         if ( _TrayIcon::IsAdded() )
         {
            ShowWindow(SW_HIDE);
         }

         break;
      }

      default:
         bHandled = FALSE;
         break;
   }

   return ( 0 );
}

LRESULT CTaskDlg::OnEndSession( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
   _ShutdownDialog();
   return ( static_cast<LRESULT>(TRUE) );
}

LRESULT CTaskDlg::OnTimer( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
   if ( static_cast<WPARAM>(TimerIdShutdown) == wParam )
   {
      /* If all tasks are complete, the window is hidden and the auto-close flag is on,
       * then shutdown the application */
      if ( (_cFileComplete == _cFileTotal) && !IsWindowVisible() && _bAutoClose )
      {
         _ShutdownDialog();
      }
   }

   return ( 0 );
}

LRESULT CTaskDlg::OnDropFiles( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
   HDROP hDrop;
   int   cFiles;
   WCHAR chFile[MAX_PATH];   
   int   idx;

   /* Initialize locals.. */
   hDrop = reinterpret_cast<HDROP>(wParam);
   if ( !hDrop )
   {
      /* Failure */
      return ( 0 );
   }

   cFiles = DragQueryFile(hDrop,
                          static_cast<UINT>(-1),
                          NULL,
                          0);

   if ( cFiles <= 0 )
   {
      /* Success */
      goto __CLEANUP;
   }

   for ( idx = 0; idx < cFiles; idx++ )
   {
      ZeroMemory(chFile,
                 sizeof(chFile));

      if ( DragQueryFile(hDrop,
                         idx,
                         chFile,
                         _countof(chFile)) > 0 )
      {
         /* Add the file to the queue.. */
         __pTaskManager->InsertQueueFile(chFile);
      }
   }

__CLEANUP:
   DragFinish(hDrop);

   /* Success / Failure */
   return ( 0 );
}

LRESULT CTaskDlg::OnQueueUpdate( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/ ) throw()
{
   PQUEUEMSG       pQueueMsg;
   QUEUEUPDATECODE eUpdateCode;

   if ( TSF_SHUTDOWN != _GetStatus() )
   {
      pQueueMsg = reinterpret_cast<PQUEUEMSG>(lParam);
      ATLASSERT(NULL != pQueueMsg);

      if ( (pQueueMsg) && (QC_FILEINSERTED == pQueueMsg->UpdateCode) )
      {
         /* Copy the parameters so we can release the thread that's waiting for
          * us to process this message */
         eUpdateCode = pQueueMsg->UpdateCode;
            
         /* This will release the thread that sent this message, which could be either the main dialog
          * thread or a random client one */
          ReplyMessage(0);

         /* Update the counters.. */
         _UpdateFileCounters(1,
                             0);
      }
   }

   return ( 0 );
}

LRESULT CTaskDlg::OnDefragUpdate( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/ ) throw()
{
   LRESULT          lRet;
   PDEFRAGMSG       pDefragMsg;
   DWORD            dwStatus;
   WTL::CWaitCursor cWaitCursor(false);
   
   /* Initialize locals.. */
   lRet       = NO_ERROR;
   pDefragMsg = reinterpret_cast<PDEFRAGMSG>(lParam);
   dwStatus   = pDefragMsg->Status;
   
   /* If we're shutting down, just bail out */
   if ( TSF_SHUTDOWN == _GetStatus() )
   {
      /* Success - Aborting */
      return ( ERROR_CANCELLED );
   }

   /* If this is the first event, update the defrag log */
   switch ( pDefragMsg->UpdateCode )
   {
      case DC_INITIALIZING:
         /* Only enable the controls if we're going to process the file */
         if ( NO_ERROR == dwStatus )
         {
            _EnableOperationControls(TRUE);
         }

         _UpdateDefragStatus(IDS_DEFRAGSTATUS_FILEINITIALIZING);         
         _UpdateDefragProgress(0);

         _UpdateDefragLog(IDS_DEFRAGLOG_FILEINITIALIZING,
                          dwStatus,
                          pDefragMsg->FileName,
                          pDefragMsg->Percent);

         break;

      case DC_DEFRAGMENTING:
         if ( _bCancel )
         {
            /* A message for IDC_CANCEL was processed, and this is the first chance we're getting to
             * propogate the message back to the TaskManager, so clear the cancelled flag and let the
             * TaskManager know that this file has been aborted.
             *
             * The TaskManager will post the complete event where we can show the cancelled state
             */
            _bCancel = false;
            
            /* Failure */
            lRet = ERROR_CANCELLED;
            break;
         }
         
         if ( _bSuspend )
         {
            /* A message for IDC_PAUSE was processed, but there was already WM_DEFRAGUPDATE
             * waiting in the queue, so we want to ignore it and ask the TaskManager to retry 
             * this one when it resumes
             */

            /* Failure */
            lRet = ERROR_RETRY;
            break;
         }

         _UpdateDefragStatus(IDS_DEFRAGSTATUS_FILEDEFRAGMENTING);
         _UpdateDefragProgress(pDefragMsg->Percent);
         break;

      case DC_COMPLETED:
         _EnableOperationControls(FALSE);

         _UpdateDefragStatus(IDS_DEFRAGSTATUS_FILECOMPLETED);
         _UpdateDefragProgress(100);

         _UpdateDefragLog(NO_ERROR == dwStatus ? IDS_DEFRAGLOG_FILECOMPLETED : IDS_DEFRAGLOG_FILEFAILED,
                          dwStatus,
                          pDefragMsg->FileName,
                          pDefragMsg->Percent);

         _UpdateFileCounters(0,
                             1);

         /* We also want to clear _bCancel here in case the button was pressed before we were
          * able to disable it. */
         _bCancel = false;

         /* If we're cancelling all the files, it's safe to flush the queue now.. */
         if ( _bCancelAll )
         {
            _bCancelAll = false;

            cWaitCursor.Set();
            __pTaskManager->FlushQueue();
            cWaitCursor.Restore();
         }

         break;

      default:
         /* Failure */
         lRet = ERROR_INVALID_PARAMETER;
         break;
   }

   /* Success / Failure */
   return ( lRet );
}

LRESULT CTaskDlg::OnCtlColorStatic( UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw()
{
   if ( reinterpret_cast<HWND>(lParam) == _ctlLog )
   {
      /* Ask for this control to be painted like a regular EDIT control */
      return ( DefWindowProc(WM_CTLCOLOREDIT, 
                             wParam, 
                             lParam) );
   }

   /* This is for some control other than the edit */
   bHandled = FALSE;
   /* Success */
   return ( 0 );
}

LRESULT CTaskDlg::OnTaskbarCreated( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
   /* If we previously added the icon, then it we're re-adding it to the new taskbar */
   if ( _TrayIcon::IsAdded() )
   {
      _TrayIcon::Add();
   }

   return ( 0 );
}

LRESULT CTaskDlg::OnPushPinClick( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
   WTL::CToolBarCtrl ctlPushPin;

   ctlPushPin  = GetDlgItem(IDC_PUSHPIN);   
   _bAutoClose = !_bAutoClose;
      
   ctlPushPin.SetButtonInfo(IDC_PUSHPIN, 
                            TBIF_IMAGE, 
                            0, 
                            0, 
                            NULL, 
                            _bAutoClose ? ImgPinUp : ImgPinDown,
                            0, 
                            0, 
                            0);
   return ( 0 );
}

LRESULT CTaskDlg::OnButtonClicked( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
   WCHAR chBuf[128];
   BOOL  bShiftKey;

   /* Get the state of the SHIFT key, either one */
   bShiftKey = (0x8000 & GetKeyState(VK_SHIFT) ? TRUE : FALSE);

   //ATL::CSimpleDialog<IDD_DEFRAGMGR, TRUE> d;
   //d.DoModal();

   switch ( wID )
   {
      case IDC_ADDFILE:
      {
         _BrowseForTaskFile();
         break;
      }
      
      case IDC_PAUSE:
      {
         ZeroMemory(chBuf,
                    sizeof(chBuf));

         _bSuspend = !_bSuspend;

         if ( _bSuspend )
         {
            _SuspendTaskManager();
         }
         else
         {
            _ResumeTaskManager();
         }

         LoadString(_AtlModule.GetMUIInstance(),
                    _bSuspend ? IDS_BUTTON_RESUME : IDS_BUTTON_PAUSE,
                    chBuf,
                    _countof(chBuf));
         ATLASSERT(L'\0' != (*chBuf));

         SetDlgItemText(IDC_PAUSE,
                        chBuf);
         break;
      }

      case IDC_CANCEL:
      {
         if ( bShiftKey )
         {
            /* Set the cancel flag so the current file gets cancelled, and the cancel-all flag
             * so the rest get cancelled after the current completes */
            _bCancel    = true;
            _bCancelAll = true;

            /* If we're in a suspended state, we'll have to resume the TaskManager */
            if ( _bSuspend )
            {
               _ResumeTaskManager();
            }
         }
         else
         {
            _bCancel = true;

            if ( _bSuspend )
            {
               /* We need to resume the defrag manager so it can update this file to the
                * cancelled state */
               _ResumeTaskManager();
            }
         }

         /* Block further control operations if possible */
         _EnableOperationControls(FALSE);

         break;
      }
   }

   return ( 0 );
}

LRESULT CTaskDlg::OnTrayMenuItem( WORD /*wItemIndex*/, WORD wID, HMENU /*hMenu*/, BOOL& bHandled )
{
   CAboutDialog cAboutDlg;

   switch ( wID )
   {
      case IDM_TRAYMENUSHOW:
      case IDM_TRAYMENUHIDE:
         if ( IsWindowVisible() )
         {
            ShowWindow(SW_HIDE);
         }
         else
         {
            ShowWindow(SW_SHOW);
            SetForegroundWindow(m_hWnd);
         }

         break;

      case IDM_TRAYMENUABOUT:
         cAboutDlg.DoModal(m_hWnd);
         break;

      case IDM_TRAYMENUEXIT:
         _ShutdownDialog();
         break;

      default:
         break;
   }

   bHandled = TRUE;

   return ( 0 );
}

LRESULT CTaskDlg::OnTrayLButtonDblClk( UINT /*uID*/, POINT /*cPos*/ )
{
   if ( IsWindowVisible() )
   {
      ShowWindow(SW_HIDE);
   }
   else
   {
      ShowWindow(SW_SHOW);
      SetForegroundWindow(m_hWnd);
   }

   return ( 0 );
}

LRESULT CTaskDlg::OnTrayRButtonUp( UINT uID, POINT cPos )
{
   return ( OnTrayContextMenu(uID, 
                              cPos) );
}

LRESULT CTaskDlg::OnTrayContextMenu( UINT /*uID*/, POINT cPos )
{
   HMENU hMenu;
   HMENU hPopup;
   BOOL  bVisible;
   UINT  uMenuCmd;

   hMenu = LoadMenu(_AtlModule.GetMUIInstance(),
                    MAKEINTRESOURCE(IDR_TRAYMENU));
   
   if ( !hMenu )
   {
      /* Failure */
      return ( 0 );
   }

   __try
   {
      hPopup = GetSubMenu(hMenu,
                          0);
      ATLASSERT(NULL != hPopup);

      bVisible = IsWindowVisible();

      /* Disable either the show/hide menu item */
      EnableMenuItem(hPopup,
                     bVisible ? IDM_TRAYMENUSHOW : IDM_TRAYMENUHIDE, 
                     MF_BYCOMMAND|MF_DISABLED|MF_GRAYED);

      SetMenuDefaultItem(hPopup, 
                         bVisible ? IDM_TRAYMENUHIDE : IDM_TRAYMENUSHOW, 
                         FALSE);

      SetForegroundWindow(m_hWnd);
      
      uMenuCmd = static_cast<UINT>(TrackPopupMenuEx(hPopup, 
                                                    GetSystemMetrics(SM_MENUDROPALIGNMENT)|TPM_BOTTOMALIGN|TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD, 
                                                    ATLEx::FixTrackMenuPopupX(cPos.x, cPos.y), 
                                                    cPos.y, 
                                                    m_hWnd, 
                                                    NULL));

      if ( 0 != uMenuCmd )
      {
         SendMessage(WM_MENUCOMMAND, 
                     MAKELONG(uMenuCmd, uMenuCmd - IDM_TRAYMENUSHOW), 
                     reinterpret_cast<LPARAM>(hPopup));
      }
   }
   __finally
   {
      DestroyMenu(hMenu);
   }

   /* This window may have been destroyed as a result of IDM_TRAYMENUEXIT handling */
   if ( IsWindow() )
   {
      PostMessage(WM_NULL);
   }

   return ( 0 );
}

HINSTANCE CTaskDlg::GetDialogInstance( ) throw()
{
   return ( _AtlModule.GetMUIInstance() );
}

/**********************************************************************

   CTaskDlg : CTaskDlg

 **********************************************************************/

CTaskDlg::CTaskDlg( ) throw()
{
   _cFileTotal    = 0;
   _cFileComplete = 0;
   _bAutoClose    = false;
   _bSuspend      = false;
   _bCancel       = false;
   _bCancelAll    = false;
   _wStatus       = TSF_STOPPED;
   _hImgList      = NULL;
}

void CTaskDlg::_LoadSettings( ) throw()
{
   DWORD dwValue;

   /* AutoClose - Default is 0 */
   dwValue = 0;
   GetSettingValue(FRAGMGX_SUBKEY,
                   FRAGMGX_CLOSEONCOMPLETION,
                   GSVF_HKEY_CURRENT_USER,
                   &dwValue);

   _bAutoClose = (0 == dwValue ? 0 : 1);
}

void CTaskDlg::_SaveSettings( ) throw()
{
   /* AutoClose */
   SetSettingValue(FRAGMGX_SUBKEY,
                   FRAGMGX_CLOSEONCOMPLETION,
                   GSVF_HKEY_CURRENT_USER,
                   static_cast<DWORD>(_bAutoClose));
}

void CTaskDlg::_BrowseForTaskFile( ) throw()
{
   WCHAR             chFilter[128];
   CTaskFileDialog*  pFileDlg;
   WCHAR             chPath[MAX_PATH*2];
   LPCWSTR           pwszFile;
   HRESULT           hr;
   ENUMSTREAMCTX     EnumCtx;

   /* Initialize locals.. */
   pFileDlg  = NULL;

   LoadString(_AtlModule.GetMUIInstance(),
              IDS_OPENFILE_FILTER,
              chFilter,
              _countof(chFilter));
   ATLASSERT(L'\0' != *chFilter);

   pFileDlg = new CTaskFileDialog(chFilter,
                                  m_hWnd);

   if ( !pFileDlg )
   {
      /* Failure */
      return;
   }

   __try
   {
      if ( IDCANCEL == pFileDlg->DoModal(m_hWnd) )
      {
         /* User canceled */
         __leave;
      }

      /* Build up the full file path.. */
      if ( 0 == pFileDlg->GetDirectory(chPath,
                                       _countof(chPath)) )
      {
         /* Failure */
         __leave;
      }

      pwszFile = pFileDlg->GetFirstFileName();
      ATLASSERT(NULL != pwszFile);

      while ( pwszFile )
      {
         if ( PathAppend(chPath,
                         _countof(chPath),
                         pwszFile) )
         {
            hr = __pTaskManager->InsertQueueFile(chPath);
            
            if ( SUCCEEDED(hr) && pFileDlg->IncludeAlternateDataStreams )
            {              
               EnumCtx.BaseFileName = chPath;

               FpEnumFileStreamInformation(chPath,
                                           FPF_STREAM_NODEFAULTDATASTREAM,
                                           &CTaskDlg::_EnumFileStreamInfoRoutine,
                                           &EnumCtx);
            }
         }

         PathRemoveFileSpec(chPath);
         pwszFile = pFileDlg->GetNextFileName();
      }
   }
   __finally
   {
      delete pFileDlg;
   }
}

void CTaskDlg::_ShutdownDialog( ) throw()
{
   MSG  Msg;
   BOOL bRemoved;

   ShowWindow(SW_HIDE);

   _SetStatus(TSF_SHUTDOWN);

   /* Just kill this incase it's active. */
   KillTimer(TimerIdShutdown);

   _TrayIcon::Uninitialize();

   /* Unhook our callbacks.. */
   _UninitializeTaskCallbacks();

   /* We need to pull any waiting WM_QUEUEUPDATE and WM_DEFRAGUPDATE messages out
    * of the queue because the TaskManager could be hung, waiting for a response. 
    * The shutdown flag has already been set, so new ones won't be sent. We could
    * also just let the calls to SendMessage(), from _TaskQueueUpdateRoutine() and
    * _TaskDefragUpdateRoutine() die when the window is destroyed, or have them use
    * SendMessageTimeout() but this works faster and is cleaner */ 
   do
   {
      bRemoved = PeekMessage(&Msg,
                             m_hWnd,
                             WM_QUEUEUPDATE,
                             WM_DEFRAGUPDATE,
                             PM_REMOVE);
      
      _dTrace(bRemoved, L"Removed pending %d from message queue during shutdown\n", Msg.message);
   }
   while ( bRemoved );
 
   if ( _hImgList )
   {
      ImageList_Destroy(_hImgList);
      _hImgList = NULL;
   }

   DestroyWindow();
}

bool CTaskDlg::_InitializeTaskCallbacks( ) throw()
{
   HRESULT hr;

   hr = __pTaskManager->SetQueueUpdateRoutine(&CTaskDlg::_TaskQueueUpdateRoutine,
                                              this);

   if ( SUCCEEDED(hr) )
   {
      hr = __pTaskManager->SetDefragUpdateRoutine(&CTaskDlg::_TaskDefragUpdateRoutine,
                                                  this);
   }

   if ( FAILED(hr) )
   {
      _UninitializeTaskCallbacks();

      SetLastError(hr);
      
      /* Failure */
      return ( false );
   }

   /* Success */
   return ( true );
}

void CTaskDlg::_UninitializeTaskCallbacks( ) throw()
{
   __pTaskManager->SetQueueUpdateRoutine(NULL,
                                         NULL);

   __pTaskManager->SetDefragUpdateRoutine(NULL,
                                          NULL);
}

bool CTaskDlg::_InitializeTrayIcon( ) throw()
{
   HICON hIcon;
   WCHAR chTip[256];

   /* Initialize locals */
   ZeroMemory(chTip,
              sizeof(chTip));
   
   /* Load the icon and the tootip message */
   hIcon = reinterpret_cast<HICON>(LoadImage(_pModule->GetModuleInstance(), 
                                             MAKEINTRESOURCE(IDI_FRAGMGX), 
                                             IMAGE_ICON, 
                                             _TrayIcon::IconWidth, 
                                             _TrayIcon::IconHeight, 
                                             LR_LOADMAP3DCOLORS|LR_LOADTRANSPARENT|LR_COPYFROMRESOURCE));
   ATLASSERT(NULL != hIcon);

   if ( !hIcon )
   {
      /* Failure */
      return ( false );
   }

   LoadString(_AtlModule.GetMUIInstance(),
              IDS_TRAY_TIP_DEFAULT,
              chTip,
              _countof(chTip));
   ATLASSERT(L'\0' != *chTip);

   /* Setup the tray icon, it gets added via task requests */
   _TrayIcon::Initialize(m_hWnd, 
                         GetCurrentThreadId(), 
                         WM_TRAYNOTIFY);

   _TrayIcon::SetVersion(NOTIFYICON_VERSION);
   
   _TrayIcon::SetTrayIcon(hIcon);
   _TrayIcon::SetTip(chTip);
   
   /* Success / Failure */
   return ( _TrayIcon::Add() ? true : false );
}

bool CTaskDlg::_InitializeDefragLog( ) throw()
{
#ifdef _DEBUG
   WTL::CEdit ctlLog;
   ctlLog = GetDlgItem(IDC_DEFRAGLOG);
   ctlLog.SetLimitText(8192);
#endif /* _DEBUG */

   /* This always returns success */
   return ( true );
}

bool CTaskDlg::_InitializeProgressBar( ) throw()
{
   WTL::CProgressBarCtrl ctlProg;
   
   ctlProg = GetDlgItem(IDC_DEFRAGPROGRESS);
   ATLASSERT(::IsWindow(ctlProg));
   ctlProg.SetRange(0,
                    100);

   /* This always succeeds */
   return ( true );
}

bool CTaskDlg::_InitializePushPin( ) throw()
{
   WTL::CToolBarCtrl ctlPushPin;
   
   /* Add the sticky button to the toolbar */
   ctlPushPin = GetDlgItem(IDC_PUSHPIN);
   ATLASSERT(::IsWindow(ctlPushPin));      
   
   ctlPushPin.SetUnicodeFormat(TRUE);   
   ctlPushPin.SetButtonStructSize();
   ctlPushPin.SetButtonSize(cxIcon, 
                            cyIcon);

   /* Load and assign the push-pin images */
   _hImgList = ImageList_LoadImage(_pModule->GetModuleInstance(),
                                   MAKEINTRESOURCE(IDB_PUSHPIN),
                                   cxIcon,
                                   0,
                                   RGB(255,0,255),
                                   IMAGE_BITMAP,
                                   0);

   if ( !_hImgList )
   {
      /* Failure */
      return ( false );
   }

   ctlPushPin.SetImageList(_hImgList,
                           0);

   ctlPushPin.InsertButton(0, 
                           IDC_PUSHPIN, 
                           BTNS_CHECK, 
                           TBSTATE_ENABLED, 
                           ImgPinUp, 
                           static_cast<LPCTSTR>(NULL), 
                           0);

   /* Check the registry for the auto-close setting */
   if ( !_bAutoClose )
   {
      ctlPushPin.CheckButton(IDC_PUSHPIN,
                             TRUE);

      /* The image must be set manually when using CheckButton() */
      ctlPushPin.SetButtonInfo(IDC_PUSHPIN, 
                               TBIF_IMAGE, 
                               0, 
                               0,
                               NULL, 
                               ImgPinDown, 
                               0, 
                               0, 
                               0);
   }

   /* This always succeeds */
   return ( true );
}

void CTaskDlg::_SuspendTaskManager( ) throw()
{
   _bSuspend = true;   
   __pTaskManager->Suspend(INFINITE);
}

void CTaskDlg::_ResumeTaskManager( ) throw()
{
   _bSuspend = false;
   __pTaskManager->Resume();
}

void CTaskDlg::_UpdateFileCounters( int cTotal, int cComplete ) throw()
{
   BOOL  bShutdown;
   WCHAR chFmt[128];
   WCHAR chBuf[512];
   
   /* Initialize locals.. */
   bShutdown = FALSE;

   _cFileTotal    += cTotal;
   _cFileComplete += cComplete;

   ATLASSERT(_cFileTotal >= 0);
   ATLASSERT(_cFileComplete >= 0);
   
   if ( _cFileComplete >= _cFileTotal )
   {
      /* Disable the controls that don't need to be now */
      _EnableOperationControls(FALSE);

      /* If no tasks have failed then set a flag to track if we should try the auto-shutdown */
      if ( _bAutoClose )
      {
         bShutdown = TRUE;
      }

      if ( _TrayIcon::IsAdded() )
      {
         LoadString(_AtlModule.GetMUIInstance(),
                    IDS_TRAY_TIP_DEFAULT,
                    chBuf,
                    _countof(chBuf));
         ATLASSERT(L'\0' != (*chBuf));

         _TrayIcon::SetTip(chBuf);
         _TrayIcon::Update();
      }

      _UpdateDefragStatus(IDS_DEFRAGSTATUS_FILECOMPLETED);
      _UpdateDefragProgress(100);
   }

   /* Update the X of X tasks completed label */
   ZeroMemory(chFmt,
              sizeof(chFmt));

   LoadString(_AtlModule.GetMUIInstance(),
              IDS_STATUS_TASKS_PROGRESS, 
              chFmt, 
              _countof(chFmt));
   ATLASSERT(L'\0' != (*chFmt));

   ZeroMemory(chBuf,
              sizeof(chBuf));

   if ( 0 != _FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                            chFmt,
                            0,
                            0,
                            chBuf,
                            _countof(chBuf),
                            _cFileComplete,
                            _cFileTotal) )
   {
      SetDlgItemText(IDC_COMPLETESTATUS, 
                     chBuf);
   }

   /* Tasks are finished... */
   if ( bShutdown )
   {
      SetTimer(static_cast<UINT_PTR>(TimerIdShutdown), 
               ShutdownDelay, 
               NULL);
   }
}

ULONG CTaskDlg::_GetStatus( ) throw()
{
   return ( _wStatus );
}

void CTaskDlg::_SetStatus( ULONG eStatus ) throw()
{
   if ( TSF_SHUTDOWN != _wStatus )
   {
      _wStatus = eStatus;
   }
}

void CTaskDlg::_EnableOperationControls( BOOL bEnable ) throw()
{
   int  idx;
   HWND hWnd;
   UINT rgID[] = {IDC_PAUSE, IDC_CANCEL};

   /* If we're in a suspend state, we can't disable the controls */
   if ( _bSuspend && !bEnable )
   {
      return;
   }

   for ( idx = 0; idx < _countof(rgID); idx++ )
   {
      hWnd = GetDlgItem(rgID[idx]);
      if ( !bEnable && (hWnd == GetFocus()) )
      {
         /* Move focus to the next control, as we're disabling this one.. */
         PostMessage(WM_NEXTDLGCTL,
                     0,
                     FALSE);
      }

      ::EnableWindow(hWnd,
                     bEnable);
   }
}

void CTaskDlg::_UpdateDefragStatus( UINT uResID ) throw()
{
   WCHAR        chBuf[256];
   WTL::CStatic ctlStatus;

   ZeroMemory(chBuf,
              sizeof(chBuf));

   LoadString(_AtlModule.GetMUIInstance(),
              uResID,
              chBuf,
              _countof(chBuf));
   ATLASSERT(L'\0' != (*chBuf));

   ctlStatus = GetDlgItem(IDC_DEFRAGSTATUS);
   ctlStatus.SetWindowText(chBuf);
}

void CTaskDlg::_UpdateDefragProgress( LONG iPercentComplete ) throw()
{
   WCHAR                 chBuf[128];
   WTL::CStatic          ctlPercent;
   WTL::CProgressBarCtrl ctlProgress;

   ZeroMemory(chBuf,
              sizeof(chBuf));

   _FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                  L"%1!d!%%%0",
                  0,
                  0,
                  chBuf,
                  _countof(chBuf),
                  iPercentComplete);

   ctlPercent = GetDlgItem(IDC_DEFRAGPERCENT);
   ctlPercent.SetWindowText(chBuf);

   ctlProgress = GetDlgItem(IDC_DEFRAGPROGRESS);
   ctlProgress.SetPos(static_cast<int>(iPercentComplete));
}

void CTaskDlg::_UpdateDefragLog( UINT uResID, DWORD dwStatus, LPCWSTR pwszFileName, LONG iPercentComplete ) throw()
{
   WTL::CEdit ctlLog;
   DWORD      dwRet;
   WCHAR      chLogFormat[512];
   WCHAR      chSysMessage[1024];
   WCHAR      chLogMessage[2024 + _countof(chLogFormat)];
   int        cchText;
   
   ZeroMemory(chLogFormat,
              sizeof(chLogFormat));

   ZeroMemory(chSysMessage,
              sizeof(chSysMessage));

   /* Load the display format string from the MUI resources */
   LoadString(_AtlModule.GetMUIInstance(),
              uResID,
              chLogFormat,
              _countof(chLogFormat));
   ATLASSERT(L'\0' != (*chLogFormat));

   if ( NO_ERROR != dwStatus ) 
   {
      /* Get the system message for this error, if any */
      dwRet = _FormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_SYSTEM,
                             NULL,
                             dwStatus,
                             MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                             chSysMessage,
                             _countof(chSysMessage));

      if ( 0 == dwRet )
      {
         /* Load the default error */
         LoadString(_AtlModule.GetMUIInstance(),
                    IDS_DEFRAGSTATUS_SYSERRORUNKNOWN,
                    chSysMessage,
                    _countof(chSysMessage));
         ATLASSERT(L'\0' != (*chSysMessage));
      }
   }

   /* Format everything. We always pass the same parameters. It's up to the format string
    * to determine how to display them */
   dwRet = _FormatMessage(FORMAT_MESSAGE_FROM_STRING,
                          chLogFormat,
                          0,
                          0,
                          chLogMessage,
                          _countof(chLogMessage),
                          dwStatus,
                          chSysMessage,
                          pwszFileName,
                          iPercentComplete);

   if ( 0 != dwRet )
   {
      ctlLog = GetDlgItem(IDC_DEFRAGLOG);

      /* Determine if there is room for this update in the edit control.. */
      cchText = ctlLog.GetWindowTextLength();
      if ( (static_cast<UINT>(cchText) + dwRet + 1) > ctlLog.GetLimitText() )
      {
         _SplitDefragLog();
      }

      ctlLog.AppendText(chLogMessage,
                        FALSE,
                        FALSE);
   }
}

void CTaskDlg::_SplitDefragLog( ) throw()
{
   WTL::CEdit ctlLog;
   int        cchText;
   LPWSTR     pwszText;
   LPWSTR     pwszChar;

   ctlLog  = GetDlgItem(IDC_DEFRAGLOG);
   cchText = ctlLog.GetWindowTextLength() + 1;

   pwszChar = NULL;
   pwszText = reinterpret_cast<LPWSTR>(malloc(cchText * sizeof(WCHAR)));
   if ( pwszText )
   {
      ZeroMemory(pwszText,
                 cchText * sizeof(WCHAR));

      if ( 0 != ctlLog.GetWindowText(pwszText,
                                     cchText) )
      {
         /* Find the first empty line at about half the current text. We'll wipe out everything
          * prior to that */
         pwszChar = wcsstr(&(pwszText[cchText / 2]),
                           L"\r\n\r\n");

         if ( pwszChar )
         {
            /* Leave 1 empty new line */
            pwszChar += _charsof(L"\r\n");
         }
      }

      if ( !pwszChar )
      {
         /* We failed to retrieve the text, or couldn't find a double line break, so just
          * assign an empty string */
         pwszChar = L"";
      }
      
      ctlLog.SetWindowText(pwszChar);
      free(pwszText);
   }
   else
   {
      /* Just clear the log.. */
      ctlLog.SetWindowText(L"");
   }
}

DWORD FRAGAPI CTaskDlg::_EnumFileStreamInfoRoutine( ULONG /*StreamCount*/, ULONGLONG /*StreamSize*/, ULONGLONG /*StreamSizeOnDisk*/, LPCWSTR StreamName, PVOID Parameter ) throw()
{
   WCHAR          chPath[MAX_PATH * 2];
   PENUMSTREAMCTX pEnumCtx;
   
   /* Initialize locals.. */
   pEnumCtx = reinterpret_cast<PENUMSTREAMCTX>(Parameter);

   ZeroMemory(chPath,
              sizeof(chPath));

   StringCchCopy(chPath,
                 _countof(chPath),
                 pEnumCtx->BaseFileName);

   if ( SUCCEEDED(StringCchCat(chPath,
                               _countof(chPath),
                               StreamName)) )
   {
      __pTaskManager->InsertQueueFile(chPath);
   }

   return ( NO_ERROR );
}

VOID CALLBACK CTaskDlg::_TaskQueueUpdateRoutine( QUEUEUPDATECODE eUpdateCode, __in_z LPCWSTR pwszFileName, __in_opt PVOID pParameter ) throw()
{
   CTaskDlg* pTaskDlg;
   QUEUEMSG  QueueMsg;

   /* Initialize locals.. */
   pTaskDlg = reinterpret_cast<CTaskDlg*>(pParameter);

   /* Since this is an inter-thread message, we check if the dialog is shutting down first, and
    * skip the call if that's the case */
   if ( TSF_SHUTDOWN != pTaskDlg->_GetStatus() )
   {
      QueueMsg.UpdateCode = eUpdateCode;
      QueueMsg.FileName   = pwszFileName;

      SendMessage(pTaskDlg->m_hWnd,
                  WM_QUEUEUPDATE,
                  0,
                  reinterpret_cast<LPARAM>(&QueueMsg));
   }
}

HRESULT CALLBACK CTaskDlg::_TaskDefragUpdateRoutine( DEFRAGUPDATECODE eUpdateCode, DWORD dwStatus, __in_z LPCWSTR pwszFileName, LONG iPercentComplete, __in_opt PVOID pParameter ) throw()
{
   LRESULT     lRet;
   CTaskDlg*   pTaskDlg;
   DEFRAGMSG   DefragMsg;
   
   /* Initialize locals */
   pTaskDlg  = reinterpret_cast<CTaskDlg*>(pParameter);

   /* If we're in a shutdown state, don't post the update */
   if ( TSF_SHUTDOWN == pTaskDlg->_GetStatus() )
   {
      lRet = ERROR_CANCELLED;     
   }
   else
   {
      DefragMsg.UpdateCode = eUpdateCode;
      DefragMsg.Status     = dwStatus;
      DefragMsg.FileName   = pwszFileName;
      DefragMsg.Percent    = iPercentComplete;

#ifdef _TASKDLG_TESTPAUSE
#ifndef _DEBUG
#error _TASKDLG_TESTPAUSE enabled for non-debug build
#endif /* _DEBUG */

      /* This is to give us a chance to click the Pause button so we can test it */
      if ( iPercentComplete >= 90 )
      {
         while ( !pTaskDlg->_bSuspend )
         {
            _dTrace(1, L"Click pause to test pause code\n");
            Sleep(500);
         }
      }
#endif /* _TASKDLG_TESTPAUSE */

      lRet = SendMessage(pTaskDlg->m_hWnd,
                         WM_DEFRAGUPDATE,
                         0,
                         reinterpret_cast<LPARAM>(&DefragMsg));

#ifdef _TASKDLG_DELAYDEFRAG
#ifndef _DEBUG
#error _TASKDLG_DELAYDEFRAG enabled for non-debug build
#endif /* _DEBUG */
      Sleep(50);
#endif /* _TASKDLG_DELAYDEFRAG */   
   }

   /* Success / Failure */
   return ( (NO_ERROR != lRet) ? __HRESULT_FROM_WIN32(lRet) : S_OK );
}