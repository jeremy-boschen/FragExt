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

/* Common.cpp
 *    Implementations of Common.h items
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 06/13/2005 - Created
 */

#include "Stdafx.h"
#include "Common.h"

#include <atldlgs.h>
#include <NumberFmt.h>

/**********************************************************************

   CFragReport

 **********************************************************************/

CFragReport::CFragReport( ) throw()
{
   _hFile = INVALID_HANDLE_VALUE;
}

HRESULT CFragReport::OpenReport( int eMode, HWND hWnd ) throw()
{
   DWORD             dwErr;
   WCHAR             chExt[MAX_PATH];
   WCHAR             chFilter[MAX_PATH];
   WCHAR             chFile[MAX_PATH];
   WCHAR             chDirectory[MAX_PATH];  
   WTL::CFileDialog  cSaveDlg(FALSE, chExt, NULL, OFN_ENABLESIZING|OFN_EXPLORER|OFN_LONGNAMES|OFN_NOREADONLYRETURN|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, chFilter, hWnd);
   DWORD             cbWritten;

   /* Initialize locals */
   dwErr = NO_ERROR;

   ZeroMemory(chExt,
              sizeof(chExt));
   
   ZeroMemory(chFile,
              sizeof(chFile));
   
   ZeroMemory(chFilter,
              sizeof(chFilter));
   
   ZeroMemory(chDirectory,
              sizeof(chDirectory));
   
   /* Load resource info */
   LoadString(_AtlModule.GetMUIInstance(),
              IDS_REPORT_DEFAULTSAVEEXT,
              chExt,
              _countof(chExt));
   ATLASSERT(L'\0' != *chExt);

   LoadString(_AtlModule.GetMUIInstance(),
              IDS_REPORT_SAVEFILTER,
              chFilter,
              _countof(chFilter));
   ATLASSERT(L'\0' != *chFilter);
      
   if ( WriteModeFile == eMode )
   {
      cSaveDlg.m_ofn.nFilterIndex = 2;

      if ( IDOK != cSaveDlg.DoModal(hWnd) )
      {
         dwErr = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }

      StringCchCopy(chFile,
                    _countof(chFile),
                    cSaveDlg.m_szFileName);
   }
   else
   {
      ATLASSERT(WriteModeClipboard == eMode);

      if ( !GetTempPath(_countof(chDirectory),
                        chDirectory) )
      {
         dwErr = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }

      if ( !GetTempFileName(chDirectory, 
                            _T("Fx"), 
                            0, 
                            chFile) )
      {
         dwErr = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }
   }

   ATLASSERT(INVALID_HANDLE_VALUE == _hFile);   
   _hFile = CreateFile(chFile,
                       GENERIC_READ|GENERIC_WRITE,
                       FILE_SHARE_READ|FILE_SHARE_WRITE, 
                       NULL, 
                       CREATE_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN|(WriteModeFile != eMode ? FILE_FLAG_DELETE_ON_CLOSE : 0), 
                       NULL);

   if ( INVALID_HANDLE_VALUE == _hFile )
   {
      dwErr = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   /* When writing to a user file, the file needs to be marked with a CP1200 header. Clipboard
    * mode doesn't require this because the code knows the text is already unicode */
   if ( WriteModeFile == eMode )
   {
      /* Write the file signature without encoding */   
      if ( !WriteFile(_hFile,
                      FILESIGNATURE_CP1200,
                      FILESIGNATURE_CP1200_SIZE,
                      &cbWritten,
                      NULL) )
      {
         dwErr = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }
   }

__CLEANUP:
   if ( NO_ERROR != dwErr )
   {
      CloseReport();
   }

   /* Success / Failure */
   return ( __HRESULT_FROM_WIN32(dwErr) );
}

void CFragReport::CloseReport( ) throw()
{
   if ( INVALID_HANDLE_VALUE != _hFile )
   {
      CloseHandle(_hFile);
      _hFile = INVALID_HANDLE_VALUE;
   }
}

HRESULT CFragReport::CopyToClipboard( HWND hWnd ) throw()
{
   HRESULT        hr;
   DWORD          dwErr;
   ULARGE_INTEGER cbFile;
   HGLOBAL        hGlobal;
   BYTE*          pbBuff;
   LARGE_INTEGER  iPointer;

   ATLASSERT(INVALID_HANDLE_VALUE != _hFile);

   /* Initialize locals */
   hr      = E_FAIL;
   hGlobal = NULL;
   pbBuff  = NULL;

   /* Ensure everything is written to the file as we're about to
    * read it all back into memory */
   FlushFileBuffers(_hFile);

   /* Check if there is data to be read */
   cbFile.LowPart = GetFileSize(_hFile,
                                &cbFile.HighPart);

   /* The file size cannot be larger than ULONG_MAX */
   if ( (INVALID_FILE_SIZE == cbFile.LowPart) && (NO_ERROR == GetLastError()) )
   {
      /* Failure */
      return ( E_FAIL );
   }

   /* Save the current file pointer */
   iPointer.LowPart = SetFilePointer(_hFile,
                                     0,
                                     &iPointer.HighPart,
                                     FILE_CURRENT);

   hGlobal = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,
                         cbFile.LowPart + sizeof(L'\0'));
   if ( !hGlobal )
   {
      /* Failure */
      return ( E_OUTOFMEMORY );
   }

   pbBuff = reinterpret_cast<BYTE*>(GlobalLock(hGlobal));
   if ( !pbBuff )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   if ( INVALID_SET_FILE_POINTER == SetFilePointer(_hFile,
                                                   0,
                                                   NULL,
                                                   FILE_BEGIN) )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   if ( !ReadFile(_hFile,
                  pbBuff,
                  cbFile.LowPart,
                  &cbFile.HighPart,
                  NULL) )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   if ( cbFile.LowPart != cbFile.HighPart )
   {
      hr = E_UNEXPECTED;
      /* Failure */
      goto __CLEANUP;
   }

   /* Append a null-term to the text */
   pbBuff[cbFile.LowPart] = L'\0';

   GlobalUnlock(hGlobal);
   pbBuff = NULL;

   if ( !OpenClipboard(hWnd) )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   EmptyClipboard();

   if ( SetClipboardData(CF_UNICODETEXT,
                                 hGlobal) )
   {
      /* The clipboard now owns the data */
      hGlobal = NULL;
      /* Success */
      hr      = S_OK;
   }

   CloseClipboard();

__CLEANUP:
   if ( pbBuff )
   {
      GlobalUnlock(hGlobal);
   }

   if ( hGlobal )
   {
      GlobalFree(hGlobal);
   }

   /* Success / Failure */
   return ( hr );
}

