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
 
/* FolderDlg.h
 *    Declaration of CFxFolderDialog
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 */

#pragma once

#include <AtlEx.h>
using namespace ATLEx;
#include <PathUtil.h>

/**********************************************************************

    CFxFolderDialog
      Dialog for choosing a list of directories for which to process
      child files.

 **********************************************************************/


typedef void (*PFOLDERSELECTPROC)( LPCWSTR pwszFolder, BOOL bChecked, PVOID pParameter );
typedef struct _FOLDERINFO
{
   HWND              hwndParent;
   int               cFolders;
   LPCWSTR*          rgszFolders;
   PFOLDERSELECTPROC pCallback;
   PVOID             pParameter;
}FOLDERINFO, *PFOLDERINFO;

class CFxFolderDialog : public ATL::CDialogImpl<CFxFolderDialog, CWindow>,
                        public WTL::CDialogResize<CFxFolderDialog>
{
      /* ATL/WTL */
public:
   enum
   {
      IDD = IDD_FOLDERSELECT
   };

   BEGIN_DLGRESIZE_MAP(CFxFolderDialog)
      DLGRESIZE_CONTROL(IDC_LSVFOLDERLIST,   DLSZ_SIZE_X|DLSZ_SIZE_Y)
      DLGRESIZE_CONTROL(IDOK,                DLSZ_MOVE_X|DLSZ_MOVE_Y)
   END_DLGRESIZE_MAP()

   BEGIN_MSG_MAP(CFxFolderDialog)
      /* dialog */
      MESSAGE_HANDLER(WM_INITDIALOG,      OnInitDialog)
      MESSAGE_HANDLER(WM_SYSCOMMAND,      OnSysCommand)
      MESSAGE_HANDLER(WM_ENDSESSION,      OnEndSession)
      
      COMMAND_HANDLER(IDOK,      BN_CLICKED, OnButtonClicked)
      COMMAND_HANDLER(IDCANCEL,  BN_CLICKED, OnButtonClicked)

      NOTIFY_HANDLER(IDC_LSVFOLDERLIST, LVN_ITEMCHANGED,  OnListViewItemChanged)
      NOTIFY_HANDLER(IDC_LSVFOLDERLIST, NM_RCLICK,        OnListViewRightClick)

      MENUCOMMAND_HANDLER(IDM_FOLDERSELCHECK,   OnFolderListMenuClick)
      MENUCOMMAND_HANDLER(IDM_FOLDERSELUNCHECK, OnFolderListMenuClick)
      
      CHAIN_MSG_MAP(CDialogResize<CFxFolderDialog>)
   END_MSG_MAP()

   LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnSysCommand( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnEndSession( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   
   LRESULT OnButtonClicked( WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled ) throw();

   LRESULT OnListViewItemChanged( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnListViewRightClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();

   LRESULT OnFolderListMenuClick( WORD wItemIndex, WORD wID, HMENU hMenu, BOOL& bHandled ) throw();

   /* CFxFolderDialog */
public:
   static void ShowFolderSelectionDialog( PFOLDERINFO pInfo ) throw();

protected:
   CFxFolderDialog( ) throw();
   
   /* Internal */
   bool _InitializeFolderList( ) throw();

   /* Assigned via ShowFolderSelectionDialog() */
   FOLDERINFO _Info;

   WTL::CCheckListViewCtrl _ctlFolderList;

};