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

/* MiscShell.h
 *    Miscellaneous shell helpers
 *
 * Copyright (C) 2005-2009 Jeremy Boschen
 */

#pragma once

#include <shlobj.h>

/**
 * _SHGetUIObjectOfName
 *    Wrapper for IShellFolder::GetUIObjectOf to return an interface on
 *    a shell item.
 *
 *  Parameters
 *    hWnd
 *       [in] Owner window of any UI notifications
 *    pszDisplayName
 *       [in] Display name to retrieve an interface for
 *    riid
 *       [in] Interface to retrieve
 *    ppv
 *       [out] Pointer to interface retrieved on return.
 *
 *  Return Value
 *    Returns S_OK if successful, otherwise an OLE error code
 */
inline HRESULT _SHGetUIObjectOfName( HWND hWnd, LPCWSTR pszDisplayName, REFIID riid, void** ppv ) throw()
{  
   HRESULT       hr;
   IShellFolder* pDesktop;
   IShellFolder* pParent;
   LPITEMIDLIST  pidl;
   LPCITEMIDLIST pidlItem;

   hr       = E_FAIL;
   pDesktop = NULL;
   pParent  = NULL;
   pidl     = NULL;
   pidlItem = NULL;

   __try
   {
      hr = SHGetDesktopFolder(&pDesktop);
      if ( FAILED(hr) )
      {
         /* Failure */
         __leave;
      }

      hr = pDesktop->ParseDisplayName(hWnd, 
                                      NULL, 
                                      const_cast<LPOLESTR>(pszDisplayName), 
                                      NULL, 
                                      &pidl, 
                                      NULL);

      if ( FAILED(hr) )
      {
         /* Failure */
         __leave;
      }

      hr = SHBindToParent(pidl, 
                          IID_IShellFolder, 
                          reinterpret_cast<void**>(&pParent), 
                          &pidlItem);

      if ( FAILED(hr) )
      {
         /* Failure */
         __leave;
      }

      hr = pParent->GetUIObjectOf(hWnd, 
                                  1, 
                                  &pidlItem, 
                                  riid, 
                                  NULL, 
                                  ppv);
   }
   __finally
   {
      if ( pidl )
      {
         CoTaskMemFree(pidl);
      }

      if ( pParent ) 
      {
         pParent->Release();
      }

      if ( pDesktop )
      {
         pDesktop->Release();
      }
   }

   return ( hr );
}


/* For dynamically linking to SHGetIconOverlayIndex */
typedef int (__stdcall* LPSHGETICONOVERLAYINDEX)(LPCTSTR, int);

/**
 *  CShellImageList
 *      Shell imagelist wrapper for drawing file icons.
 */
class CShellImageList : public WTL::CImageList
{
public:
   CShellImageList( ) throw() : WTL::CImageList(NULL), _bIsCC6(false)
   {
      DWORD dwMajor;
      DWORD dwMinor;

      csIcon.cx = 0;
      csIcon.cy = 0;

      dwMajor = 0;
      dwMinor = 0;

      if ( SUCCEEDED(AtlGetCommCtrlVersion(&dwMajor, &dwMinor)) && dwMajor >= 6 )
      {
         _bIsCC6 = true;
      }
   }

   void Load( bool bLarge ) throw()
   {
      SHFILEINFO              FileInfo;
      LPSHGETICONOVERLAYINDEX pSHGetIconOverlayIndex;

      ZeroMemory(&FileInfo,
                 sizeof(SHFILEINFO));

      csIcon.cx = 0;
      csIcon.cy = 0;

      m_hImageList = reinterpret_cast<HIMAGELIST>(SHGetFileInfo(L"C:\\", 
                                                                0, 
                                                                &FileInfo, 
                                                                sizeof(SHFILEINFO), 
                                                                SHGFI_SYSICONINDEX|(bLarge ? 0 : SHGFI_SMALLICON)));

      if ( m_hImageList )
      {
         GetIconSize(csIcon);

         /* Force the shell to copy icon-overlays */
         pSHGetIconOverlayIndex = reinterpret_cast<LPSHGETICONOVERLAYINDEX>(GetProcAddress(GetModuleHandle(L"SHELL32.DLL"), "SHGetIconOverlayIndex"));

         if ( pSHGetIconOverlayIndex )
         {
            pSHGetIconOverlayIndex(NULL, 
                                   IDO_SHGIOI_SHARE);
         }
      }
   }

