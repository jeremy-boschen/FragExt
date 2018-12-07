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
 
/* PropPage.h
 *    Declaration of CFxPropertyPage
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 07/13/2004 - Created
 */

#pragma once

#include <FragEng.h>

#include <AtlEx.h>
using namespace ATLEx;
#include <ShellUtil.h>

#define MAX_STREAM_PATH (MAX_PATH + 36)

/**
 * CLSID_PropertyPage
 *    {8935BD84-0BDB-4AE5-869C-18EEA4E81D77}
 */
extern "C" const CLSID CLSID_PropertyPage = {0x8935BD84, 0x0BDB, 0x4AE5, {0x86, 0x9C, 0x18, 0xEE, 0xA4, 0xE8, 0x1D, 0x77}};

/**
 * CFxPropertyPage
 *    Shell property sheet extension handler.
 */
class ATL_NO_VTABLE CFxPropertyPage : public CComObjectRootEx<CComSingleThreadModel>,
                                      public CComCoClass<CFxPropertyPage, &CLSID_PropertyPage>,
                                      public IShellExtInit,
                                      public IShellPropSheetExt,
                                      public WTL::CPropertyPageImpl<CFxPropertyPage>
{
   /* ATL Interface */
public:
   DECLARE_NO_REGISTRY()

   DECLARE_NOT_AGGREGATABLE(CFxPropertyPage)

   BEGIN_COM_MAP(CFxPropertyPage)
	   COM_INTERFACE_ENTRY(IShellExtInit)
      COM_INTERFACE_ENTRY(IShellPropSheetExt)
   END_COM_MAP()

   DECLARE_PROTECT_FINAL_CONSTRUCT()

   HRESULT FinalConstruct( ) throw();
   void FinalRelease( ) throw();

   /* WTL requirements */
public:
   enum
   {
      IDD              = IDD_FRAGMENT_INFORMATION,
      WM_UPDATECONTROL = WM_APP + 74
   };

   BEGIN_MSG_MAP(CFxPropertyPage)
      /* IDD_FRAGMENT_INFORMATION */
      MESSAGE_HANDLER(WM_SETTINGCHANGE,   OnSettingChange)
      MESSAGE_HANDLER(WM_SYSCOLORCHANGE,  OnSysColorChange)
      MESSAGE_HANDLER(WM_THEMECHANGED,    OnThemeChanged)
      MESSAGE_HANDLER(WM_INITDIALOG,      OnInitDialog)
      MESSAGE_HANDLER(WM_SIZE,            OnSize)      
      MESSAGE_HANDLER(WM_UPDATECONTROL,   OnUpdateControl)

      /* IDC_LSVFRAGMENTS */
      NOTIFY_HANDLER(IDC_LSVFRAGMENTS, LVN_ITEMCHANGED,  OnListViewItemChanged)
      NOTIFY_HANDLER(IDC_LSVFRAGMENTS, LVN_GETDISPINFO,  OnListViewGetDispInfo)
      NOTIFY_HANDLER(IDC_LSVFRAGMENTS, LVN_GETINFOTIP,   OnListViewGetInfoTip)
      NOTIFY_HANDLER(IDC_LSVFRAGMENTS, LVN_COLUMNCLICK,  OnListViewColumnClick)
      NOTIFY_HANDLER(IDC_LSVFRAGMENTS, NM_CUSTOMDRAW,    OnListViewCustomDraw)
      NOTIFY_HANDLER(IDC_LSVFRAGMENTS, NM_RCLICK,        OnListViewRightClick)
      
      /* IDC_HDRFRAGMENTS */
      NOTIFY_HANDLER(IDC_HDRFRAGMENTS, NM_RCLICK,        OnHeaderRightClick)
      NOTIFY_HANDLER(IDC_HDRFRAGMENTS, HDN_ITEMCHANGING, OnHeaderItemChanging)      
      
      /* Tool menu commands */
      MENUCOMMAND_HANDLER(IDM_PROPDEFRAGMENTFILE,  OnToolMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPREFRESHFILE,     OnToolMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPREPORTTOCLIP,    OnToolMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPREPORTTOFILE,    OnToolMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPSHOWCOMPRESSED,  OnToolMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPPROPERTIES,      OnToolMenuClick)

      /* Column menu commands */      
      MENUCOMMAND_HANDLER(IDM_PROPCOLSEQUENCE,     OnColumnMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPCOLSIZE,         OnColumnMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPCOLPERCENTAGE,   OnColumnMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPCOLCLUSTERS,     OnColumnMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPCOLEXTENTS,      OnColumnMenuClick)
      MENUCOMMAND_HANDLER(IDM_PROPCOLAUTOSIZE,     OnColumnMenuClick)
      
      
      /* PropertyPage */
      CHAIN_MSG_MAP(WTL::CPropertyPageImpl<CFxPropertyPage>)

   /* IDC_LSVFRAGMENTS MSG_MAP */
   ALT_MSG_MAP(1)
      MESSAGE_HANDLER(WM_HSCROLL,         OnScListViewHScroll)
      MESSAGE_HANDLER(WM_KEYDOWN,         OnScListViewKeyDown)
      MESSAGE_HANDLER(WM_LBUTTONDOWN,     OnScListViewLButtonDown)
      MESSAGE_HANDLER(WM_RBUTTONDOWN,     OnScListViewRButtonDown)

      /* We only do custom double buffering in the absence of CC6 */
      if ( !_bIsCC6 )
      {
         MESSAGE_HANDLER(WM_PAINT,        OnScListViewPaint)
         MESSAGE_HANDLER(WM_ERASEBKGND,   OnScListViewEraseBkgnd)
      }
   END_MSG_MAP()

   LRESULT OnInitDialog( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnSettingChange( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnSysColorChange( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnThemeChanged( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnUpdateControl( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();

   LRESULT OnListViewGetDispInfo( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnListViewGetInfoTip( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnListViewCustomDraw( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnListViewItemChanged( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnListViewRightClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnListViewColumnClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnHeaderRightClick( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   LRESULT OnToolMenuClick( WORD wItemIndex, WORD wID, HMENU hMenu, BOOL& bHandled ) throw();
   LRESULT OnColumnMenuClick( WORD wItemIndex, WORD wID, HMENU hMenu, BOOL& bHandled ) throw();

   LRESULT OnScListViewHScroll( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnScListViewKeyDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnScListViewLButtonDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnScListViewRButtonDown( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnScListViewPaint( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   LRESULT OnScListViewEraseBkgnd( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled ) throw();
   
   LRESULT OnHeaderItemChanging( int idCtrl, LPNMHDR pnmh, BOOL& bHandled ) throw();
   
   /* WTL::CPropertyPageImpl<CFxPropertyPage> */
public:
   bool OnPageAddRef( ) throw();
   void OnPageRelease( ) throw();

   /* IShellExtInit */
public:
   STDMETHOD(Initialize)( LPCITEMIDLIST pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID ) throw();

   /* IShellPropSheetExt */
public:
   STDMETHOD(AddPages)( LPFNSVADDPROPSHEETPAGE pfnAddPage, LPARAM lParam ) throw();
   STDMETHOD(ReplacePage)( EXPPS uPageID, LPFNSVADDPROPSHEETPAGE pfnReplaceWith, LPARAM lParam ) throw();

   /* Class Interface */
public:
	CFxPropertyPage( ) throw();

   /* Restricted */
private:
   
   /* Structure for storing file information to be displayed
    * by IDC_LSVFRAGMENTS */
   typedef struct _FILEINFO
   {
      enum
      {         
         CFRAGINDEXMAX        = 0x7fff,/* 32,767 */
         CFILEINDEXMAX        = 0xffff,/* 65,535 */
         CLISTVIEWMAX         = INT_MAX,
         
         /* Miscellaneous flags for dwFlags */
         F_DONOTSHOW          = 0x00000001,
         F_REFRESHSUMMARY     = 0x00000002,
         F_FRAGSUNKNOWN       = 0x00000004,
         F_SIZEUNKNOWN        = 0x00000008,
         /*
         F_FSISFAT            = 0x00000010,
         F_FSISFAT32          = 0x00000020,
         F_FSISNTFS           = 0x00000040,
         F_FSISUNKNOWN        = 0x00000080,
          */
         F_ISRESTRICTED       = 0x00000100,
         F_ISHIDDENEXT        = 0x00000200,
         F_ISFILESTREAM       = 0x00000400,

         /* Extra list items added for summary info */
         CGROUPITEMSEXTRA     = 3,

         /* Size of summary text */
         CCHSUMMARY           = 768
      };

      /* Miscellaneous bits */
      DWORD                            dwFlags;

      /* Base listview index for the file */
      int                              iItemBase;

      /* Display attributes from CShellImageList::GetFileInfo */
      int                              iIconIndex;
      DWORD                            dwInfoAttrib;

      /* Frag context handle for the associated file */
      HANDLE                           hFragCtx;
      ULONG                            cFragments;

      #pragma warning( disable : 4201 )
      union
      {
         /* Fully-qualified path of the file */
         TCHAR                         chFilePath[MAX_STREAM_PATH];

         struct
         {
            /* Associated FILEINFO record for the stream */
            struct _FILEINFO*          pAssociatedFile;
            /* NTFS stream name */
            TCHAR                      chStreamName[MAX_STREAM_PATH];
         };
      };
      #pragma warning( default : 4201 )
      /* Summary text for the item */
      TCHAR                            chSummary[CCHSUMMARY];
   }FILEINFO, *PFILEINFO;
   typedef const FILEINFO* PCFILEINFO;
   
   /* Records stored in _rgItemInfo for translating an item index
    * from IDC_LSVFRAGMENTS to the data it represents */
   typedef struct _ITEMINFO
   {
      enum
      {
         INVALIDFILEINDEX = ULONG_MAX,
         INVALIDFRAGINDEX = ULONG_MAX
      };

      /* _rgFileInfo index */
      ULONG FileIndex;
      /* FRAGMENT_INFORMATION::Sequence value, or INVALIDFRAGINDEX for group items */
      ULONG FragIndex;
   }ITEMINFO, *PITEMINFO;

   /* Records used by _LoadFileStreams() & _EnumFileStreamInfoProc() to
    * initialize stream data */
   typedef struct _STREAMDATA
   {
      ULONGLONG cbSize;
      WCHAR     chName[MAX_STREAM_PATH];
   }STREAMDATA, *PSTREAMDATA;

   typedef struct _FISTREAMINFO
   {
      ULONG       cStreams;
      STREAMDATA  siData[1];
   }FISTREAMINFO, *PFISTREAMINFO;

   enum
   {
      /* Trace level for atlTraceCOM category */
      eTraceLevelCOM  = 1,

      /* Values for _bColMap[] and _wSortCol */
      MapIdSequence   = 0,
      MapIdSize       = 1,
      MapIdPercentage = 2,
      MapIdClusters   = 3,
      MapIdExtents    = 4,
   #ifdef _DEBUG
      /* The ItemData column is added in debug builds for viewing an item's item data value */
      MapIdItemInfo   = 5,
   #endif
      cColMap,

      /* Values for _UpdateControl::dwFlags */
      FXUC_REFRESHLIST = 0x00000001,
      FXUC_REFRESHDATA = 0x00000002,

      /* Values for LOWORD of _InvalidateListViewArea::dwAreaInfo */
      FXILVA_GROUPS    = 0x0001,
      FXILVA_GRADIENTS = 0x0002
   };


   /* Listview helpers... */
   void _UpdateControl( DWORD dwFlags ) throw();
   int _GetNextListViewItem( int iItem, UINT uFlags ) throw();
   void _InvalidateListViewArea( DWORD dwAreaInfo ) throw();   
   void _UpdateColumns( bool bInitialUpdate ) throw();
   void _CreateCachedGroupSeperator( ) throw();
   
   /* Data helpers */
   void _ResetData( ) throw();
   
   HRESULT _LoadFilesFromHDROP( HDROP hDrop ) throw();
   void _ClearFileTable( ) throw();
   HRESULT _UpdateFileInfoTable( ) throw();
   void _UpdateFileFragmentInfo( PFILEINFO pFileInfo ) throw();   
   void _UpdateFileSummaryInfo( PFILEINFO pFileInfo, HDC hDC, UINT cxWidth ) throw();
   
   static 
   BOOLEAN 
   APIENTRY 
   _EnumFileStreamInfoProc(    
      __in ULONG StreamCount, 
      __in ULONGLONG StreamSize, 
      __in ULONGLONG StreamSizeOnDisk,
      __in_z LPCWSTR StreamName, 
      __in_opt PVOID CallbackParameter
   ) throw();
   
   HRESULT _BuildItemInfoTable( ) throw();
   void _ClearItemInfoTable( ) throw();
   PITEMINFO _GetItemInfo( int iItem ) throw();
   PFILEINFO _GetItemFileInfo( int iItem ) throw();
   int _GetItemFileIndex( int iItem ) throw();
   ULONG _GetItemFragIndex( int iItem ) throw();
   bool _IsFileInfoItem( int iItem ) throw();
   bool _IsFragInfoItem( int iItem ) throw();
   void _SortItemInfoTable( ) throw();
   int _CompareItemInfo( const ITEMINFO* pItem1, const ITEMINFO* pItem2 ) throw();
   static int __cdecl _CompareItemInfoProc( void* pContext, const void* pElem1, const void* pElem2 ) throw();
   void _GetItemFragmentInfo( int iItem, PFRAGMENT_INFORMATION pFragInfo ) throw();

   static bool _IsHiddenFileExtension( LPCTSTR pszPath ) throw();   
   void _LoadAltColorSetting( ) throw();
   void _LoadSettings( ) throw();
   void _SaveSettings( ) throw();

   static HRESULT _GetBaseFilePath( PFILEINFO pFileInfo, LPTSTR pszBuf, size_t cchBuf ) throw();
   static HRESULT _GetFullFilePath( PFILEINFO pFileInfo, LPTSTR pszBuf, size_t cchBuf ) throw();
   
   void _OnDefragmentFile( int iFile, bool bShiftKey ) throw();
   void _OnSaveReportTo( int iFile, bool bShiftKey, int eReportMode ) throw();
   void _OnFileProperties( int iFile, bool bShiftKey ) throw();

   /* File(s) selected by the user - Allocated on the heap */
   int                  _cFileInfo;
   PFILEINFO            _rgFileInfo;
   /* Listview item data*/
   int                  _cItemInfo;
   PITEMINFO            _rgItemInfo;
   /* Previously selected item, if any */
   int                  _iCurSel;
   /* Subclassed listview control */
   typedef CContainedWindowT<WTL::CListViewCtrl> DlgListView;
   DlgListView          _ctlListView;
   /* Theme support */
   WTL::CTheme          _ctlTheme;
   /* FILEINFO icons */
   CShellImageList      _ctlShellIL;
   /* Text color for compressed fragments */
   COLORREF             _clrCompressed;
   /* Cached group view gradient bitmap */
   WTL::CBitmapT<true>  _bmGradient;
   /* NTFS stream overlay icon */
   HICON                _hStreamIco;
   /* Used to map sub-item to known column index */
   BYTE                 _bColMap[cColMap];
   #pragma warning( disable : 4201 )
   struct
   {
      /* Currently sorted column Id */
      ULONG             _wSortCol : 16;
      /* Current sort direction */
      ULONG             _bSortASC : 1;
      /* Common Controls 6+ presence */
      ULONG             _bIsCC6   : 1;
      /* */
      ULONG             _bShowCompressed  : 1;

      ULONG             _bShowFileStreams : 1;

      /* Column visibility */
      ULONG             _bShowColSize       : 1;
      ULONG             _bShowColPercentage : 1;
      ULONG             _bShowColClusters   : 1;
      ULONG             _bShowColExtents    : 1;

      /* Single/Multiple files */
      ULONG             _bMultiFilePage     : 1;
   };
   #pragma warning( default : 4201 )
};

OBJECT_ENTRY_AUTO(CLSID_PropertyPage, CFxPropertyPage)
