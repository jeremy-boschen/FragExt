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

/* Column.h
 *    Header file for CColumnProvider
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 07/18/2004 - Created
 */

#pragma once

#include <ColProv.h>
#include <FragEng.h>

/**********************************************************************

    PROPID listing for CColumnProvider

 **********************************************************************/

#define PID_COLNUMBEROF_FRAGMENTS      ((PROPID)0)
#define PID_TIPNUMBEROF_FRAGMENTS      ((PROPID)1)

#define PID_COLNUMBEROF_EXTENTS        ((PROPID)2)
#define PID_TIPNUMBEROF_EXTENTS        ((PROPID)3)

#define PID_COLNUMBEROF_CLUSTERS       ((PROPID)4)
#define PID_TIPNUMBEROF_CLUSTERS       ((PROPID)5)

#define PID_COLSIZEOF_ONDISK           ((PROPID)6)
#define PID_TIPSIZEOF_ONDISK           ((PROPID)7)

#define PID_COLNUMBEROF_DATASTREAMS    ((PROPID)8)
#define PID_TIPNUMBEROF_DATASTREAMS    ((PROPID)9)

#define PID_COLPERCENTOF_ONDISK        ((PROPID)10)
#define PID_TIPPERCENTOF_ONDISK        ((PROPID)11)

/* CLSID_ColumnProvider
 *     {242ED098-D606-4FA8-9DDE-89AEDFE4EAD7}
 */
extern "C" const CLSID CLSID_ColumnProvider = {0x242ED098, 0xD606, 0x4FA8, {0x9D, 0xDE, 0x89, 0xAE, 0xDF, 0xE4, 0xEA, 0xD7}};

/* CColumnProvider
 */
