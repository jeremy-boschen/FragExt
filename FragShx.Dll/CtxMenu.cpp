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

/* CtxMenu.cpp
 *    Header file for CShxContextMenu
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 11/14/2004 - Created
 */

#include "Stdafx.h"
#include "Common.h"
#include "CtxMenu.h"
#include "FolderDlg.h"

/**********************************************************************

    CShxContextMenu : ATL

 **********************************************************************/

HRESULT CShxContextMenu::FinalConstruct( ) throw()
{
   /* Success */
   return ( S_OK );
}

void CShxContextMenu::FinalRelease( ) throw()
{
   _ResetData();   
   _ResetSite();
}

/**********************************************************************

    CShxContextMenu : IShellExtInit

 **********************************************************************/

STDMETHODIMP CShxContextMenu::Initialize( LPCITEMIDLIST pidlFolder, IDataObject* pDataObj, HKEY hkProgID ) throw()
{
   HRESULT     hr;
   FORMATETC   FormatEtc;
   STGMEDIUM   StgMedium;
   HDROP       hDrop;
   UINT        idx;
   UINT        cFiles;
   UINT        cchRet;
   PPATHINFO   rgPathInfo;

   UNREFERENCED_PARAMETER(pidlFolder);
   UNREFERENCED_PARAMETER(hkProgID);

#if _DEBUG
   {
      BOOL    dbgbRet;
      WCHAR   dbgchPath[MAX_PATH];
      
      ZeroMemory(dbgchPath,
                 sizeof(dbgchPath));

      dbgbRet = SHGetPathFromIDList(pidlFolder,
                                    dbgchPath);

      DbgPrintf(_T(__FUNCTION__) L"!pidlFolder=%s\n", dbgchPath);
   }
#endif /* _DEBUG */

   /* This method always clears the current data */
   _ResetData();

   if ( !pDataObj )
   {
      /* Success */
      return ( S_OK );
   }

   /* Initialize locals */
   hDrop      = NULL;
   rgPathInfo = NULL;

   ZeroMemory(&FormatEtc,
              sizeof(FORMATETC));

   ZeroMemory(&StgMedium,
              sizeof(STGMEDIUM));

   /* Get the data into an HDROP structure */
   FormatEtc.cfFormat = CF_HDROP;
   FormatEtc.dwAspect = DVASPECT_CONTENT;
   FormatEtc.lindex   = -1;
   FormatEtc.tymed    = TYMED_HGLOBAL;

   hr = pDataObj->GetData(&FormatEtc,
                          &StgMedium);

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   hDrop = reinterpret_cast<HDROP>(GlobalLock(StgMedium.hGlobal));
   if ( !hDrop )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Get a count of files in the drop and allocate the storage for them */
   cFiles = DragQueryFile(hDrop,
                          0xffffffff,
                          NULL,
                          0);
   if ( cFiles <= 0 )
   {
      hr = E_INVALIDARG;
      /* Failure */
      goto __CLEANUP;
   }

   rgPathInfo = reinterpret_cast<PATHINFO*>(malloc(sizeof(PATHINFO) * cFiles));
   if ( !rgPathInfo )
   {
      hr = E_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   ZeroMemory(rgPathInfo,
              sizeof(PATHINFO) * cFiles);

   /* Save all the filenames in the drop */
   for ( idx = 0; idx < cFiles; idx++ )
   {
      cchRet = DragQueryFile(hDrop,
                             idx,
                             NULL,
                             0);

      if ( cchRet < _countof(rgPathInfo[idx].chFilePath) )
      {
         DragQueryFile(hDrop,
                       idx,
                       rgPathInfo[idx].chFilePath,
                       _countof(rgPathInfo[idx].chFilePath));         
      }
   }

   /* Save the data to the class */
   Lock();
   {
      _ResetData();

      _cFiles     = cFiles;
      _rgPathInfo = rgPathInfo;
      /* Reset this so cleanup code doesn't free it */
      rgPathInfo  = NULL;
   }
   Unlock();

   /* Success */
   hr = S_OK;

__CLEANUP:
   if ( hDrop )
   {
      GlobalUnlock(StgMedium.hGlobal);
   }

   if ( StgMedium.hGlobal )
   {
      ReleaseStgMedium(&StgMedium);
   }

   if ( rgPathInfo )
   {
      free(rgPathInfo);
   }      

   /* Success / Failure */
   return ( hr );
}

/**********************************************************************

    CShxContextMenu : IContextMenu

 **********************************************************************/

STDMETHODIMP CShxContextMenu::QueryContextMenu( HMENU hMenu, UINT uIdx, UINT idCmdFirst, UINT idCmdLast, UINT uFlags ) throw()
{
   HRESULT      hr;
   DWORD        dwErr;
   BOOL         bRet;
   HMENU        hMenuCtx;
   MENUITEMINFO ItemInfo;
   UINT         iItem;
   UINT         uCmdID;
   
   /* This extention doesn't modify the default verb, so when CMF_DEFAULTONLY is set do nothing */
   if ( CMF_DEFAULTONLY & uFlags )
   {
      hr = MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
      /* Success */
      return ( hr  );
   }

   /* If the host isn't providing enough command IDs for this menu then fail */
   if ( (idCmdLast - idCmdFirst) < CmdIDRequired )
   {
      /* Failure */
      return ( E_FAIL );
   }

   /* Initialize locals */
   hMenuCtx = NULL;

   /* Load the FragExt menu and update each items command ID with those provided by the host */
   hMenuCtx = LoadMenu(_AtlModule.GetMUIInstance(),
                       MAKEINTRESOURCE(IDR_SHELLCONTEXTMENU));

   if ( !hMenuCtx )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   /* Setup the new command ID for the menu items. Skip the first one which will be used
    * later by the FragExt menu item */
   uCmdID = idCmdFirst + 1;

   /* Update the IDs of all the subitems using the ones provided by the host */
   for ( iItem = 0; iItem < _countof(_rgIDMap); iItem++ )
   {
      ItemInfo.cbSize = sizeof(ItemInfo);
      ItemInfo.fMask  = MIIM_ID;
      ItemInfo.wID    = uCmdID++;

      bRet = SetMenuItemInfo(hMenuCtx,
                             _rgIDMap[iItem].CmdID,
                             FALSE,
                             &ItemInfo);

      if ( !bRet )
      {
         dwErr = GetLastError();
         hr    = __HRESULT_FROM_WIN32(dwErr);
         /* Failure */
         goto __CLEANUP;
      }
   }

   /* Now insert the menu info the context menu */
   ItemInfo.cbSize     = sizeof(ItemInfo);
   ItemInfo.fMask      = MIIM_SUBMENU|(IsNTDDIVersion(g_NTVersion, NTDDI_WIN2K) ? MIIM_FTYPE|MIIM_STRING : MIIM_TYPE);
   ItemInfo.fType      = MFT_STRING;
   ItemInfo.wID        = idCmdFirst;
   ItemInfo.hSubMenu   = hMenuCtx;
   ItemInfo.dwTypeData = L"&FragExt";

   bRet = InsertMenuItem(hMenu,
                         uIdx,
                         TRUE,
                         &ItemInfo);

   if ( !bRet )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   /* Build the return value which informs the host of how many IDs were used */
   hr = MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, uCmdID - idCmdFirst);

__CLEANUP:
   if ( FAILED(hr) )
   {
      DestroyMenu(hMenuCtx);
   }

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CShxContextMenu::InvokeCommand( LPCMINVOKECOMMANDINFO lpici ) throw()
{
   HRESULT                 hr;
   int                     idx;
   int                     uCmdIdx;
   bool                    bUnicode;
   LPCMINVOKECOMMANDINFOEX lpicix;

   /* Initialize locals */
   lpicix   = ((sizeof(CMINVOKECOMMANDINFOEX) == lpici->cbSize) ? reinterpret_cast<LPCMINVOKECOMMANDINFOEX>(lpici) : NULL);
   bUnicode = ((lpicix) && (CMIC_MASK_UNICODE == (CMIC_MASK_UNICODE & lpici->fMask)) ? true : false);
   uCmdIdx  = IdxInvalid;

   /* Determine the command being invoked either by its verb or its command offset */
   if ( bUnicode && !IS_INTRESOURCE(lpicix->lpVerbW) )
   {
      /* Compare lpVerbW to known commands */
      for ( idx = 0; idx < _countof(_rgIDMap); idx++ )
      {
         if ( 0 == lstrcmpiW(_rgIDMap[idx].VerbW,
                             lpicix->lpVerbW) )
         {
            uCmdIdx = _rgIDMap[idx].CmdIdx;
            /* Success */
            break;
         }
      }
   }
   else if ( !bUnicode && !IS_INTRESOURCE(lpici->lpVerb) )
   {
      /* Compare lpVerb to known commands */
      for ( idx = 0; idx < _countof(_rgIDMap); idx++ )
      {
         if ( 0 == lstrcmpiA(_rgIDMap[idx].VerbA,
                             lpici->lpVerb) )
         {
            uCmdIdx = _rgIDMap[idx].CmdIdx;
            /* Success */
            break;
         }
      }
   }
   else
   {
      /* The lpVerb, regardless of whether the UNICODE flag is set, contains the
       * index of the menu item selected */
      uCmdIdx = LOWORD(lpici->lpVerb);
   }

   switch ( uCmdIdx )
   {
      case IdxDefragment:
         hr = _OnDefragmentFile();
         break;

      case IdxReportToClip:
         hr = _OnSaveReportTo(true);
         break;

      case IdxReportToFile:
         hr = _OnSaveReportTo(false);
         break;

      default:
         /* Failure */
         hr = E_INVALIDARG;
         break;
   }

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CShxContextMenu::GetCommandString( UINT_PTR idCmd, UINT uType, UINT* /*pwReserved*/, LPSTR pszName, UINT cchMax ) throw()
{
   HRESULT hr;
   UINT    uIdx;
   int     cch;

   /* Lookup the index in the ID map */
   for ( uIdx = 0; uIdx < _countof(_rgIDMap); uIdx++ )
   {
      if ( idCmd == _rgIDMap[uIdx].CmdIdx )
      {
         /* Check if the caller is only requesting validation of idCmd */
         if ( (GCS_VALIDATEA == uType) || (GCS_VALIDATEW == uType) )
         {
            /* Success */
            return ( NOERROR );
         }

         if ( (GCS_VERBA == uType) || (GCS_VERBW == uType) )
         {
            if ( GCS_UNICODE & uType )
            {
               hr = StringCchCopyW(reinterpret_cast<LPWSTR>(pszName),
                                   cchMax,
                                   _rgIDMap[uIdx].VerbW);
            }
            else
            {
               hr = StringCchCopyA(pszName,
                                   cchMax,
                                   _rgIDMap[uIdx].VerbA);
            }

            /* Success / Failure */
            return ( hr );
         }

         if ( (GCS_HELPTEXTA == uType) || (GCS_HELPTEXTW == uType) )
         {
            if ( GCS_UNICODE & uType )
            {
               cch = LoadStringW(_AtlModule.GetMUIInstance(),
                                 _rgIDMap[uIdx].ResID,
                                 reinterpret_cast<LPWSTR>(pszName),
                                 cchMax);
            }
            else
            {
               cch = LoadStringA(_AtlModule.GetMUIInstance(),
                                 _rgIDMap[uIdx].ResID,
                                 pszName,
                                 cchMax);
            }

            /* Success / Failure */
            return ( cch > 0 ? S_OK : E_FAIL );
         }

         /* Unknown flags... */
         break;
      }
   }
   
   /* Failure */
   return ( E_INVALIDARG );
}

/**********************************************************************

    CShxContextMenu : IObjectWithSite

 **********************************************************************/

STDMETHODIMP CShxContextMenu::GetSite( REFIID riid, void** ppvSite ) throw()
{
   HRESULT hr;

   /* Initialize outputs */
   (*ppvSite) = NULL;

   /* Initialize locals */
   hr = E_NOINTERFACE;

   Lock();
   {
      if ( _pUnkSite )
      {
         hr = _pUnkSite->QueryInterface(riid,
                                        ppvSite);
      }
   }
   Unlock();

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CShxContextMenu::SetSite( IUnknown* pUnkSite ) throw()
{
   HRESULT           hr;
   HWND              hWnd;
   IServiceProvider* pService;
   IShellBrowser*    pShellBrowser;
   IOleWindow*       pOleWindow;

   /* If pUnkSite is NULL the host is telling us to reset the site and return */
   if ( !pUnkSite )
   {
      Lock();
      {
         _ResetSite();
      }
      Unlock();

      /* Success */
      return ( S_OK );
   }

   /* Initialize locals */
   hWnd          = NULL;
   pService      = NULL;
   pShellBrowser = NULL;
   pOleWindow    = NULL;

   /* Attempt to get an HWND from the site. If this fails, we still succeed */
   hr = pUnkSite->QueryInterface(IID_IServiceProvider,
                                 reinterpret_cast<void**>(&pService));

   if ( FAILED(hr) )
   {
      /* Success */
      goto __CLEANUP;
   }

   hr = pService->QueryService(SID_SShellBrowser,
                               IID_IShellBrowser,
                               reinterpret_cast<void**>(&pShellBrowser));

   if ( FAILED(hr) )
   {
      /* Success */
      goto __CLEANUP;
   }

   hr = pShellBrowser->QueryInterface(IID_IOleWindow,
                                      reinterpret_cast<void**>(&pOleWindow));

   if ( FAILED(hr) )
   {
      /* Success */
      goto __CLEANUP;
   }

   hr = pOleWindow->GetWindow(&hWnd);

__CLEANUP:
   /* Save the site */
   Lock();
   {
      _ResetSite();

      if ( SUCCEEDED(hr) )
      {
         _pUnkSite = pUnkSite;
         pUnkSite->AddRef();
         _hWnd     = hWnd;
      }
   }
   Unlock();

   if ( pOleWindow )
   {
      pOleWindow->Release();
   }

   if ( pShellBrowser )
   {
      pShellBrowser->Release();
   }

   if ( pService )
   {
      pService->Release();
   }

   /* Success / Failure */
   return ( hr );
}

/**********************************************************************

    CShxContextMenu : CShxContextMenu

 **********************************************************************/
CShxContextMenu::CShxContextMenu( ) throw()
{
   _cFiles     = 0;
   _rgPathInfo = NULL;
   _pUnkSite   = NULL;
   _hWnd       = NULL;
}

HRESULT CShxContextMenu::_OnDefragmentFile( ) throw()
{
   HRESULT         hr;
   IDefragManager* pDefragMgr;
   UINT            idx;
   
   /* Initialize locals */
   pDefragMgr = NULL;

   /* Check for directories and whether to process them or not */
   _DoProcessChildrenCheck();
   
   /* Load up the defragmenter.. */
   hr = CoCreateInstance(__uuidof(DefragManager),
                         NULL,
                         CLSCTX_LOCAL_SERVER,
                         __uuidof(IDefragManager),
                         reinterpret_cast<LPVOID*>(&pDefragMgr));

   if ( FAILED(hr) )
   {
      _dTrace(TRUE, L"Failed to create DefragManager instance!0x%08lx\n", hr);
      /* Failure */
      return ( hr );
   }
   
   /* Queue each file for defrag */
   Lock();
   {
      for ( idx = 0; idx < _cFiles; idx++ )
      {
         hr = pDefragMgr->QueueFile(_rgPathInfo[idx].chFilePath);
         ATLASSERT(SUCCEEDED(hr));

         /* If this is a directory then load all child files */
         if ( _rgPathInfo[idx].bIsDirectory && _rgPathInfo[idx].bProcessChildren )
         {
            hr = _DefragmentDirectory(pDefragMgr,
                                      _rgPathInfo[idx].chFilePath);
         }
      }
   }
   Unlock();

   pDefragMgr->Release();

   /* Success */
   return ( S_OK );
}

HRESULT CShxContextMenu::_DefragmentDirectory( IDefragManager* pDefragMgr, LPCWSTR pwszDirectory ) throw()
{
   HRESULT         hr;
   WCHAR           chPath[MAX_PATH+36];
   HANDLE          hFind;
   WIN32_FIND_DATA wFindData;

   /* Initialize locals */
   hFind = INVALID_HANDLE_VALUE;

   ZeroMemory(chPath,
              sizeof(chPath));

   ZeroMemory(&wFindData,
              sizeof(WIN32_FIND_DATA));

   /* Build the search string */
   hr = StringCchCopyW(chPath,
                       _countof(chPath),
                       pwszDirectory);

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   if ( !PathCchAppend(chPath,
                       _countof(chPath),
                       L"*") )
   {
      hr = E_INVALIDARG;
      /* Failure */
      goto __CLEANUP;
   }

   hFind = FindFirstFile(chPath,
                         &wFindData);

   if ( INVALID_HANDLE_VALUE != hFind )
   {
      do
      {
         if ( !PathCchIsDotDirectory(wFindData.cFileName) )
         {
            /* Build the full path for this file */
            if ( PathCchReplaceFileSpec(chPath,
                                        _countof(chPath),
                                        wFindData.cFileName) )
            {
               hr = pDefragMgr->QueueFile(chPath);
            }
         }
      }
      while ( FindNextFile(hFind,
                           &wFindData) );
   }

__CLEANUP:
   if ( INVALID_HANDLE_VALUE != hFind )
   {
      FindClose(hFind);
   }

   /* Success / Failure */
   return ( hr );   
}

HRESULT CShxContextMenu::_OnSaveReportTo( bool bToClipboard ) throw()
{
   HRESULT     hr;
   CFragReport cFragReport;
   UINT        idx;
   DWORD       dwErr;
   HANDLE      hFragCtx;
   DWORD       dwShowCompressed;

   /* Initialize locals */
   hFragCtx = NULL;

   /* Try opening a new report... */
   hr = cFragReport.OpenReport(bToClipboard ? CFragReport::WriteModeClipboard : CFragReport::WriteModeFile,
                               _hWnd);

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   dwShowCompressed = 0;
   GetRegistryValueDword(RVF_HKCU,
                         FRAGSHX_SUBKEY,
                         FRAGSHX_SHOWCOMPRESSEDUNITS,
                         &dwShowCompressed);          

   /* Loop through the files and append each one to the report */
   for ( idx = 0; idx < _cFiles; idx++ )
   {
      dwErr = FpCreateContext(_rgPathInfo[idx].chFilePath,
                              (0 != dwShowCompressed ? FPF_CTX_RAWEXTENTDATA : 0)|FPF_CTX_DETACHED,
                              &hFragCtx);

      if ( NO_ERROR == dwErr )
      {
         hr = cFragReport.AppendFile(_rgPathInfo[idx].chFilePath,
                                     hFragCtx);

         FpCloseContext(hFragCtx);
         hFragCtx = NULL;
      }
   }
   
__CLEANUP:
   if ( hFragCtx )
   {
      FpCloseContext(hFragCtx);
   }

   if ( SUCCEEDED(hr) && bToClipboard )
   {
      cFragReport.CopyToClipboard(_hWnd);
   }

   cFragReport.CloseReport();

   /* Success / Failure */
   return ( hr );
}

HRESULT CShxContextMenu::_DoProcessChildrenCheck( ) throw()
{  
   HRESULT     hr;
   UINT        idx;
   UINT        cDir;
   LPCWSTR*    rgDir;
   FOLDERINFO  fldInfo;

   /* Allocate an array for all the possible directories and populate it */
   cDir  = 0;
   rgDir = reinterpret_cast<LPCWSTR*>(malloc(sizeof(LPCWSTR) * _cFiles));
   if ( !rgDir )
   {
      hr = E_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }   
   
   ZeroMemory(rgDir,
              sizeof(LPCWSTR) * cDir);

   /* First scan the PATHINFO array and populate the directory array */
   for ( idx = 0; idx < _cFiles; idx++ )
   {
      if ( PathCchDirectoryExists(_rgPathInfo[idx].chFilePath) )
      {
         rgDir[cDir++] = &(_rgPathInfo[idx].chFilePath[0]);

         /* Also update the flags for later processing */
         _rgPathInfo[idx].bIsDirectory     = 1;
         _rgPathInfo[idx].bProcessChildren = 1;
      }
   }

   if ( !(cDir > 0) )
   {
      hr = S_OK;
      /* There are no directories to process, so just return */
      goto __CLEANUP;
   }

   ZeroMemory(&fldInfo,
              sizeof(FOLDERINFO));

   fldInfo.hwndParent  = _hWnd;
   fldInfo.cFolders    = cDir;
   fldInfo.rgszFolders = rgDir;
   fldInfo.pCallback   = &CShxContextMenu::_FolderSelectCallback;
   fldInfo.pParameter  = reinterpret_cast<PVOID>(this);

   CFxFolderDialog::ShowFolderSelectionDialog(&fldInfo);

   /* Success */
   hr = S_OK;
   
__CLEANUP:
   if ( rgDir )
   {
      free(rgDir);
   }

   /* Success / Failure */
   return ( hr );
}

void CShxContextMenu::_FolderSelectCallback( LPCWSTR pwszFolder, BOOL bChecked, PVOID pParameter ) throw()
{
   CShxContextMenu* pThis;
   pThis = reinterpret_cast<CShxContextMenu*>(pParameter);

   pThis->_OnFolderSelect(pwszFolder,
                          bChecked);
}

void CShxContextMenu::_OnFolderSelect( LPCWSTR pwszFolder, BOOL bChecked ) throw()
{
   PPATHINFO pInfo;

   /* pwszFolder is a pointer to an item in the PATHINFO array, so we can use it to calculate the
    * base of the owning element */
   pInfo = reinterpret_cast<PPATHINFO>(reinterpret_cast<PCHAR>(const_cast<LPWSTR>(pwszFolder)) - FIELD_OFFSET(PATHINFO, chFilePath));

   _dTrace(L"%s is %s\n", pwszFolder, bChecked ? L"selected" : L"unselected");
   pInfo->bProcessChildren = (bChecked ? 1 : 0);      
}

void CShxContextMenu::_ResetData( ) throw()
{
   if ( _rgPathInfo )
   {
      free(_rgPathInfo);
      _rgPathInfo = NULL;
   }

   _cFiles = 0;
}

void CShxContextMenu::_ResetSite( ) throw()
{
   if ( _pUnkSite )
   {
      _pUnkSite->Release();
      _pUnkSite = NULL;
   }
}
