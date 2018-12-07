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
 
/* FolderDlg.cpp
 *    CFxFolderDialog implementation
 * 
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "FolderDlg.h"

/**********************************************************************

    CFxFolderDialog : ATL/WTL

 **********************************************************************/

LRESULT CFxFolderDialog::OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/ )
{
   PFOLDERINFO pInfo;

   /* Extract the create param */
   pInfo = reinterpret_cast<PFOLDERINFO>(lParam);

   /* Setup our local copy of the folder info. We don't populate the callback until after everything
    * is initialized because startup code uses it as a flag to indicate that we shouldn't be posting
    * callbacks yet */
   _Info.hwndParent  = pInfo->hwndParent;
   _Info.cFolders    = pInfo->cFolders;
   _Info.rgszFolders = pInfo->rgszFolders;
   
   /* Setup the auto-resize stuff */
   DlgResize_Init(true, 
                  true, 
                  0);

   /* Setup the check list */
   if ( !_InitializeFolderList() )
   {
      /* Failure */
      EndDialog(-1);
   }

   _Info.pParameter = pInfo->pParameter;
   _Info.pCallback  = pInfo->pCallback;
   
   return ( TRUE );
}

LRESULT CFxFolderDialog::OnSysCommand( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled ) throw()
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

LRESULT CFxFolderDialog::OnEndSession( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
   EndDialog(-1);

   return ( TRUE );
}

LRESULT CFxFolderDialog::OnButtonClicked( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ ) throw()
{
   EndDialog(0);
   return ( TRUE );
}

LRESULT CFxFolderDialog::OnListViewItemChanged( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   LPNMLISTVIEW pNmListView;

   /* Initialize locals */
   pNmListView = reinterpret_cast<LPNMLISTVIEW>(pnmh);

   if ( -1 != pNmListView->iItem )
   {
      if ( !_Info.pCallback )
      {
         /* Success */
         return ( 0 );
      }

      /* Determine if this item has had it's check state changed */
      if ( (pNmListView->uOldState & LVIS_STATEIMAGEMASK) != (pNmListView->uNewState & LVIS_STATEIMAGEMASK) )
      {
         _Info.pCallback(reinterpret_cast<LPCWSTR>(pNmListView->lParam),
                         INDEXTOSTATEIMAGEMASK(1) == (pNmListView->uNewState & LVIS_STATEIMAGEMASK) ? FALSE : TRUE,
                         _Info.pParameter);
      }
   }

   /* Success */
   return ( TRUE );
}


LRESULT CFxFolderDialog::OnListViewRightClick( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   LPNMITEMACTIVATE pItemActivate;
   HMENU            hMenu;
   HMENU            hPopup;
   POINT            cpPos;
   UINT             uMenuCmd;
   MENUITEMINFO     MenuItemInfo;
   
   /* We don't run on anything less than 4.71, so this is safe to assume */
   pItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);

   hMenu = LoadMenu(_AtlModule.GetMUIInstance(),
                    MAKEINTRESOURCE(IDR_FOLDERSELCONTEXT));

   if ( !hMenu )
   {
      _dTrace(L"Failed to load menu(IDR_FOLDERSELCONTEXT)!%#08lx\n", GetLastError());
      /* Failure */
      return ( 0 );
   }

   hPopup = GetSubMenu(hMenu,
                       0);

   cpPos = pItemActivate->ptAction;
   _ctlFolderList.ClientToScreen(&cpPos);

   uMenuCmd = static_cast<UINT>(TrackPopupMenuEx(hPopup,
                                                 TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD, 
                                                 FixTrackMenuPopupX(cpPos.x, cpPos.y), 
                                                 cpPos.y, 
                                                 m_hWnd, 
                                                 NULL));

   if ( 0 != uMenuCmd )
   {
      ZeroMemory(&MenuItemInfo,
                 sizeof(MenuItemInfo));

      MenuItemInfo.cbSize     = sizeof(MenuItemInfo);
      MenuItemInfo.fMask      = MIIM_DATA;
      MenuItemInfo.dwItemData = pItemActivate->iItem;

      SetMenuItemInfo(hPopup,
                      uMenuCmd,
                      FALSE,
                      &MenuItemInfo);

      SendMessage(WM_MENUCOMMAND,
                  MAKELONG(uMenuCmd, uMenuCmd - IDR_FOLDERSELCONTEXT + 1),
                  reinterpret_cast<LONG_PTR>(hPopup));
   }

   DestroyMenu(hMenu);
   
   /* Success */
   return ( static_cast<LRESULT>(TRUE) );
}

LRESULT CFxFolderDialog::OnFolderListMenuClick( WORD /*wItemIndex*/, WORD wID, HMENU /*hMenu*/, BOOL& /*bHandled*/ ) throw()
{
   BOOL bCheck;
   int  idx;

   switch ( wID )
   {
      case IDM_FOLDERSELCHECK:
      case IDM_FOLDERSELUNCHECK:
         bCheck = (IDM_FOLDERSELCHECK == wID) ? TRUE : FALSE;

         for ( idx = 0; idx < _Info.cFolders; idx++ )
         {
            _ctlFolderList.SetCheckState(idx,
                                         bCheck);
         }

         break;
   }

   return ( TRUE );
}

/**********************************************************************

    CFxFolderDialog : CFxFolderDialog

 **********************************************************************/

CFxFolderDialog::CFxFolderDialog( ) throw()
{
   ZeroMemory(&_Info,
              sizeof(FOLDERINFO));
}

void CFxFolderDialog::ShowFolderSelectionDialog( PFOLDERINFO pInfo ) throw()
{
   CFxFolderDialog dlg;

   dlg.DoModal(pInfo->hwndParent,
               reinterpret_cast<LPARAM>(pInfo));
}

bool CFxFolderDialog::_InitializeFolderList( ) throw()
{  
   RECT     rcClient;
   LVCOLUMN lvColumn;
   WCHAR    chColumnText[128];   
   int      idx;

   /* Hookup the class */
   _ctlFolderList.SubclassWindow(GetDlgItem(IDC_LSVFOLDERLIST));

   /* The folder list will have a single column, unnamed that doesn't show up but
    * is necessary for adding items and allowing the user to resize the view */
   _ctlFolderList.GetClientRect(&rcClient);

   /* Load the column's text */
   ZeroMemory(chColumnText,
              sizeof(chColumnText));
   LoadString(_AtlModule.GetMUIInstance(),
              IDS_FOLDERSELECT_COLPATH, 
              chColumnText, 
              _countof(chColumnText));
   ATLASSERT(L'\0' != *chColumnText);

   ZeroMemory(&lvColumn,
              sizeof(LVCOLUMN));
   lvColumn.mask     = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
   lvColumn.fmt      = LVCFMT_LEFT;
   lvColumn.cx       = rcClient.right - rcClient.left;
   lvColumn.pszText  = chColumnText;
   lvColumn.iSubItem = 0;

   _ctlFolderList.InsertColumn(1,
                               &lvColumn);

   /* Insert all the folders. Default is checked */
   for ( idx = 0; idx < _Info.cFolders; idx++ ) 
   {
      _ctlFolderList.InsertItem(LVIF_PARAM|LVIF_TEXT,
                                idx, 
                                _Info.rgszFolders[idx],
                                0,
                                0,
                                0,
                                reinterpret_cast<LPARAM>(_Info.rgszFolders[idx]));

      _ctlFolderList.SetCheckState(idx,                                   
                                   TRUE);
   }

   return ( true );
}