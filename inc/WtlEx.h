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

/* WtlEx.h
 *    Miscellaneous extensions for WTL
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __WTLUTIL_H__
#define __WTLUTIL_H__

#include <atlgdi.h>

namespace WTLEx
{

/**********************************************************************

    Drawing Stuff

 **********************************************************************/

class CMemDC
{
public:
   HBITMAP  _hbmOffscreen;
   HBITMAP  _hbmOriginal;
   HDC      _hdcOffscreen;
   HDC      _hdcOriginal;
   RECT     _rcDraw;

   CMemDC( ) throw() : _hbmOffscreen(NULL), _hbmOriginal(NULL), _hdcOriginal(NULL)
   {
      _rcDraw.left = _rcDraw.top = _rcDraw.right = _rcDraw.bottom = 0;
   }

   BOOL Create( HDC hDC, LPRECT prc )
   {
      ATLASSERT(NULL != hDC);
      
      if ( prc )
      {
         _rcDraw = *prc;
      }
      else
      {
         if ( !::GetClipBox(hDC, &_rcDraw) )
         {
            /* Failure */
            return ( FALSE );
         }
      }

      _hdcOffscreen = ::CreateCompatibleDC(hDC);
      if ( _hdcOffscreen && ::LPtoDP(hDC, 
                                     reinterpret_cast<LPPOINT>(&_rcDraw), 
                                     2) )
      {
         _hbmOffscreen = ::CreateCompatibleBitmap(hDC, 
                                                  _rcDraw.right - _rcDraw.left, 
                                                  _rcDraw.bottom - _rcDraw.top);
         if ( _hbmOffscreen )
         {
            _hbmOriginal = reinterpret_cast<HBITMAP>(::SelectObject(_hdcOffscreen, 
                                                                    _hbmOffscreen));
            
            if ( _hbmOriginal && ::DPtoLP(hDC, 
                                          reinterpret_cast<LPPOINT>(&_rcDraw), 
                                          2) )
            {
               if ( ::SetWindowOrgEx(_hdcOffscreen, 
                                     _rcDraw.left, 
                                     _rcDraw.top, 
                                     NULL) )

               {
                  _hdcOriginal = hDC;
                  /* Success */
                  return ( TRUE );
               }
            }
         }
      }

      /* Failure */
      return ( FALSE );
   }

   void Destroy( ) throw()
   {   
      if ( _hdcOriginal )
      {
         _ASSERTE(NULL != _hdcOffscreen);
         _ASSERTE(NULL != _hbmOriginal);

         ::BitBlt(_hdcOriginal, _rcDraw.left, _rcDraw.top, _rcDraw.right - _rcDraw.left, _rcDraw.bottom - _rcDraw.top, _hdcOffscreen, _rcDraw.left, _rcDraw.top, SRCCOPY);
         ::SelectObject(_hdcOriginal, _hbmOriginal);         
         
         _hdcOriginal = NULL;
      }

      if ( _hbmOffscreen )
      {
         ::DeleteObject(_hbmOffscreen);
         _hbmOffscreen = NULL;
      }

      if ( _hbmOriginal )
      {
         ::DeleteObject(_hbmOriginal);
         _hbmOriginal = NULL;
      }

      if ( _hdcOffscreen )
      {
         ::DeleteDC(_hdcOffscreen);
         _hdcOffscreen = NULL;
      }
   }

   operator HDC( ) const throw()
   {
      return ( _hdcOffscreen );
   }
};

class CMemDCAuto : public CMemDC
{
public:
   CMemDCAuto( HDC hDC, LPRECT prc = NULL )
   {
      Create(hDC, prc);
   }

   ~CMemDCAuto( )
   {
      Destroy();
   }
};

/**
 *  DrawGradientLine
 *      Draws a horizontal gradient line.
 *
 *  Remarks
 *      This isn't meant to be efficient and should be used as a cached
 *      bitmap
 */
inline void __stdcall DrawGradientLine( HDC hDC, int x, int y, int cx, int cy, COLORREF clrBegin, COLORREF clrEnd )
{
   BYTE rb = GetRValue(clrBegin);
   BYTE re = GetRValue(clrEnd);

   BYTE gb = GetGValue(clrBegin);
   BYTE ge = GetGValue(clrEnd);

   BYTE bb = GetBValue(clrBegin);
   BYTE be = GetBValue(clrEnd);

   for ( int ix = 0; ix < cx; ix++ )
   {
      for ( int iy = 0; iy < cy; iy++ )
      {
         ::SetPixel(hDC, x + ix, y + iy, RGB((rb * (cx - ix) + re * ix) / cx, (gb * (cx - ix) + ge * ix) / cx, (bb * (cx - ix) + be * ix) / cx));
      }
   }
}

