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

/* PropList.h
 *    PropertyList control
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once


// ... here.. need some property maps or something for defining tree items

#define PCTL_TYPE_CHECKBOX       1
#define PCTL_TYPE_DROPDOWNLIST   2
#define PCTL_TYPE_DROPDOWNEDIT   3
#define PCTL_TYPE_TEXTBOX        4
#define PCTL_TYPE_UPDOWN         5

/*
   1) property name/text
   2) initial property value
   3) set property value
   4) get property value
 */

class ATL_NO_VTABLE IProperty
{
public:
   virtual GetValue( void* pData, size_t cbData ) = 0;
   virtual SetValue( void* pData, size_t cbData ) = 0;
};

class IPropertyCtl
{
public:
   virtual Create( HWND hwndParent ) = 0;
};

BEGIN_PROPCTL_MAP( _Class )
   PROPCTL_ENTRY(IDS_
END_PROPCTL_MAP()


typedef ATL::CWinTraits<WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE> CPropertyCtrlTraits;

template <class T, class TBase = ATL::CWindow, class TWinTraits = CPropertyCtrlTraits >
class ATL_NO_VTABLE CPropertyCtrlImpl : public ATL::CWindowImpl<T, TBase, TWinTraits>
{
   /* ATL/WTL */
public:
   typedef  ATL::CWindowImpl<T, TBase, TWinTraits> _AtlWindowImpl;

	DECLARE_WND_CLASS(NULL)

   HWND Create( HWND hWndParent, _U_RECT rect = rcDefault, _U_MENUorID nID = 0U )
   {
      return ( _AtlWindowImpl::Create(hWndParent, rect, NULL, TWinTraits::GetWndStyle(0), TWinTraits::GetWndExStyle(0), nID, NULL) );
   }

   BOOL SubclassWindow( HWND hWnd )
   {
      BOOL bRet;
      bRet = _AtlWindowImpl::SubclassWindow(hWnd);
      if ( bRet ) 
      {
         bRet = _Initialize();
      }

      return ( bRet );
   }

  	BEGIN_MSG_MAP(CPropertyCtrlImpl)
      MESSAGE_HANDLER(WM_CREATE, OnCreate)
      MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
      MESSAGE_HANDLER(WM_SIZE, OnSize)
   END_MSG_MAP()

   LRESULT OnCreate( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
	{
      T*   pT;
      BOOL bRet;

      /* Initialize the control */
      pT = static_cast<T*>(this);
      bRet = pT->_Initialize();
      if ( !bRet ) 
      {
         return ( -1 );
      }

      return ( 0 );
	}

   LRESULT OnDestroy( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ )
   {
      LRESULT lRet;

      /* Do default processing */
      lRet = DefWindowProc(uMsg,
                           wParam,
                           lParam);

      if ( NULL != _hFont )
      {
         DeleteObject(_hFont);
      }

      return ( lRet );
   }

   LRESULT OnSize( UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/ )
   {
      LRESULT lRet;

      /* Let the window do its thing first */
      lRet = DefWindowProc(uMsg,
                           wParam,
                           lParam);

      _UpdateLayout();

      return ( lRet );
   }

   /* CPropertyCtlImpl */
public:
   HFONT              _hFont;
   WTL::CHeaderCtrl   _ctlHeader;
   WTL::CTreeViewCtrl _ctlTree;

   CPropertyCtrlImpl( ) throw()
   {
      _hFont = NULL;
   }

   BOOL _Initialize( ) 
   {
      LOGFONT LogFont;

      /* Load the shared font */
      ZeroMemory(&LogFont,
                 sizeof(LOGFONT));

      SystemParametersInfo(SPI_GETICONTITLELOGFONT, 
                           0, 
                           &LogFont, 
                           0);

      _hFont = CreateFontIndirect(&LogFont);
      
      /* Build the header control */
      if ( !_InitializeHeader() )
      {
         /* Failure */
         return ( FALSE );
      }

      /* Build the tree control */
      if ( !_InitializeTreeView() )
      {
         /* Failure */
         return ( FALSE );
      }

      return ( TRUE );
   }

   BOOL _InitializeHeader( )
   {
      HDITEM Col;
      
      if ( !_ctlHeader.Create(m_hWnd, 
                              0, 
                              NULL, 
                              WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_VISIBLE|HDS_HORZ, 
                              0, 
                              0U, 
                              NULL) )
      {
         /* Failure */
         return ( FALSE );
      }

      _ctlHeader.SetUnicodeFormat(TRUE);
      _ctlHeader.SetFont(NULL != _hFont ? _hFont : GetFont(), 
                         FALSE);

      /* Add two text columns. The text of each will be assigned via the owner, sizing will be done 
       * when everything is setup */
      ZeroMemory(&Col,
                 sizeof(HDITEM));

      Col.mask = HDI_FORMAT;
      Col.fmt  = HDF_LEFT|HDF_STRING;

      _ctlHeader.InsertItem(0,
                            &Col);

      _ctlHeader.InsertItem(1,
                            &Col);

      /* Success */
      return ( TRUE );
   }

   BOOL _InitializeTreeView( )
   {
      if ( !_ctlTree.Create(m_hWnd,
                            NULL,
                            NULL,
                            WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS|WS_GROUP|WS_TABSTOP|WS_VISIBLE|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_DISABLEDRAGDROP|TVS_SHOWSELALWAYS,
                            0,
                            0U,
                            NULL) )
      {
         /* Failure */
         return ( FALSE );
      }

      _ctlTree.SetUnicodeFormat(TRUE);
      _ctlTree.SetFont(NULL != _hFont ? _hFont : GetFont(),
                       FALSE);

      /* Success */
      return ( TRUE );
   }

   void _UpdateLayout( )
   {
      RECT      rcClient;
      HDLAYOUT  hdLayout;
      WINDOWPOS wPos;

      GetClientRect(&rcClient);

      ZeroMemory(&wPos,
                 sizeof(WINDOWPOS));
      hdLayout.prc   = &rcClient;
      hdLayout.pwpos = &wPos;
      _ctlHeader.Layout(&hdLayout);

      _ctlHeader.SetWindowPos(wPos.hwndInsertAfter,
                              wPos.x,
                              wPos.y,
                              wPos.cx,
                              wPos.cy,
                              wPos.flags);

      _ctlTree.SetWindowPos(_ctlHeader,
                            rcClient.left,
                            rcClient.top,
                            rcClient.right - rcClient.left,
                            rcClient.bottom - rcClient.top,
                            wPos.flags);
   }
};


class CPropertyCtrl : public CPropertyCtrlImpl<CPropertyCtrl>
{
public:
	DECLARE_WND_SUPERCLASS(_T("PropertyCtrl"), GetWndClassName())
};


class CTestDlg : public ATL::CDialogImpl<CTestDlg, CWindow>
{
public:
   
   enum
   {
      IDD = IDD_TESTDLG
   };

   BEGIN_MSG_MAP(CTestDlg)
      MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
      MESSAGE_HANDLER(WM_SYSCOMMAND, OnSysCommand)
   END_MSG_MAP()

   CPropertyCtrl _ctlProp;

   LRESULT OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ ) throw()
   {
      RECT rcClient;
      GetClientRect(&rcClient);
      InflateRect(&rcClient, -7, -7);

      _ctlProp.Create(m_hWnd, rcClient);
      
      return TRUE;
   }

   LRESULT OnSysCommand( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled ) throw()
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
};
