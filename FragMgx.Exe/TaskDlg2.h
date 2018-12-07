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

/* TaskDlg.h
 *    CTaskDlg class and helpers
 *       The CTaskDlg class is for the main dialog of FragMgx,
 *       which is stored as a DIALOGEX resource
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include <AtlEx.h>
#include <WtlEx.h>
#include <TrayIcon.h>
#include <PathUtil.h>
#include <SpinLock.h>
#include <ShellUtil.h>

#include <FragSvx.h>
#include <FragEng.h>

#include "TaskMgr.h"

inline DWORD __cdecl _FormatMessage( __in DWORD dwFlags, __in LPCVOID lpSource, __in DWORD dwMessageId, __in DWORD dwLanguageId, __out LPWSTR lpBuffer, __in DWORD nSize, ... ) throw()
{
   DWORD   dwRet;
   va_list args;

   va_start(args,
            nSize);

   dwRet = FormatMessage(dwFlags,
                         lpSource,
                         dwMessageId,
                         dwLanguageId,
                         lpBuffer,
                         nSize,
                         &args);

   va_end(args);

   return ( dwRet );
}

/**********************************************************************

    CTaskFileDialog
      Multi-file selection dialog which adds the checkbox for also
      loading any NTFS alternate data streams associated with files

 **********************************************************************/
class CTaskFileDialog : public WTL::CMultiFileDialogImpl<CTaskFileDialog>
{
public:
   typedef WTL::CMultiFileDialogImpl<CTaskFileDialog> _MultiFileDialogImpl;

   CTaskFileDialog( LPCTSTR lpszFilter, HWND hWndParent ) throw();
   BOOL OnFileOK( LPOFNOTIFY lpon ) throw();
   void OnInitDone( LPOFNOTIFY lpon ) throw();

   bool IncludeAlternateDataStreams;

   enum
   {
      CTFD_OPENFILEFLAGS = OFN_ALLOWMULTISELECT|OFN_DONTADDTORECENT|OFN_NOCHANGEDIR|OFN_NODEREFERENCELINKS|OFN_NONETWORKBUTTON|OFN_NOTESTFILECREATE|OFN_NOVALIDATE|OFN_SHAREAWARE
   };
};

class CAboutDialog : public ATL::CDialogImpl<CAboutDialog, CWindow>
{
   /* ATL */
public:
   typedef ATL::CDialogImpl<CAboutDialog, CWindow> _DialogImpl;

   enum
   {
      IDD = IDD_ABOUTBOX
   };

   BEGIN_MSG_MAP(CAboutDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)

      COMMAND_HANDLER(IDOK, BN_CLICKED, OnButtonClicked)
      
      NOTIFY_HANDLER(IDC_LINKFRAGEXT, NM_RETURN,  OnLinkClicked)
      NOTIFY_HANDLER(IDC_LINKFRAGEXT, NM_CLICK,   OnLinkClicked)
   END_MSG_MAP()

   LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnSysCommand( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnButtonClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled ) throw();
   LRESULT OnLinkClicked( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();

   /* CAboutDialog */
private:
   void _FormatTimeDateStamp( DWORD dwTimeDateStamp, LPWSTR pwszBuf, DWORD cchBuf ) throw();
};

/**********************************************************************

    CTaskDlg

 **********************************************************************/

class CTaskDlg : public ATLEx::CCustomDialogImpl<CTaskDlg>,
                 public WTL::CDialogResize<CTaskDlg>,
                 public CTrayIcon<CTaskDlg> 
{
   /* ATL/WTL */
public:
   enum
   {
      IDD             = IDD_DEFRAGMANAGER,
      WM_QUEUEUPDATE  = WM_APP + 74,
      WM_DEFRAGUPDATE = WM_APP + 75,
      WM_TRAYNOTIFY   = WM_APP + 76,

      WM_TASKMIN      = WM_QUEUEUPDATE,
      WM_TASKMAX      = WM_TRAYNOTIFY
   };

   DECLARE_DLG_CLASSNAME(_T(FRAGMGXDLGCLS))

   BEGIN_DLGRESIZE_MAP(CTaskDlg)
      DLGRESIZE_CONTROL(IDC_DEFRAGPERCENT,   DLSZ_MOVE_X)
      DLGRESIZE_CONTROL(IDC_DEFRAGPROGRESS,  DLSZ_SIZE_X)
      DLGRESIZE_CONTROL(IDC_PAUSE,           DLSZ_MOVE_X)
      DLGRESIZE_CONTROL(IDC_CANCEL,          DLSZ_MOVE_X)
      DLGRESIZE_CONTROL(IDC_ADDFILE,         DLSZ_MOVE_X)
      DLGRESIZE_CONTROL(IDC_DEFRAGLOG,       DLSZ_SIZE_X|DLSZ_SIZE_Y)
      DLGRESIZE_CONTROL(IDC_COMPLETESTATUS,  DLSZ_SIZE_X|DLSZ_MOVE_Y)
      DLGRESIZE_CONTROL(IDC_PUSHPIN,         DLSZ_MOVE_X|DLSZ_MOVE_Y)
   END_DLGRESIZE_MAP()

   BEGIN_MSG_MAP(CTaskDlg)
      /* dialog */
      MESSAGE_HANDLER(WM_CREATE,          OnCreate)
      MESSAGE_HANDLER(WM_INITDIALOG,      OnInitDialog)
      MESSAGE_HANDLER(WM_SYSCOMMAND,      OnSysCommand)
      MESSAGE_HANDLER(WM_ENDSESSION,      OnEndSession)
      MESSAGE_HANDLER(WM_TIMER,           OnTimer)
      MESSAGE_HANDLER(WM_DROPFILES,       OnDropFiles)
      MESSAGE_HANDLER(WM_QUEUEUPDATE,     OnQueueUpdate)
      MESSAGE_HANDLER(WM_DEFRAGUPDATE,    OnDefragUpdate)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC,  OnCtlColorStatic)
      MESSAGE_HANDLER(_WmTaskbarCreated,  OnTaskbarCreated)
   
      COMMAND_HANDLER(IDC_PAUSE,    BN_CLICKED, OnButtonClicked)
      COMMAND_HANDLER(IDC_CANCEL,   BN_CLICKED, OnButtonClicked)
      COMMAND_HANDLER(IDC_ADDFILE,  BN_CLICKED, OnButtonClicked)
      COMMAND_HANDLER(IDC_PUSHPIN,  BN_CLICKED, OnPushPinClick)
      
      MENUCOMMAND_RANGE_HANDLER(IDM_TRAYMENUFIRST, IDM_TRAYMENULAST, OnTrayMenuItem)

      CHAIN_MSG_MAP(CDialogResize<CTaskDlg>)
      CHAIN_MSG_MAP(_TrayIcon)
   END_MSG_MAP()

   void RunMessageLoop( ) throw();
   void OnFinalMessage( HWND hWnd ) throw();
   