inline HBITMAP CreateGradientBitmap( HWND hWnd, int cx, int cy, COLORREF clrBegin, COLORREF clrEnd )
{
   HDC     hDC     = NULL;
   HDC     hMemDC  = NULL;
   HBITMAP hbmOld  = NULL;
   HBITMAP hbmGrad = NULL;

   __try
   {
      hDC = ::GetDC(hWnd);
      if ( !hDC )
      {
         /* Failure */
         __leave;
      }

      hMemDC = ::CreateCompatibleDC(hDC);
      if ( !hMemDC )
      {
         /* Failure */
         __leave;
      }
      
      hbmGrad = ::CreateCompatibleBitmap(hDC, cx, cy);      
      if ( !hbmGrad )
      {
         /* Failure */
         __leave;
      }

      hbmOld = (HBITMAP)::SelectObject(hMemDC, hbmGrad);      
      DrawGradientLine(hMemDC, 0, 0, cx, cy, clrBegin, clrEnd);
  }
   __finally
   {
      if ( hbmOld )
      {
         ::SelectObject(hMemDC, hbmOld);
      }

      if ( hMemDC )
      {
         ::DeleteDC(hMemDC);
      }

      if ( hDC )
      {
         ::ReleaseDC(hWnd, hDC);
      }
   }

   return ( hbmGrad );
}

/**
 * CMultipleFileDialogT
 *    WTL::CFileDialog for multiple file selection 
 */
template < class T >
class CMultipleFileDialogT : public WTL::CFileDialogImpl<T>
{
public:
   typedef CMultipleFileDialogT<T> _MultipleFileDialogT;
   typedef WTL::CFileDialogImpl<T> _FileDialogImpl;

   LPCTSTR pszFolder;
   LPCTSTR pszFiles;   

   LPTSTR  _pszData;
   int     _cchData;
   
   CMultipleFileDialogT( BOOL bOpenFileDialog, LPCTSTR lpszDefExt = NULL, LPCTSTR lpszFileName = NULL, DWORD dwFlags = OFN_ALLOWMULTISELECT|OFN_OVERWRITEPROMPT, LPCTSTR lpszFilter = NULL, HWND hWndParent = NULL ) throw() : _FileDialogImpl(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent), _pszData(m_szFileName), _cchData(_countof(m_szFileName)), pszFiles(NULL), pszFolder(NULL)
   {
      ZeroMemory(m_szFileName, 
                 sizeof(m_szFileName));
   }

   INT_PTR DoModal( HWND hWndParent = ::GetActiveWindow() )
   {
      INT_PTR iRet = _FileDialogImpl::DoModal(hWndParent);

      if ( IDOK != iRet )
      {
         FreeFileData();
      }
      else
      {
         pszFolder = _pszData;
         pszFiles  = _pszData + m_ofn.nFileOffset;
         _pszData[m_ofn.nFileOffset > 0 ? m_ofn.nFileOffset - 1 : 0] = _T('\0');
      }

      return ( iRet );
   }

   void FreeFileData( )
   {
      if ( _pszData != m_szFileName )
      {
         free(_pszData);
      }

      _pszData = NULL;
      _cchData = 0;
      
      pszFolder = NULL;
      pszFiles  = NULL;
   }
   
   void OnSelChange( LPOFNOTIFY /*lpon*/ )
	{
      TCHAR chTmp     = _T('\0');
      int   cchFolder = GetFolderPath(&chTmp, 1) + 1;
      int   cchFiles  = GetSpec(&chTmp, 1) + 2;
      int   cchData   = cchFolder + cchFiles;

      if ( cchData > _cchData )
      {
         FreeFileData();
         
         _pszData = reinterpret_cast<TCHAR*>(malloc(sizeof(TCHAR) * cchData));
         if ( _pszData )
         {
            ZeroMemory(_pszData, 
                       cchData * sizeof(TCHAR));

            _cchData  = cchData;
         }
      }

      if ( !_pszData )
      {
         _pszData = m_szFileName;
         _cchData = _countof(m_szFileName);
      }

      m_ofn.lpstrFile = _pszData;
      m_ofn.nMaxFile  = _cchData;
	}
};

} /* namespace WTLEx */

#endif /* __WTLUTIL_H__ */