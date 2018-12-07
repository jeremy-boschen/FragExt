

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* Compiler settings for cofrag.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 7.00.0555 
    protocol : all , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 500
#endif

/* verify that the <rpcsal.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCSAL_H_VERSION__
#define __REQUIRED_RPCSAL_H_VERSION__ 100
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __cofrag_h__
#define __cofrag_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFileFragment_FWD_DEFINED__
#define __IFileFragment_FWD_DEFINED__
typedef interface IFileFragment IFileFragment;
#endif 	/* __IFileFragment_FWD_DEFINED__ */


#ifndef __FileFragment_FWD_DEFINED__
#define __FileFragment_FWD_DEFINED__

#ifdef __cplusplus
typedef class FileFragment FileFragment;
#else
typedef struct FileFragment FileFragment;
#endif /* __cplusplus */

#endif 	/* __FileFragment_FWD_DEFINED__ */


#ifndef __IFileFragments_FWD_DEFINED__
#define __IFileFragments_FWD_DEFINED__
typedef interface IFileFragments IFileFragments;
#endif 	/* __IFileFragments_FWD_DEFINED__ */


#ifndef __FileFragments_FWD_DEFINED__
#define __FileFragments_FWD_DEFINED__

#ifdef __cplusplus
typedef class FileFragments FileFragments;
#else
typedef struct FileFragments FileFragments;
#endif /* __cplusplus */

#endif 	/* __FileFragments_FWD_DEFINED__ */


#ifndef __IFileDefragmenter_FWD_DEFINED__
#define __IFileDefragmenter_FWD_DEFINED__
typedef interface IFileDefragmenter IFileDefragmenter;
#endif 	/* __IFileDefragmenter_FWD_DEFINED__ */


#ifndef __DFileDefragmenterEvents_FWD_DEFINED__
#define __DFileDefragmenterEvents_FWD_DEFINED__
typedef interface DFileDefragmenterEvents DFileDefragmenterEvents;
#endif 	/* __DFileDefragmenterEvents_FWD_DEFINED__ */


#ifndef __IFileDefragmenterEvents_FWD_DEFINED__
#define __IFileDefragmenterEvents_FWD_DEFINED__
typedef interface IFileDefragmenterEvents IFileDefragmenterEvents;
#endif 	/* __IFileDefragmenterEvents_FWD_DEFINED__ */


#ifndef __FileDefragmenter_FWD_DEFINED__
#define __FileDefragmenter_FWD_DEFINED__

#ifdef __cplusplus
typedef class FileDefragmenter FileDefragmenter;
#else
typedef struct FileDefragmenter FileDefragmenter;
#endif /* __cplusplus */

#endif 	/* __FileDefragmenter_FWD_DEFINED__ */


#ifndef __IFileStream_FWD_DEFINED__
#define __IFileStream_FWD_DEFINED__
typedef interface IFileStream IFileStream;
#endif 	/* __IFileStream_FWD_DEFINED__ */


#ifndef __FileStream_FWD_DEFINED__
#define __FileStream_FWD_DEFINED__

#ifdef __cplusplus
typedef class FileStream FileStream;
#else
typedef struct FileStream FileStream;
#endif /* __cplusplus */

#endif 	/* __FileStream_FWD_DEFINED__ */


#ifndef __IFileStreams_FWD_DEFINED__
#define __IFileStreams_FWD_DEFINED__
typedef interface IFileStreams IFileStreams;
#endif 	/* __IFileStreams_FWD_DEFINED__ */


#ifndef __FileStreams_FWD_DEFINED__
#define __FileStreams_FWD_DEFINED__

#ifdef __cplusplus
typedef class FileStreams FileStreams;
#else
typedef struct FileStreams FileStreams;
#endif /* __cplusplus */

#endif 	/* __FileStreams_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_cofrag_0000_0000 */
/* [local] */ 

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



extern RPC_IF_HANDLE __MIDL_itf_cofrag_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_cofrag_0000_0000_v0_0_s_ifspec;


#ifndef __FragEng_LIBRARY_DEFINED__
#define __FragEng_LIBRARY_DEFINED__

/* library FragEng */
/* [version][lcid][helpstring][uuid] */ 