   LRESULT OnCreate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnSysCommand( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnEndSession( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnTimer( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnDropFiles( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnQueueUpdate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnDefragUpdate( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnCtlColorStatic( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnTaskbarCreated( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
      
   LRESULT OnButtonClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled ) throw();
   LRESULT OnPushPinClick( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled ) throw();
   
   LRESULT OnTrayMenuItem( WORD wItemIndex, WORD wID, HMENU hMenu, BOOL& bHandled ) throw();
   
   LRESULT OnTrayLButtonDblClk( UINT uID, POINT cPos ) throw();
   LRESULT OnTrayRButtonUp( UINT uID, POINT cPos ) throw();
   LRESULT OnTrayContextMenu( UINT uID, POINT cPos ) throw();

   HINSTANCE GetDialogInstance( ) throw();
   
   /* CTaskDlg */
public:
   CTaskDlg( ) throw();

private:
   enum
   {
      /* _dTrace levels */
      eTraceWarn           = 1,
      eTraceError          = 1,
      eTraceInfo           = 0,

      /* Status.bmp image sizes */
      cxIcon               = 16,
      cyIcon               = 16,
      /* Status.bmp image indexes */
      ImgPinDown           = 0,
      ImgPinUp             = 1,

      /* Miliseconds to wait after all operations have completed to destroy this dialog... */
      TimerIdShutdown      = 1900,
#ifdef _DEBUG
      ShutdownDelay        = (1000 * 15),
#else /* _DEBUG */
      ShutdownDelay        = (1000 * 60) * 2,
#endif /* _DEBUG */

      /* _wStatus states */
      TSF_STOPPED          = 1,
      TSF_RUNNING          = 2,
      TSF_SHUTDOWN         = 3,
   };
   
   typedef struct _ENUMSTREAMCTX
   {
      LPCWSTR BaseFileName;
   }ENUMSTREAMCTX, *PENUMSTREAMCTX;

   typedef struct _QUEUEMSG
   {
      QUEUEUPDATECODE UpdateCode;
      LPCWSTR         FileName;
   }QUEUEMSG, *PQUEUEMSG;

   typedef struct _DEFRAGMSG
   {
      DEFRAGUPDATECODE  UpdateCode;
      DWORD             Status;
      LPCWSTR           FileName;
      LONG              Percent;
   }DEFRAGMSG, *PDEFRAGMSG;

   /* Message broadcast by the system when the taskbar is created. We use this to re-add our icon
    * to the system tray when explorer crashes and restarts */
   UINT              _WmTaskbarCreated;
   /* File counters */
   int               _cFileTotal;
   int               _cFileComplete;  
   /* Option/State flags */
   ULONG             _bAutoClose    : 1;
   ULONG             _bSuspend      : 1;
   ULONG             _bCancel       : 1;
   ULONG             _bCancelAll    : 1;
   ULONG             _wStatus;

   /* Image list for the push-pin toolbar */
   HIMAGELIST        _hImgList;

   WTL::CEdit        _ctlLog;

   void _LoadSettings( ) throw();
   void _SaveSettings( ) throw();
   
   void _BrowseForTaskFile( );
   void _ShutdownDialog( );
   
   bool _InitializeTaskCallbacks( ) throw();
   void _UninitializeTaskCallbacks( ) throw();
   
   bool _InitializeTrayIcon( ) throw();
   bool _InitializeDefragLog( ) throw();
   bool _InitializeProgressBar( ) throw();
   bool _InitializePushPin( ) throw();

   void _SuspendTaskManager( ) throw();
   void _ResumeTaskManager( ) throw();

   void _UpdateFileCounters( int cTotal, int cComplete ) throw();
   ULONG _GetStatus( ) throw();
   void _SetStatus( ULONG eStatus ) throw();
   void _EnableOperationControls( BOOL bEnable ) throw();

   void _UpdateDefragStatus( UINT uResID ) throw();

   void _UpdateDefragProgress( LONG iPercentComplete ) throw();
   void _UpdateDefragLog( UINT uResID, DWORD dwStatus, LPCWSTR pwszFileName, LONG iPercentComplete ) throw();
   void _SplitDefragLog( ) throw();

   /* Callback stubs */
   static DWORD FRAGAPI _EnumFileStreamInfoRoutine( ULONG StreamCount, ULONGLONG StreamSize, ULONGLONG StreamSizeOnDisk, LPCWSTR StreamName, PVOID Parameter ) throw();
   static VOID CALLBACK _TaskQueueUpdateRoutine( QUEUEUPDATECODE eUpdateCode, __in_z LPCWSTR pwszFileName, __in_opt PVOID pParameter ) throw();
   static HRESULT CALLBACK _TaskDefragUpdateRoutine( DEFRAGUPDATECODE eUpdateCode, DWORD dwStatus, __in_z LPCWSTR pwszFileName, LONG iPercentComplete, __in_opt PVOID pParameter ) throw();
};