class ATL_NO_VTABLE CColumnProvider : public CComObjectRootEx<CComMultiThreadModel>,
                                      public CComCoClass<CColumnProvider, &CLSID_ColumnProvider>,
                                      public IColumnProviderImpl<CColumnProvider>
{
   /* ATL */
public:
#ifdef _DEBUG
   DECLARE_REGISTRY_RESOURCEID(IDR_COLUMNHANDLER)
#else /* _DEBUG */
   DECLARE_NO_REGISTRY()
#endif /* _DEBUG */

   DECLARE_NOT_AGGREGATABLE(CColumnProvider)

   BEGIN_COM_MAP(CColumnProvider)
	   COM_INTERFACE_ENTRY(IColumnProvider)
   END_COM_MAP()

   HRESULT FinalConstruct( );
   void FinalRelease( );
   
   /* IColumnProviderImpl */
public:
   BEGIN_COLPROVIDER_MAP(CColumnProvider, CLSID_ColumnProvider)
      /* Text versions for the info-tip require SHCOLSTATE_HIDDEN
       * so that they don't show up in the column selection */

      /* Column:Fragments */
      COLPROVIDER_ENTRY(PID_COLNUMBEROF_FRAGMENTS,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_TYPE_INT|SHCOLSTATE_SLOW|SHCOLSTATE_EXTENDED|SHCOLSTATE_PREFER_VARCMP, 
                        MAKEINTRESOURCE(IDS_COLFRAGMENTSTITLE), 
                        MAKEINTRESOURCE(IDS_COLFRAGMENTSDESC), 
                        _GetColumnItemData)
      /* Column:Extents */
      COLPROVIDER_ENTRY(PID_COLNUMBEROF_EXTENTS,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_TYPE_INT|SHCOLSTATE_SLOW|SHCOLSTATE_SECONDARYUI|SHCOLSTATE_EXTENDED|SHCOLSTATE_PREFER_VARCMP,
                        MAKEINTRESOURCE(IDS_COLEXTENTSTITLE), 
                        MAKEINTRESOURCE(IDS_COLEXTENTSDESC),
                        _GetColumnItemData)
      /* Column:Clusters */
      COLPROVIDER_ENTRY(PID_COLNUMBEROF_CLUSTERS,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_TYPE_INT|SHCOLSTATE_SLOW|SHCOLSTATE_SECONDARYUI|SHCOLSTATE_EXTENDED|SHCOLSTATE_PREFER_VARCMP, 
                        MAKEINTRESOURCE(IDS_COLCLUSTERSTITLE),
                        MAKEINTRESOURCE(IDS_COLCLUSTERSDESC),
                        _GetColumnItemData)

      /* Column:Size on Disk */
      COLPROVIDER_ENTRY(PID_COLSIZEOF_ONDISK,
                        VT_BSTR,
                        0, 
                        0, 
                        SHCOLSTATE_TYPE_INT|SHCOLSTATE_SLOW|SHCOLSTATE_SECONDARYUI|SHCOLSTATE_EXTENDED|SHCOLSTATE_PREFER_VARCMP, 
                        MAKEINTRESOURCE(IDS_COLSIZEONDISKTITLE), 
                        MAKEINTRESOURCE(IDS_COLSIZEONDISKDESC), 
                        _GetColumnItemData)
      /* Column:Streams */
      COLPROVIDER_ENTRY(PID_COLNUMBEROF_DATASTREAMS,
                        VT_BSTR,
                        0, 
                        0, 
                        SHCOLSTATE_TYPE_INT|SHCOLSTATE_SLOW|SHCOLSTATE_SECONDARYUI|SHCOLSTATE_EXTENDED|SHCOLSTATE_PREFER_VARCMP, 
                        MAKEINTRESOURCE(IDS_COLDATASTREAMSTITLE), 
                        MAKEINTRESOURCE(IDS_COLDATASTREAMSDESC), 
                        _GetStreamItemData)
      /* Column:% on Disk */
      COLPROVIDER_ENTRY(PID_COLPERCENTOF_ONDISK,
                        VT_BSTR,
                        0, 
                        0, 
                        SHCOLSTATE_TYPE_INT|SHCOLSTATE_SLOW|SHCOLSTATE_SECONDARYUI|SHCOLSTATE_EXTENDED|SHCOLSTATE_PREFER_VARCMP, 
                        MAKEINTRESOURCE(IDS_COLPERCENTONDISKSTITLE), 
                        MAKEINTRESOURCE(IDS_COLPERCENTONDISKDESC), 
                        _GetColumnItemData)
      /* Tip:Fragments */
      COLPROVIDER_ENTRY(PID_TIPNUMBEROF_FRAGMENTS,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_SLOW|SHCOLSTATE_HIDDEN, 
                        MAKEINTRESOURCE(IDS_TIPFRAGMENTS),
                        NULL, 
                        _GetInfoTipItemData)
      /* Tip:Extents */
      COLPROVIDER_ENTRY(PID_TIPNUMBEROF_EXTENTS, 
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_SLOW|SHCOLSTATE_HIDDEN, 
                        MAKEINTRESOURCE(IDS_TIPEXTENTS),
                        NULL, 
                        _GetInfoTipItemData)
      /* Tip:Clusters */
      COLPROVIDER_ENTRY(PID_TIPNUMBEROF_CLUSTERS,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_SLOW|SHCOLSTATE_HIDDEN, 
                        MAKEINTRESOURCE(IDS_TIPCLUSTERS),
                        NULL, 
                        _GetInfoTipItemData)
      /* Tip:Size on Disk */
      COLPROVIDER_ENTRY(PID_TIPSIZEOF_ONDISK,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_SLOW|SHCOLSTATE_HIDDEN, 
                        MAKEINTRESOURCE(IDS_TIPSIZEONDISK),
                        NULL, 
                        _GetInfoTipItemData)
      /* Tip:Streams */
      COLPROVIDER_ENTRY(PID_TIPNUMBEROF_DATASTREAMS,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_SLOW|SHCOLSTATE_HIDDEN, 
                        MAKEINTRESOURCE(IDS_TIPDATASTREAMS),
                        NULL, 
                        _GetInfoTipItemData)
      /* Tip:% on Disk */
      COLPROVIDER_ENTRY(PID_TIPPERCENTOF_ONDISK,
                        VT_BSTR, 
                        0, 
                        0, 
                        SHCOLSTATE_SLOW|SHCOLSTATE_HIDDEN, 
                        MAKEINTRESOURCE(IDS_TIPPERCENTONDISK),
                        NULL, 
                        _GetInfoTipItemData)
   END_COLPROVIDER_MAP()

   int _GetColumnString( UINT uID, LPTSTR lpBuffer, int cchBufferMax ) throw();

   /* CColumnProvider */
public:
   CColumnProvider( ) throw();

   /* Restricted */
private:
   HRESULT _GetColumnItemData( LPCTSTR pszFolder, DWORD pid, LPCSHCOLUMNDATA pColData, VARIANT* pVarData, const _COLUMNINFO* pColInfo ) throw();
   HRESULT _GetInfoTipItemData( LPCTSTR pszFolder, DWORD pid, LPCSHCOLUMNDATA pColData, VARIANT* pVarData, const _COLUMNINFO* pColInfo ) throw();
   HRESULT _GetStreamItemData( LPCTSTR pszFolder, DWORD pid, LPCSHCOLUMNDATA pColData, VARIANT* pVarData, const _COLUMNINFO* pColInfo ) throw();

   HRESULT _FormatItemData( PROPID pID, VARIANT& agData, VARIANT* pVarData ) throw();
   /* !!!MUST BE CALLED WITHIN A Lock()/Unlock() BLOCK!!! */
   HRESULT _LoadFragItemData( LPCWSTR pszFile, PROPID pID, VARIANT& agData ) throw();

   static PROPID _GetMatchingColumnId( PROPID pID ) throw();
   
   BOOL _bConvertToText;
};

OBJECT_ENTRY_AUTO(CLSID_ColumnProvider, CColumnProvider)
