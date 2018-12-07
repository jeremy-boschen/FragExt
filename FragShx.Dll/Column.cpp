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
 
/* Column.cpp
 *    CColumnProvider implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "Column.h"

#include <RegUtil.h>
#include <NumberFmt.h>

#include <StreamUtil.h>

/**********************************************************************

   CColumnProvider : ATL

 **********************************************************************/
HRESULT CColumnProvider::FinalConstruct( )
{
   DWORD dwValue;

   dwValue = 1;
   if ( NO_ERROR == GetRegistryValueDword(RVF_HKCU,
                                          FRAGSHX_SUBKEY,   
                                          FRAGSHX_COLUMNNOTEXTCONVERT,
                                          &dwValue) )
   {
      _bConvertToText = (0 == dwValue ? FALSE : TRUE);
   }

   return ( S_OK );
}

void CColumnProvider::FinalRelease( )
{
}

/**********************************************************************

    CColumnProvider : IColumnProviderImpl

 **********************************************************************/
int CColumnProvider::_GetColumnString( UINT uID, LPTSTR lpBuffer, int cchBufferMax ) throw()
{
   return ( LoadString(_AtlModule.GetMUIInstance(),
                       uID,
                       lpBuffer,
                       cchBufferMax) );
}

/**********************************************************************

    CColumnProvider : CColumnProvider

 **********************************************************************/

CColumnProvider::CColumnProvider( ) throw()
{
   _bConvertToText = TRUE;
}

HRESULT CColumnProvider::_GetColumnItemData( LPCTSTR pszFolder, DWORD pid, LPCSHCOLUMNDATA pColData, VARIANT* pVarData, const _COLUMNINFO* pColInfo ) throw()
{
   HRESULT hr;
   VARIANT agData;

   UNREFERENCED_PARAMETER(pszFolder);
   UNREFERENCED_PARAMETER(pColData);
   UNREFERENCED_PARAMETER(pColInfo);

   /* Initialize locals */
   VariantInit(&agData);

   /* Load or reset the data cache for this file */
   Lock();
   {
      hr = _LoadFragItemData(pColData->wszFile,
                             pid,
                             agData);
   }
   Unlock();

   if ( SUCCEEDED(hr) )
   {
      hr = _FormatItemData(pid, 
                           agData, 
                           pVarData);
   }

   VariantClear(&agData);

   /* Success / Failure */
   return ( hr );
}

