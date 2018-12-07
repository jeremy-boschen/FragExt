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
 
/* ColProv.h
 *    Implemetation of IColumnProviderImpl.
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 07/24/2004 - Created
 */

#pragma once

/**
 * _COLUMNINFOT
 */
#pragma warning( disable : 4201 )
template < class T > struct _COLUMNINFOT
{
   LPCGUID       pfmtid;
   DWORD         pid;
   VARTYPE       vt;
   DWORD         fmt;
   int           cChars;
   DWORD         csFlags;
   LPWSTR        pszTitle;
   LPWSTR        pszDescription;
   HRESULT (T::* pmfnHandler)( LPCTSTR pszFolder, DWORD pId, LPCSHCOLUMNDATA pColData, VARIANT* pVarData, const _COLUMNINFOT<T>* pCinfo );
};
#pragma warning( default : 4201 )

/* BEGIN_COLPROVIDER_MAP
 */
#define BEGIN_COLPROVIDER_MAP(_class, _fmtid) \
   typedef _class _ColProviderClass; \
   static const _COLUMNINFOT< _ColProviderClass >* _GetColumnMap( ) throw() \
   { \
      static LPCGUID __pfmtid = (LPCGUID)&_fmtid; \
      static _COLUMNINFOT< _ColProviderClass > __columnMap[] = \
      { 


/* COLPROVIDER_ENTRY
 */
#define COLPROVIDER_ENTRY(_pid, _vt, _fmt, _cchars, _csflags, _wztitle, _wzdesc, _mfnhandler) \
         {__pfmtid, _pid, _vt, _fmt, _cchars, _csflags, _wztitle, _wzdesc, &_ColProviderClass::##_mfnhandler},\

/* END_COLPROVIDER_MAP
 */
#define END_COLPROVIDER_MAP() \
         {NULL, 0, VT_NULL, 0, 0, 0, NULL, NULL, NULL} \
      }; \
      return ( __columnMap ); \
   }

/* IColumnProviderImpl
 */
template < class T > class ATL_NO_VTABLE IColumnProviderImpl : public IColumnProvider
{
   /* IColumnProviderImpl */
public:
   enum
   {
      PaddingChars = 2
   };

   IColumnProviderImpl( ) throw()
   {
      ZeroMemory(_chFolder,
                 sizeof(_chFolder));
   }

   typedef _COLUMNINFOT<T> _COLUMNINFO, *_PCOLUMNINFO;

   /* IColumnProvider */
public:
   STDMETHOD(Initialize)( LPCSHCOLUMNINIT psci )
   {
      ATLASSERT(NULL != psci);

      if ( !psci )
      {
         return ( E_POINTER );
      }

      return ( StringCchCopy(_chFolder, 
                             sizeof(_chFolder) / sizeof(WCHAR), 
                             psci->wszFolder) );
   }

   STDMETHOD(GetColumnInfo)( DWORD dwIndex, SHCOLUMNINFO* psci )
   {
      ATLASSERT(NULL != psci);

      if ( !psci )
      {
         return ( E_POINTER );
      }

      HRESULT hr;
      T*      pT;
      const _COLUMNINFO* pCi;

      pT  = static_cast<T*>(this);
      pCi = _LookupColumnInfo(dwIndex);

      if ( pCi )
      {
         psci->scid.fmtid = *(pCi->pfmtid);
         psci->scid.pid   = dwIndex;
         psci->vt         = pCi->vt;
         psci->fmt        = pCi->fmt;
         psci->cChars     = pCi->cChars;
         psci->csFlags    = pCi->csFlags;

         if ( !(SHCOLSTATE_HIDDEN & pCi->csFlags) )
         {
            if ( pCi->pszTitle )
            {
               if ( IS_INTRESOURCE(pCi->pszTitle) )
               {
                  psci->cChars = pT->_GetColumnString(LOWORD(pCi->pszTitle), 
                                                      psci->wszTitle, 
                                                      MAX_COLUMN_NAME_LEN) + pT->PaddingChars;
               }
               else
               {
                   hr = StringCchCopyW(psci->wszTitle, 
                                       MAX_COLUMN_NAME_LEN, 
                                       pCi->pszTitle);

                   if ( FAILED(hr) )
                   {
                      /* Failure */
                      return ( hr );
                   }
               }
            }

            if ( pCi->pszDescription )
            {
               if ( IS_INTRESOURCE(pCi->pszDescription) )
               {
                  pT->_GetColumnString(LOWORD(pCi->pszDescription), 
                                       psci->wszDescription, 
                                       MAX_COLUMN_NAME_LEN);
               }
               else
               {
                  hr = StringCchCopyW(psci->wszDescription, 
                                      MAX_COLUMN_DESC_LEN, 
                                      pCi->pszDescription);

                  if ( FAILED(hr) )
                  {
                     /* Failure */
                     return ( hr );
                  }
               }
            }
         }

         /* Success */
         return ( S_OK );
      }

      /* Success - Not found */
      return ( S_FALSE );
   }

   STDMETHOD(GetItemData)( LPCSHCOLUMNID pscid, LPCSHCOLUMNDATA pscd, VARIANT* pVarData )
   {
      const _COLUMNINFO* pCi;
      T*                 pT;

      ATLASSERT(NULL != pscid);
      ATLASSERT(NULL != pscd);
      ATLASSERT(NULL != pVarData);

      if ( !pscid || !pscd || !pVarData )
      {
         return ( E_POINTER );
      }

      /* Initialize the output data */
      VariantInit(pVarData);

      pCi = _LookupColumnInfo(pscid->pid);
      pT  = static_cast<T*>(this);

      return ( (pT->*pCi->pmfnHandler)(_chFolder, 
                                       pscid->pid, 
                                       pscd, 
                                       pVarData, 
                                       pCi) );
   }

   static const _COLUMNINFO* _LookupColumnInfo( DWORD pid ) throw()
   {
      const _COLUMNINFO* pCi = T::_GetColumnMap();
      ATLASSERT(NULL != pCi);

      while ( pCi->pfmtid )
      {
         if ( pid == pCi->pid )
         {
            return ( pCi );
         }

         pCi++;
      }

      return ( NULL );
   }

   int _GetColumnString( UINT uID, LPTSTR lpBuffer, int cchBufferMax ) throw()
   {
      return ( AtlLoadString(uID, lpBuffer, cchBufferMax) );
   }

   WCHAR _chFolder[MAX_PATH];
};
