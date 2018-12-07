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

/* CtxMenu.h
 *    Header file for CShxContextMenu
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 11/14/2004 - Created
 */

#pragma once

#include <FragEng.h>
#include <FragMgx.h>

#include <RegUtil.h>
#include <PathUtil.h>

/**********************************************************************

    

 **********************************************************************/

/* CLSID_ShxContextMenu
 *    {B23E896C-5CC0-40AB-916D-3BA3328FEADD}
 */
extern "C" const CLSID CLSID_ShxContextMenu = {0xB23E896C, 0x5CC0, 0x40AB, {0x91, 0x6D, 0x3B, 0xA3, 0x32, 0x8F, 0xEA, 0xDD}};

/* CShxContextMenu
 */
class ATL_NO_VTABLE CShxContextMenu : public CComObjectRootEx<CComMultiThreadModel>,
                                      public CComCoClass<CShxContextMenu, &CLSID_ShxContextMenu>,
                                      public IShellExtInit,
                                      public IContextMenu,
                                      public IObjectWithSite
{
   /* ATL */
public:
   DECLARE_NO_REGISTRY()

   DECLARE_NOT_AGGREGATABLE(CShxContextMenu)

   BEGIN_COM_MAP(CShxContextMenu)
      COM_INTERFACE_ENTRY(IShellExtInit)
      COM_INTERFACE_ENTRY(IContextMenu)
      COM_INTERFACE_ENTRY(IObjectWithSite)
   END_COM_MAP()

   HRESULT FinalConstruct( ) throw();
   void FinalRelease( ) throw();
    
   /* IShellExtInit */
public:
   STDMETHOD(Initialize)( LPCITEMIDLIST pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID ) throw();

   /* IContextMenu */
public:
   STDMETHOD(QueryContextMenu)( HMENU hMenu, UINT uIdx, UINT idCmdFirst, UINT idCmdLast, UINT uFlags ) throw();
   STDMETHOD(InvokeCommand)( LPCMINVOKECOMMANDINFO lpici ) throw();
   STDMETHOD(GetCommandString)( UINT_PTR idCmd, UINT uType, UINT* pwReserved, LPSTR pszName, UINT cchMax ) throw();

   /* IObjectWithSite */
public:
   STDMETHOD(GetSite)( REFIID riid, void** ppvSite ) throw();
   STDMETHOD(SetSite)( IUnknown* pUnkSite ) throw();

    /* CShxContextMenu */
protected:
   CShxContextMenu( ) throw();

private:
   enum
   {
      eTraceLevelCOM    = 0,

      /* Menu command ID offsets */
      IdxInvalid        = -1,
      IdxFragExt        = 0,
      IdxDefragment     = 1,
      IdxReportToClip   = 2,
      IdxReportToFile   = 3,

      /* Number of command IDs required to host the menu */
      CmdIDRequired     = 4
   };

   typedef struct _PATHINFO
   {
      ULONG bIsDirectory      : 1;
      ULONG bProcessChildren  : 1;
      TCHAR chFilePath[MAX_PATH+36];
   }PATHINFO, *PPATHINFO;

   typedef struct _IDMAP
   {
      USHORT  CmdIdx;
      USHORT  CmdID;
      USHORT  ResID;
      LPCSTR  VerbA;
      LPCWSTR VerbW;
   }IDMAP, *PIDMAP;
   typedef const IDMAP* PCIDMAP;

   UINT                 _cFiles;
   PPATHINFO            _rgPathInfo;
   HWND                 _hWnd;
   IUnknown*            _pUnkSite;
   static const IDMAP   _rgIDMap[];
   
   HRESULT _OnDefragmentFile( ) throw();
   HRESULT _OnSaveReportTo( bool fToClipboard ) throw();   
   
   HRESULT _DoProcessChildrenCheck( ) throw();
   static void _FolderSelectCallback( LPCWSTR pwszFolder, BOOL bChecked, PVOID pParameter ) throw();
   void _OnFolderSelect( LPCWSTR pwszFolder, BOOL bChecked ) throw();

   HRESULT _DefragmentDirectory( IDefragManager* pDefragMgr, LPCWSTR pwszDirectory ) throw();

   void _ResetData( ) throw();
   void _ResetSite( ) throw();
};

__declspec(selectany) const CShxContextMenu::IDMAP CShxContextMenu::_rgIDMap[]=
{
   {IdxDefragment,   IDM_SHELLDEFRAGMENTFILE, IDS_MENUHELPDEFRAGMENT, "defragment",   L"defragment"},
   {IdxReportToClip, IDM_SHELLREPORTTOCLIP,   IDS_MENUHELPREPORTCLIP, "reporttoclip", L"reporttoclip"},
   {IdxReportToFile, IDM_SHELLREPORTTOFILE,   IDS_MENUHELPREPORTFILE, "reporttofile", L"reporttofile"}
};

OBJECT_ENTRY_AUTO(CLSID_ShxContextMenu, CShxContextMenu)