HRESULT CColumnProvider::_GetInfoTipItemData( LPCTSTR pszFolder, DWORD pid, LPCSHCOLUMNDATA pColData, VARIANT* pVarData, const _COLUMNINFO* pColInfo )
{
   HRESULT  hr;
   VARIANT  agData;
   WCHAR    chTip[1024];

   UNREFERENCED_PARAMETER(pszFolder);
   UNREFERENCED_PARAMETER(pColInfo);

   /* Initialize locals */
   VariantInit(&agData);

   if ( PID_TIPNUMBEROF_DATASTREAMS == pid )
   {
      hr = _GetStreamItemData(pszFolder,
                              PID_COLNUMBEROF_DATASTREAMS,
                              pColData,
                              &agData,
                              pColInfo);
   }
   else
   {
      Lock();
      {
         hr = _LoadFragItemData(pColData->wszFile,
                                _GetMatchingColumnId(pid),
                                agData);
      }
      Unlock();
   }

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Convert the data to its string representation */
   hr = VariantChangeType(&agData,
                          &agData,
                          0,
                          VT_BSTR);

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Load the info-tip title */
   ZeroMemory(chTip,
              sizeof(chTip));

   /* Assume this will succeed */
   LoadString(_AtlModule.GetMUIInstance(),
              LOWORD(pColInfo->pszTitle),
              chTip,
              _countof(chTip));
   ATLASSERT(L'\0' != *chTip);

   /* Append the column text to the info tip */
   hr = StringCchCat(chTip,
                     _countof(chTip),
                     agData.bstrVal);

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Build the return BSTR */
   pVarData->bstrVal = SysAllocString(chTip);
   if ( !pVarData->bstrVal )
   {
      hr = E_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   pVarData->vt = VT_BSTR;
   /* Success */
   hr = S_OK;
   
__CLEANUP:
   VariantClear(&agData);

   /* Success / Failure */
   return ( hr );
}

HRESULT CColumnProvider::_GetStreamItemData( LPCTSTR pszFolder, DWORD pid, LPCSHCOLUMNDATA pColData, VARIANT* pVarData, const _COLUMNINFO* pColInfo )
{
   HRESULT hr;
   DWORD   dwErr;
   VARIANT agData;

   UNREFERENCED_PARAMETER(pszFolder);
   UNREFERENCED_PARAMETER(pid);
   UNREFERENCED_PARAMETER(pColInfo);

   dwErr = FpGetFileStreamCount(const_cast<LPWSTR>(pColData->wszFile),
                                0,
                                &agData.ulVal);

   if ( NO_ERROR != dwErr )
   {
      hr = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      return ( hr );
   }

   agData.vt = VT_UI4;
   hr = _FormatItemData(pid,
                        agData,
                        pVarData);

   /* Success / Failure */
   return ( hr );
}

PROPID CColumnProvider::_GetMatchingColumnId( PROPID pid ) throw()
{
   struct COLTIPMAP
   {
      PROPID   ColumnId;
      PROPID   InfoTipId;
   };

   size_t                 idx;
   static const COLTIPMAP rgMap[] =
   {
      {PID_COLNUMBEROF_FRAGMENTS,   PID_TIPNUMBEROF_FRAGMENTS},
      {PID_COLNUMBEROF_EXTENTS,     PID_TIPNUMBEROF_EXTENTS},
      {PID_COLNUMBEROF_CLUSTERS,    PID_TIPNUMBEROF_CLUSTERS},
      {PID_COLSIZEOF_ONDISK,        PID_TIPSIZEOF_ONDISK},
      {PID_COLPERCENTOF_ONDISK,     PID_TIPPERCENTOF_ONDISK}
   };

   for ( idx = 0; idx < _countof(rgMap); idx++ )
   {
      if ( rgMap[idx].InfoTipId == pid )
      {
         return ( rgMap[idx].ColumnId );
      }
   }

   /* Should never get here */
   ATLASSERT(FALSE);
   return ( static_cast<PROPID>(-1) );
}

HRESULT CColumnProvider::_LoadFragItemData( LPCWSTR pszFile, PROPID pid, VARIANT& agData ) throw()
{
   HRESULT     hr;
   DWORD       dwErr;
   HANDLE      hFragCtx;
   FRAGCTX_INFORMATION CtxInfo;

   /* Initialize locals */
   hFragCtx = NULL;

   /*TODO:With FPF_CTX_DETACHED implemented, the column could cache the context without locking the file */
   /* Open the new context */
   dwErr = FpCreateContext(pszFile,
                         FPF_CTX_DETACHED,
                         &hFragCtx);

   if ( NO_ERROR != dwErr )
   {
      hr = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      return ( hr );
   }

   __try
   {
      /* Retrieve the information... */
      dwErr = FpGetContextInformation(hFragCtx,
                            &CtxInfo);

      if ( NO_ERROR != dwErr )
      {
         /* Failure */
         __leave;
      }

      switch ( pid )
      {
         case PID_COLNUMBEROF_FRAGMENTS:
            agData.vt    = VT_UI4;
            agData.ulVal = CtxInfo.FragmentCount;
            break;

         case PID_COLNUMBEROF_EXTENTS:
            agData.vt    = VT_UI4;
            agData.ulVal = CtxInfo.ExtentCount;
            break;

         case PID_COLNUMBEROF_CLUSTERS:
            agData.vt     = VT_UI8;
            agData.ullVal = CtxInfo.ClusterCount;
            break;

         case PID_COLSIZEOF_ONDISK:
            agData.vt     = VT_UI8;
            agData.ullVal = CtxInfo.FileSizeOnDisk;
            break;

         case PID_COLPERCENTOF_ONDISK:
            agData.vt     = VT_R8;
            if ( CtxInfo.FileSize > 0 )
            {
               agData.dblVal = (100.0f * (static_cast<double>(CtxInfo.FileSizeOnDisk) / static_cast<double>(CtxInfo.FileSize)));
               /* Limit the precision of this to 2 decimal places */
               agData.dblVal = static_cast<double>(static_cast<ULONGLONG>(agData.dblVal * 100)) / 100;
            }
            else
            {
               agData.dblVal = 0.0f;
            }
            break;

         default:
            /* Shouldn't get here */
            ATLASSERT(FALSE);
            /* Failure */
            dwErr = ERROR_INVALID_PARAMETER;
            break;
      }
   }
   __finally
   {
      FpCloseContext(hFragCtx);
   }

   /* Success / Failure */
   return ( (NO_ERROR != dwErr) ? __HRESULT_FROM_WIN32(dwErr) : S_OK );
}

HRESULT CColumnProvider::_FormatItemData( PROPID pid, VARIANT& agData, VARIANT* pVarData ) throw()
{
   HRESULT     hr;
   TCHAR       chBuf[512];
   CNUMBERFMT  NumberFmt;
   
   if ( !_bConvertToText )
   {
      /* Just copy the data and return */
      hr = VariantCopy(pVarData,
                       &agData);

      /* Success / Failure */
      return ( hr );
   }

   /* Initialize locals */
   hr = E_FAIL;

   ZeroMemory(chBuf,
              sizeof(chBuf));
   
   NumberFmt.Initialize(LOCALE_USER_DEFAULT);
   
   switch ( pid )
   {
      case PID_COLNUMBEROF_FRAGMENTS:
      case PID_COLNUMBEROF_EXTENTS:
      case PID_COLNUMBEROF_DATASTREAMS:
         ATLASSERT(VT_UI4 == agData.vt);

         NumberFmt.NumDigits = 0;

         if ( GetLocaleFormattedNumber(L"%u", 
                                       chBuf, 
                                       _countof(chBuf), 
                                        NumberFmt, 
                                        agData.ulVal) > 0 )
         {
            hr = S_OK;
         }

         break;

      case PID_COLNUMBEROF_CLUSTERS:
         ATLASSERT(VT_UI8 == agData.vt);
         
         NumberFmt.NumDigits = 0;

         if ( GetLocaleFormattedNumber(L"%I64u",
                                       chBuf,
                                       _countof(chBuf),
                                       NumberFmt,
                                       agData.ullVal) > 0 )
         {
            hr = S_OK;
         }
         
         break;

      case PID_COLSIZEOF_ONDISK:
         ATLASSERT(VT_UI8 == agData.vt);

         if ( StrFormatByteSizeW(agData.ullVal,
                                         chBuf,
                                         _countof(chBuf)) )
         {
            hr = S_OK;
         }

         break;

      case PID_COLPERCENTOF_ONDISK:
         ATLASSERT(VT_R8 == agData.vt);
         
         if ( GetLocaleFormattedNumber(L"%.2f", 
                                       chBuf, 
                                       _countof(chBuf), 
                                       NumberFmt, 
                                       agData.dblVal) > 0 )
         {
            hr = S_OK;
         }

         break;

      default:
         /* Shouldn't get here... */
         ATLASSERT(FALSE);
         break;
   }

   if ( SUCCEEDED(hr) )
   {
      pVarData->vt      = VT_BSTR;
      pVarData->bstrVal = SysAllocString(chBuf);

      if ( !pVarData->bstrVal )
      {
         pVarData->vt = VT_EMPTY;
         hr           = E_FAIL;
      }
   }

   /* Success / Failure */
   return ( hr );
}
