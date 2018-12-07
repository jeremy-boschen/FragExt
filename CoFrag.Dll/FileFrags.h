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

/* FileFrags.h
 *    CFileFragment, CFileFragments, CEnumFileFragments, CFileDefragmenter,
 *    and CFileFragmenter implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 10/31/2004 - Created
 */

#pragma once

#include <AtlEx.h>
using namespace ATLEx;
#include <SpinLock.h>

#include <FragEng.h>

/**********************************************************************

    CFileFragment

 **********************************************************************/

/**
 * CFileFragment
 */
class ATL_NO_VTABLE CFileFragment : public  CComObjectRootEx<CComMultiThreadModelNoCS>,
                                    public  IDispatchImpl<IFileFragment, &IID_IFileFragment, &LIBID_FragEng, 1, 0>,
                                    public  CoFreeThreadedMarshalerImpl<CFileFragment>
{
   /* ATL */
public:
   DECLARE_NOT_AGGREGATABLE(CFileFragment)

   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(CFileFragment)
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IFileFragment)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, reinterpret_cast<_ATL_CREATORARGFUNC*>(CoFTMImpl::QueryMarshalInterface))
   END_COM_MAP()

   /* IFileFragment */
public:
   STDMETHOD(get_Sequence)( long* piSequence ) throw();
   STDMETHOD(get_LogicalCluster)( double* piLogicalCluster ) throw();
   STDMETHOD(get_ExtentCount)( long* piExtentCount ) throw();
   STDMETHOD(get_ClusterCount)( double* piClusterCount ) throw();

   /* CFileFragment */
public:
   static HRESULT CreateFromFragmentInfo( const FRAGMENT_INFORMATION* pFragInfo, REFIID riid, void** ppObject ) throw();

protected:
   CFileFragment( ) throw(); 

private:
   typedef CComObject<CFileFragment> CoFileFragment;

   FRAGMENT_INFORMATION _FragInfo;
};

/**********************************************************************

    CFileFragments

 **********************************************************************/

/**
 * CFileFragments
 */
class ATL_NO_VTABLE CFileFragments : public CComObjectRootEx<CComMultiThreadModel>,
                                     public CComCoClass<CFileFragments, &CLSID_FileFragments>,
                                     public IDispatchImpl<IFileFragments, &IID_IFileFragments, &LIBID_FragEng, 1, 0>,
                                     public CoFreeThreadedMarshalerImpl<CFileFragments>
{
   /* ATL */
public:
   DECLARE_NO_REGISTRY()

   DECLARE_NOT_AGGREGATABLE(CFileFragments)

   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(CFileFragments)      
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IFileFragments)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, reinterpret_cast<_ATL_CREATORARGFUNC*>(CoFTMImpl::QueryMarshalInterface))
   END_COM_MAP()

   void FinalRelease() throw();

   /* IFileFragments */
public:
   STDMETHOD(get__NewEnum)( IUnknown** ppEnum ) throw();
   STDMETHOD(get_Item)( long iIndex, IDispatch** ppFragment ) throw();
   STDMETHOD(get_Count)( long* plResult ) throw();

   STDMETHOD(OpenFile)( BSTR bszPath, VARIANT_BOOL bIncludeCompressed, VARIANT_BOOL* pbOpened ) throw();
   STDMETHOD(get_FragmentCount)( long* plResult ) throw();
   STDMETHOD(get_ExtentCount)( long* plResult ) throw();
   STDMETHOD(get_ClusterCount)( double* pdblResult ) throw();
   STDMETHOD(get_FileSize)( double* pdblResult ) throw();
   STDMETHOD(get_FileSizeOnDisk)( double* pdblResult ) throw();
   STDMETHOD(get_ClusterSize)( long* plResult ) throw();

public:
   CFileFragments( ) throw();
   bool _IsExpired( DWORD dwTick ) throw();
   
private:
   DWORD  _dwTick;
   HANDLE _hFragCtx;
   
   /*!!!!
      THIS MUST BE CALLED WITHIN A Lock()/Unlock() BLOCK
     !!!!*/
   void _ResetData( ) throw();
};

/*lint -save -e19 Useless declaration */
OBJECT_ENTRY_AUTO(__uuidof(FileFragments), CFileFragments)
/*lint -restore */

/**********************************************************************

    CEnumFileFragments

 **********************************************************************/

class ATL_NO_VTABLE CEnumFileFragments : public CComObjectRootEx<CComMultiThreadModel>,
                                         public IEnumVARIANT,
                                         public CoFreeThreadedMarshalerImpl<CEnumFileFragments>
{
   /* ATL */
public:
   DECLARE_NOT_AGGREGATABLE(CEnumFileFragments)
   
   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(CEnumFileFragments)
      COM_INTERFACE_ENTRY(IEnumVARIANT)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, reinterpret_cast<_ATL_CREATORARGFUNC*>(CoFTMImpl::QueryMarshalInterface))
   END_COM_MAP()

   void FinalRelease( ) throw();

   /* IEnumVARIANT */