HRESULT CFragReport::AppendFile( LPCWSTR FileName, HANDLE hFragCtx ) throw()
{
   HRESULT hr;

   /* Write out the report header... */
   hr = _WriteReportHeadInfo(FileName,
                             hFragCtx);

   if ( SUCCEEDED(hr) )
   {
      /* Write out the fragment info */
      hr = _WriteReportFragInfo(hFragCtx);
   
      if ( SUCCEEDED(hr) )
      {
         /* Add a few lines to seperate this report from any subsequent ones */
         hr = _WriteToReportFile(L"\r\n\r\n", 
                                 4, 
                                 NULL); 
      }
   }

   /* Success / Failure */
   return ( hr );
}

HRESULT CFragReport::_WriteReportHeadInfo( LPCWSTR FileName, HANDLE hFragCtx ) throw()
{
   HRESULT     hr;
   bool        bRet;
   DWORD       dwErr;
   /* General info */
   FRAGCTX_INFORMATION CtxInfo;
   WCHAR       chFileSize[384];
   WCHAR       chFileSizeOnDisk[384];
   WCHAR       chFragments[384];
   CNUMBERFMT  NumberFmt;

   /* Initialize locals */
   ZeroMemory(&CtxInfo,
              sizeof(FRAGCTX_INFORMATION));
   
   ZeroMemory(chFileSize,
              sizeof(chFileSize));

   ZeroMemory(chFileSizeOnDisk,
              sizeof(chFileSizeOnDisk));

   ZeroMemory(chFragments,
              sizeof(chFragments));

   /* Load the general data */
   dwErr = FpGetContextInformation(hFragCtx,
                                   &CtxInfo);

   if ( NO_ERROR != dwErr )
   {
      hr = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      return ( hr );
   }

   hr = _WriteToReportFilef(MAKEINTRESOURCE(IDS_REPORT_FILEPATH),
                            FileName);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   bRet = (NULL != StrFormatByteSizeW(CtxInfo.FileSize,
                                      chFileSize,
                                      _countof(chFileSize)));

   if ( bRet )
   {
      bRet = (NULL != StrFormatByteSizeW(CtxInfo.FileSizeOnDisk,
                                         chFileSizeOnDisk,
                                         _countof(chFileSizeOnDisk)));
   }

   if ( !bRet )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
   }

   if ( SUCCEEDED(hr) )
   {
      hr = _WriteToReportFilef(MAKEINTRESOURCE(IDS_REPORT_FILESIZE),
                               chFileSize,
                               chFileSizeOnDisk);
   }

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }   

   /* Fragment count... */
   NumberFmt.Initialize(LOCALE_USER_DEFAULT);
   NumberFmt.NumDigits = 0;

   if ( GetLocaleFormattedNumber(L"%u",
                                 chFragments,
                                 _countof(chFragments),
                                 NumberFmt,
                                 CtxInfo.FragmentCount) <= 0 )
   {
      /* Failure */
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
   }
   else
   {
      hr = _WriteToReportFilef(MAKEINTRESOURCE(IDS_REPORT_FILEFRAGMENTS),
                               chFragments);
   }

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   /* Header seperator... */
   hr = _WriteToReportFilef(MAKEINTRESOURCE(IDS_REPORT_FRAGMENTSHEADER),
                            L"");

   /* Success / Failure */
   return ( hr );
}

