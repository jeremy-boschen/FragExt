
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

/* FileFrags.cpp
 *    CFileFragment, CFileFragments, CEnumFileFragments, CFileDefragmenter,
 *    and CFileFragmenter implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 10/31/2004 - Created
 */

#include "Stdafx.h"
#include "FileFrags.h"

#undef _SIMULATEDEFRAG

/**********************************************************************

   CFileFragment : IFileFragment

 **********************************************************************/

STDMETHODIMP CFileFragment::get_Sequence( LONG* piSequence ) throw()
{
   ATLASSERT(NULL != piSequence);

   if ( !piSequence ) 
   {
      /* Failure */
		return ( E_POINTER );
   }

   (*piSequence) = static_cast<LONG>(_FragInfo.Sequence);

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileFragment::get_LogicalCluster( double* piLogicalCluster ) throw()
{
   ATLASSERT(NULL != piLogicalCluster);

   if ( !piLogicalCluster   )
   {
      /* Failure */
		return ( E_POINTER );
   }
   
   (*piLogicalCluster) = IsVirtualClusterNumber(_FragInfo.LogicalCluster) ? -1.0f : static_cast<double>(_FragInfo.LogicalCluster);

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileFragment::get_ExtentCount( LONG* piExtentCount ) throw()
{
   ATLASSERT(NULL != piExtentCount);

   if ( !piExtentCount ) 
   {
      /* Failure */
		return ( E_POINTER );
   }

   (*piExtentCount) = static_cast<LONG>(_FragInfo.ExtentCount);

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileFragment::get_ClusterCount( double* piClusterCount ) throw()
{
   ATLASSERT(NULL != piClusterCount);

   if ( !piClusterCount )
   {
      /* Failure */
		return ( E_POINTER );
   }

   (*piClusterCount) = static_cast<double>(_FragInfo.ClusterCount);

   /* Success */
   return ( S_OK );
}

/**********************************************************************

   CFileFragment : CFileFragment

 **********************************************************************/

CFileFragment::CFileFragment( ) throw()
{
   ZeroMemory(&_FragInfo,
              sizeof(_FragInfo));
}

HRESULT CFileFragment::CreateFromFragmentInfo( const FRAGMENT_INFORMATION* pFragInfo, REFIID riid, void** ppObject ) throw()
{
   HRESULT         hr;
   CoFileFragment* pFileFrag;

   ATLASSERT(NULL != pFragInfo);
   ATLASSERT(NULL != ppObject);
   
   /* Create a new instance of this class */
   hr = CreateObjectInstance<CFileFragment>(riid,
                                            ppObject,
                                            &pFileFrag);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   /* Copy the frag data and request the initial interface. Requesting the interface will add
    * a reference count if successful, otherwise the initial one will be left intact */
   pFileFrag->_FragInfo = (*pFragInfo);

   /* Release the initial reference. If the interface wasn't acquired, this will free the object */
   pFileFrag->Release();

   /* Success / Failure */
   return ( hr );
}

/**********************************************************************

   CFileFragments : ATL

 **********************************************************************/

void CFileFragments::FinalRelease( ) 
{
   /* There won't be any other references touching this now, so it
    * doesn't need to be in a lock */
   _ResetData();
}

/**********************************************************************

   CFileFragments : IFileFragments

 **********************************************************************/

STDMETHODIMP CFileFragments::get_Count( LONG* piCount ) throw()
{
   HRESULT hr;

   ATLASSERT(NULL != piCount);

   if ( !piCount )
   {
      /* Failure */
      return ( E_POINTER );
   }

   hr = get_FragmentCount(piCount);

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CFileFragments::OpenFile( BSTR bszFile, VARIANT_BOOL bIncludeCompressed, VARIANT_BOOL* pbOpened ) throw()
{
   DWORD  dwRet;
   HANDLE hFragCtx;
   
   /* Validate parameters */
   if ( !bszFile )
   {
      /* Failure */
      return ( E_POINTER );
   }

   /* Initialize outputs */

   /* pbOpened is for scripting clients, so don't require it */
   if ( pbOpened )
   {
      (*pbOpened) = VARIANT_FALSE;
   }

   /* Initialize locals */
   hFragCtx = NULL;

   /* Open a context for the file. The context holds all the data. Methods on this object
    * simply access them to extract the necessary values */
   dwRet = FpCreateContext(bszFile,
                         (VARIANT_TRUE == bIncludeCompressed ? FPF_CTX_RAWEXTENTDATA : 0)|FPF_CTX_DETACHED,
                         &hFragCtx);

   Lock();
   {
      _ResetData();

      _dwTick   = GetTickCount();
      _hFragCtx = hFragCtx;
   }
   Unlock();

   /* Success / Failure */
   if ( pbOpened )
   {
      (*pbOpened) = (NO_ERROR != dwRet ? VARIANT_FALSE : VARIANT_TRUE);
   }
   
   /* Success / Failure */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

STDMETHODIMP CFileFragments::get_FragmentCount( LONG* piFragmentCount  ) throw()
{
   DWORD dwRet;
   ULONG iFragments;

   ATLASSERT(NULL != piFragmentCount);

   if ( !piFragmentCount )
   {
      /* Failure */
		return ( E_POINTER );
   }

   iFragments = 0;

   Lock();
   {
      dwRet = FpGetContextFragmentCount(_hFragCtx,
                                     &iFragments);
   }
   Unlock();

   (*piFragmentCount) = static_cast<LONG>(iFragments);

   /* Success */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

STDMETHODIMP CFileFragments::get_ExtentCount( LONG* piExtentCount ) throw()
{
   DWORD dwRet;
   ULONG iExtents;

   ATLASSERT(NULL != piExtentCount);

   if ( !piExtentCount )
   {
      /* Failure */
		return ( E_POINTER );
   }

   iExtents = 0;

   Lock();
   {
      dwRet = FpGetContextExtentCount(_hFragCtx,
                                   &iExtents);
   }
   Unlock();

   (*piExtentCount) = static_cast<LONG>(iExtents);

   /* Success */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

STDMETHODIMP CFileFragments::get_ClusterCount( double* piClusterCount  ) throw()
{
   DWORD     dwRet;
   ULONGLONG iClusters;

   ATLASSERT(NULL != piClusterCount);

   if ( !piClusterCount  )
   {
      /* Failure */
		return ( E_POINTER );
   }

   iClusters = 0;

   Lock();
   {
      dwRet = FpGetContextClusterCount(_hFragCtx,
                                    &iClusters);
   }
   Unlock();

   (*piClusterCount) = static_cast<double>(iClusters);

   /* Success */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

STDMETHODIMP CFileFragments::get_FileSize( double* pcbFileSize ) throw()
{
   DWORD     dwRet;
   ULONGLONG cbFile;

   ATLASSERT(NULL != pcbFileSize);

   if ( !pcbFileSize  )
   {
      /* Failure */
		return ( E_POINTER );
   }

   cbFile = 0;

   Lock();
   {
      dwRet = FpGetContextFileSize(_hFragCtx,
                            &cbFile);
   }
   Unlock();
   
   (*pcbFileSize) = static_cast<double>(cbFile);

   /* Success */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

STDMETHODIMP CFileFragments::get_FileSizeOnDisk( double* pcbSizeOnDisk ) throw()
{
   DWORD     dwRet;
   ULONGLONG cbFileOnDisk;

   ATLASSERT(NULL != pcbSizeOnDisk);

   if ( !pcbSizeOnDisk  )
   {
      /* Failure */
		return ( E_POINTER );
   }

   cbFileOnDisk = 0;

   Lock();
   {
      dwRet = FpGetContextFileSizeOnDisk(_hFragCtx,
                                         &cbFileOnDisk);
   }
   Unlock();

   (*pcbSizeOnDisk) = static_cast<double>(cbFileOnDisk);

   /* Success */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

STDMETHODIMP CFileFragments::get_ClusterSize( LONG* pcbClusterSize ) throw()
{
   DWORD dwRet;
   ULONG cbCluster;

   ATLASSERT(NULL != pcbClusterSize);

   if ( !pcbClusterSize )
   {
      /* Failure */
      return ( E_POINTER );
   }

   cbCluster = 0;

   Lock();
   {
      dwRet = FpGetContextClusterSize(_hFragCtx,
                                   &cbCluster);
   }
   Unlock();

   (*pcbClusterSize) = static_cast<LONG>(cbCluster);

   /* Success */
   return ( NO_ERROR != dwRet ?  __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

STDMETHODIMP CFileFragments::get_Item( LONG iIndex, IDispatch** ppFragment ) throw()
{
   HRESULT      hr;
   DWORD        dwRet;
   FRAGMENT_INFORMATION FragInfo;
   IDispatch*   pFragment;

   ATLASSERT(NULL != ppFragment);

   if ( !ppFragment )
   {
      /* Failure */
		return ( E_POINTER );
   }

   pFragment = NULL;

   Lock();
   {
      dwRet = FpGetContextFragmentInformation(_hFragCtx,
                                    static_cast<ULONG>(iIndex) + 1,
                                    &FragInfo,
                                    1,
                                    NULL);

      if ( NO_ERROR == dwRet )
      {
         hr = CFileFragment::CreateFromFragmentInfo(&FragInfo,
                                                    IID_IDispatch,
                                                    reinterpret_cast<void**>(&pFragment));
      }
      else if ( ERROR_INVALID_INDEX == dwRet )
      {
         hr = DISP_E_BADINDEX;
      }
      else
      {
         hr = __HRESULT_FROM_WIN32(dwRet);
      }
   }
   Unlock();

   (*ppFragment) = pFragment;

   /* Success / Failure */   
   return ( hr );
}

STDMETHODIMP CFileFragments::get__NewEnum( IUnknown** ppEnum ) throw()
{
   HRESULT   hr;
   IUnknown* pEnum;

   ATLASSERT(NULL != ppEnum);

   if ( !ppEnum )
   {
      /* Failure */
		return ( E_POINTER );
   }

   pEnum = NULL;

   Lock();
   {
      hr = CEnumFileFragments::CreateInstance(this,
                                              _dwTick,
                                              0,
                                              IID_IUnknown,
                                              reinterpret_cast<void**>(&pEnum));
   }
   Unlock();

   (*ppEnum) = pEnum;

   /* Success / Failure */
   return ( hr );
}

/**********************************************************************

   CFileFragments : CFileFragments

 **********************************************************************/

CFileFragments::CFileFragments( ) throw()
{
   _dwTick   = 0;
   _hFragCtx = NULL;
}

bool CFileFragments::_IsExpired( DWORD dwTick ) throw()
{
   /* Success / Failure */
   return ( dwTick == _dwTick ? false : true );
}

void CFileFragments::_ResetData( ) throw()
{
   if ( _hFragCtx )
   {
      FpCloseContext(_hFragCtx);
      _hFragCtx = NULL;
   }

   _dwTick = 0;
}

/**********************************************************************

   CEnumFileFragments : ATL

 **********************************************************************/

void CEnumFileFragments::FinalRelease( ) throw()
{
   if ( _pFileFrags )
   {
      /* Release the data holder.. */
      _pFileFrags->Release();
   }
}

/**********************************************************************

   CEnumFileFragments : IEnumVARIANT

 **********************************************************************/

STDMETHODIMP CEnumFileFragments::Next( ULONG celt, VARIANT* rgelt, ULONG* pceltFetched ) throw()
{
   HRESULT     hr;
   VARIANT*    pElem;
   ULONG       cRemaining;
   ULONG       cFetch;
   IDispatch*  pDispatch;

   ATLASSERT(celt > 0);
   ATLASSERT(NULL != rgelt);

   /* Validate parameters */
   if ( !celt )
   {
      /* Failure */
      return ( E_INVALIDARG );
   }

   if ( !rgelt || ((1 != celt) && !pceltFetched) )
   {
      /* Failure */
      return ( E_POINTER );
   }

   /* Initialize outputs */
   if ( pceltFetched )
   {
      (*pceltFetched) = NULL;
   }
   
   /* Initialize locals */
   hr    = E_FAIL;
   pElem = rgelt;
   
   /* Lock this object and the contained CFileFragments instance while servicing 
    * this method */
   Lock();

   __try
   {
      _pFileFrags->Lock();
       
      __try
      {
         if ( _pFileFrags->_IsExpired(_dwTick) )
         {
            hr = __HRESULT_FROM_WIN32(ERROR_CONTEXT_EXPIRED);
            /* Failure */
            __leave;
         }

         cRemaining = (_cElem - _cIter);
         cFetch     = (celt < cRemaining ? celt : cRemaining);
         
         while ( cFetch-- )
         {
            hr = _pFileFrags->get_Item(_cIter,
                                       &pDispatch);

            if ( FAILED(hr) )
            {
               /* Failure */
               __leave;
            }

            pElem->vt       = VT_DISPATCH;
            pElem->pdispVal = pDispatch;
            pElem           += 1;
            _cIter          += 1;
         }
      }
      __finally
      {
         _pFileFrags->Unlock();
      }
   }
   __finally
   {
      Unlock();
   }

   if ( SUCCEEDED(hr) )
   {
      hr = (pElem < &rgelt[celt - 1] ? S_FALSE : S_OK);

      if ( pceltFetched )
      {
         (*pceltFetched) = (ULONG)(&rgelt[celt] - pElem);
      }
   }
   else
   {
      while ( pElem > rgelt )
      {
         VariantClear(pElem);
      }
   }

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CEnumFileFragments::Skip( ULONG celt ) throw()
{
   ULONG cRemaining;
   ULONG cSkip;

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
   return ( celt == cSkip ? S_OK : S_FALSE );
}

STDMETHODIMP CEnumFileFragments::Reset( ) throw()
{
   Lock();
   {
      _cIter = 0;
   }
   Unlock();

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CEnumFileFragments::Clone( IEnumVARIANT** ppEnum ) throw()
{
   HRESULT       hr;
   IEnumVARIANT* pEnum;

   ATLASSERT(NULL != ppEnum);

   if ( !ppEnum )
   {
      /* Failure */
      return ( E_POINTER );
   }

   pEnum = NULL;

   Lock();
   {
      _pFileFrags->Lock();
      {
         hr = CEnumFileFragments::CreateInstance(_pFileFrags,
                                                 _dwTick,
                                                 _cIter,
                                                 IID_IEnumVARIANT,
                                                 reinterpret_cast<void**>(&pEnum));
      }
      _pFileFrags->Unlock();
   }
   Unlock();

   (*ppEnum) = pEnum;

   /* Success / Failure */
   return ( hr );
}


/**********************************************************************

   CEnumFileFragments : CEnumFileFragments

 **********************************************************************/

CEnumFileFragments::CEnumFileFragments( ) throw()
{
   _dwTick     = 0;
   _cIter      = 0;
   _cElem      = 0;
   _pFileFrags = NULL;
}

HRESULT CEnumFileFragments::CreateInstance( CFileFragments* pFileFrags, DWORD dwTick, ULONG cIterator, REFIID riid, void** ppObject ) throw()
{
   HRESULT              hr;
   LONG                 iCount;
   CoEnumFileFragments* pEnum;

   /* Query the total number of fragments, which becomes this enumerators element count */
   hr = pFileFrags->get_Count(&iCount);
   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   hr = CreateObjectInstance<CEnumFileFragments>(riid,
                                                 ppObject,
                                                 &pEnum);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   /* Setup the enum data references */
   pEnum->_dwTick     = dwTick;
   pEnum->_cIter      = cIterator;
   pEnum->_cElem      = iCount;
   pEnum->_pFileFrags = pFileFrags;
   
   /* Lock the data holder */
   pFileFrags->AddRef();
   
   /* Lose this method's reference on the enumerator object */
   pEnum->Release();

   /* Success / Failure */
   return ( hr );
}

/**********************************************************************

   CFileDefragmenter : ATL

 **********************************************************************/
HRESULT CFileDefragmenter::FinalConstruct( ) throw()
{
   /* Success */
   return ( S_OK );
}

void CFileDefragmenter::FinalRelease( ) throw()
{
   /* There are no outstanding references, so locking isn't required */
   if ( _pCallback )
   {
      reinterpret_cast<IUnknown*>(_pCallback)->Release();
   }
}

/**********************************************************************

   CFileDefragmenter : IFileDefragmenter

 **********************************************************************/

STDMETHODIMP CFileDefragmenter::DefragmentFile( BSTR bszPath, LONG iCookie, VARIANT_BOOL* pbResult ) throw()
{
   HRESULT   hr;
   DISPID    DispID;
   void*     pCallback;
   
   /* Validate in/out parameters */
   if ( !bszPath )
   {
      /* Failure */
      return ( E_POINTER );
   }

   if ( pbResult )
   {
      (*pbResult) = VARIANT_FALSE;
   }

   /* The main lock controls reference data, the state has its own lock */
   AcquireSpinLock(&_StateLock);
   {
      _eState = PGS_INACTIVE;
      _bAbort = FALSE;
   }
   ReleaseSpinLock(&_StateLock);

   /* Initialize locals */
   hr = E_FAIL;
   
   Lock();
   {
      DispID    = _DispID;
      pCallback = _pCallback;

      if ( pCallback )
      {
         reinterpret_cast<IUnknown*>(pCallback)->AddRef();
      }
   }
   Unlock();
   
   __try
   {
      hr = _DefragmentFileWorker(pCallback, 
                                 DispID, 
                                 bszPath, 
                                 iCookie);      
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
      hr = E_UNEXPECTED;
   }

   if ( SUCCEEDED(hr) && pbResult )
   {
      (*pbResult) = VARIANT_TRUE;
   }

   if ( pCallback )
   {
      reinterpret_cast<IUnknown*>(pCallback)->Release();
   }
   
   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CFileDefragmenter::Cancel( ) throw()
{
   AcquireSpinLock(&_StateLock);
   {
      _bAbort  = TRUE;
      _hResult = __HRESULT_FROM_WIN32(ERROR_CANCELLED);
   }
   ReleaseSpinLock(&_StateLock);

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileDefragmenter::put_OnProgressUpdate( IUnknown* pCallback ) throw()
{
   HRESULT                  hr;
   IDispatch*               pCallbackDisp;
   IFileDefragmenterEvents* pCallbackDefrag;
   DISPID                   DispID;
   LPOLESTR                 szName;

   /* Initialize locals */
   hr              = E_FAIL;
   pCallbackDisp   = NULL;
   pCallbackDefrag = NULL;
   DispID          = DISPID_UNKNOWN;
   szName          = L"onprogressupdate";
   
   __try
   {
      /* A NULL callback clears the current one */
      if ( !pCallback )
      {
         hr = S_OK;
         /* Success */
         __leave;
      }

      /* Try for the non-dispatch interface first, if that doesn't work revert to dispatch */
      hr = pCallback->QueryInterface(__uuidof(IFileDefragmenterEvents), 
                                     reinterpret_cast<void**>(&pCallbackDefrag));

      if ( FAILED(hr) )
      {
         hr = pCallback->QueryInterface(IID_IDispatch, 
                                        reinterpret_cast<void**>(&pCallbackDisp));

         if ( FAILED(hr) )
         {
            hr = E_INVALIDARG;
            /* Failure */
            __leave;
         }
         
         hr = pCallbackDisp->GetIDsOfNames(IID_NULL, 
                                           &szName, 
                                           1, 
                                           LOCALE_NEUTRAL, 
                                           &DispID);

         if ( FAILED(hr) )
         {
            /* Use the default DISPID for this method */
            DispID = DISPID_ONPROGRESSUPDATE;
         }

         /* Reset success for syncs that don't have an event named OnProgressUpdate */
         hr = S_OK;
      }
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
      hr = E_UNEXPECTED;
   }

   Lock();
   {
      /* First clear the current values... */
      if ( _pCallback )
      {
         reinterpret_cast<IUnknown*>(_pCallback)->Release();
         _pCallback = NULL;
      }
      
      _DispID = DISPID_UNKNOWN;

      if ( SUCCEEDED(hr) )
      {
         _DispID = DispID;

         if ( DISPID_UNKNOWN == DispID )
         {
            _pCallback = reinterpret_cast<PVOID*>(pCallbackDefrag);
         }
         else
         {
            _pCallback = reinterpret_cast<PVOID*>(pCallbackDisp);
         }         
      }
   }
   Unlock();

   if ( pCallbackDefrag )
   {
      pCallbackDefrag->Release();
   }

   if ( pCallbackDisp )
   {
      pCallbackDisp->Release();
   }

   /* Success / Failure */
   return ( hr );
}

STDMETHODIMP CFileDefragmenter::get_ProgressState( PROGRESSSTATE* peState ) throw()
{
   ATLASSERT(NULL != peState);

   if ( !peState )
   {
      /* Failure */
		return ( E_POINTER );
   }

   (*peState) = _eState;
   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileDefragmenter::get_ResultCode( LONG* plResult ) throw()
{
   ATLASSERT(NULL != plResult);

   if ( !plResult )
   {
      /* Failure */
      return ( E_POINTER );
   }

   (*plResult) = static_cast<LONG>(_hResult);

   /* Success */
   return ( S_OK );
}

STDMETHODIMP CFileDefragmenter::get_PercentComplete( LONG* piPercent ) throw()
{
   ATLASSERT(NULL != piPercent);

   if ( !piPercent )
   {
      /* Failure */
      return ( E_POINTER );
   }

   (*piPercent) = _iPercent;

   /* Success */
   return ( S_OK );
}

/**********************************************************************

   CFileDefragmenter : CFileDefragmenter

 **********************************************************************/

CFileDefragmenter::CFileDefragmenter( ) throw()
{
   _eState     = PGS_INACTIVE;
   _hResult    = E_PENDING;
   _iPercent   = 0;
   _bAbort     = FALSE;
   _pCallback  = NULL;
   _DispID     = DISPID_UNKNOWN;
   
   InitializeSpinLock(&_StateLock);
}

HRESULT CFileDefragmenter::_DefragmentFileWorker( PVOID pCallback, DISPID DispID, BSTR bszFile, LONG iCookie ) throw()
{
   ATLASSERT(NULL != bszFile);

   HRESULT     hr;
   DWORD       dwRet;
   HANDLE      hFragCtx;
   ULONG       iFragCount;
   DEFRAGCTX   DefragCtx;

   /* Initialize locals */
   hr       = E_FAIL;
   hFragCtx = NULL;

   __try
   {
      hr = _FireProgressUpdate(pCallback, 
                               DispID, 
                               bszFile, 
                               iCookie, 
                               PGS_INITIALIZING, 
                               0, 
                               E_PENDING);

      if ( FAILED(hr) )
      {
         /* Failure - An error or caller cancelled */
         __leave;
      }

      /* Open a context for the file */
      dwRet = FpCreateContext(bszFile,
                              0,
                              &hFragCtx);

      if ( NO_ERROR != dwRet )
      {
         hr = __HRESULT_FROM_WIN32(dwRet);
         /* Failure */
         __leave;
      }

      /* Determine if the file needs to be defragmented */
      dwRet = FpGetContextFragmentCount(hFragCtx,
                                        &iFragCount);

      if ( NO_ERROR != dwRet )
      {
         hr = __HRESULT_FROM_WIN32(dwRet);
         /* Failure */
         __leave;
      }
      
      /* Close the context */
      FpCloseContext(hFragCtx);
      hFragCtx = NULL;

      if ( iFragCount > 1 )
      {
         ZeroMemory(&DefragCtx,
                    sizeof(DEFRAGCTX));

         /* Defragment the file */
         DefragCtx.FileDefragmenter = this;
         DefragCtx.Callback         = pCallback;
         DefragCtx.DispID           = DispID;
         DefragCtx.FileName         = bszFile;
         DefragCtx.Cookie           = iCookie;

         dwRet = FpDefragmentFile(bszFile,
                                  0,
                                  &CFileDefragmenter::_DefragmentFileRoutine,
                                  &DefragCtx);

         if ( NO_ERROR != dwRet )
         {
            hr = __HRESULT_FROM_WIN32(dwRet);
         }
         else
         {
            hr = S_OK;
         }
      }
   
      /* The return value for this call is ignored */
      _FireProgressUpdate(pCallback, 
                          DispID, 
                          bszFile, 
                          iCookie, 
                          PGS_COMPLETED, 
                          100, 
                          hr);
   }
   /* This is a COM method, exceptions cannot propogate */
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
      hr = E_UNEXPECTED;
   }

   if ( hFragCtx )
   {
      FpCloseContext(hFragCtx);
   }

   /* Success / Failure */
   return ( hr );
}

DWORD FRAGAPI CFileDefragmenter::_DefragmentFileRoutine( ULONGLONG ClustersTotal, ULONGLONG ClustersMoved, PVOID Parameter ) throw()
{
   HRESULT        hr;
   PDEFRAGCTX     pDefragCtx;
   double         iPercent;
   FILETIME       SysTime;
   ULARGE_INTEGER TimeDiff;
   BOOL           bCallback;

   /* Initialize locals */
   hr         = S_OK;
   pDefragCtx = reinterpret_cast<PDEFRAGCTX>(Parameter);
   bCallback  = FALSE;

   /* This can be called with ClustersTotal & ClustersMoved both being 0, when the file fits inside the MFT
    * record and FpDefragmentFile() just posts a single completion callback, so make sure we're not about
    * to divide by 0 */
   if ( 0 == ClustersTotal )
   {
      ClustersMoved = 1;
      ClustersTotal = 1;
   }

   /* Calculate the percentage completed */
   iPercent = 100.0f * (static_cast<double>(ClustersMoved) / static_cast<double>(ClustersTotal));

   /* Get the current system time.. */
   GetSystemTimeAsFileTime(&SysTime);
   TimeDiff.LowPart  = SysTime.dwLowDateTime;
   TimeDiff.HighPart = SysTime.dwHighDateTime;

   /* If the new percent is greater than the last one, we'll post a callback.. */
   if ( static_cast<LONG>(iPercent) > pDefragCtx->Percent )
   {
      bCallback = TRUE;
   }
   else if ( TimeDiff.QuadPart >= pDefragCtx->CallTime )
   {
      /* If the calltime has expired, we'll post a callback */
      bCallback = TRUE;
   }

   /* Update the tracking info, if we're doing a callback for any reason */
   if ( bCallback )
   {      
      pDefragCtx->Percent  = static_cast<LONG>(iPercent);
      /* Set the next timeout.. */
      pDefragCtx->CallTime = TimeDiff.QuadPart + CALLBACK_DELAY;
   }
   
   if ( bCallback )
   {
      hr = pDefragCtx->FileDefragmenter->_FireProgressUpdate(pDefragCtx->Callback,
                                                             pDefragCtx->DispID,
                                                             pDefragCtx->FileName,
                                                             pDefragCtx->Cookie,
                                                             PGS_DEFRAGMENTING,
                                                             static_cast<LONG>(iPercent),
                                                             S_OK);
   }

   /* Success / Failure */
   return ( FAILED(hr) ? static_cast<DWORD>(hr) : NO_ERROR );
}

HRESULT CFileDefragmenter::_FireProgressUpdate( PVOID pCallback, DISPID DispID, BSTR bszFile, LONG iCookie, PROGRESSSTATE eState, LONG iPercent, HRESULT hResultCode ) throw()
{
   ATLASSERT(NULL != bszFile);

   HRESULT                    hr;
   IFileDefragmenterEvents*   pDefrag;
   IDispatch*                 pDispatch;
   VARIANT                    rgParam[4];
   VARIANT                    agResult;
   DISPPARAMS                 dParams;
   VARIANT_BOOL               bCancel;

   /* Initialize locals */
   hr = S_OK;

   AcquireSpinLock(&_StateLock);
   {
      if ( _bAbort )
      {
         hResultCode = E_ABORT;
      }

      _eState   = eState;
      _iPercent = iPercent;
      _hResult  = hResultCode;
   }
   ReleaseSpinLock(&_StateLock);

   if ( E_ABORT == hResultCode )
   {
      /* Another thread has cancelled */
      return ( E_ABORT );
   }

   if ( !pCallback )
   {
      /* Success - Nothing to call */
      return ( S_OK );
   }

   __try
   {
      /* When the DispID is DISPID_UNKNOWN, we assume this is a vtable sink */
      if ( DISPID_UNKNOWN == DispID )
      {
         /* Default is to cancel defrag */
         bCancel = VARIANT_FALSE;

         /* pCallback is expected to be a IFileDefragmenterEvents* when DispID is DISPID_UNKNOWN, set in put_OnProgressUpdate() */
         pDefrag = reinterpret_cast<IFileDefragmenterEvents*>(pCallback);

         if ( !_bAbort )
         {
            hr = pDefrag->OnProgressUpdate(bszFile, 
                                           iCookie, 
                                           eState,
                                           iPercent,
                                           &bCancel);

            if ( SUCCEEDED(hr) && (VARIANT_FALSE != bCancel) )
            {
               /* Caller has cancelled */
               hr = E_ABORT;
            }
         }
      }
      else 
      {
         /* DISPPARAMS are in reverse order, so this is sPath, iCookie, eState, iPercent. bCancel is a retval param so it
          * is returned via agResult */
         rgParam[0].vt      = VT_I4;
         rgParam[0].lVal    = iPercent;
         rgParam[1].vt      = VT_I4;
         rgParam[1].lVal    = static_cast<LONG>(eState);
         rgParam[2].vt      = VT_I4;
         rgParam[2].lVal    = iCookie;
         rgParam[3].vt      = VT_BSTR;
         rgParam[3].bstrVal = bszFile;

         dParams.rgvarg            = rgParam;
         dParams.rgdispidNamedArgs = NULL;
         dParams.cArgs             = _countof(rgParam);
         dParams.cNamedArgs        = 0;
         
         VariantInit(&agResult);
         
         if ( !_bAbort )
         {
            /* pCallback is expected to be an IDispatch*, set in put_OnProgressUpdate() */
            pDispatch = reinterpret_cast<IDispatch*>(pCallback);

            hr = pDispatch->Invoke(DispID,
                                   IID_NULL,
                                   0,
                                   DISPATCH_METHOD,
                                   &dParams,
                                   &agResult,
                                   NULL,
                                   NULL);

            if ( SUCCEEDED(hr) )
            {
               /* The result needs to be a bool value */
               if ( (VT_BOOL == agResult.vt) && (VARIANT_FALSE != agResult.boolVal) )
               {
                  /* Caller has cancelled */
                  hr = E_ABORT;
               }
            }
         }
      }
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
      hr = E_UNEXPECTED;
   }

   if ( E_ABORT == hr )
   {
      AcquireSpinLock(&_StateLock);
      {
         _bAbort = TRUE;
      }
      ReleaseSpinLock(&_StateLock);
   }

   /* This could be something other than VT_BOOL, so always clear it */
   VariantClear(&agResult);

   /* Success / Failure */
   return ( hr );
}
