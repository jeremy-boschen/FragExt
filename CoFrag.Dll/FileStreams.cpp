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

/* FileStreams.cpp
 *    CFileStream and CFileStreams implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 09/03/2005 - Created
 */

#include "Stdafx.h"
#include "FileStreams.h"

/**********************************************************************

    CFileStream : IFileStream

 **********************************************************************/
STDMETHODIMP CFileStream::get_Name( BSTR* pbszName )
{
   BSTR bszName;

   /* Validate parameters */
   if ( !pbszName )
   {
      /* Failure */
      return ( E_POINTER );
   }

   /* If the pointer is invalid, this will cause an exception */
   (*pbszName) = NULL;

   /* Create a copy of the stream name. We always allocate fixed sizes to help
    * enable memory reuse */
   bszName = SysAllocStringLen(NULL,
                               _countof(_chName));
   if ( !bszName )
   {
      /* Failure */
      return ( E_OUTOFMEMORY );
   }

   StringCchCopy(bszName,
                 _countof(_chName),
                 _chName);

   (*pbszName) = bszName;

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileStream::get_Size( double* pdblSize )
{
   /* Validate parameters */
   if ( !pdblSize )
   {
      /* Failure */
      return ( E_POINTER );
   }

   (*pdblSize) = static_cast<double>(_cbSize);

   /* Success */
   return ( S_OK );
}

/**********************************************************************

    CFileStream : CFileStream

 **********************************************************************/
HRESULT CFileStream::CreateFromStreamInfo( LPCWSTR pszName, ULONGLONG cbStream, REFIID riid, void** ppObject ) throw()
{
   HRESULT       hr;
   CoFileStream* pStream;

   /* Initialize locals */
   hr      = E_FAIL;
   pStream = NULL;

   hr = CreateObjectInstance<CFileStream>(riid,
                                          ppObject,
                                          &pStream);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   hr = StringCchCopy(pStream->_chName, 
                      _countof(pStream->_chName), 
                      pszName);

   if ( FAILED(hr) )
   {
      hr = E_INVALIDARG;
      /* Failure */
      goto __CLEANUP;
   }

   pStream->_cbSize = cbStream;

__CLEANUP:
   pStream->Release();

   /* Success / Failure */
   return ( hr );
}

CFileStream::CFileStream( ) throw() 
{
   _cbSize = 0;

   ZeroMemory(_chName,
              sizeof(_chName));
}

/**********************************************************************

    CFileStreams : ATL

 **********************************************************************/
void CFileStreams::FinalRelease( )
{
   /* No lock is needed here because there are no outstanding references */
   _ResetData();
}

/**********************************************************************

    CFileStreams : IFileStreams

 **********************************************************************/
STDMETHODIMP CFileStreams::get__NewEnum( IUnknown** ppEnum )
{
   HRESULT hr;

   /* Validate parameters */
   if ( !ppEnum )
   {
      /* Failure */
      return ( E_POINTER );
   }

   (*ppEnum) = NULL;

   Lock();
   {
      hr = CEnumFileStreams::CreateInstance(_rgStream, 
                                            _cElem, 
                                            0, 
                                            IID_IUnknown, 
                                            reinterpret_cast<void**>(ppEnum));
   }
   Unlock();

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CFileStreams::get_Item( LONG iIndex, IDispatch** ppStream )
{
   HRESULT hr;

   /* Validate parameters */
   if ( !ppStream )
   {
      /* Failure */
      return ( E_POINTER );
   }

   (*ppStream) = NULL;

   /* iIndex needs to be validated in a lock... */

   Lock();
   {
      #pragma warning( suppress : 4127 )
      while ( 1 )
      {
         if ( static_cast<ULONG>(iIndex) >= _cElem )
         {
            hr = DISP_E_BADINDEX;
            /* Failure */
            break;
         }

         (*ppStream) = _rgStream[iIndex].pdispVal;
         (*ppStream)->AddRef();

         hr = S_OK;
         /* Success */
         break;
      }
   }
   Unlock();

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CFileStreams::get_Count( LONG* piCount )
{
   /* piCount parameters */
   if ( !piCount )
   {
      /* Failure */
      return ( E_POINTER );
   }

   (*piCount) = static_cast<LONG>(_cElem);

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileStreams::OpenFile( BSTR bszPath, VARIANT_BOOL* pbOpened )
{
   DWORD    dwRet;
   ENUMCTX  EnumCtx;
   
   /* Validate parameters */
   if ( !bszPath )
   {
      /* Failure */
      return ( E_POINTER );
   }

   if ( pbOpened )
   {
      (*pbOpened) = VARIANT_FALSE;
   }

   /* Initialize locals */
   ZeroMemory(&EnumCtx,
              sizeof(ENUMCTX));

   /* Load the stream data via the enum proc */
   dwRet = FpEnumFileStreamInformation(bszPath,
                                       FPF_OPENBY_FILENAME,
                                       &CFileStreams::_EnumFileStreamRoutine,
                                       &EnumCtx);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      goto __CLEANUP;
   }


   Lock();
   {
      _ResetData();

      _cElem    = EnumCtx.Count;
      _rgStream = EnumCtx.Streams;
   }
   Unlock();

   /* Clear this so the cleanup code ignores it */
   EnumCtx.Streams = NULL;

   if ( pbOpened )
   {
      (*pbOpened) = VARIANT_TRUE;
   }

   
__CLEANUP:
   if ( EnumCtx.Streams )
   {
      while ( EnumCtx.Count-- )
      {
         VariantClear(&EnumCtx.Streams[EnumCtx.Count]);
      }

      free(EnumCtx.Streams);
   }

   /* Success / Failure */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

/**********************************************************************

    CFileStreams : CFileStreams

 **********************************************************************/

CFileStreams::CFileStreams( ) throw()
{
   _cElem    = 0;
   _rgStream = NULL;
}

/* !!!MUST BE CALLED INSIDE A LOCK!!! */
void CFileStreams::_ResetData( ) throw()
{
   while ( _cElem-- )
   {
      ATLASSERT(NULL != _rgStream);
      VariantClear(&_rgStream[_cElem]);
   }

   free(_rgStream);
   _rgStream = NULL;
   _cElem    = 0;
}

DWORD FRAGAPI CFileStreams::_EnumFileStreamRoutine( ULONG StreamCount, ULONGLONG StreamSize, ULONGLONG /*StreamSizeOnDisk*/, LPCWSTR StreamName, PVOID Parameter ) throw()
{
   HRESULT    hr;
   PENUMCTX   pEnumCtx;
   IDispatch* pDispatch;

   /* Initialize locals */
   pEnumCtx  = reinterpret_cast<PENUMCTX>(Parameter);
   pDispatch = NULL;

   /* Allocate the VARIANT array if it hasn't been */
   if ( !pEnumCtx->Streams )
   {
      pEnumCtx->Count   = StreamCount;
      pEnumCtx->Streams = reinterpret_cast<VARIANT*>(malloc(sizeof(VARIANT) * StreamCount));
      if ( !pEnumCtx->Streams )
      {
         /* Failure */
         return ( ERROR_OUTOFMEMORY );
      }

      ZeroMemory(pEnumCtx->Streams,
                 sizeof(VARIANT) * StreamCount);
   }

   ATLASSERT(pEnumCtx->Count == StreamCount);
   ATLASSERT(pEnumCtx->Count > pEnumCtx->Index);
   
   /* Create the stream object */
   hr = CFileStream::CreateFromStreamInfo(StreamName,
                                          StreamSize,
                                          IID_IDispatch,
                                          reinterpret_cast<void**>(&pDispatch));

   if ( SUCCEEDED(hr) )
   {
      pEnumCtx->Streams[pEnumCtx->Index].vt       = VT_DISPATCH;
      pEnumCtx->Streams[pEnumCtx->Index].pdispVal = pDispatch;
      pEnumCtx->Index += 1;

      /* Success */
      return ( NO_ERROR );
   }

   /* Failure */
   return ( static_cast<DWORD>(hr) );
}

/**********************************************************************

    CEnumFileStreams : ATL

 **********************************************************************/
void CEnumFileStreams::FinalRelease( )
{
   _ResetData();
}

/**********************************************************************

    CEnumFileStreams : IEnumVARIANT

 **********************************************************************/
STDMETHODIMP CEnumFileStreams::Next( ULONG celt, VARIANT* rgelt, ULONG* peltFetched )
{
   HRESULT hr;
   ULONG   cRemaining;
   ULONG   cFetched;
   ULONG   iIdx;
      
   /* Validate parameters */
   if ( !celt )
   {
      /* Failure */
      return ( E_INVALIDARG );
   }

   if ( !rgelt || ((1 != celt) && !peltFetched) )
   {
      /* Failure */
      return ( E_POINTER );
   }   

   /* Initialize outputs */
   if ( peltFetched )
   {
      (*peltFetched) = NULL;
   }

   if ( rgelt )
   {
      ZeroMemory(rgelt,
                 sizeof(VARIANT) * celt);
   }

   /* Initialze locals */
   hr       = S_FALSE;
   cFetched = 0;

   Lock();

   __try
   {
      cRemaining = (_cElem - _cIter);
      cFetched   = (celt < cRemaining ? celt : cRemaining);

      for ( iIdx = 0; iIdx < cFetched; iIdx++, _cIter++ )
      {
         hr = VariantCopy(&rgelt[iIdx], 
                          &_rgStream[_cIter]);

         if ( FAILED(hr) )
         {
            /* Failure */
            break;
         }
      }
   }
   __finally
   {
      Unlock();
   }

   if ( FAILED(hr) )
   {
      for ( iIdx = 0; iIdx < cFetched; iIdx++ )
      {
         VariantClear(&rgelt[iIdx]);
      }

      /* Failure */
      return ( hr );
   }
      
   if ( peltFetched )
   {
      (*peltFetched) = cFetched;
   }

   hr = (cFetched < celt ? S_FALSE : S_OK);

   /* Success */
   return ( hr );
}

STDMETHODIMP CEnumFileStreams::Skip( ULONG celt )
{
   ULONG cRemaining;
   ULONG cSkip;
    
   /* Validate parameters */
   if ( 0 == celt )
   {
      /* Failure */
      return ( E_INVALIDARG );
   }

   Lock();
   {
      cRemaining = (_cElem - _cIter);
      cSkip      = (celt > cRemaining ? cRemaining : celt);      
      _cIter    += cSkip;
   }
   Unlock();
   
   /* Success */
   return ( celt != cSkip ? S_FALSE : S_OK );
}

STDMETHODIMP CEnumFileStreams::Reset( )
{
   Lock();
   {
      _cIter = 0;
   }
   Unlock();

   return ( S_OK );
}

STDMETHODIMP CEnumFileStreams::Clone( IEnumVARIANT** ppEnum )
{
   HRESULT       hr;
   IEnumVARIANT* pEnum;

   /* Validate parameters */
   if ( !ppEnum )
   {
      /* Failure */
      return ( E_POINTER );
   }

   /* Initialize outputs */
   pEnum = NULL;

   Lock();
   {
      hr = CEnumFileStreams::CreateInstance(_rgStream, 
                                            _cElem, 
                                            _cIter, 
                                            IID_IEnumVARIANT, 
                                            reinterpret_cast<void**>(&pEnum));
   }
   Unlock();

   (*ppEnum) = pEnum;

   /* Success / Failure */
   return ( hr );
}

/**********************************************************************

    CEnumFileStreams : CEnumFileStreams

 **********************************************************************/
CEnumFileStreams::CEnumFileStreams( ) throw()
{
   _cIter    = 0;
   _cElem    = 0;
   _rgStream = 0;
}

HRESULT CEnumFileStreams::CreateInstance( VARIANT* rgStreams, ULONG cElem, ULONG cIter, REFIID riid, void** ppObject ) throw()
{
   HRESULT            hr;
   CoEnumFileStreams* pEnum;
   VARIANT*           rgTmp;
   ULONG              iIdx;

   /* Initialize locals */
   hr    = E_FAIL;
   pEnum = NULL;
   rgTmp = NULL;
   
   /* Create the enumerator object */
   hr = CreateObjectInstance<CEnumFileStreams>(riid,
                                               ppObject,
                                               &pEnum);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   /* Allocate and initialize a copy of the VARIANT array */
   rgTmp = reinterpret_cast<VARIANT*>(malloc(sizeof(VARIANT) * cElem));
   if ( !rgTmp )
   {
      hr = E_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   ZeroMemory(rgTmp,
              sizeof(VARIANT) * cElem);

   for ( iIdx = 0; iIdx < cElem; iIdx++ )
   {
      hr = VariantCopy(&rgTmp[iIdx],
                       &rgStreams[iIdx]);

      if ( FAILED(hr) )
      {
         /* Failure */
         goto __CLEANUP;
      }
   }

   /* Assign the copy to the enumerator object */
   pEnum->_cElem    = cElem;
   pEnum->_cIter    = cIter;
   pEnum->_rgStream = rgTmp;

   /* Clear the local VARIANT array so the cleanup code doesn't free it */
   rgTmp = NULL;

   /* Success */
   hr = S_OK;

__CLEANUP:
   free(rgTmp);

   if ( FAILED(hr) )
   {
      reinterpret_cast<IUnknown*>(*ppObject)->Release();
      (*ppObject) = NULL;
   }

   pEnum->Release();

   /* Success / Failure */
   return ( hr );
}

void CEnumFileStreams::_ResetData( ) throw()
{
   ULONG iIdx;

   for ( iIdx = 0; iIdx < _cElem; iIdx++ )
   {
      ATLASSERT(NULL != _rgStream);
      VariantClear(&_rgStream[iIdx]);
   }

   free(_rgStream);
   _rgStream  = NULL;
   _cElem     = 0;
   _cIter     = 0;
}