HRESULT CFragReport::_WriteReportFragInfo( HANDLE hFragCtx ) throw()
{
   HRESULT        hr;
   DWORD          dwErr;

   CNUMBERFMT     NumberFmt;
   UINT           iNumDigits;

   FRAGCTX_INFORMATION    CtxInfo;

   ULONG          idx;
   ULONGLONG      cbFragSize;
   double         fPercentOf;
   FRAGMENT_INFORMATION   FragInfo;

   WCHAR          chVCN[384];
   WCHAR          chLCN[384];
   WCHAR          chExtents[384];
   WCHAR          chClusters[384];
   WCHAR          chFragSize[384];
   WCHAR          chPercentOf[384];
      
   /* Initialize locals */
   ZeroMemory(&CtxInfo,
              sizeof(CtxInfo));

   /* Get general data */
   dwErr = FpGetContextInformation(hFragCtx,
                                   &CtxInfo);

   if ( NO_ERROR != dwErr )
   {
      hr = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      return ( hr );
   }

   NumberFmt.Initialize(LOCALE_USER_DEFAULT);
   iNumDigits = NumberFmt.NumDigits;
   
   for ( idx = 1; idx <= CtxInfo.FragmentCount; idx++ )
   {
      ZeroMemory(chVCN,
                 sizeof(chVCN));

      ZeroMemory(chLCN,
                 sizeof(chLCN));

      ZeroMemory(chExtents,
                 sizeof(chExtents));

      ZeroMemory(chClusters,
                 sizeof(chClusters));

      ZeroMemory(chFragSize,
                 sizeof(chFragSize));

      ZeroMemory(chPercentOf,
                 sizeof(chPercentOf));

      dwErr = FpGetContextFragmentInformation(hFragCtx,  
                                              idx,
                                              &FragInfo,
                                              1,
                                              NULL);

      if ( NO_ERROR != dwErr )
      {
         hr = __HRESULT_FROM_WIN32(dwErr);
         /* Failure */
         return ( hr );
      }

      /* Write out the sequence */
      NumberFmt.NumDigits = 0;

      GetLocaleFormattedNumber(L"%u",
                               chVCN,
                               _countof(chVCN),
                               NumberFmt,
                               FragInfo.Sequence);

      /* LCN */
      GetLocaleFormattedNumber(IsVirtualClusterNumber(FragInfo.LogicalCluster) ? L"%I64d" : L"%I64u",
                               chLCN,
                               _countof(chLCN),
                               NumberFmt,
                               FragInfo.LogicalCluster);


      /* Extents */
      GetLocaleFormattedNumber(L"%u", 
                               chExtents, 
                               _countof(chExtents), 
                               NumberFmt, 
                               FragInfo.ExtentCount);

      /* Clusters */
      GetLocaleFormattedNumber(L"%I64u",
                               chClusters,
                               _countof(chClusters),
                               NumberFmt,
                               FragInfo.ClusterCount);

      /* Size */
      cbFragSize = static_cast<LONGLONG>(CtxInfo.ClusterSize) * static_cast<LONGLONG>(FragInfo.ClusterCount);
      StrFormatByteSizeW(cbFragSize,
                         chFragSize,
                         _countof(chFragSize));

      /* Percent of File */
      fPercentOf = 100.0f * (static_cast<double>(cbFragSize) / static_cast<double>(CtxInfo.FileSizeOnDisk));
      NumberFmt.NumDigits = iNumDigits;
      GetLocaleFormattedNumber(L"%.15f",
                               chPercentOf,
                               _countof(chPercentOf),
                               NumberFmt,
                               fPercentOf);

      hr = _WriteToReportFilef(L"%-24s%-24s%-24s%-24s%-24s%s\r\n",
                               chVCN,
                               chLCN,
                               chExtents,
                               chClusters,
                               chFragSize,
                               chPercentOf);

      if ( FAILED(hr) )
      {
         /* Failure */
         return ( hr );
      }
   }

   /* Success */
   return ( S_OK );
}