public:
   STDMETHOD(Next)( ULONG celt, VARIANT* rgelt, ULONG* peltFetched ) throw();
   STDMETHOD(Skip)( ULONG celt ) throw();
   STDMETHOD(Reset)( ) throw();
   STDMETHOD(Clone)( IEnumVARIANT** ppEnum ) throw();

   /* CEnumFileFragments */
public:
   static HRESULT CreateInstance( CFileFragments* pFileFrags, DWORD dwTick, ULONG cIterator, REFIID riid, void** ppObject ) throw();

protected:   
   CEnumFileFragments( ) throw();

private:
   ULONG           _dwTick;
   ULONG           _cIter;
   ULONG           _cElem;
   CFileFragments* _pFileFrags;

   typedef CComObject<CEnumFileFragments> CoEnumFileFragments;
};

/**********************************************************************

    CFileDefragmenter

 **********************************************************************/

/**
 * CFileDefragmenter
 */
class ATL_NO_VTABLE CFileDefragmenter : public CComObjectRootEx<CComMultiThreadModel>,
                                        public CComCoClass<CFileDefragmenter, &CLSID_FileDefragmenter>,
                                        public IDispatchImpl<IFileDefragmenter, &IID_IFileDefragmenter, &LIBID_FragEng, 1, 0>,
                                        public CoFreeThreadedMarshalerImpl<CFileDefragmenter>
{
   /* ATL */
public:
   DECLARE_NO_REGISTRY()

   DECLARE_NOT_AGGREGATABLE(CFileDefragmenter)

   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(CFileDefragmenter)
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IFileDefragmenter)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, reinterpret_cast<_ATL_CREATORARGFUNC*>(CoFTMImpl::QueryMarshalInterface))
   END_COM_MAP()

   HRESULT FinalConstruct( ) throw();
   void FinalRelease( ) throw();

   /* IFileDefragmenter */
public:
   STDMETHOD(DefragmentFile)( BSTR bszPath, long iCookie, VARIANT_BOOL* pbResult ) throw();
   STDMETHOD(Cancel)( ) throw();
   STDMETHOD(put_OnProgressUpdate)( IUnknown* pEventHandler ) throw();
   STDMETHOD(get_ProgressState)( PROGRESSSTATE* peState ) throw();
   STDMETHOD(get_ResultCode)( long* plResult ) throw();
   STDMETHOD(get_PercentComplete)( long* piPercent ) throw();

   /* CFileDefragmenter */
public:
   CFileDefragmenter( ) throw();

private:   
   enum
   {
      eTraceInfo = 0,

      /* Time used for the callback delay, in nanoseconds */
      ONE_MICROSECOND  = 10,
      ONE_MILISECOND   = ONE_MICROSECOND * 1000,
      ONE_SECOND       = ONE_MILISECOND * 1000,
      CALLBACK_DELAY   = ONE_SECOND * 1
   };

   /* Operation state & status */
   PROGRESSSTATE  _eState;
   HRESULT        _hResult;
   LONG           _iPercent;
   BOOL           _bAbort;

   /* This is used to lock access to the state info above. The state data needs to be protected
    * separately because any thread is allowed to call in and cancel a defrag */
   SPINLOCK       _StateLock;

   /* Eventing. These are protected by the primary lock */
   PVOID          _pCallback;
   DISPID         _DispID;
   
   /* Used to track defrag state for _DefragmentFileRoutine() */
   typedef struct _DEFRAGCTX
   {
      CFileDefragmenter*   FileDefragmenter;
      PVOID                Callback;
      DISPID               DispID;
      BSTR                 FileName;
      LONG                 Cookie;

      HRESULT              Result;
      LONG                 Percent;
      ULONGLONG            CallTime;
   }DEFRAGCTX, *PDEFRAGCTX;

   HRESULT _DefragmentFileWorker( PVOID pCallback, DISPID DispID, BSTR bszFile, LONG iCookie ) throw();
   static DWORD FRAGAPI _DefragmentFileRoutine( ULONGLONG ClustersTotal, ULONGLONG ClustersMoved, PVOID Parameter ) throw();

   HRESULT _FireProgressUpdate( PVOID pCallback, DISPID DispID, BSTR bszFile, LONG iCookie, PROGRESSSTATE eState, LONG iPercent, HRESULT hResultCode ) throw();
 };

/*lint -save -e19 Useless declaration */
OBJECT_ENTRY_AUTO(__uuidof(FileDefragmenter), CFileDefragmenter)
/*lint -restore */
