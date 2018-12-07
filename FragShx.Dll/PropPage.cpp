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
 
/* PropPage.cpp
 *    CFxPropertyPage implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 */

#include "Stdafx.h"
#include "Common.h"
#include "PropPage.h" 

#include <WtlEx.h>
using namespace WTLEx;
#include <RegUtil.h>
#include <PathUtil.h>
#include <StreamUtil.h>
#include <NumberFmt.h>

#include <FragMgx.h>

/**********************************************************************
   TODO - CFxPropertyPage

   2K/XP - [BUG] Double clicking a column seperator discards the overlay mask
           for icons using it until the item is refreshed.
   2K/XP - [CR] Add better support for UNC paths. Currently, only works with
           paths < MAX_STREAM_PATH
 **********************************************************************/

/**********************************************************************

   ATL/WTL

 **********************************************************************/

HRESULT CFxPropertyPage::FinalConstruct( ) throw()
{
   DWORD dwMajor;
   DWORD dwMinor;
   
   /* We also need to change CPropertyPageImpl::m_psp::hInstance here because
    * the resource DLL may not have been loaded when its constructor was
    * called, causing it to use this DLL's instance instead */
   m_psp.hInstance = _AtlModule.GetMUIInstance();
   
   AtlGetCommCtrlVersion(&dwMajor,
                         &dwMinor);

   if ( dwMajor >= 6 )
   {
      _bIsCC6 = 1;
   }
   
   /* Common Controls versions prior to 5.8 do not call OnPageAddRef(), so for those
    * libraries it is done manually */
   if ( MAKELONG(dwMinor, dwMajor) < MAKELONG(8,5) )
   {
      OnPageAddRef();
   }

   /* Success */
   return ( S_OK );
}

void CFxPropertyPage::FinalRelease( )
{
   _ResetData();
}

/**********************************************************************

   CFxPropertyPage

 **********************************************************************/

CFxPropertyPage::CFxPropertyPage( ) throw() : _ctlListView(this, 1)
{
   _cFileInfo           = 0;
   _rgFileInfo          = NULL;
   _cItemInfo           = 0;
   _rgItemInfo          = NULL;
   _iCurSel             = -1;
   _clrCompressed       = RGB(0,0,255);
   _hStreamIco          = NULL;
   _wSortCol            = 0;
   _bSortASC            = 0;
   _bIsCC6              = 0;
   _bShowCompressed     = 0;
   _bShowFileStreams    = 0;
   _bShowColSize        = 0;
   _bShowColPercentage  = 0;
   _bShowColClusters    = 0;
   _bShowColExtents     = 0;
   _bMultiFilePage      = 0;

   /* Initialize the column map with bogus column values */
   FillMemory(_bColMap,
              sizeof(_bColMap),
              0xff);

   /* Load settings from the registry.. */
   _LoadSettings();
}

LRESULT CFxPropertyPage::OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled )
{
   WTL::CListViewCtrl ctlListView;
   WTL::CHeaderCtrl   ctlHeader;

   /* We subclass the listview so scroll messages can be caught for updating the gradient and 
    * so we can use custom double-buffering to be consistant across all platforms */
   ctlListView = GetDlgItem(IDC_LSVFRAGMENTS);
   _ctlListView.SubclassWindow(ctlListView);

   /* Set the properties on the listview that can't be done in the .RC file */
   ctlListView.SetUnicodeFormat(TRUE);
   /* Overwrite all extended list view styles. For CC6+ we use its internal double buffering,
    * but for previous versions we handle it ourselves */
   ctlListView.SetExtendedListViewStyle((_bIsCC6 ? /*LVS_EX_DOUBLEBUFFER*/ 0: 0)|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP|LVS_EX_INFOTIP|LVS_EX_LABELTIP, 
                                        0);

   /* Drawing code expects that the header will be clipped */
   ATLASSERT(0 != (WS_CLIPCHILDREN & _ctlListView.GetStyle()));
   
   /* Set the properties on the header that can't be done in the .RC file */
   ctlHeader = _ctlListView.GetHeader();   
   ctlHeader.SetWindowLongPtr(GWL_STYLE, 
                              HDS_FULLDRAG|ctlHeader.GetStyle());
   ctlHeader.SetWindowLongPtr(GWLP_ID, 
                              IDC_HDRFRAGMENTS);
   ctlHeader.SetUnicodeFormat(TRUE);

   /* Initialize the shell image list. If this fails, oh well */
   _ctlShellIL.Load(false);
   ATLASSERT(!_ctlShellIL.IsNull());

   /* Initialize the columns */
   _UpdateColumns(true);

   /* Simulate a theme change so theme related data can be initialized */
   OnThemeChanged(WM_THEMECHANGED, 
                  0, 
                  0, 
                  bHandled);

   /* Load the frags */
   OnUpdateControl(WM_UPDATECONTROL, 
                   (WPARAM)FXUC_REFRESHDATA|FXUC_REFRESHLIST, 
                   0,
                   bHandled);
   
   /* Success */
   return ( TRUE );
}