#define DISPID_FILEFRAG_SEQUENCE       1
#define DISPID_FILEFRAG_EXTENTCOUNT    3
#define DISPID_FILEFRAG_CLUSTERCOUNT   4
#define DISPID_FILEFRAG_LOGICALCLUSTER 2
#define DISPID_FILEFRAGS_COUNT             1
#define DISPID_FILEFRAGS_OPENFILE          2
#define DISPID_FILEFRAGS_FRAGMENTCOUNT     3
#define DISPID_FILEFRAGS_EXTENTCOUNT       4
#define DISPID_FILEFRAGS_CLUSTERCOUNT      5
#define DISPID_FILEFRAGS_FILESIZE          6
#define DISPID_FILEFRAGS_FILESIZEONDISK    7
#define DISPID_FILEFRAGS_CLUSTERSIZE       8
typedef /* [uuid] */  DECLSPEC_UUID("14EEECA3-36D6-46A8-974F-59BF1B7C25AE") 
enum _PROGRESSSTATE
    {	PGS_INACTIVE	= 0,
	PGS_INITIALIZING	= 1,
	PGS_DEFRAGMENTING	= 2,
	PGS_COMPLETED	= 3
    } 	PROGRESSSTATE;

#define DISPID_FILEDEFRAG_DEFRAGMENTFILE    1
#define DISPID_FILEDEFRAG_CANCEL            2
#define DISPID_FILEDEFRAG_ONPROGRESSUPDATE  3
#define DISPID_FILEDEFRAG_PROGRESSSTATE     4
#define DISPID_FILEDEFRAG_RESULTCODE        5
#define DISPID_FILEDEFRAG_PERCENTCOMPLETE   6
#define DISPID_ONPROGRESSUPDATE 0
#define IsVirtualCluster( iLCN ) (-1 == (iLCN))
#define DISPID_FILESTREAM_NAME 0
#define DISPID_FILESTREAM_SIZE 1
#define DISPID_FILESTREAMS_COUNT    1
#define DISPID_FILESTREAMS_OPENFILE 2

EXTERN_C const IID LIBID_FragEng;

#ifndef __IFileFragment_INTERFACE_DEFINED__
#define __IFileFragment_INTERFACE_DEFINED__