HRESULT CFragReport::_WriteToReportFile( LPCWSTR pszBuf, size_t cchWrite, LPDWORD pdwWritten ) throw()
{
   DWORD dwErr;
   DWORD cbToWrite;
   DWORD cbWritten;   
   
   cbToWrite     = static_cast<DWORD>(cchWrite) * sizeof(WCHAR);
   pdwWritten    = (pdwWritten ? pdwWritten : &cbWritten);
   (*pdwWritten) = 0;

   if ( !WriteFile(_hFile, 
                   pszBuf, 
                   cbToWrite, 
                   pdwWritten, 
                   NULL) )
   {
      dwErr = GetLastError();
      /* Failure */
      return ( __HRESULT_FROM_WIN32(dwErr) );
   }

   /* Success */
   return ( S_OK );
}

HRESULT __cdecl CFragReport::_WriteToReportFilef( LPCWSTR pwszFormat, ... ) throw()
{
   HRESULT hr;
   va_list args;
   int     cchFmt;
   size_t  cchLeft;
   WCHAR   chFmt[512];
   WCHAR   chBuf[1024];

   /* Initialize locals */
   ZeroMemory(chFmt,
              sizeof(chFmt));

   ZeroMemory(chBuf,
              sizeof(chBuf));

   /* Load the format string */
   if ( IS_INTRESOURCE(pwszFormat) )
   {
      cchFmt = LoadString(_AtlModule.GetMUIInstance(),
                          LOWORD(reinterpret_cast<ULONG_PTR>(pwszFormat)),
                          chFmt,
                          _countof(chFmt));

      if ( cchFmt <= 0 )
      {
         /* Failure */
         return ( __HRESULT_FROM_WIN32(ERROR_RESOURCE_NOT_FOUND) );
      }

      pwszFormat = chFmt;
   }

   /* Format the output text */
   va_start(args,
            pwszFormat);

   hr = StringCchVPrintfExW(chBuf,
                            _countof(chBuf),
                            NULL,
                            &cchLeft,
                            0,
                            pwszFormat,
                            args);

   va_end(args);

   if ( SUCCEEDED(hr) )
   {
      /* Dump to the report file */
      hr = _WriteToReportFile(chBuf,
                              _countof(chBuf) - cchLeft,
                              NULL);
   }

   /* Success / Failure */
   return ( hr );
}