LRESULT CFxPropertyPage::OnSize( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled ) throw()
{
   int               cxParam;
   int               cyParam;
   RECT              rcLabel;
   WTL::CStatic      ctlLabel;
   WTL::CHeaderCtrl  ctlHeader;
   RECT              rcItem;
   RECT              rcHeader;
   int               cyItem;
   int               cyHeader;
   int               cyBorder;
   int               cyListView;
   int               cxHeader;
   int               cxScroll;
   int               cColumns;
   HDITEM            hdItem;
   int               idx;
 
   /* Spacing required around the list-view, in DLUs */
   RECT rcPadding = 
   {
      7, /* Left   */
      2, /* Top    */
      7, /* Right  */
      5  /* Bottom */
   };

   /* Dividends used to adjust the width of header columns. Each index is the
      column position. 
      
         ColumnWidth = (RemainingWidth / Value)
      
      The value 0 means not shown. Column 1 is always shown, while the rest may 
      be turned off which results in 5 possible width layouts. 
      
      When this method is called, if 3 columns are visible then the 3rd element 
      is used. The first column is 14% of the full width, the 2nd is 50% of the 
      remaining width, and the 3rd is 100% of the full remaining width */
   static const int rgSize[][5] = 
   {
   /* Col1  Col2  Col3  Col4  Col5 */
      {5,   0,    0,    0,    0},
      {6,   2,    0,    0,    0},
      {7,   2,    1,    0,    0}, 
      {8,   2,    2,    1,    0},
      {9,   3,    3,    2,    1}
   };
  
   /* Convert the DLU padding to pixels */
   MapDialogRect(&rcPadding);
   
   /* Extract the width & height of the dialog */
   cxParam = GET_X_LPARAM(lParam);
   cyParam = GET_Y_LPARAM(lParam);
   
   /* Stretch the label control to fill the dialog width, excluding padding */
   ctlLabel = GetDlgItem(IDC_STCFRAGMENTS);
   ctlLabel.GetWindowRect(&rcLabel);
   ScreenToClient(&rcLabel);
   ctlLabel.SetWindowPos(NULL, 
                         0, 
                         0, 
                         cxParam - rcPadding.left - rcPadding.right, 
                         rcLabel.bottom - rcLabel.top,
                         SWP_NOZORDER|SWP_NOMOVE|SWP_NOACTIVATE);

   /* Adjust the listview to the width of the dialog - spacing, and the 
    * maximum "approximate view rect" - spacing */
   _ctlListView.GetItemRect(0, 
                            &rcItem, 
                            LVIR_BOUNDS);
   
   ctlHeader = _ctlListView.GetHeader();   
   ctlHeader.GetWindowRect(&rcHeader);
   
   cyItem   = (rcItem.bottom - rcItem.top);
   cyHeader = (rcHeader.bottom - rcHeader.top);
   /* Non-CC6 borders are a little thicker and need to be adjusted for */
   cyBorder = ((_bIsCC6 ? 2 : 3) * GetSystemMetrics(SM_CYEDGE));

   /* Adjust the listview control. Align the bottom of the listview control to the
    * item height. This ensures that no partial items are shown at the bottom */
   cyListView = (cyParam - rcPadding.bottom) - (rcLabel.bottom + rcPadding.top);
   cyListView = ((((cyListView - (cyHeader + cyBorder)) / cyItem) * cyItem) + cyHeader + cyBorder);

   _ctlListView.SetWindowPos(NULL, 
                             rcPadding.left, 
                             rcLabel.bottom + rcPadding.top, 
                             cxParam - rcPadding.right - rcPadding.left, 
                             cyListView, 
                             SWP_NOZORDER|SWP_NOACTIVATE);

   /* Adjust the columns for maximum width */
   ctlHeader.GetClientRect(&rcHeader);
   cxHeader = (rcHeader.right - rcHeader.left);

   /* Adjust for scrollbars */
   if ( WS_VSCROLL & _ctlListView.GetStyle() )
   {
      cxScroll  = (_ctlTheme.IsThemeNull() ? GetSystemMetrics(SM_CXVSCROLL) : _ctlTheme.GetThemeSysSize(SM_CXVSCROLL));
      cxHeader -= cxScroll;
   }
   
   cColumns = ctlHeader.GetItemCount();

#ifdef _DEBUG
   /* Leave 68 pixels for the item data column */
   enum { cxItemInfoCol = 48 };
   cxHeader -= cxItemInfoCol;
   cColumns -= 1;
#endif /* _DEBUG */
   ATLASSERT(cColumns <= _countof(rgSize));


   ZeroMemory(&hdItem,
              sizeof(hdItem));

   for ( idx = 0; idx < cColumns; idx++ )
   {
      ATLASSERT(0 != rgSize[cColumns - 1][idx]);
      hdItem.mask = HDI_WIDTH;
      hdItem.cxy  = (cxHeader / rgSize[cColumns - 1][idx]);
      ctlHeader.SetItem(idx, 
                        &hdItem);

      /* Reduce the available width for the next iteration */
      cxHeader -= hdItem.cxy;      
   }

#ifdef _DEBUG
   /* Set the width of the item info column */
   hdItem.mask = HDI_WIDTH;
   hdItem.cxy  = cxItemInfoCol;
   ctlHeader.SetItem(cColumns, 
                     &hdItem);
#endif /* _DEBUG */

   /* Recreate the gradient bitmap */
   _CreateCachedGroupSeperator();

   /* Allow default processing */
   bHandled = FALSE;

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnSettingChange( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled ) throw()
{
   if ( 0 == wParam )
   {
      /* Refresh the fragment data because the user may have turned on/off a setting that
       * controls what is collected */
      PostMessage(WM_UPDATECONTROL, 
                  FXUC_REFRESHDATA|FXUC_REFRESHLIST, 
                  0);
   }

   /* Allow for default processing of this message */
   bHandled = FALSE;

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnSysColorChange( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled ) throw()
{
   WTL::CHeaderCtrl ctlHeader;
   int              cColumns;
   int              iColumn;
   HDITEM           hdItem;

   /* If this isn't CC6, reload any bitmaps on the columns. */
   if ( !_bIsCC6 )
   {
      ctlHeader = _ctlListView.GetHeader();
      cColumns  = ctlHeader.GetItemCount();
      
      ZeroMemory(&hdItem,
                 sizeof(hdItem));

      for ( iColumn = 0; iColumn < cColumns; iColumn++ )
      {
         /* If the column has a bitmap, reload it */
         hdItem.mask = HDI_BITMAP|HDI_FORMAT;
         hdItem.hbm  = NULL;

         if ( ctlHeader.GetItem(iColumn,
                                &hdItem) && (hdItem.hbm) )
         {
            hdItem.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
            hdItem.hbm  = reinterpret_cast<HBITMAP>(LoadImage(GetModuleHandle(L"SHELL32.DLL"), 
                                                              MAKEINTRESOURCE(133 + (_bSortASC ? 0 : 1)), 
                                                              IMAGE_BITMAP,
                                                              0,
                                                              0,
                                                              LR_LOADMAP3DCOLORS|LR_SHARED));

            ctlHeader.SetItem(iColumn, 
                              &hdItem);
         }
      }
   }

   /* Recreate the gradient bitmap */
   _CreateCachedGroupSeperator();

   /* Assign the compressed/sparse color from the system */
   _LoadAltColorSetting();

   /* Load the NTFS stream overlay icon. This icon is stored in the neutral library */
   _hStreamIco = reinterpret_cast<HICON>(LoadImage(_pModule->GetModuleInstance(),
                                                   MAKEINTRESOURCE(IDI_STREAMOVERLAY),
                                                   IMAGE_ICON,
                                                   _ctlShellIL.csIcon.cx,
                                                   _ctlShellIL.csIcon.cy,
                                                   LR_DEFAULTCOLOR));
   ATLASSERT(NULL != _hStreamIco);

   /* Refresh the fragment data. This could be lengthy, so do it async */
   PostMessage(WM_UPDATECONTROL, 
               (WPARAM)FXUC_REFRESHLIST, 
               0);

   /* Allow default processing, which is important for CC6 as it handles
    * drawing the sorting arrow */
   bHandled = FALSE;

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnThemeChanged( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled ) throw()
{
   /* Reload any theme data. This needs to be checked because the dialog will call this as a wrapper for 
    * refreshing all theme/color data */
   if ( !_ctlTheme.IsThemeNull() )
   {
      _ctlTheme.CloseThemeData();
   }

   if ( _ctlTheme.IsThemingSupported() )
   {
      _ctlTheme.OpenThemeData(m_hWnd, 
                              L"LISTVIEW");
   }

   /* Simulate a color change to refresh color data */
   OnSysColorChange(WM_SYSCOLORCHANGE, 
                    0, 
                    0, 
                    bHandled);

   /* Allow default processing */
   bHandled = FALSE;

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnUpdateControl( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/ ) throw()
{
   int         iItem;
   CWaitCursor cWait(true);
   
   ATLASSERT(0 != wParam);

   /* Save the item which is current being displayed at the top so the listview
    * can be scrolled to the same position after the data is updated */
   iItem = _ctlListView.GetTopIndex();

   /* Reload the data */
   _UpdateControl(static_cast<DWORD>(wParam));

   if ( (-1 != iItem) && (0 != iItem) )
   {      
      /* The count-per-page is 1 based, so 1 is subtracted to return it to
       * a 0 based result */
      _ctlListView.EnsureVisible(iItem + _ctlListView.GetCountPerPage() - 1, 
                                 FALSE);
   }

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnListViewGetDispInfo( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   LPLVITEM       pLvItem;
   UINT           iItem;
   PITEMINFO      pItemInfo;
   PFILEINFO      pFileInfo;
   DWORD          dwErr;
   FRAGMENT_INFORMATION   FragInfo;
   FRAGCTX_INFORMATION    CtxInfo;
   double         dbPercentOnDisk;

   CNUMBERFMT     NumberFmt;
   WCHAR          chBuf[1024];

   /* Initialize locals */
   pLvItem   = &((NMLVDISPINFO*)pnmh)->item;
   iItem     = static_cast<UINT>(pLvItem->iItem);
   pItemInfo = _GetItemInfo(iItem);
   
   ZeroMemory(chBuf,
              sizeof(chBuf));

   ZeroMemory(&CtxInfo,
              sizeof(CtxInfo));

   /* We only set text for frag records */
   if ( (LVIF_TEXT & pLvItem->mask) && _IsFragInfoItem(iItem) )
   {  
      pFileInfo = _GetItemFileInfo(iItem);
      ATLASSERT(NULL != pFileInfo);
      
      /* Get fragment data used by this method */
      dwErr = FpGetContextFragmentInformation(pFileInfo->hFragCtx,
                                              pItemInfo->FragIndex,
                                              &FragInfo,
                                              1,
                                              NULL);
      ATLASSERT(NO_ERROR == dwErr || ERROR_MORE_DATA == dwErr);

      dwErr = FpGetContextInformation(pFileInfo->hFragCtx,
                                      &CtxInfo);
      ATLASSERT(NO_ERROR == dwErr);
      
      NumberFmt.Initialize(LOCALE_USER_DEFAULT);

      switch ( _bColMap[pLvItem->iSubItem] )
      {
         /* # */
         case MapIdSequence:
         {
            NumberFmt.NumDigits = 0;

            if ( GetLocaleFormattedNumber(L"%u", 
                                          chBuf, 
                                          _countof(chBuf), 
                                          NumberFmt, 
                                          FragInfo.Sequence) <= 0 )
            {
               /* Failed to format */
               InitializeString(chBuf, 
                                L"<?>");
            }

            break;
         }

         /* Size */
         case MapIdSize:
         {
            if ( !StrFormatByteSizeW(FragInfo.ClusterCount * static_cast<LONGLONG>(CtxInfo.ClusterSize), 
                                     chBuf, 
                                     _countof(chBuf)) )
            {
               InitializeString(chBuf, 
                                L"<?>");
            }            

            break;
         }

         /* % of File */
         case MapIdPercentage:
         {
            if ( IsVirtualClusterNumber(FragInfo.LogicalCluster) )
            {
               NumberFmt.NumDigits = 0;

               if ( GetLocaleFormattedNumber(L"%d", 
                                             chBuf, 
                                             _countof(chBuf), 
                                             NumberFmt, 
                                             0) <= 0 )
               {
                  InitializeString(chBuf, 
                                   L"0");
               }
            }
            else
            {

               if ( 0 == CtxInfo.FileSizeOnDisk )
               {
                  dbPercentOnDisk = 100.0f;
               }
               else
               {
                  dbPercentOnDisk  = (static_cast<double>(FragInfo.ClusterCount) * static_cast<double>(CtxInfo.ClusterSize));
                  dbPercentOnDisk /= static_cast<double>(CtxInfo.FileSizeOnDisk);
                  dbPercentOnDisk *= 100.0f;
               }

               /* NLS doesn't support more than 9 fractional digits, so we always the max (DBL_DIG)
                * and let NLS truncate */
               if ( GetLocaleFormattedNumber(L"%.15f", 
                                             chBuf, 
                                             _countof(chBuf), 
                                             NumberFmt, 
                                             dbPercentOnDisk) <= 0 )
               {
                  /* Failed to format */
                  InitializeString(chBuf, 
                                   L"<?>");
               }
            }
          
            break;
         }

         /* Clusters */
         case MapIdClusters:
         {
            NumberFmt.NumDigits = 0;

            if ( GetLocaleFormattedNumber(L"%u", 
                                          chBuf, 
                                          _countof(chBuf), 
                                          NumberFmt, 
                                          FragInfo.ClusterCount) <= 0 )
            {
               /* Failed to format */
               InitializeString(chBuf, 
                                L"<?>");
            }

            break;
         }

         /* Extents */
         case MapIdExtents:
         {
            /* No decimals */
            NumberFmt.NumDigits = 0;

            if ( GetLocaleFormattedNumber(L"%d", 
                                          chBuf, 
                                          _countof(chBuf), 
                                          NumberFmt, 
                                          FragInfo.ExtentCount) <= 0 )
            {
               /* Failed to format */
               InitializeString(chBuf, 
                                L"<?>");
            }

            break;

         }

      #ifdef _DEBUG
         case MapIdItemInfo:
         {
            StringCchPrintf(chBuf, 
                            _countof(chBuf), 
                            L"%d/%d", 
                            pItemInfo->FileIndex,
                            pItemInfo->FragIndex);

            break;
         }
      #endif /* _DEBUG */

         DEFAULT_UNREACHABLE;
      }            

      StringCchCopy(pLvItem->pszText, 
                    pLvItem->cchTextMax, 
                    chBuf);
   }

   /* Success */
   return ( 0 );   
}

LRESULT CFxPropertyPage::OnListViewGetInfoTip( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   LPNMLVGETINFOTIP pLvInfoTip;
   UINT             iItem;
   PFILEINFO        pFileInfo;
   int              cchTip;

   pLvInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pnmh);
   iItem      = static_cast<UINT>(pLvInfoTip->iItem);

   /* InfoTip's are only shown for file items */
   if ( _IsFileInfoItem(iItem) )
   {
      pFileInfo = _GetItemFileInfo(iItem);

      /* Load the prefix */
      cchTip = LoadString(_AtlModule.GetMUIInstance(),
                          FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags ? IDS_TIP_FILESTREAM : IDS_TIP_FILE,
                          pLvInfoTip->pszText,
                          pLvInfoTip->cchTextMax);
      ATLASSERT(cchTip > 0);

      /* Append the full path of the file */
      _GetFullFilePath(pFileInfo, 
                       pLvInfoTip->pszText + cchTip, 
                       pLvInfoTip->cchTextMax - cchTip);
   }
   
   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnListViewCustomDraw( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{  
   LPNMLVCUSTOMDRAW  pLvCustomDraw;
   UINT              iItem;
   FRAGMENT_INFORMATION      FragInfo;

   HRESULT           hr;
   RECT              rcItem;
   RECT              rcLvClient;
   PFILEINFO         pFileInfo;
   int               cxEdge;
   int               cyEdge;
   int               xGrad;
   int               cyItem;
   int               cxScroll;

   int               cyText;
   int               iBkMode;
   RECT              rcFill;
   RECT              rcExtent;

   HDC               hCompDC;
   HBITMAP           hbmOld;
   SIZE              csImage;

   /* Initialze locals */
   pLvCustomDraw = (LPNMLVCUSTOMDRAW)pnmh;
   iItem         = static_cast<UINT>(pLvCustomDraw->nmcd.dwItemSpec);

   /* Determine if custom drawing needs to be done or not */
   switch ( pLvCustomDraw->nmcd.dwDrawStage )
   {
      case CDDS_PREPAINT:
      case CDDS_PREERASE:
         /* Success */
         return ( CDRF_NOTIFYITEMDRAW );

      case CDDS_ITEMPREPAINT:
         if ( _IsFragInfoItem(iItem) )
         {
            /* When showing unallocated ranges, we show such blocks with the compressed color */
            if ( _bShowCompressed )
            {
               _GetItemFragmentInfo(iItem,
                                    &FragInfo);

               if ( IsCompressionFragment(&FragInfo) )
               {
                  pLvCustomDraw->clrText = _clrCompressed;
                  
                  /* Success - Let the listview do the drawing with the updated color */
                  return ( CDRF_DODEFAULT|CDRF_NEWFONT );
               }
            }

            /* Success - Let the listview do the drawing */
            return ( CDRF_DODEFAULT );
         }

         /* This is an item we want to paint */
         break;

      default:
         /* Success - Not handling this notification */
         return ( CDRF_DODEFAULT );
   }
   

   /* Initialize drawing data */
   pFileInfo = _GetItemFileInfo(iItem);
   
   _ctlListView.GetClientRect(&rcLvClient);

   _ctlListView.GetItemRect(static_cast<int>(pFileInfo->iItemBase),
                            &rcItem,
                            LVIR_BOUNDS);

   cxEdge = GetSystemMetrics(SM_CXEDGE);
   cyEdge = GetSystemMetrics(SM_CYEDGE);
   xGrad  = rcItem.left;
   cyItem = (rcItem.bottom - rcItem.top);

   if ( WS_VSCROLL & _ctlListView.GetStyle() )
   {
      cxScroll = (_ctlTheme.IsThemeNull() ? GetSystemMetrics(SM_CXVSCROLL) : _ctlTheme.GetThemeSysSize(SM_CXVSCROLL));
   }
   else
   {
      cxScroll = 0;
   }

   /* Use the full width of the list-view for drawing text, less some padding */
   rcItem.right = (rcLvClient.right - rcLvClient.left - (4 * cxEdge) - cxScroll);
   /* Adjust the bottom of the drawing rect to use all the extra items */
   rcItem.bottom += (static_cast<LONG>(FILEINFO::CGROUPITEMSEXTRA - 1) * cyItem);

   /* If the icon for the file is known, draw it */
   if ( !_ctlShellIL.IsNull() && (-1 != pFileInfo->iIconIndex) )
   {
      /* Put some padding between the icon and the border */
      rcItem.left += (2 * cxEdge);      
      /* Center the icon vertically in the group items */
      rcItem.top += (((rcItem.bottom - rcItem.top) / 2) - (_ctlShellIL.csIcon.cy / 2));
      
      if ( _ctlShellIL.DrawFileInfoIcon(pFileInfo->iIconIndex, 
                                        pFileInfo->dwInfoAttrib, 
                                        pLvCustomDraw->nmcd.hdc, 
                                        rcItem.left, 
                                        rcItem.top) )
      {
         if ( (FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags) && (_hStreamIco) )
         {
            /* Draw the stream overlay. This isn't done using an overlay in the system 
             * image list to because it would be an ugly hack */
            DrawIconEx(pLvCustomDraw->nmcd.hdc, 
                       rcItem.left, 
                       rcItem.top, 
                       _hStreamIco,
                       _ctlShellIL.csIcon.cx,
                       _ctlShellIL.csIcon.cy,
                       0,
                       NULL,
                       DI_NORMAL);
         }

         ExcludeClipRect(pLvCustomDraw->nmcd.hdc, 
                         rcItem.left, 
                         rcItem.top - (_ctlShellIL.csIcon.cy / 2), 
                         rcItem.left + _ctlShellIL.csIcon.cx - 1, 
                         rcItem.top + _ctlShellIL.csIcon.cy - 1);

         /* Move the text to the left of the icon */
         rcItem.left += _ctlShellIL.csIcon.cx;
      }

      /* Adjust the center of the text */
      rcItem.top -= (_ctlShellIL.csIcon.cy / 3);
   }
   else
   {
      /* Adjust the text for the missing icon */
      rcItem.top += ((rcItem.bottom - rcItem.top) / FILEINFO::CGROUPITEMSEXTRA);
   }
         
   /* Put some padding between the icon, or the border, and the text */
   rcItem.left += (2 * cxEdge);

   /* This is where the summary text is refreshed, if required. It's done here
    * so that the drawing DC can be used to calculate fonts & width, etc. */
   if ( FILEINFO::F_REFRESHSUMMARY & pFileInfo->dwFlags )
   {
      _UpdateFileSummaryInfo(pFileInfo, 
                             pLvCustomDraw->nmcd.hdc, 
                             rcItem.right - rcItem.left);

      pFileInfo->dwFlags &= ~FILEINFO::F_REFRESHSUMMARY;
   }
         
   cyText = (rcItem.bottom - rcItem.top) - (cyEdge * 2);

   if ( _ctlTheme.IsThemeNull() )
   {
      /* We let DrawText fill the bkgnd so we don't have to manage a brush */
      iBkMode = SetBkMode(pLvCustomDraw->nmcd.hdc, 
                          OPAQUE);

      cyText = DrawText(pLvCustomDraw->nmcd.hdc, 
                        pFileInfo->chSummary, 
                        -1, 
                        &rcItem, 
                        DT_EXTERNALLEADING|DT_LEFT|DT_TOP|DT_WORDBREAK|DT_END_ELLIPSIS);

      SetBkMode(pLvCustomDraw->nmcd.hdc, 
                iBkMode);
   }
   else
   {
      /* The background needs to be filled when using visual styles, or we end up with 
       * aliased text. We inflate the fill rect and clip inside the borders */
      if ( _ctlTheme.IsThemeBackgroundPartiallyTransparent(LVP_LISTITEM,
                                                           LISS_NORMAL) )
      {
         ::DrawThemeParentBackground(_ctlListView,
                                     pLvCustomDraw->nmcd.hdc,
                                     NULL);
      }

      rcFill.left   = -1;
      rcFill.top    = -1;
      rcFill.right  = rcLvClient.right  * 2;
      rcFill.bottom = rcLvClient.bottom * 2;

      _ctlTheme.DrawThemeBackground(pLvCustomDraw->nmcd.hdc, 
                                    LVP_LISTITEM, 
                                    LISS_NORMAL, 
                                    &rcFill, 
                                    &rcItem);
                  
      /* Reuse rcFill to calculate the text extents */
      hr = _ctlTheme.GetThemeTextExtent(pLvCustomDraw->nmcd.hdc, 
                                        LVP_LISTITEM, 
                                        LISS_NORMAL,
                                        pFileInfo->chSummary, 
                                        -1, 
                                        DT_EXTERNALLEADING|DT_LEFT|DT_TOP|DT_WORDBREAK|DT_END_ELLIPSIS, 
                                        &rcItem, 
                                        &rcExtent);
      if ( SUCCEEDED(hr) )
      {
         cyText = (rcExtent.bottom - rcExtent.top);

         _ctlTheme.DrawThemeText(pLvCustomDraw->nmcd.hdc, 
                                 LVP_LISTITEM, 
                                 LISS_NORMAL, 
                                 pFileInfo->chSummary, 
                                 -1, 
                                 DT_EXTERNALLEADING|DT_LEFT|DT_TOP|DT_WORDBREAK|DT_END_ELLIPSIS, 
                                 0, 
                                 &rcItem);
      }
   }

   /* Draw the group seperator*/
   hCompDC = CreateCompatibleDC(pLvCustomDraw->nmcd.hdc);
   if ( hCompDC )
   {
      hbmOld = reinterpret_cast<HBITMAP>(SelectObject(hCompDC,
                                         _bmGradient.m_hBitmap));

      if ( hbmOld )
      {
         if ( _bmGradient.GetSize(csImage) )
         {
            BitBlt(pLvCustomDraw->nmcd.hdc, 
                   xGrad, 
                   rcItem.top + cyText + (cyEdge * 2),
                   csImage.cx, 
                   csImage.cy, 
                   hCompDC, 
                   0, 
                   0, 
                   SRCCOPY);
         }

         SelectObject(hCompDC,
                      hbmOld);
      }

      DeleteDC(hCompDC);
   }

   /* Success */
   return ( CDRF_SKIPDEFAULT );
}

LRESULT CFxPropertyPage::OnListViewItemChanged( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   LPNMLISTVIEW pNmListView;

   /* Initialize locals */
   pNmListView = reinterpret_cast<LPNMLISTVIEW>(pnmh);

   if ( (LVIS_SELECTED|LVIS_FOCUSED) & pNmListView->uNewState )
   {
      /* Track the current selection */
      _iCurSel = pNmListView->iItem;
   }

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnListViewRightClick( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   LPNMITEMACTIVATE  pItemActivate;
   HMENU             hMenu;
   HMENU             hPopup;
   int               cFiles;
   PFILEINFO         pFileInfo;
   PFILEINFO         pFileInfoTail;
   POINT             cpPos;
   UINT              uMenuCmd;
   DWORD             dwFileAttributes;
   BOOLEAN           bCompressedFiles;              
   MENUITEMINFO      MenuItemInfo;

   /* We don't run on anything less than 4.71, so this is safe to assume */
   pItemActivate  = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);

   if ( -1 == pItemActivate->iItem )
   {
      /* Right-click not on an item */
      return ( 0 );
   }

   hMenu = LoadMenu(_AtlModule.GetMUIInstance(),
                    MAKEINTRESOURCE(IDR_PROPTOOL));

   if ( !hMenu )
   {
      _dTrace(L"Failed to load menu(IDR_PROPTOOL)!%#08lx\n", GetLastError());
      /* Failure */
      return ( 0 );
   }

   hPopup = GetSubMenu(hMenu,
                       0);

   /* Count the number of visible files for determing whether to enable
    * the "Properties" menu item or not */
   cFiles           = 0;
   pFileInfo        = _rgFileInfo;
   pFileInfoTail    = &_rgFileInfo[_cFileInfo];
   bCompressedFiles = FALSE;

   while ( pFileInfo < pFileInfoTail )
   {
      if ( !(FILEINFO::F_DONOTSHOW & pFileInfo->dwFlags) )
      {
         cFiles += 1;
      }

      dwFileAttributes = GetFileAttributes(pFileInfo->chFilePath);
      if ( (INVALID_FILE_ATTRIBUTES != dwFileAttributes) && FlagOn((FILE_ATTRIBUTE_COMPRESSED|FILE_ATTRIBUTE_SPARSE_FILE), dwFileAttributes) )
      {
         bCompressedFiles = TRUE;
      }

      pFileInfo++;
   }

   if ( bCompressedFiles )
   {
      EnableMenuItem(hPopup,
                     IDM_PROPSHOWCOMPRESSED,
                     MF_BYCOMMAND|MF_ENABLED);

      if ( _bShowCompressed )
      {
         CheckMenuItem(hPopup,
                       IDM_PROPSHOWCOMPRESSED,
                       MF_BYCOMMAND|MF_CHECKED);
      }
   }

   if ( _bMultiFilePage )
   {
      EnableMenuItem(hPopup,
                     IDM_PROPPROPERTIES,
                     MF_BYCOMMAND|MF_ENABLED);
   }

   cpPos = pItemActivate->ptAction;
   _ctlListView.ClientToScreen(&cpPos);

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
      MenuItemInfo.dwItemData = _GetItemFileIndex(pItemActivate->iItem);

      if ( 0x8000 & ::GetKeyState(VK_SHIFT) )
      {
         MenuItemInfo.dwItemData |= 0x80000000;
      }

      SetMenuItemInfo(hPopup,
                      uMenuCmd,
                      FALSE,
                      &MenuItemInfo);

      SendMessage(WM_MENUCOMMAND,
                  MAKELONG(uMenuCmd, uMenuCmd - IDM_PROPDEFRAGMENTFILE),
                  reinterpret_cast<LONG_PTR>(hPopup));
   }

   DestroyMenu(hMenu);

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnHeaderRightClick( int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/ ) throw()
{
   HMENU hMenu;
   HMENU hPopup;
   POINT cpPos;
   UINT  uMenuCmd;

   hMenu = LoadMenu(_AtlModule.GetMUIInstance(),
                    MAKEINTRESOURCE(IDR_PROPCOLUMS));

   if ( !hMenu )
   {
      /* Failure */
      return ( 0 );
   }

   hPopup = GetSubMenu(hMenu,
                       0);
   ATLASSERT(NULL != hPopup);

   GetCursorPos(&cpPos);

   CheckMenuItem(hPopup,
                 IDM_PROPCOLSIZE, 
                 MF_BYCOMMAND|(_bShowColSize ? MF_CHECKED : MF_UNCHECKED));

   CheckMenuItem(hPopup, 
                 IDM_PROPCOLPERCENTAGE, 
                 MF_BYCOMMAND|(_bShowColPercentage ? MF_CHECKED : MF_UNCHECKED));

   CheckMenuItem(hPopup, 
                 IDM_PROPCOLCLUSTERS, 
                 MF_BYCOMMAND|(_bShowColClusters ? MF_CHECKED : MF_UNCHECKED));

   CheckMenuItem(hPopup, 
                 IDM_PROPCOLEXTENTS, 
                 MF_BYCOMMAND|(_bShowColExtents ? MF_CHECKED : MF_UNCHECKED));

   uMenuCmd = static_cast<UINT>(TrackPopupMenuEx(hPopup, 
                                                 TPM_LEFTALIGN|TPM_TOPALIGN|TPM_LEFTBUTTON|TPM_NONOTIFY|TPM_RETURNCMD, 
                                                 FixTrackMenuPopupX(cpPos.x, cpPos.y), 
                                                 cpPos.y, 
                                                 m_hWnd, 
                                                 NULL));

   if ( 0 != uMenuCmd )
   {
      SendMessage(WM_MENUCOMMAND, 
                  MAKELONG(uMenuCmd, uMenuCmd - IDM_PROPCOLSEQUENCE), 
                  reinterpret_cast<LONG_PTR>(hPopup));
   }

   DestroyMenu(hMenu);
   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnToolMenuClick( WORD /*wItemIndex*/, WORD wID, HMENU hMenu, BOOL& /*bHandled*/ ) throw()
{
   MENUITEMINFO      MenuItemInfo;
   int               iFile;
   bool              bShiftKey;
   WTL::CHeaderCtrl  ctlHeader;
   HDITEM            HdItem;
   
   ZeroMemory(&MenuItemInfo,
              sizeof(MenuItemInfo));

   MenuItemInfo.cbSize = sizeof(MenuItemInfo);
   MenuItemInfo.fMask  = MIIM_DATA;
   GetMenuItemInfo(hMenu,
                   wID,
                   FALSE,
                   &MenuItemInfo);

   iFile     = static_cast<int>(MenuItemInfo.dwItemData & ~0x80000000);
   bShiftKey = (0x80000000 & MenuItemInfo.dwItemData ? true : false);

   switch ( wID )
   {
      case IDM_PROPDEFRAGMENTFILE:
         _OnDefragmentFile(iFile, 
                           bShiftKey);
         break;

      case IDM_PROPREFRESHFILE:
      {
         ZeroMemory(&HdItem,
                    sizeof(HdItem));

         /* Clear the arrow for the previous column */
         ctlHeader   = _ctlListView.GetHeader();
         HdItem.mask = (_bIsCC6 ? 0 : HDI_BITMAP)|HDI_FORMAT;
         ctlHeader.GetItem(_wSortCol,
                           &HdItem);
         ATLASSERT(!HdItem.hbm || !_bIsCC6);

         HdItem.hbm  = NULL;
         HdItem.fmt &= ~(HDF_BITMAP|HDF_BITMAP_ON_RIGHT|HDF_SORTDOWN|HDF_SORTUP);
         ctlHeader.SetItem(_wSortCol, 
                           &HdItem);

         /* Get rid of the sorting data */
         _wSortCol = 0;
         _bSortASC = 1;

         /* Reload the data, asynchronously */
         PostMessage(WM_UPDATECONTROL, 
                     FXUC_REFRESHDATA|FXUC_REFRESHLIST, 
                     0);
         break;
      }

      case IDM_PROPREPORTTOCLIP:
         _OnSaveReportTo(iFile, 
                         bShiftKey, 
                         CFragReport::WriteModeClipboard);
         break;

      case IDM_PROPREPORTTOFILE:
         _OnSaveReportTo(iFile, 
                         bShiftKey, 
                         CFragReport::WriteModeFile);
         break;

      case IDM_PROPSHOWCOMPRESSED:
         _bShowCompressed = !_bShowCompressed;

         /* Reload the data, asynchronously */
         PostMessage(WM_UPDATECONTROL, 
                     FXUC_REFRESHDATA|FXUC_REFRESHLIST, 
                     0);         
         break;

      case IDM_PROPPROPERTIES:
         _OnFileProperties(iFile, 
                           bShiftKey);

         break;

      DEFAULT_UNREACHABLE;
   }

   return ( 0 );
}

LRESULT CFxPropertyPage::OnColumnMenuClick( WORD /*wItemIndex*/, WORD wID, HMENU /*hMenu*/, BOOL& /*bHandled*/ ) throw()
{
   SIZE csScroll;
   RECT rcPage;

   switch ( wID )
   {
      case IDM_PROPCOLSIZE:
         _bShowColSize = !_bShowColSize;
         break;

      case IDM_PROPCOLPERCENTAGE:
         _bShowColPercentage = !_bShowColPercentage;
         break;

      case IDM_PROPCOLCLUSTERS:
         _bShowColClusters = !_bShowColClusters;
         break;

      case IDM_PROPCOLEXTENTS:
         _bShowColExtents = !_bShowColExtents;
         break;

      case IDM_PROPCOLAUTOSIZE:
      {
         /* If the horz-scrollbar is shown, get rid of it */
         if ( WS_HSCROLL & _ctlListView.GetStyle() )
         {
            csScroll.cx = -_ctlListView.GetScrollPos(SB_HORZ);

            if ( 0 != csScroll.cx )
            {
               csScroll.cy = 0;
               _ctlListView.Scroll(csScroll);
            }
         }

         /* Force the OnSize handler to be executed */
         GetClientRect(&rcPage);
         SendMessage(WM_SIZE,
                     SIZE_RESTORED,
                     MAKELPARAM(rcPage.right, rcPage.bottom));
         /* Success */
         return ( 0 );
      }
   }

   /* Force the columns to refresh so that bitmaps/etc can be cleared */
   _UpdateColumns(false);

   /* Save the updated settings and return */
   _SaveSettings();

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnScListViewHScroll( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled ) throw()
{
   /* Need to invalidate the group so the gradient can be redrawn */
   _InvalidateListViewArea(MAKELONG(FXILVA_GRADIENTS, 0));

   /* Allow default processing */
   bHandled = FALSE;
   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnScListViewKeyDown( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled ) throw()
{
   int   iCurSel;
   int   iNewSel;
   int   cPerPage;
   RECT  rcItem;
   SIZE  csScroll;

   /*
    * Shift and Ctrl keys are ignored by this routine
    */
   
   /* Restore the selection, if any */
   switch ( wParam )
   {
      case VK_PRIOR:
      case VK_NEXT:
      case VK_HOME:
      case VK_END:
      case VK_UP:
      case VK_DOWN:
         if ( -1 != _iCurSel )
         {
            _ctlListView.SetItemState(_iCurSel,
                                      LVIS_SELECTED|LVIS_FOCUSED,
                                      LVIS_SELECTED);
         }
         break;
   }

   switch ( wParam )
   {  
      case VK_PRIOR:
         cPerPage = _ctlListView.GetCountPerPage();
         iNewSel  = (-1 != _iCurSel ? _iCurSel : _ctlListView.GetTopIndex());
         iNewSel -= cPerPage;

         if ( iNewSel < 0 )
         {
            iNewSel = 0;
         }

         _ctlListView.EnsureVisible(iNewSel,
                                    FALSE);

         iNewSel = _iCurSel - cPerPage;
         if ( (iNewSel > 0) && _IsFragInfoItem(iNewSel) )
         {
            _ctlListView.SelectItem(iNewSel);
         }
         else
         {
            iCurSel  = _ctlListView.GetTopIndex();
            iCurSel -= min(iCurSel, 1);
            iNewSel  = _GetNextListViewItem(iCurSel,
                                           LVNI_BELOW);

            if ( (-1 != iNewSel) && (iNewSel <= (iCurSel + cPerPage)) )
            {
               _ctlListView.SelectItem(iNewSel);
            }
         }

         break;
      case VK_NEXT:
         _ctlListView.GetItemRect(0,
                                  &rcItem,
                                  LVIR_BOUNDS);

         cPerPage    = _ctlListView.GetCountPerPage();
         csScroll.cx = 0;
         csScroll.cy = (rcItem.bottom - rcItem.top) * cPerPage;         
         _ctlListView.Scroll(csScroll);

         /* If there is there is a selectable item at the the exact position of
          * previous-selection + page count, select that item */
         iNewSel  = _iCurSel + cPerPage;
         if ( (iNewSel > 0) && (iNewSel < _cItemInfo) && _IsFragInfoItem(iNewSel) )
         {
            _ctlListView.SelectItem(iNewSel);
         }
         else
         {
            iCurSel  = _ctlListView.GetTopIndex();
            iNewSel  = _GetNextListViewItem(iCurSel + cPerPage + 1,
                                           LVNI_ABOVE);

            if ( (-1 != iNewSel) && (iNewSel >= iCurSel) )
            {
               _ctlListView.SelectItem(iNewSel);
            }
         }

         break;
      case VK_HOME:
         /* Find the first selectable item on the first page. If there isn't one,
          * clear the current selection and make the first page visible */
         iNewSel = _GetNextListViewItem(FILEINFO::CGROUPITEMSEXTRA - 1,
                                        LVNI_BELOW);

         if ( -1 != iNewSel )
         {
            cPerPage = _ctlListView.GetCountPerPage();
            
            if ( iNewSel < cPerPage )
            {
               /* Select the item */
               _ctlListView.SelectItem(iNewSel);
               /* Scroll the entire first page into view */
               _ctlListView.EnsureVisible(0,
                                          FALSE);
               break;
            }
         }

         /* Clear the current selection and jump to the first item */
         iCurSel = _ctlListView.GetSelectedIndex();
         if ( -1 != iCurSel )
         {
            _ctlListView.SetItemState(iCurSel,
                                      0,
                                      LVIS_SELECTED|LVIS_FOCUSED);
         }

         /* Scroll to the first page */
         _ctlListView.EnsureVisible(0,
                                    FALSE);

         break;
      case VK_END:
         /* Find the last selectable item, or the last page */
         iNewSel = _GetNextListViewItem(_cItemInfo,
                                        LVNI_ABOVE);

         if ( -1 != iNewSel )
         {
            cPerPage = _ctlListView.GetCountPerPage();
            /* Prevent underflowing _cItemInfo */
            cPerPage = min(cPerPage, _cItemInfo);

            if ( iNewSel > (_cItemInfo - cPerPage) )
            {
               _ctlListView.SelectItem(iNewSel);
               /* Bring the full page into view */
               _ctlListView.EnsureVisible(_cItemInfo - 1,
                                          FALSE);
               break;
            }
         }

         /* Clear the current selection, if any */
         iCurSel = _ctlListView.GetSelectedIndex();
         if ( -1 != iCurSel )
         {
            _ctlListView.SetItemState(iCurSel,
                                      0,
                                      LVIS_SELECTED|LVIS_FOCUSED);
         }

         /* Bring the last page into view */
         _ctlListView.EnsureVisible(_cItemInfo - 1,
                                    FALSE);

         break;

      case VK_UP:
         iCurSel = _ctlListView.GetSelectedIndex();

         if ( (-1 == iCurSel) || (iCurSel <= FILEINFO::CGROUPITEMSEXTRA) )
         {
            /* Either no selection has been made yet, or the first frag item is selected. In
             * both of these cases, the key is ignored.
             *
             * Checking for <= prevents the possibility using "0 - 1" for an ITEMINFO index
             *
             * This may be an up key as the user scrolled up through the items, so the last
             * one that succeeded could have left the group items partially hidden. For this 
             * we ensure that the item's first group item is visible
             */
            _ctlListView.EnsureVisible(0,
                                       FALSE);
            break;
         }
        
         /* Find the previous non-file item */
         iNewSel = _GetNextListViewItem(iCurSel,
                                        LVNI_ABOVE);

         if ( -1 == iNewSel )
         {
            /* There is no selectable item above the current one, so ignore the key */
            break;
         }

         /* Make the selection and consume the key */
         _ctlListView.SelectItem(iNewSel);
         break;

      case VK_DOWN:
         iCurSel = _ctlListView.GetSelectedIndex();

         if ( -1 == iCurSel )
         {
            /* There is no selection, so use the top index as a starting point */
            iCurSel = _ctlListView.GetTopIndex();
         }

         /* Find the next non-file item */
         iNewSel = _GetNextListViewItem(iCurSel,
                                        LVNI_BELOW);

         if ( -1 == iNewSel )
         {
            /* Nothing available to select */
            break;
         }

         /* Make the selection and consume the key */
         _ctlListView.SelectItem(iNewSel);
         break;

      default:
         bHandled = FALSE;
   }
   
   return ( 0 );
}

LRESULT CFxPropertyPage::OnScListViewLButtonDown( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled ) throw()
{
   POINT cpPos;
   int   iItem;

   cpPos.x = GET_X_LPARAM(lParam);
   cpPos.y = GET_Y_LPARAM(lParam);
   iItem   = _ctlListView.HitTest(cpPos,
                                  NULL);
   
   if ( (-1 != iItem) && _IsFileInfoItem(iItem) )
   {
      /* Clear the selection but leave the focus, if any */      
      if ( -1 != _iCurSel )
      {
         _ctlListView.SetItemState(_iCurSel,
                                   0, 
                                   LVIS_SELECTED);
      }

      /* Ignore the message */
      return ( 0 );
   }

   /* Allow the listview to see the message */
   bHandled = FALSE;
   /* Failure */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnScListViewRButtonDown( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled ) throw()
{
   int            iItem;
   LVHITTESTINFO  HitInfo;
   NMITEMACTIVATE ItemActivate;

   ZeroMemory(&HitInfo,
              sizeof(HitInfo));

   HitInfo.pt.x = GET_X_LPARAM(lParam);
   HitInfo.pt.y = GET_Y_LPARAM(lParam);
   iItem = _ctlListView.HitTest(&HitInfo);

   
   if ( (-1 != iItem) && _IsFileInfoItem(iItem) )
   {
      /* Clear the selection but leave the focus, if any */      
      if ( -1 != _iCurSel )
      {
         _ctlListView.SetItemState(_iCurSel,
                                   0, 
                                   LVIS_SELECTED);
      }

      /* Generate an NM_CLICK notification */
      ZeroMemory(&ItemActivate,
                 sizeof(ItemActivate));

      ItemActivate.hdr.hwndFrom = m_hWnd;
      ItemActivate.hdr.code     = NM_CLICK;
      ItemActivate.hdr.idFrom   = IDC_LSVFRAGMENTS;
      ItemActivate.iItem        = iItem;
      ItemActivate.iSubItem     = HitInfo.iSubItem;
      ItemActivate.ptAction     = HitInfo.pt;

      OnListViewRightClick(IDC_LSVFRAGMENTS,
                           &ItemActivate.hdr,
                           bHandled);

      /* Ignore the message */
      bHandled = TRUE;
      /* Success */
      return ( 0 );
   }

   /* Let the listview see the message */
   bHandled = FALSE;
   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnScListViewEraseBkgnd( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ ) throw()
{
   /* The background is filled in OnListViewPaint */
   return ( 1 );
}

LRESULT CFxPropertyPage::OnScListViewPaint( UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ ) throw()
{
   HDC         hDC;
   CMemDC      cMemDC;
   CRect       rcClient;
   PAINTSTRUCT Paint;
   
   hDC = reinterpret_cast<HDC>(wParam);
   if ( !hDC )
   {
      hDC = _ctlListView.BeginPaint(&Paint);
   }

   if ( cMemDC.Create(hDC, 
                      !wParam ? NULL : &Paint.rcPaint) )
   {
      SetBkColor(cMemDC,
                 GetSysColor(COLOR_WINDOW));

      FillRect(cMemDC,
               &cMemDC._rcDraw,
               reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

      _ctlListView.DefWindowProc(WM_PAINT,
                                 reinterpret_cast<LONG_PTR>(static_cast<HDC>(cMemDC)),
                                 lParam);
      cMemDC.Destroy();
   }

   if ( !wParam )
   {
      _ctlListView.EndPaint(&Paint);
   }

   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnListViewColumnClick( int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/ ) throw()
{
   LPNMLISTVIEW     pNmListView;
   WTL::CHeaderCtrl ctlHeader;
   HDITEM           HdItem;
   CWaitCursor      cWait(false);

   pNmListView = reinterpret_cast<LPNMLISTVIEW>(pnmh);
   ctlHeader   = _ctlListView.GetHeader();
   
   ZeroMemory(&HdItem,
              sizeof(HdItem));

   /* Sorting a new column always resets the order. Sorting the
    * same column only updates the order. */
   if ( pNmListView->iSubItem != static_cast<int>(_wSortCol) )
   {      
      /* Clear the arrow for the previous column */
      HdItem.mask = (_bIsCC6 ? 0 : HDI_BITMAP)|HDI_FORMAT;
      ctlHeader.GetItem(_wSortCol, 
                        &HdItem);
      ATLASSERT(!HdItem.hbm || !_bIsCC6);

      HdItem.hbm  = NULL;
      HdItem.fmt &= ~(HDF_BITMAP|HDF_BITMAP_ON_RIGHT|HDF_SORTDOWN|HDF_SORTUP);
      ctlHeader.SetItem(_wSortCol, 
                        &HdItem);

      _wSortCol = static_cast<WORD>(pNmListView->iSubItem);
      _bSortASC = 1;
   }
   else
   {
      _bSortASC = !_bSortASC;
   }

   /* Set the arrow for the active column */
   HdItem.mask = (_bIsCC6 ? 0 : HDI_BITMAP)|HDI_FORMAT;
   ctlHeader.GetItem(_wSortCol, 
                     &HdItem);         
   ATLASSERT(!HdItem.hbm || !_bIsCC6);

   if ( _bIsCC6 )
   {
      HdItem.fmt &= ~(HDF_SORTUP|HDF_SORTDOWN);
      HdItem.fmt |= (_bSortASC ? HDF_SORTUP : HDF_SORTDOWN);
   }
   else
   {
      HdItem.fmt |= HDF_BITMAP|HDF_BITMAP_ON_RIGHT;
      HdItem.hbm  = reinterpret_cast<HBITMAP>(LoadImage(GetModuleHandle(L"SHELL32.DLL"), 
                                                        MAKEINTRESOURCE(133 + (_bSortASC ? 0 : 1)), 
                                                        IMAGE_BITMAP, 
                                                        0, 
                                                        0, 
                                                        LR_LOADMAP3DCOLORS));
      ATLASSERT(NULL != HdItem.hbm);
   }

   ctlHeader.SetItem(_wSortCol, 
                     &HdItem);

   
   cWait.Set();
      _SortItemInfoTable();
      _ctlListView.RedrawItems(0,
                               _cItemInfo);
   cWait.Restore();
   
   /* Success */
   return ( 0 );
}

LRESULT CFxPropertyPage::OnHeaderItemChanging( int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled ) throw()
{
   LPNMHEADER pNmHeader;

   pNmHeader = reinterpret_cast<LPNMHEADER>(pnmh);
   _InvalidateListViewArea(MAKELONG(FXILVA_GROUPS, pNmHeader->iItem));

   /* Do default processing */
   bHandled = FALSE;
   /* Success */
   return ( 0 );
}

STDMETHODIMP CFxPropertyPage::Initialize( LPCITEMIDLIST /*pidlFolder*/, IDataObject* pDataObj, HKEY /*hkProgID*/ ) throw()
{
   HRESULT     hr;
   DWORD       dwErr;
   FORMATETC   formatEtc;
   STGMEDIUM   stgMedium;
   HDROP       hDrop;

   /* Always clear the current data */
   _ResetData();

   if ( !pDataObj )
   {
      /* Success */
      return ( S_OK );
   }

   /* Initialize locals */
   hDrop = NULL;

   ZeroMemory(&formatEtc,
              sizeof(formatEtc));

   ZeroMemory(&stgMedium,
              sizeof(stgMedium));

   formatEtc.cfFormat = CF_HDROP;
   formatEtc.dwAspect = DVASPECT_CONTENT;
   formatEtc.lindex   = -1;
   formatEtc.tymed    = TYMED_HGLOBAL;

   hr = pDataObj->GetData(&formatEtc,
                          &stgMedium);

   if ( FAILED(hr) )
   {
      /* Failure - Possible unsupported drop format */
      goto __CLEANUP;
   }

   hDrop = reinterpret_cast<HDROP>(GlobalLock(stgMedium.hGlobal));
   if ( !hDrop )
   {
      dwErr = GetLastError();
      hr = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   hr = _LoadFilesFromHDROP(hDrop);

__CLEANUP:
   ATLASSERT(SUCCEEDED(hr));

   if ( hDrop )
   {
      GlobalUnlock(stgMedium.hGlobal);
   }

   if ( stgMedium.hGlobal )
   {
      ReleaseStgMedium(&stgMedium);
   }

   if ( FAILED(hr) )
   {
      _ResetData();
   }

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CFxPropertyPage::AddPages( LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam ) throw()
{
   BOOL           bRet;
   HPROPSHEETPAGE hPage;
   
   hPage = Create();
   if ( !hPage )
   {
      /* Failure */
      return ( E_FAIL );
   }

   bRet = pfnAddPage(hPage,
                     lParam);
   if ( !bRet )
   {
      DestroyPropertySheetPage(hPage);

      /* Failure */
      return ( E_FAIL );
   }

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFxPropertyPage::ReplacePage( EXPPS /*uPageID*/, LPFNSVADDPROPSHEETPAGE /*pfnReplaceWith*/, LPARAM /*lParam*/ ) throw()
{
   return ( E_NOTIMPL );
}

inline bool CFxPropertyPage::OnPageAddRef( ) throw()
{
   AddRef();
   /* Success */
   return ( true );
}

inline void CFxPropertyPage::OnPageRelease( ) throw()
{
   Release();
}

int CFxPropertyPage::_GetNextListViewItem( int iItem, UINT uDirection ) throw()
{
   if ( iItem > _cItemInfo )
   {
      iItem = _cItemInfo;
   }

   if ( LVNI_ABOVE == uDirection )
   {
      while ( iItem-- > 0 )
      {
         if ( _IsFragInfoItem(iItem) )
         {
            /* Success */
            return ( iItem );
         }
      }
   }
   else if ( LVNI_BELOW == uDirection )
   {
      while ( ++iItem < _cItemInfo )
      {
         if ( _IsFragInfoItem(iItem) )
         {
            /* Success */
            return ( iItem );
         }
      }
   }

   /* Failure */
   return ( -1 );
}

void CFxPropertyPage::_UpdateControl( DWORD dwFlags ) throw()
{
   WTL::CHeaderCtrl ctlHeader;
   /* Enumerators of _rgFileInfo */
   PFILEINFO        pFileInfo;
   PFILEINFO        pFileInfoTail;
   WCHAR            chBasePath[MAX_STREAM_PATH];
   
   /* Turn off drawing on the listview while this update is in progress */
   _ctlListView.SetRedraw(FALSE);

   /* If the caller has requested that the backing data be refreshed, then do so */
   if ( FXUC_REFRESHDATA & dwFlags )
   {
      /* Clear the list view and reset the cached item count */
      _ctlListView.DeleteAllItems();

      /* Reload file & NTFS stream data */
      _UpdateFileInfoTable();
      /* Build the ITEMINFO table */
      _BuildItemInfoTable();

      /* Refresh the icon info for each file */
      pFileInfo     = _rgFileInfo;
      pFileInfoTail = &_rgFileInfo[_cFileInfo];

      while ( pFileInfo < pFileInfoTail )
      {
         if ( _ctlShellIL.IsNull() )
         {
            /* Shell icons could not be loaded, so clear the icon index and item info */
            pFileInfo->iIconIndex   = -1;
            pFileInfo->dwInfoAttrib = 0;
         }
         else
         {
            /* This file could be a normal file, or a file stream, so retrieve the base file
             * path and ask the shell for the base file's item info */
            _GetBaseFilePath(pFileInfo,
                             chBasePath,
                             _countof(chBasePath));

            pFileInfo->dwInfoAttrib = _ctlShellIL.GetFileInfo(chBasePath, 
                                                              pFileInfo->iIconIndex);
         }

         /* On to the next file in the table */
         pFileInfo++;
      }

      /* Reset the listview's item count */
      _ctlListView.SetItemCountEx(static_cast<int>(_cItemInfo),
                                  0);
   }

   if ( FXUC_REFRESHLIST & dwFlags )
   {
      /* If the data was refreshed, then the listview items were deleted and this section inserts
       * all the items. If the items are only being refreshed, then this simply does a redraw */

      /* Force the summary info for all the files to be updated */
      pFileInfo     = _rgFileInfo;
      pFileInfoTail = &_rgFileInfo[_cFileInfo];

      while ( pFileInfo < pFileInfoTail )
      {
         /* Set a flag to force the drawing code to refresh the summary data. The summary info
          * is refreshed in the drawing code because a device context is required */
         pFileInfo->dwFlags |= FILEINFO::F_REFRESHSUMMARY;
         pFileInfo++;
      }
         
      /* Force a redraw of every item */
      _ctlListView.RedrawItems(0, 
                               _cItemInfo);
   }

   /* Turn drawing back on for the listview control */
   _ctlListView.SetRedraw(TRUE);
}

void CFxPropertyPage::_UpdateFileFragmentInfo( PFILEINFO pFileInfo ) throw()
{
   HRESULT  hr;
   DWORD    dwErr;
   WCHAR    chPath[MAX_STREAM_PATH];

   pFileInfo->dwFlags |= FILEINFO::F_SIZEUNKNOWN|FILEINFO::F_FRAGSUNKNOWN;

   /* Clear the current frag context, if any */
   if ( pFileInfo->hFragCtx )
   {
      FpCloseContext(pFileInfo->hFragCtx);
      pFileInfo->hFragCtx   = NULL;
      pFileInfo->cFragments = 0;
   }

   hr = _GetFullFilePath(pFileInfo,
                         chPath,
                         _countof(chPath));

   if ( FAILED(hr) )
   {
      /* Failure - This file is displayed as having unknown data */
      return;
   }

   dwErr = FpCreateContext(chPath,
                           (_bShowCompressed ? FPF_CTX_RAWEXTENTDATA : 0)|FPF_CTX_DETACHED,
                           &pFileInfo->hFragCtx);

   if ( NO_ERROR != dwErr )
   {
      /* Failure */
      return;
   }

   /* Cache the total number of fragments for the file */
   dwErr = FpGetContextFragmentCount(pFileInfo->hFragCtx,
                                  &pFileInfo->cFragments);

   if ( NO_ERROR != dwErr )
   {
      /* Failure */
      return;
   }

   pFileInfo->dwFlags &= ~(FILEINFO::F_SIZEUNKNOWN|FILEINFO::F_FRAGSUNKNOWN);
}

inline void CFxPropertyPage::_ClearFileTable( ) throw()
{
   PFILEINFO pFileInfo;
   PFILEINFO pFileInfoTail;

   /* Wipe out the file table */
   if ( _rgFileInfo )
   {
      pFileInfo     = _rgFileInfo;
      pFileInfoTail = &_rgFileInfo[_cFileInfo];

      while ( pFileInfo < pFileInfoTail )
      {
         if ( pFileInfo->hFragCtx )
         {
            FpCloseContext(pFileInfo->hFragCtx);
            pFileInfo->hFragCtx = NULL;
         }

         pFileInfo++;
      }

      free(_rgFileInfo);
      _rgFileInfo = NULL;
   }

   _cFileInfo = 0;
}

inline void CFxPropertyPage::_ResetData( ) throw()
{
   _ClearFileTable();
   _ClearItemInfoTable();

   _wSortCol = 0;
   _bSortASC = 1;
}

void CFxPropertyPage::_UpdateFileSummaryInfo( PFILEINFO pFileInfo, HDC hDC, UINT cxWidth ) throw()
{
   WCHAR       chFilePath[MAX_STREAM_PATH * 2];
   WCHAR       chDisplayName[MAX_STREAM_PATH * 2];
   WCHAR       chFormatStr[128];
   WCHAR       chFileSize[512];
   WCHAR       chFragCount[128];
   PFILEINFO   pAssociatedFile;
   RECT        rcClip;
   CNUMBERFMT  NumberFmt;
   ULONGLONG   cbSize;
   bool        bHasSize;
   HRESULT     hr;

   /* Initialize locals */
   cbSize = 0;

   _GetFullFilePath(pFileInfo,
                    chFilePath,
                    _countof(chFilePath));

   
   pAssociatedFile = (FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags ? pFileInfo->pAssociatedFile : pFileInfo);

   StringCchCopy(chDisplayName,
                 _countof(chDisplayName),
                 /*_cSettings.Shell.ShowFullPaths ? pAssociatedFile->chFilePath : */PathCchFindFileName(pAssociatedFile->chFilePath));

   if ( _IsHiddenFileExtension(pAssociatedFile->chFilePath) )
   {
      pAssociatedFile->dwFlags |= FILEINFO::F_ISHIDDENEXT;
      PathCchRemoveExtension(chDisplayName);
   }

   if ( FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags )
   {
      StringCchCat(chDisplayName, 
                   _countof(chDisplayName), 
                   pFileInfo->chStreamName);
   }

   /* Make the path fit in the width if requested */
   if ( 0 != cxWidth )
   {
      ATLASSERT(NULL != hDC);
      /* DrawThemeText doesn't seem to work well with both DT_END_ELLIPSIS & DT_PATH_ELLIPSIS so 
       * so DrawText is used. PathCompactPath doesn't work for this either. */
      rcClip.left   = 0;
      rcClip.top    = 0;
      rcClip.right  = cxWidth;
      rcClip.bottom = -1;

      DrawText(hDC, 
               chDisplayName, 
               -1, 
               &rcClip, 
               DT_END_ELLIPSIS|DT_LEFT|DT_MODIFYSTRING|DT_PATH_ELLIPSIS);
   }

   NumberFmt.Initialize(LOCALE_USER_DEFAULT);
   NumberFmt.NumDigits = 0;

   LoadString(_AtlModule.GetMUIInstance(),
              IDS_FMTFILEFINFO,
              chFormatStr,
              _countof(chFormatStr));
   ATLASSERT(L'\0' != *chFormatStr);

   bHasSize = (FILEINFO::F_SIZEUNKNOWN & pFileInfo->dwFlags ? false : true);

   if ( bHasSize )
   {
      FpGetContextFileSize(pFileInfo->hFragCtx,
                           &cbSize);
   }

   if ( !bHasSize || !StrFormatByteSizeW(cbSize,
                                         chFileSize,
                                         _countof(chFileSize)) )
   {
      LoadString(_AtlModule.GetMUIInstance(),
                 IDS_LABELUNKNOWN, 
                 chFileSize, 
                 _countof(chFileSize));
      ATLASSERT(L'\0' != *chFileSize);
   }


   if ( pFileInfo->cFragments )
   {
      GetLocaleFormattedNumber(L"%u",
                               chFragCount,
                               _countof(chFragCount),
                               NumberFmt,
                               pFileInfo->cFragments);
      ATLASSERT(L'\0' != *chFragCount);
   }
   else
   {
      LoadString(_AtlModule.GetMUIInstance(),
                 IDS_LABELUNKNOWN,
                 chFragCount,
                 _countof(chFragCount));
      ATLASSERT(L'\0' != *chFragCount);
   }

   hr = StringCchPrintf(pFileInfo->chSummary,
                        _countof(pFileInfo->chSummary),
                        chFormatStr,
                        chDisplayName,
                        chFileSize,
                        chFragCount);
   if ( FAILED(hr) )
   {
      StringCchCopy(pFileInfo->chSummary,
                    _countof(pFileInfo->chSummary),
                    chDisplayName);
   }
}

bool CFxPropertyPage::_IsHiddenFileExtension( LPCTSTR pszPath ) throw()
{
   typedef HRESULT (__stdcall* PASSOCQUERYKEY)(ASSOCF, ASSOCKEY, LPCTSTR, LPCTSTR, HKEY*);
   
   bool           bRet;
   SHELLFLAGSTATE ShellState;
   HKEY           hKey;
   HRESULT        hr;
   HMODULE        hModule;
   PASSOCQUERYKEY pAssocQueryKey;
   DWORD          cbData;
   
   /* Initialize locals */
   bRet    = false;
   hModule = NULL;
   hKey    = NULL;

   ZeroMemory(&ShellState,
              sizeof(ShellState));

   SHGetSettings(&ShellState,
                 SSF_SHOWEXTENSIONS);

   if ( ShellState.fShowExtensions )
   {
      /* Success */
      return ( false ); 
   }
      
   hModule = LoadLibrary(L"SHLWAPI.DLL");
   if ( !hModule )
   {
      /* Default to showing extensions */
      return ( false );
   }

   pAssocQueryKey = (PASSOCQUERYKEY)GetProcAddress(hModule,
                                                   "AssocQueryKeyW");
   if ( !pAssocQueryKey )
   {
      goto __CLEANUP;
   }

   hr = pAssocQueryKey(ASSOCF_INIT_DEFAULTTOSTAR, 
                       ASSOCKEY_CLASS, 
                       PathCchFindExtension(pszPath), 
                       NULL, 
                       &hKey);

   if ( FAILED(hr) )
   {
      goto __CLEANUP;
   }

   cbData = 0;
   if ( ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         L"NeverShowExt",
                                         NULL,
                                         NULL,
                                         NULL,
                                         &cbData) )
   {
      bRet = true;
   }

   cbData = 0;
   if ( ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         L"AlwaysShowExt",
                                         NULL,
                                         NULL,
                                         NULL,
                                         &cbData) )
   {
      bRet = false;
   }

__CLEANUP:
   if ( hKey )
   {
      RegCloseKey(hKey);
   }

   if ( hModule )
   {
      FreeLibrary(hModule);
   }

   /* Success / Failure */
   return ( bRet );
}

void CFxPropertyPage::_LoadSettings( ) throw()
{
   DWORD dwValue;

   dwValue = 0;
   GetRegistryValueDword(RVF_HKCU,
                         FRAGSHX_SUBKEY,
                         FRAGSHX_SHOWCOMPRESSEDUNITS,
                         &dwValue);

   _bShowCompressed = (0 == dwValue ? 0 : 1);

   dwValue = 0;
   if ( NO_ERROR != GetRegistryValueDword(RVF_HKCU,
                                          FRAGSHX_SUBKEY,
                                          FRAGSHX_SHOWFILESTREAMS,
                                          &dwValue) )
   {
      dwValue = 1;
   }

   _bShowFileStreams = (0 == dwValue ? 0 : 1);   

   dwValue = 0;
   if ( NO_ERROR != GetRegistryValueDword(RVF_HKCU,
                                          FRAGSHX_SUBKEY,
                                          FRAGSHX_PROPCOLUMNMASK,
                                          &dwValue) )
   {
      /* We'll default to showing all but extents */
      dwValue = 0x00000007;
   }

   _bShowColSize       = (0x00000001 & dwValue ? 1 : 0);
   _bShowColPercentage = (0x00000002 & dwValue ? 1 : 0);
   _bShowColClusters   = (0x00000004 & dwValue ? 1 : 0);
   _bShowColExtents    = (0x00000008 & dwValue ? 1 : 0);   

   _LoadAltColorSetting();
}

void CFxPropertyPage::_SaveSettings( ) throw()
{
   DWORD dwValue;

   dwValue = _bShowCompressed;
   SetRegistryValueDword(RVF_HKCU,
                         FRAGSHX_SUBKEY,
                         FRAGSHX_SHOWCOMPRESSEDUNITS,
                         dwValue);

   dwValue = _bShowFileStreams;
   SetRegistryValueDword(RVF_HKCU,
                         FRAGSHX_SUBKEY,
                         FRAGSHX_SHOWFILESTREAMS,
                         dwValue);

   dwValue  = _bShowColSize;
   dwValue |= (_bShowColPercentage << 1);
   dwValue |= (_bShowColClusters   << 2);
   dwValue |= (_bShowColExtents    << 3);
   SetRegistryValueDword(RVF_HKCU,
                         FRAGSHX_SUBKEY,
                         FRAGSHX_PROPCOLUMNMASK,
                         dwValue);
}

void CFxPropertyPage::_LoadAltColorSetting( ) throw()
{
   HKEY  hKey;
   DWORD dwType;
   DWORD dwValue;
   DWORD cbData;

   /* Initialize locals */
   hKey = NULL;

   /* Reset the cached alternate color setting */
   _clrCompressed = RGB(0, 0, 255);

   /* Check the user hive for a custom color setting */
   if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER, 
                                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", 
                                      0, 
                                      KEY_QUERY_VALUE, 
                                      &hKey) )
   {
      /* Failure */
      return;
   }

   dwType  = 0;
   dwValue = 0;
   cbData  = sizeof(dwValue);

   if ( ERROR_SUCCESS == RegQueryValueEx(hKey,
                                         L"AltColor",
                                         NULL,
                                         &dwType,
                                         reinterpret_cast<LPBYTE>(&dwValue),
                                         &cbData) )
   {
      if ( (REG_BINARY == dwType) || (REG_DWORD == dwType) )
      {
         _clrCompressed = dwValue;
      }
   }

   RegCloseKey(hKey);
}

void CFxPropertyPage::_GetItemFragmentInfo( int iItem, PFRAGMENT_INFORMATION pFragInfo ) throw()
{
   DWORD     dwErr;
   ULONG     iFragIdx;
   PFILEINFO pFileInfo;

   iFragIdx  = _GetItemFragIndex(iItem);
   pFileInfo = _GetItemFileInfo(iItem);

   dwErr = FpGetContextFragmentInformation(pFileInfo->hFragCtx,
                                           iFragIdx,
                                           pFragInfo,
                                           1,
                                           NULL);
   ATLASSERT(NO_ERROR == dwErr || ERROR_MORE_DATA == dwErr);
}

void CFxPropertyPage::_InvalidateListViewArea( DWORD dwAreaInfo ) throw()
{
   /* Invalidate portions of the listview control that are used for 
    * drawing summary info during a header resize. */

   HRGN              hrgnItem;
   HRGN              hrgnUpdate;
   HDC               hdcListView;
   RECT              rcListView;
   RECT              rcHeader;
   RECT              rcItem;
   RECT              rcHdItem;
   int               cyItem;
   int               iItem;
   PFILEINFO         pFileInfo;
   PFILEINFO         pFileTail;
   WTL::CHeaderCtrl  ctlHeader;

   /* Initialize locals... */
   hrgnItem    = NULL;
   hrgnUpdate  = NULL;
   hdcListView = NULL;
   ctlHeader   = _ctlListView.GetHeader();

   hdcListView = _ctlListView.GetDC();
   if ( !hdcListView )
   {
      /* Failure */
      goto __CLEANUP;
   }

   _ctlListView.GetClientRect(&rcListView);
   ctlHeader.GetClientRect(&rcHeader);

   ctlHeader.GetItemRect(HIWORD(dwAreaInfo),
                         &rcHdItem);

   pFileInfo = _rgFileInfo;
   pFileTail = &_rgFileInfo[_cFileInfo];

   while ( pFileInfo < pFileTail )
   {
      ZeroMemory(&rcItem,
                 sizeof(rcItem));
            
      /* If the file's summary items fit within the current page, add them to the
       * update region */
      _ctlListView.GetItemRect(pFileInfo->iItemBase,
                               &rcItem,
                               LVIR_BOUNDS);

      cyItem = (rcItem.bottom - rcItem.top);
      /* Adjust the rect to contain what should be the entire group area */
      rcItem.right  += rcListView.right;
      rcItem.bottom += (FILEINFO::CGROUPITEMSEXTRA * cyItem);

      /* The gradient is always somewhere in the last item */
      if ( FXILVA_GRADIENTS & LOWORD(dwAreaInfo) )
      {
         rcItem.top = (rcItem.bottom - cyItem);
      }

      if ( rcItem.bottom < rcListView.top )
      {
         pFileInfo++;
         /* Not in bounds */
         continue;
      }

      if ( rcItem.top > rcListView.bottom )
      {
         /* Finished gathering region data */
         break;
      }

      /* Clip the header from the update rect */
      if ( rcItem.top <= rcHeader.bottom )
      {
         rcItem.top = rcHeader.bottom;
      }

      if ( 0 != (FXILVA_GROUPS & LOWORD(dwAreaInfo)) )
      {
         rcItem.left += rcHdItem.right;
      }

      DPtoLP(hdcListView, 
             reinterpret_cast<LPPOINT>(&rcItem), 
             2);           
         
      if ( hrgnItem )
      {
         /* Reset the region coords */
         SetRectRgn(hrgnItem, 
                    rcItem.left, 
                    rcItem.top, 
                    rcItem.right, 
                    rcItem.bottom);
      }
      else
      {   
         hrgnItem = CreateRectRgnIndirect(&rcItem);
         ATLASSERT(NULL != hrgnItem);
      }
               
      if ( hrgnUpdate )
      {
         CombineRgn(hrgnUpdate, 
                    hrgnUpdate, 
                    hrgnItem, 
                    RGN_OR);
      }
      else
      {
         hrgnUpdate = CreateRectRgnIndirect(&rcItem);
         ATLASSERT(NULL != hrgnUpdate);
      }

      pFileInfo++;
   }

   /* Also include the selection area, if any */         
   iItem = _ctlListView.GetNextItem(-1, 
                                    LVNI_ALL|LVNI_FOCUSED);

   if ( -1 != iItem )
   {
      _ctlListView.GetItemRect(iItem,
                               &rcItem,
                               LVIR_SELECTBOUNDS);

      if ( (rcItem.bottom >= rcListView.top) || (rcItem.top <= rcListView.bottom) )
      {      
         DPtoLP(hdcListView, 
                reinterpret_cast<LPPOINT>(&rcItem), 
                2);

         if ( hrgnItem )
         {
            /* Reset the region coords */
            SetRectRgn(hrgnItem, 
                       rcItem.left, 
                       rcItem.top, 
                       rcItem.right, 
                       rcItem.bottom);
         }
         else
         {
            hrgnItem = ::CreateRectRgnIndirect(&rcItem);
            ATLASSERT(NULL != hrgnItem);
         }

         if ( hrgnUpdate )
         {
            CombineRgn(hrgnUpdate, 
                       hrgnUpdate, 
                       hrgnItem, 
                       RGN_OR);
         }
         else
         {
            hrgnUpdate = CreateRectRgnIndirect(&rcItem);
            ATLASSERT(NULL != hrgnUpdate);
         }
      }
   }

   /* If there is a region to invalidate, do so */
   if ( hrgnUpdate )
   {
      _ctlListView.InvalidateRgn(hrgnUpdate);
   }

__CLEANUP:
   if ( hrgnItem )
   {
      DeleteObject(hrgnItem);
   }

   if ( hrgnUpdate )
   {
      DeleteObject(hrgnUpdate);
   }

   if ( hdcListView )
   {
      _ctlListView.ReleaseDC(hdcListView);
   }
}

HRESULT CFxPropertyPage::_LoadFilesFromHDROP( HDROP hDrop ) throw()
{
   HRESULT hr;
   int     cFiles;
   int     iFile;
   
   /* Get a count of the files dropped */
   cFiles = DragQueryFile(hDrop, 
                          static_cast<UINT>(-1), 
                          NULL, 
                          0);

   if ( cFiles <= 0 )
   {
      hr = __HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES);
      /* Failure */
      return ( hr );
   }

   _bMultiFilePage = (cFiles > 1 ? 1 : 0);

   /* Allocate and initialize the file list and if anything fails, bail
    * and let the cleanup code do its thing */
   _rgFileInfo = reinterpret_cast<FILEINFO*>(malloc(sizeof(FILEINFO) * cFiles));

   if ( !_rgFileInfo )
   {
      hr = E_OUTOFMEMORY;
      /* Failure */
      return ( hr );
   }

   ZeroMemory(_rgFileInfo,
              sizeof(FILEINFO) * cFiles);

   _cFileInfo = cFiles;

   for ( iFile = 0; iFile < cFiles; iFile++ )
   {
      /* This cannot fail given that parameters passed to it have been validated */
      DragQueryFile(hDrop,
                    iFile,
                    _rgFileInfo[iFile].chFilePath,
                    _countof(_rgFileInfo[iFile].chFilePath));
   }

   /* Success */
   return ( S_OK );
}

inline HRESULT CFxPropertyPage::_GetFullFilePath( PFILEINFO pFileInfo, LPTSTR pszBuf, size_t cchBuf ) throw()
{
   HRESULT hr;

   if ( FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags )
   {
      /* Copy the base file name then append the stream name */
      hr = StringCchCopy(pszBuf,
                         cchBuf, 
                         pFileInfo->pAssociatedFile->chFilePath);
      
      if ( SUCCEEDED(hr) )
      {
         hr = StringCchCat(pszBuf, 
                           cchBuf, 
                           pFileInfo->chStreamName);
      }
   }
   else
   {
      hr = StringCchCopy(pszBuf, 
                         cchBuf, 
                         pFileInfo->chFilePath);
   }

   /* Success / Failure */
   return ( hr );
}

inline HRESULT CFxPropertyPage::_GetBaseFilePath( PFILEINFO pFileInfo, LPTSTR pszBuf, size_t cchBuf ) throw()
{
   HRESULT hr;
   hr = StringCchCopy(pszBuf, 
                      cchBuf, 
                      FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags ? pFileInfo->pAssociatedFile->chFilePath : pFileInfo->chFilePath);

   /* Success / Failure */
   return ( hr );
}

HRESULT CFxPropertyPage::_UpdateFileInfoTable( ) throw()
{
   /*
      This method updates the FILEINFO array stored in the _rgFileInfo member variable. It is called
      from the _UpdateControl() method when the FXUC_REFRESHDATA flag is specified.

      The main purpose is to reload NTFS stream data for the base file list. The base file list is
      assigned in _LoadFilesFromHDROP() and does not change for the lifetime of the control. 

      To do this, the entire table is rebuilt with new elements inserted for each NTFS stream found
      for a base file.

    */

   HRESULT        hr;
   DWORD          dwErr;
   
   PFILEINFO      pFileInfo;
   PFILEINFO      pFileInfoTail;
   PFILEINFO      rgFileInfoNew;
   PFILEINFO      pFileInfoNew;
   PFILEINFO      pFileInfoBase;

   ULONG          cBaseFiles;
   ULONG          cStreamInfos;
   ULONG          cFileInfos;
   
   PFISTREAMINFO* rgpFiStreamInfo;
   PFISTREAMINFO* ppFiStreamInfo;
   PFISTREAMINFO  pFiStreamInfo;
   PSTREAMDATA    pStreamData;
   PSTREAMDATA    pStreamDataTail;

   /* Initialize locals */
   hr              = E_FAIL;
   pFileInfoTail   = &_rgFileInfo[_cFileInfo];
   rgFileInfoNew   = NULL;
   rgpFiStreamInfo = NULL;
   cFileInfos      = 0;

   /* Scan the current table and disable any existing streams. This allows this method to
    * return without replacing the table */
   pFileInfo = _rgFileInfo;

   while ( pFileInfo < pFileInfoTail )
   {
      if ( FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags )
      {
         pFileInfo->dwFlags |= FILEINFO::F_DONOTSHOW;
      }

      pFileInfo++;
   }

   if ( !_bShowFileStreams )
   {
      hr = S_OK;
      /* Success */
      goto __CLEANUP;
   }

   /* Get a count of base file records. This cannot change during the lifetime of the property page, but the
    * number of records in _rgFileInfo may, which is why it needs to be scanned to determine the actual count */
   cBaseFiles = 0;
   pFileInfo  = _rgFileInfo;

   while ( pFileInfo < pFileInfoTail )
   {
      if ( !(FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags) )
      {
         cBaseFiles += 1;
      }

      pFileInfo++;
   }

   /* Allocate an array of pointers to FISTREAMINFO structures, 1 for each base file. */
   cStreamInfos    = cBaseFiles;
   rgpFiStreamInfo = reinterpret_cast<PFISTREAMINFO*>(malloc(sizeof(PFISTREAMINFO) * cStreamInfos));;
   if ( !rgpFiStreamInfo )
   {
      hr = E_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   ZeroMemory(rgpFiStreamInfo,
              sizeof(PFISTREAMINFO) * cStreamInfos);

   /* Enumerate the base file records. For each one, retrieve stream info */
   pFileInfo      = _rgFileInfo;
   cFileInfos     = cBaseFiles;
   ppFiStreamInfo = rgpFiStreamInfo;
   
   while ( pFileInfo < pFileInfoTail )
   {
      if ( !(FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags) )
      {
         /* Populate this base file's stream data */
         ATLASSERT(ppFiStreamInfo < &rgpFiStreamInfo[cStreamInfos]);

         dwErr = FpEnumFileStreamInformation(pFileInfo->chFilePath,
                                             FSX_STREAM_NODEFAULTDATASTREAM,
                                             &CFxPropertyPage::_EnumFileStreamInfoProc,
                                             ppFiStreamInfo);

         if ( (NO_ERROR == dwErr) && ((*ppFiStreamInfo)) )
         {
            /* Add the nubmer of streams for this base file to the total */
            cFileInfos += (*ppFiStreamInfo)->cStreams;
         }

         /* Move on to the next element in rgpFiStreamInfo */
         ppFiStreamInfo++;
      }

      /* Move on to the next element in _rgFileInfo */
      pFileInfo++;
   }

   /* If none of the base files had file streams, then cFileInfos and cBaseFiles will be equal. Existing
    * streams have already been disabled so there is nothing to do but return */
   if ( cFileInfos == cBaseFiles )
   {
      hr = S_OK;
      /* Success */
      goto __CLEANUP;
   }

   /* 
    * There are now 2 arrays that need to be joined into a new FILEINFO table. The current set of 
    * base files in _rgFileInfo and their associated streams in rgpFiStreamInfo
    */
   
   /* Allocate a new array large enough to hold both the base files and their associated streams */
   rgFileInfoNew = reinterpret_cast<FILEINFO*>(malloc(sizeof(FILEINFO) * cFileInfos));
   if ( !rgFileInfoNew )
   {
      hr = E_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   ZeroMemory(rgFileInfoNew,
              sizeof(FILEINFO) * cFileInfos);


   /* Enumerate the file base records. Copy each one to the new array. If the file has streams,
    * insert those immediately after their associated file */
   pFileInfo      = _rgFileInfo;
   pFileInfoNew   = rgFileInfoNew;
   ppFiStreamInfo = rgpFiStreamInfo;
   
   while ( pFileInfo < pFileInfoTail )
   {
      if ( !(FILEINFO::F_ISFILESTREAM & pFileInfo->dwFlags) )
      {
         /* Copy the base file record */
         CopyMemory(pFileInfoNew,
                    pFileInfo,
                    sizeof(FILEINFO));

         /* We need to manually clear old copy of hFragCtx here so that _ResetData() doesn't free it */
         pFileInfo->hFragCtx = NULL;
         
         /* Save this FILEINFO record for updating stream data */
         pFileInfoBase = pFileInfoNew;
         /* Move on to the next FILEINFO record in the new array */
         pFileInfoNew++;
         
         /* Enumerate the streams. Set up the FILEINFO record for each one */
         pFiStreamInfo = (*ppFiStreamInfo);            
         if ( pFiStreamInfo )
         {
            pStreamData     = pFiStreamInfo->siData;
            pStreamDataTail = &pFiStreamInfo->siData[pFiStreamInfo->cStreams];

            while ( pStreamData < pStreamDataTail )
            {
               pFileInfoNew->dwFlags         = FILEINFO::F_ISFILESTREAM;
               pFileInfoNew->pAssociatedFile = pFileInfoBase;
               pFileInfoNew->iIconIndex      = -1;
               pFileInfoNew->dwInfoAttrib    = 0;

               /* Copy the stream name to the FILEINFO record. The full file path is built by
                * concatenating the stream name to the associated file's chFilePath member */
               hr = StringCchCat(pFileInfoNew->chStreamName,
                                 _countof(pFileInfoNew->chStreamName),
                                 pStreamData->chName);

               /* Move on to the next stream data record */
               pStreamData++;
               /* Move on to the next FILEINFO record in the new array */
               pFileInfoNew++;
            }

            /* Free the FISTREAMINFO record, which is allocated by _EnumFileStreamInfoProc(). Note that
             * we're freeing the data, not changing the pointer which we'll access after exiting this block */
            free(pFiStreamInfo);
         }

         /* Increment the index for rgStreamInfo */
         ppFiStreamInfo++;
      }

      /* Move on to the next file in _rgFileInfo */
      pFileInfo++;
   }

   /* 
    * At this point, the new FILEINFO table has been built, so free the current one and assign the new
    */
   _ResetData();

   _rgFileInfo   = rgFileInfoNew;
   rgFileInfoNew = NULL;
   _cFileInfo    = cFileInfos;

__CLEANUP:
   if ( rgFileInfoNew )
   {
      free(rgFileInfoNew);
   }

   if ( rgpFiStreamInfo )
   {
      while ( cFileInfos-- )
      {
         if ( rgpFiStreamInfo[cFileInfos] )
         {
            //free(rgpFiStreamInfo[cFileInfos]);
         }
      }

      free(rgpFiStreamInfo);
   }

   /* Success / Failure */
   return ( hr );
}

BOOLEAN 
APIENTRY 
CFxPropertyPage::_EnumFileStreamInfoProc( 
   __in ULONG StreamCount, 
   __in ULONGLONG StreamSize, 
   __in ULONGLONG /*StreamSizeOnDisk*/,
   __in_z LPCWSTR StreamName, 
   __in_opt PVOID CallbackParameter
) throw()
{
   HRESULT        hr;
   DWORD          dwErr;
   size_t         cbData;
   ULONG          iIndex;
   PFISTREAMINFO  pEnumInfo;
   PFISTREAMINFO* ppEnumInfo;

   /* Initialize locals */
   ppEnumInfo = reinterpret_cast<PFISTREAMINFO*>(CallbackParameter);
   pEnumInfo  = (*ppEnumInfo);

   if ( !pEnumInfo )
   {
      /* This is the first iteration, so allocate the stream array */
      cbData    = sizeof(ULONG) + (sizeof(STREAMDATA) * StreamCount);
      pEnumInfo = reinterpret_cast<PFISTREAMINFO>(malloc(cbData));
      if ( !pEnumInfo )
      {
         dwErr = ERROR_OUTOFMEMORY;
         /* Failure */
         return ( FALSE );
      }

      ZeroMemory(pEnumInfo,
                 cbData);

      /* Save the stream array in the pointer provided by the caller */
      (*ppEnumInfo) = pEnumInfo;
   }

   /* Populate the current element in the stream array. The cStreams member is used to
    * track the index to be populated by the current invocation of this method. */
   iIndex = pEnumInfo->cStreams;   
   pEnumInfo->siData[iIndex].cbSize = StreamSize;

   hr = StringCchCopy(pEnumInfo->siData[iIndex].chName,
                      _countof(pEnumInfo->siData[iIndex].chName),
                      StreamName);

   if ( FAILED(hr) )
   {
      /* Free the array */
      free(pEnumInfo);

      /* Clear the caller's pointer */
      (*ppEnumInfo) = NULL;

      dwErr = HRESULT_CODE(hr);
      /* Failure */
      return ( FALSE );
   }

   /* Update the counter for the next iteration */
   pEnumInfo->cStreams += 1;
   ATLASSERT(pEnumInfo->cStreams <= StreamCount);

   /* Success */
   return ( TRUE );
}

void CFxPropertyPage::_UpdateColumns( bool bInitialUpdate ) throw()
{
   WTL::CHeaderCtrl  ctlHeader;
   int               cColumns;
   int               iColumn;
   int               iMapIdx;
   BYTE              bID;
   UINT              rgColumns[_countof(_bColMap)];
   WCHAR             chColumnText[128];
   int               cxColumnText;
   LVCOLUMN          lvColumn;
   
   /* Initialize locals */
   rgColumns[0] = MAKELONG(IDS_COLSEQUENCE,     TRUE);
   rgColumns[1] = MAKELONG(IDS_COLSIZE,         _bShowColSize);
   rgColumns[2] = MAKELONG(IDS_COLPERCENTAGE,   _bShowColPercentage);
   rgColumns[3] = MAKELONG(IDS_COLCLUSTERS,     _bShowColClusters);
   rgColumns[4] = MAKELONG(IDS_COLEXTENTS,      _bShowColExtents);
#ifdef _DEBUG
   /* This gets reset to 'Item Data' on the initial update */
   rgColumns[5] = MAKELONG(IDS_COLSEQUENCE,    TRUE);
#endif /* _DEBUG */

   /* Update the columns which have already been added */
   ctlHeader = _ctlListView.GetHeader();
   cColumns  = ctlHeader.GetItemCount();     
   iColumn   = 0;

   while ( iColumn < cColumns )
   {
      bID = _bColMap[iColumn];
      
      /* _bColMap is initialized with 0xff to indicate invalid elements */
      if ( 0xff != bID )
      {
         if ( HIWORD(rgColumns[bID]) )
         {
            /* The column is already displayed, prevent it from being duplicated */
            rgColumns[bID] &= 0x0000ffff;
         }
         else
         {
            /* First, copy back the remaining column's map indexes because the listview is 
             * going to immediately begin sending LVN_GETDISPINFO requests */
            for ( iMapIdx = iColumn + 1; iMapIdx < _countof(_bColMap); iMapIdx++ )
            {
               bID = _bColMap[iMapIdx];
               _bColMap[iMapIdx - 1] = bID;
            }

            /* Invalidate the last column */
            _bColMap[_countof(_bColMap) - 1] = 0xff;

            /* Remove the column... */
            _ctlListView.DeleteColumn(iColumn);

            /* Start from the current index, as it now has a new value */
            continue;
         }
      }

      /* Move on to the next column */
      iColumn++;
   }

   /* Update currently displayed columns */
   cColumns = ctlHeader.GetItemCount();
   iMapIdx  = cColumns;
   iColumn  = 0;

   if ( bInitialUpdate )
   {
      /* Add a dummy column to allow right-aligning the *first* column */
      _ctlListView.InsertColumn(cColumns++, 
                                L"");
   }

   ZeroMemory(&lvColumn,
              sizeof(lvColumn));

   while ( iColumn < _countof(rgColumns) )
   {
      if ( HIWORD(rgColumns[iColumn]) )
      {
         /* Update this column's map index */
         _bColMap[iMapIdx++] = (BYTE)iColumn;

         /* Load the column's text */
         LoadString(_AtlModule.GetMUIInstance(),
                    LOWORD(rgColumns[iColumn]), 
                    chColumnText, 
                    _countof(chColumnText));
         ATLASSERT(L'\0' != *chColumnText);
      
         /* Calculate the pixel width of the column text */
         cxColumnText = _ctlListView.GetStringWidth(chColumnText);

         /* Update the column */
         lvColumn.mask     = LVCF_FMT|LVCF_SUBITEM|LVCF_TEXT|LVCF_WIDTH;
         lvColumn.fmt      = LVCFMT_RIGHT;
         lvColumn.cx       = (cxColumnText > 0 ? cxColumnText + 16 : 65);
         lvColumn.pszText  = chColumnText;
         lvColumn.iSubItem = ++cColumns;

         _ctlListView.InsertColumn(cColumns, 
                                   &lvColumn);
      }

      iColumn++;
   }

   if ( bInitialUpdate )
   {
#ifdef _DEBUG
      lvColumn.mask    = LVCF_TEXT;
      lvColumn.pszText = L"Item Info";
      _ctlListView.SetColumn(cColumns - 1, 
                             &lvColumn);
#endif /* _DEBUG */
      /* Remove the dummy column */
      _ctlListView.DeleteColumn(0);
   }
}

void CFxPropertyPage::_CreateCachedGroupSeperator( ) throw()
{
   COLORREF clrGradBegin;
   COLORREF clrGradEnd;
   RECT     rcListView;

   if ( !_ctlTheme.IsThemeNull() )
   {
      clrGradBegin = _ctlTheme.GetThemeSysColor(COLOR_ACTIVECAPTION);
      clrGradEnd   = _ctlTheme.GetThemeSysColor(COLOR_WINDOW);
   }
   else
   {
      clrGradBegin = GetSysColor(COLOR_ACTIVECAPTION);
      clrGradEnd   = GetSysColor(COLOR_WINDOW);
   }
      
   _ctlListView.GetClientRect(&rcListView);
   _bmGradient = CreateGradientBitmap(_ctlListView, 
                                      ((rcListView.right - rcListView.left) / 10) * 10, 
                                      1, 
                                      clrGradBegin, 
                                      clrGradEnd);   
}

void CFxPropertyPage::_OnDefragmentFile( int iFile, bool bShiftKey ) throw()
{
   HRESULT         hr;
   IDefragManager* pDefragMgr;
   int             iLast;
   PFILEINFO       pFileInfo;
   WCHAR           chBuf[MAX_STREAM_PATH];
   
   /* Initialize locals.. */
   pDefragMgr = NULL;

   hr = CoCreateInstance(__uuidof(DefragManager),
                         NULL,
                         CLSCTX_LOCAL_SERVER,
                         __uuidof(IDefragManager),
                         reinterpret_cast<LPVOID*>(&pDefragMgr));

   if ( FAILED(hr) )
   {
      _dTrace(TRUE, L"Failed to create DefragManager instance!0x%08lx\n", hr);
      /* Failure */
      return;
   }
   
   iFile = (bShiftKey ? 0 : iFile);
   iLast = (bShiftKey ? _cFileInfo : iFile + 1);

   while ( iFile < iLast )
   {
      pFileInfo = &_rgFileInfo[iFile++];
      ATLASSERT(NULL != pFileInfo);

      if ( !(FILEINFO::F_DONOTSHOW & pFileInfo->dwFlags) )
      {
         _GetFullFilePath(pFileInfo,
                          chBuf,
                          _countof(chBuf));
         ATLASSERT(L'\0' != *chBuf);

         hr = pDefragMgr->QueueFile(chBuf);
         ATLASSERT(SUCCEEDED(hr));
      }
   }

   pDefragMgr->Release();
}

void CFxPropertyPage::_OnSaveReportTo( int iFile, bool bShiftKey, int eReportMode ) throw()
{
   HRESULT     hr;
   PFILEINFO   pFileInfo;
   PFILEINFO   pFileTail;
   CFragReport cReport;

   hr = cReport.OpenReport(eReportMode,
                           m_hWnd);

   if ( SUCCEEDED(hr) )
   {
      pFileInfo = (bShiftKey ? _rgFileInfo : &_rgFileInfo[iFile]);
      pFileTail = (bShiftKey ? &_rgFileInfo[_cFileInfo] : pFileInfo + 1);

      while ( pFileInfo < pFileTail )
      {
         if ( !(FILEINFO::F_DONOTSHOW & pFileInfo->dwFlags) )
         {
            hr = cReport.AppendFile(pFileInfo->chFilePath,
                                    pFileInfo->hFragCtx);

            if ( FAILED(hr) )
            {
               break;
            }
         }

         pFileInfo++;
      }

      if ( SUCCEEDED(hr) )
      {
         if ( CFragReport::WriteModeClipboard == eReportMode )
         {
            cReport.CopyToClipboard(m_hWnd);
         }
      }

      cReport.CloseReport();
   }
}

void CFxPropertyPage::_OnFileProperties( int iFile, bool /*bShiftKey*/ ) throw()
{
   WCHAR chPath[MAX_STREAM_PATH];

   /* The shell doesn't support NTFS stream names, so use the base file */
   _GetBaseFilePath(&_rgFileInfo[iFile], 
                    chPath, 
                    _countof(chPath));
   
   _SHFileProperties(m_hWnd,
                     chPath);
}

HRESULT CFxPropertyPage::_BuildItemInfoTable( ) throw()
{
   ULONG     idx;
   ULONG     iFile;
   UINT      iItem;
   UINT      cItemInfo;
   PITEMINFO rgItemInfo;
   PITEMINFO pItemInfo;
   PFILEINFO pFileInfo;
   PFILEINFO pFileInfoTail;

   /* Determine the number of required ITEMINFO structures */
   cItemInfo     = 0;
   pFileInfo     = _rgFileInfo;
   pFileInfoTail = &_rgFileInfo[_cFileInfo];

   while ( pFileInfo < pFileInfoTail )
   {
      cItemInfo += FILEINFO::CGROUPITEMSEXTRA;

      _UpdateFileFragmentInfo(pFileInfo);

      cItemInfo += pFileInfo->cFragments;

      pFileInfo++;
   }

   /* Allocate memory for the ITEMINFO structures */
   rgItemInfo = reinterpret_cast<ITEMINFO*>(malloc(sizeof(ITEMINFO) * cItemInfo));
   if ( !rgItemInfo )
   {
      /* Failure */
      return ( E_OUTOFMEMORY );
   }

   ZeroMemory(rgItemInfo,
              sizeof(ITEMINFO) * cItemInfo);

   /* Populate the ITEMINFO table */
   iItem     = 0;
   iFile     = 0;
   pItemInfo = rgItemInfo;
   pFileInfo = _rgFileInfo;

   while ( pFileInfo < pFileInfoTail )
   {
      /* Update the file's item base */
      pFileInfo->iItemBase = iItem;

      /* Build the group items */
      for ( idx = 0; idx < FILEINFO::CGROUPITEMSEXTRA; idx++ )
      {
         pItemInfo->FileIndex = iFile;
         pItemInfo->FragIndex = ULONG_MAX;
         pItemInfo++;
      }

      iItem += FILEINFO::CGROUPITEMSEXTRA;

      /* Build the fragment items */
      for ( idx = 1; idx <= pFileInfo->cFragments; idx++ )
      {
         pItemInfo->FileIndex = iFile;
         pItemInfo->FragIndex = idx;
         pItemInfo++;
      }

      iItem += pFileInfo->cFragments;
      iFile += 1;
      pFileInfo++;
   }

   /* Assign the new table to the class instance */
   _cItemInfo  = cItemInfo;
   _rgItemInfo = rgItemInfo;

   /* Success */
   return ( S_OK );
}

void CFxPropertyPage::_ClearItemInfoTable( ) throw()
{
   if ( _rgItemInfo )
   {
      free(_rgItemInfo);
      _rgItemInfo = NULL;
   }

   _cItemInfo = 0;
}

CFxPropertyPage::PITEMINFO CFxPropertyPage::_GetItemInfo( int iItem ) throw()
{
   PITEMINFO pItemInfo;
   ATLASSERT(iItem < _cItemInfo);
   ATLASSERT(iItem >= 0);   
   pItemInfo = &_rgItemInfo[iItem];
   /* Success */
   return ( pItemInfo );
}

CFxPropertyPage::PFILEINFO CFxPropertyPage::_GetItemFileInfo( int iItem ) throw()
{
   PITEMINFO pItemInfo;
   PFILEINFO pFileInfo;

   pItemInfo = _GetItemInfo(iItem);
   pFileInfo = &_rgFileInfo[pItemInfo->FileIndex];

   /* Success */
   return ( pFileInfo );
}

int CFxPropertyPage::_GetItemFileIndex( int iItem ) throw()
{
   PITEMINFO pItemInfo;
   ULONG     iFileIdx;

   pItemInfo = _GetItemInfo(iItem);
   iFileIdx  = pItemInfo->FileIndex;

   /* Success */
   return ( iFileIdx );
}

ULONG CFxPropertyPage::_GetItemFragIndex( int iItem ) throw()
{
   PITEMINFO pItemInfo;
   ULONG     iFragIdx;

   pItemInfo = _GetItemInfo(iItem);
   iFragIdx  = pItemInfo->FragIndex;

   /* Success */
   return ( iFragIdx );
}

bool CFxPropertyPage::_IsFileInfoItem( int iItem ) throw()
{
   PITEMINFO pItemInfo;
   bool      bRet;

   pItemInfo = _GetItemInfo(iItem);
   bRet = (pItemInfo->FragIndex == ULONG_MAX ? true : false);
   
   /* Success / Failure */
   return ( bRet );
}

bool CFxPropertyPage::_IsFragInfoItem( int iItem ) throw()
{
   bool bRet;
   bRet = !_IsFileInfoItem(iItem);

   /* Success / Failure */
   return ( bRet );
}

void CFxPropertyPage::_SortItemInfoTable( ) throw()
{
   qsort_s(_rgItemInfo,
           _cItemInfo,
           sizeof(ITEMINFO),
           &CFxPropertyPage::_CompareItemInfoProc,
           this);
}

int CFxPropertyPage::_CompareItemInfo( const ITEMINFO* pItem1, const ITEMINFO* pItem2 ) throw()
{
   int          iRet;
   BYTE         bMapID;
   ULONG        iFile1;
   ULONG        iFile2;
   ULONG        iFrag1;
   ULONG        iFrag2;
   DWORD        dwErr;
   FRAGMENT_INFORMATION FragInfo1;
   FRAGMENT_INFORMATION FragInfo2;

   /* Initialize locals */
   iRet   = 0;
   bMapID = _bColMap[_wSortCol];

   ZeroMemory(&FragInfo1,
              sizeof(FRAGMENT_INFORMATION));

   ZeroMemory(&FragInfo2,
              sizeof(FRAGMENT_INFORMATION));

   iFile1 = pItem1->FileIndex;
   iFrag1 = pItem1->FragIndex;

   iFile2 = pItem2->FileIndex;
   iFrag2 = pItem2->FragIndex;

   /* If the files are different, sort by their index, regardless of the sorting direction */
   if ( iFile1 != iFile2 )
   {
      return ( iFile1 > iFile2 ? 1 : -1 );
   }
            
   /* If either of the frag indexes are invalid, revert to sorting
    * on the sequence number */
   if ( (ITEMINFO::INVALIDFRAGINDEX == iFrag1) || (ITEMINFO::INVALIDFRAGINDEX == iFrag2) )
   {
      bMapID = MapIdSequence;
   }

   /* Load fragment data if sorting on it. This doesn't occur when sorting by 
    * the sequence number */
   if ( MapIdSequence != bMapID )
   {
      dwErr = FpGetContextFragmentInformation(_rgFileInfo[iFile1].hFragCtx,
                                              iFrag1,
                                              &FragInfo1,
                                              1,
                                              NULL);
      ATLASSERT(NO_ERROR == dwErr || ERROR_MORE_DATA == dwErr);

      dwErr = FpGetContextFragmentInformation(_rgFileInfo[iFile2].hFragCtx,
                                              iFrag2,
                                              &FragInfo2,
                                              1,
                                              NULL);
      ATLASSERT(NO_ERROR == dwErr || ERROR_MORE_DATA == dwErr);
   }

   /* Initialize locals */
   iRet = 0;

   while ( true )
   {
      switch ( bMapID )
      {
         case MapIdSequence:
            /* If both items are for the group item of the file then leave them in place */
            if ( (ITEMINFO::INVALIDFRAGINDEX == iFrag1) && (ITEMINFO::INVALIDFRAGINDEX == iFrag2) )
            {
               iRet = 0;
            }
            /* If only the first item is a group item, make sure it is greater, regardless of the
             * current sorting direction */
            else if ( ITEMINFO::INVALIDFRAGINDEX == iFrag1 )
            {
               iRet = (_bSortASC ? -1 : 1);
            }
            /* If only the second item is a group item, make sure it is greater, regardless of the
             * current sorting direction */
            else if ( ITEMINFO::INVALIDFRAGINDEX == iFrag2 )
            {
               iRet = (_bSortASC ? 1 : -1);
            }
            /* The items are for different fragments, sort on their indexes */
            else
            {
               iRet = (iFrag1 > iFrag2 ? 1 : -1);
            }

            break;

         case MapIdSize:
            /* This point should not be reached unless the frag indexes are valid */
            ATLASSERT(ITEMINFO::INVALIDFRAGINDEX != iFrag1 && ITEMINFO::INVALIDFRAGINDEX != iFrag2);

            /* If the cluster counts are equal, sort on the sequence instead */
            if ( FragInfo1.ClusterCount == FragInfo2.ClusterCount )
            {
               bMapID = MapIdSequence;
               continue;
            }

            /* Return the larger clsuter size */
            iRet = (FragInfo1.ClusterCount > FragInfo2.ClusterCount ? 1 : -1);
            break;

         /* % of File */
         case MapIdPercentage:
         {
            /* This point should not be reached unless the frag indexes are valid */
            ATLASSERT(ITEMINFO::INVALIDFRAGINDEX != iFrag1 && ITEMINFO::INVALIDFRAGINDEX != iFrag2);

            /* If both fragments are COMPRESSED/SPARSE units, fall back to sorting on
             * their size */
            if ( IsVirtualClusterNumber(FragInfo1.LogicalCluster) && IsVirtualClusterNumber(FragInfo2.LogicalCluster) )
            {
               bMapID = MapIdSize;
               continue;
            }
            /* If only the first item is a C/S unit, force it to be less than */
            else if ( IsVirtualClusterNumber(FragInfo1.LogicalCluster) )
            {
               iRet = -1;
            }
            /* If only the second item is a C/S unint, force it to be the greater */
            else if ( IsVirtualClusterNumber(FragInfo2.LogicalCluster) )
            {
               iRet = 1;
            }
            /* The items are not C/S units, so fall back to sorting on size */
            else
            {
               bMapID = MapIdSize;
               continue;
            }

            break;
         }

         /* Clusters - This is a factor of size, so sorting on size has the same effect */
         case MapIdClusters:
         {
            bMapID = MapIdSize;
            continue;
         }

         /* Extents */
         case MapIdExtents:
         {
            /* This point should not be reached unless the frag indexes are valid */
            ATLASSERT(ITEMINFO::INVALIDFRAGINDEX != iFrag1 && ITEMINFO::INVALIDFRAGINDEX != iFrag2);

            if ( FragInfo1.ExtentCount == FragInfo2.ExtentCount )
            {
               /* Extent sizes are equal, so sort on the size */
               bMapID = MapIdSize;
               continue;
            }

            iRet = (FragInfo1.ExtentCount > FragInfo2.ExtentCount ? 1 : -1);
            break;
         }

#ifdef _DEBUG
         /* Item Data */
         case MapIdItemInfo:
         {            
            iRet = (iFrag1 > iFrag2 ? 1 : -1);

            break;
         }
#endif /* _DEBUG */

         default:
            /* This point should never be reached */
            ATLASSERT(FALSE);
            __assume(0);
      }

      /* Finished sorting */
      break;
   }

   /* Apply a conversion based on the sorting direction that will invert the
    * result when sorting in DESCENDING mode */
   iRet = _bSortASC ? iRet : -1 * iRet;

   /* Success */
   return ( iRet  );
}

int __cdecl CFxPropertyPage::_CompareItemInfoProc( void* pContext, const void* pElem1, const void* pElem2 ) throw()
{
   int              iRet;
   CFxPropertyPage* pThis;
   const ITEMINFO*  pItem1;
   const ITEMINFO*  pItem2;

   pThis  = reinterpret_cast<CFxPropertyPage*>(pContext);
   pItem1 = reinterpret_cast<const ITEMINFO*>(pElem1);
   pItem2 = reinterpret_cast<const ITEMINFO*>(pElem2);

   iRet = pThis->_CompareItemInfo(pItem1,
                                  pItem2);

   /* Success */
   return ( iRet );
}