/* interface IFileFragment */
/* [helpstring][unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IFileFragment;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1D046FDF-FCD7-4AB8-9C21-5FB01B463263")
    IFileFragment : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Sequence( 
            /* [retval][out] */ __RPC__out LONG *piResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ExtentCount( 
            /* [retval][out] */ __RPC__out LONG *piResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ClusterCount( 
            /* [retval][out] */ __RPC__out double *pdbiResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LogicalCluster( 
            /* [retval][out] */ __RPC__out double *pdbiResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileFragmentVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFileFragment * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFileFragment * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFileFragment * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            __RPC__in IFileFragment * This,
            /* [out] */ __RPC__out UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            __RPC__in IFileFragment * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            __RPC__in IFileFragment * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
            /* [range][in] */ __RPC__in_range(0,16384) UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFileFragment * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Sequence )( 
            __RPC__in IFileFragment * This,
            /* [retval][out] */ __RPC__out LONG *piResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExtentCount )( 
            __RPC__in IFileFragment * This,
            /* [retval][out] */ __RPC__out LONG *piResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterCount )( 
            __RPC__in IFileFragment * This,
            /* [retval][out] */ __RPC__out double *pdbiResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_LogicalCluster )( 
            __RPC__in IFileFragment * This,
            /* [retval][out] */ __RPC__out double *pdbiResult);
        
        END_INTERFACE
    } IFileFragmentVtbl;

    interface IFileFragment
    {
        CONST_VTBL struct IFileFragmentVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileFragment_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFileFragment_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFileFragment_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFileFragment_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IFileFragment_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IFileFragment_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IFileFragment_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IFileFragment_get_Sequence(This,piResult)	\
    ( (This)->lpVtbl -> get_Sequence(This,piResult) ) 

#define IFileFragment_get_ExtentCount(This,piResult)	\
    ( (This)->lpVtbl -> get_ExtentCount(This,piResult) ) 

#define IFileFragment_get_ClusterCount(This,pdbiResult)	\
    ( (This)->lpVtbl -> get_ClusterCount(This,pdbiResult) ) 

#define IFileFragment_get_LogicalCluster(This,pdbiResult)	\
    ( (This)->lpVtbl -> get_LogicalCluster(This,pdbiResult) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFileFragment_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FileFragment;

#ifdef __cplusplus

class DECLSPEC_UUID("D5DE6F41-07AE-4DA2-A816-C456DC642185")
FileFragment;
#endif

#ifndef __IFileFragments_INTERFACE_DEFINED__
#define __IFileFragments_INTERFACE_DEFINED__

/* interface IFileFragments */
/* [helpstring][unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IFileFragments;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0B66D1B3-0769-4CAA-8581-1B6AF1E1FB1B")
    IFileFragments : public IDispatch
    {
    public:
        virtual /* [hidden][restricted][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ __RPC__deref_out_opt IUnknown **ppEnum) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG iIndex,
            /* [retval][out] */ __RPC__deref_out_opt IDispatch **ppFragment) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ __RPC__out LONG *plCount) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OpenFile( 
            /* [in] */ __RPC__in BSTR bszFile,
            /* [in] */ VARIANT_BOOL bIncludeCompressed,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbOpened) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FragmentCount( 
            /* [retval][out] */ __RPC__out LONG *piResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ExtentCount( 
            /* [retval][out] */ __RPC__out LONG *piResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ClusterCount( 
            /* [retval][out] */ __RPC__out double *pdbiResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FileSize( 
            /* [retval][out] */ __RPC__out double *pdbiResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FileSizeOnDisk( 
            /* [retval][out] */ __RPC__out double *pdbiResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ClusterSize( 
            /* [retval][out] */ __RPC__out LONG *piResult) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileFragmentsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFileFragments * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFileFragments * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFileFragments * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            __RPC__in IFileFragments * This,
            /* [out] */ __RPC__out UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            __RPC__in IFileFragments * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            __RPC__in IFileFragments * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
            /* [range][in] */ __RPC__in_range(0,16384) UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFileFragments * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [hidden][restricted][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__deref_out_opt IUnknown **ppEnum);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            __RPC__in IFileFragments * This,
            /* [in] */ LONG iIndex,
            /* [retval][out] */ __RPC__deref_out_opt IDispatch **ppFragment);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__out LONG *plCount);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OpenFile )( 
            __RPC__in IFileFragments * This,
            /* [in] */ __RPC__in BSTR bszFile,
            /* [in] */ VARIANT_BOOL bIncludeCompressed,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbOpened);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FragmentCount )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__out LONG *piResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ExtentCount )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__out LONG *piResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterCount )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__out double *pdbiResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FileSize )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__out double *pdbiResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_FileSizeOnDisk )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__out double *pdbiResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ClusterSize )( 
            __RPC__in IFileFragments * This,
            /* [retval][out] */ __RPC__out LONG *piResult);
        
        END_INTERFACE
    } IFileFragmentsVtbl;

    interface IFileFragments
    {
        CONST_VTBL struct IFileFragmentsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileFragments_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFileFragments_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFileFragments_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFileFragments_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IFileFragments_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IFileFragments_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IFileFragments_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IFileFragments_get__NewEnum(This,ppEnum)	\
    ( (This)->lpVtbl -> get__NewEnum(This,ppEnum) ) 

#define IFileFragments_get_Item(This,iIndex,ppFragment)	\
    ( (This)->lpVtbl -> get_Item(This,iIndex,ppFragment) ) 

#define IFileFragments_get_Count(This,plCount)	\
    ( (This)->lpVtbl -> get_Count(This,plCount) ) 

#define IFileFragments_OpenFile(This,bszFile,bIncludeCompressed,pbOpened)	\
    ( (This)->lpVtbl -> OpenFile(This,bszFile,bIncludeCompressed,pbOpened) ) 

#define IFileFragments_get_FragmentCount(This,piResult)	\
    ( (This)->lpVtbl -> get_FragmentCount(This,piResult) ) 

#define IFileFragments_get_ExtentCount(This,piResult)	\
    ( (This)->lpVtbl -> get_ExtentCount(This,piResult) ) 

#define IFileFragments_get_ClusterCount(This,pdbiResult)	\
    ( (This)->lpVtbl -> get_ClusterCount(This,pdbiResult) ) 

#define IFileFragments_get_FileSize(This,pdbiResult)	\
    ( (This)->lpVtbl -> get_FileSize(This,pdbiResult) ) 

#define IFileFragments_get_FileSizeOnDisk(This,pdbiResult)	\
    ( (This)->lpVtbl -> get_FileSizeOnDisk(This,pdbiResult) ) 

#define IFileFragments_get_ClusterSize(This,piResult)	\
    ( (This)->lpVtbl -> get_ClusterSize(This,piResult) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFileFragments_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FileFragments;

#ifdef __cplusplus

class DECLSPEC_UUID("2970890C-BA3B-45F6-A7EC-85606DE2ADB9")
FileFragments;
#endif

#ifndef __IFileDefragmenter_INTERFACE_DEFINED__
#define __IFileDefragmenter_INTERFACE_DEFINED__

/* interface IFileDefragmenter */
/* [helpstring][unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IFileDefragmenter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A9E4A961-5640-4EE9-98B2-AF2C26CF1C25")
    IFileDefragmenter : public IDispatch
    {
    public:
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE DefragmentFile( 
            /* [in] */ __RPC__in BSTR bszFile,
            /* [in] */ LONG iCookie,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbResult) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE Cancel( void) = 0;
        
        virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_OnProgressUpdate( 
            /* [in] */ __RPC__in_opt IUnknown *pCallback) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ProgressState( 
            /* [retval][out] */ __RPC__out PROGRESSSTATE *peState) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ResultCode( 
            /* [retval][out] */ __RPC__out LONG *piResult) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_PercentComplete( 
            /* [retval][out] */ __RPC__out LONG *piPercent) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileDefragmenterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFileDefragmenter * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFileDefragmenter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFileDefragmenter * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            __RPC__in IFileDefragmenter * This,
            /* [out] */ __RPC__out UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            __RPC__in IFileDefragmenter * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            __RPC__in IFileDefragmenter * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
            /* [range][in] */ __RPC__in_range(0,16384) UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFileDefragmenter * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *DefragmentFile )( 
            __RPC__in IFileDefragmenter * This,
            /* [in] */ __RPC__in BSTR bszFile,
            /* [in] */ LONG iCookie,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbResult);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *Cancel )( 
            __RPC__in IFileDefragmenter * This);
        
        /* [propput][id] */ HRESULT ( STDMETHODCALLTYPE *put_OnProgressUpdate )( 
            __RPC__in IFileDefragmenter * This,
            /* [in] */ __RPC__in_opt IUnknown *pCallback);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ProgressState )( 
            __RPC__in IFileDefragmenter * This,
            /* [retval][out] */ __RPC__out PROGRESSSTATE *peState);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_ResultCode )( 
            __RPC__in IFileDefragmenter * This,
            /* [retval][out] */ __RPC__out LONG *piResult);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_PercentComplete )( 
            __RPC__in IFileDefragmenter * This,
            /* [retval][out] */ __RPC__out LONG *piPercent);
        
        END_INTERFACE
    } IFileDefragmenterVtbl;

    interface IFileDefragmenter
    {
        CONST_VTBL struct IFileDefragmenterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileDefragmenter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFileDefragmenter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFileDefragmenter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFileDefragmenter_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IFileDefragmenter_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IFileDefragmenter_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IFileDefragmenter_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IFileDefragmenter_DefragmentFile(This,bszFile,iCookie,pbResult)	\
    ( (This)->lpVtbl -> DefragmentFile(This,bszFile,iCookie,pbResult) ) 

#define IFileDefragmenter_Cancel(This)	\
    ( (This)->lpVtbl -> Cancel(This) ) 

#define IFileDefragmenter_put_OnProgressUpdate(This,pCallback)	\
    ( (This)->lpVtbl -> put_OnProgressUpdate(This,pCallback) ) 

#define IFileDefragmenter_get_ProgressState(This,peState)	\
    ( (This)->lpVtbl -> get_ProgressState(This,peState) ) 

#define IFileDefragmenter_get_ResultCode(This,piResult)	\
    ( (This)->lpVtbl -> get_ResultCode(This,piResult) ) 

#define IFileDefragmenter_get_PercentComplete(This,piPercent)	\
    ( (This)->lpVtbl -> get_PercentComplete(This,piPercent) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFileDefragmenter_INTERFACE_DEFINED__ */


#ifndef __DFileDefragmenterEvents_DISPINTERFACE_DEFINED__
#define __DFileDefragmenterEvents_DISPINTERFACE_DEFINED__

/* dispinterface DFileDefragmenterEvents */
/* [helpstring][uuid][hidden] */ 


EXTERN_C const IID DIID_DFileDefragmenterEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("A97059DB-1B04-425B-80B6-9238A1338463")
    DFileDefragmenterEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct DFileDefragmenterEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in DFileDefragmenterEvents * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in DFileDefragmenterEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in DFileDefragmenterEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            __RPC__in DFileDefragmenterEvents * This,
            /* [out] */ __RPC__out UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            __RPC__in DFileDefragmenterEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            __RPC__in DFileDefragmenterEvents * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
            /* [range][in] */ __RPC__in_range(0,16384) UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            DFileDefragmenterEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } DFileDefragmenterEventsVtbl;

    interface DFileDefragmenterEvents
    {
        CONST_VTBL struct DFileDefragmenterEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define DFileDefragmenterEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define DFileDefragmenterEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define DFileDefragmenterEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define DFileDefragmenterEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define DFileDefragmenterEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define DFileDefragmenterEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define DFileDefragmenterEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* __DFileDefragmenterEvents_DISPINTERFACE_DEFINED__ */


#ifndef __IFileDefragmenterEvents_INTERFACE_DEFINED__
#define __IFileDefragmenterEvents_INTERFACE_DEFINED__

/* interface IFileDefragmenterEvents */
/* [helpstring][unique][oleautomation][uuid][hidden][object] */ 


EXTERN_C const IID IID_IFileDefragmenterEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("113B0A4A-AEE5-4899-982D-299774CF0D6C")
    IFileDefragmenterEvents : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnProgressUpdate( 
            /* [in] */ __RPC__in BSTR bszFile,
            /* [in] */ LONG iCookie,
            /* [in] */ PROGRESSSTATE eState,
            /* [in] */ LONG iPercent,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbCancel) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileDefragmenterEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFileDefragmenterEvents * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFileDefragmenterEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFileDefragmenterEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnProgressUpdate )( 
            __RPC__in IFileDefragmenterEvents * This,
            /* [in] */ __RPC__in BSTR bszFile,
            /* [in] */ LONG iCookie,
            /* [in] */ PROGRESSSTATE eState,
            /* [in] */ LONG iPercent,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbCancel);
        
        END_INTERFACE
    } IFileDefragmenterEventsVtbl;

    interface IFileDefragmenterEvents
    {
        CONST_VTBL struct IFileDefragmenterEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileDefragmenterEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFileDefragmenterEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFileDefragmenterEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFileDefragmenterEvents_OnProgressUpdate(This,bszFile,iCookie,eState,iPercent,pbCancel)	\
    ( (This)->lpVtbl -> OnProgressUpdate(This,bszFile,iCookie,eState,iPercent,pbCancel) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFileDefragmenterEvents_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FileDefragmenter;

#ifdef __cplusplus

class DECLSPEC_UUID("D06EDAF1-7E7D-41D7-8D1C-CB31BFD20B97")
FileDefragmenter;
#endif

#ifndef __IFileStream_INTERFACE_DEFINED__
#define __IFileStream_INTERFACE_DEFINED__

/* interface IFileStream */
/* [helpstring][unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IFileStream;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3E096844-2F07-41F5-A226-618C26784FA9")
    IFileStream : public IDispatch
    {
    public:
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Name( 
            /* [retval][out] */ __RPC__deref_out_opt BSTR *pbszName) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Size( 
            /* [retval][out] */ __RPC__out double *pdblSize) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileStreamVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFileStream * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFileStream * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFileStream * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            __RPC__in IFileStream * This,
            /* [out] */ __RPC__out UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            __RPC__in IFileStream * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            __RPC__in IFileStream * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
            /* [range][in] */ __RPC__in_range(0,16384) UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFileStream * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Name )( 
            __RPC__in IFileStream * This,
            /* [retval][out] */ __RPC__deref_out_opt BSTR *pbszName);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Size )( 
            __RPC__in IFileStream * This,
            /* [retval][out] */ __RPC__out double *pdblSize);
        
        END_INTERFACE
    } IFileStreamVtbl;

    interface IFileStream
    {
        CONST_VTBL struct IFileStreamVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileStream_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFileStream_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFileStream_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFileStream_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IFileStream_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IFileStream_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IFileStream_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IFileStream_get_Name(This,pbszName)	\
    ( (This)->lpVtbl -> get_Name(This,pbszName) ) 

#define IFileStream_get_Size(This,pdblSize)	\
    ( (This)->lpVtbl -> get_Size(This,pdblSize) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFileStream_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FileStream;

#ifdef __cplusplus

class DECLSPEC_UUID("74CCF722-3BA8-4026-8DBE-5282F057E292")
FileStream;
#endif

#ifndef __IFileStreams_INTERFACE_DEFINED__
#define __IFileStreams_INTERFACE_DEFINED__

/* interface IFileStreams */
/* [helpstring][unique][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IFileStreams;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5DA0A7DE-77D0-4E9B-A528-1081B655AB02")
    IFileStreams : public IDispatch
    {
    public:
        virtual /* [hidden][restricted][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum( 
            /* [retval][out] */ __RPC__deref_out_opt IUnknown **ppEnum) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Item( 
            /* [in] */ LONG iIndex,
            /* [retval][out] */ __RPC__deref_out_opt IDispatch **ppStream) = 0;
        
        virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count( 
            /* [retval][out] */ __RPC__out LONG *plCount) = 0;
        
        virtual /* [id] */ HRESULT STDMETHODCALLTYPE OpenFile( 
            /* [in] */ __RPC__in BSTR bszFile,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbOpened) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFileStreamsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFileStreams * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFileStreams * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFileStreams * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            __RPC__in IFileStreams * This,
            /* [out] */ __RPC__out UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            __RPC__in IFileStreams * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            __RPC__in IFileStreams * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
            /* [range][in] */ __RPC__in_range(0,16384) UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IFileStreams * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [hidden][restricted][propget][id] */ HRESULT ( STDMETHODCALLTYPE *get__NewEnum )( 
            __RPC__in IFileStreams * This,
            /* [retval][out] */ __RPC__deref_out_opt IUnknown **ppEnum);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Item )( 
            __RPC__in IFileStreams * This,
            /* [in] */ LONG iIndex,
            /* [retval][out] */ __RPC__deref_out_opt IDispatch **ppStream);
        
        /* [propget][id] */ HRESULT ( STDMETHODCALLTYPE *get_Count )( 
            __RPC__in IFileStreams * This,
            /* [retval][out] */ __RPC__out LONG *plCount);
        
        /* [id] */ HRESULT ( STDMETHODCALLTYPE *OpenFile )( 
            __RPC__in IFileStreams * This,
            /* [in] */ __RPC__in BSTR bszFile,
            /* [retval][out] */ __RPC__out VARIANT_BOOL *pbOpened);
        
        END_INTERFACE
    } IFileStreamsVtbl;

    interface IFileStreams
    {
        CONST_VTBL struct IFileStreamsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFileStreams_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFileStreams_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFileStreams_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFileStreams_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IFileStreams_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IFileStreams_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IFileStreams_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IFileStreams_get__NewEnum(This,ppEnum)	\
    ( (This)->lpVtbl -> get__NewEnum(This,ppEnum) ) 

#define IFileStreams_get_Item(This,iIndex,ppStream)	\
    ( (This)->lpVtbl -> get_Item(This,iIndex,ppStream) ) 

#define IFileStreams_get_Count(This,plCount)	\
    ( (This)->lpVtbl -> get_Count(This,plCount) ) 

#define IFileStreams_OpenFile(This,bszFile,pbOpened)	\
    ( (This)->lpVtbl -> OpenFile(This,bszFile,pbOpened) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFileStreams_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_FileStreams;

#ifdef __cplusplus

class DECLSPEC_UUID("C22183C2-84A4-4BB7-B354-73B6998CCC85")
FileStreams;
#endif
#endif /* __FragEng_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