   DWORD GetFileInfo( LPCTSTR pszPath, int& iIcon ) throw()
   {
      SHFILEINFO FileInfo;

      ATLASSERT(!IsNull());
      ATLASSERT(NULL != pszPath);

      ZeroMemory(&FileInfo,
                 sizeof(SHFILEINFO));

      if ( reinterpret_cast<HIMAGELIST>(SHGetFileInfo(pszPath, 
                                                              0, 
                                                              &FileInfo, 
                                                              sizeof(SHFILEINFO), 
                                                              SHGFI_ATTRIBUTES|SHGFI_SMALLICON|SHGFI_SYSICONINDEX)) )
      {
         iIcon = FileInfo.iIcon;
         /* Success */
         return ( FileInfo.dwAttributes );
      }

      iIcon = -1;
      /* Failure */
      return ( 0 );
   }
    
   bool DrawFileInfoIcon( int iIcon, DWORD dwAttributes, HDC hDC, int x, int y ) throw()
   {
      ATLASSERT(-1 != iIcon);

      IMAGELISTDRAWPARAMS DrawParams;
      
      ZeroMemory(&DrawParams,
                 sizeof(IMAGELISTDRAWPARAMS));

      DrawParams.cbSize = (_bIsCC6 ? sizeof(IMAGELISTDRAWPARAMS) : IMAGELISTDRAWPARAMS_V3_SIZE);
      DrawParams.himl   = m_hImageList;
      DrawParams.i      = iIcon;
      DrawParams.hdcDst = hDC;
      DrawParams.x      = x; 
      DrawParams.y      = y; 
      DrawParams.rgbBk  = CLR_NONE;
      DrawParams.rgbFg  = CLR_DEFAULT;
      DrawParams.fStyle = ILD_NORMAL;
      DrawParams.fState = ILS_NORMAL;

      if ( SFGAO_SHARE & dwAttributes )
      {
         DrawParams.fStyle |= INDEXTOOVERLAYMASK(1);
      }

      if ( SFGAO_LINK & dwAttributes )
      {
         DrawParams.fStyle |= INDEXTOOVERLAYMASK(2);
      }

      if ( SFGAO_ISSLOW & dwAttributes )
      {
         DrawParams.fStyle |= INDEXTOOVERLAYMASK(3);
      }

      /* Hidden items are 50% transparent */
      if ( (SFGAO_GHOSTED & dwAttributes) || (SFGAO_HIDDEN & dwAttributes) )
      {
         DrawParams.rgbFg   = CLR_NONE;
         DrawParams.fStyle |= ILD_BLEND50;
      }

      return ( DrawIndirect(&DrawParams) ? true : false );
   }                    

   SIZE  csIcon;

private:
    bool _bIsCC6;
};

inline BOOL _SHFileProperties( HWND hWnd, LPCWSTR pwszPath ) throw()
{
   SHELLEXECUTEINFO ExecInfo;

   ZeroMemory(&ExecInfo,
              sizeof(SHELLEXECUTEINFO));

   ExecInfo.cbSize   = sizeof(SHELLEXECUTEINFO);
   ExecInfo.fMask    = SEE_MASK_FLAG_NO_UI|SEE_MASK_HMONITOR|SEE_MASK_INVOKEIDLIST;
   ExecInfo.hwnd     = hWnd;
   ExecInfo.lpVerb   = L"properties";
   ExecInfo.lpFile   = pwszPath;
   ExecInfo.nShow    = SW_SHOWNORMAL;
   ExecInfo.hMonitor = MonitorFromWindow(hWnd,
                                         MONITOR_DEFAULTTONEAREST);
   return ( ShellExecuteEx(&ExecInfo) );
}