

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* Compiler settings for fragmgx.idl:
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


#ifndef __fragmgx_h__
#define __fragmgx_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDefragManager_FWD_DEFINED__
#define __IDefragManager_FWD_DEFINED__
typedef interface IDefragManager IDefragManager;
#endif 	/* __IDefragManager_FWD_DEFINED__ */


#ifndef __DefragManager_FWD_DEFINED__
#define __DefragManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class DefragManager DefragManager;
#else
typedef struct DefragManager DefragManager;
#endif /* __cplusplus */

#endif 	/* __DefragManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fragmgx_0000_0000 */
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


/* FragMgx.idl
 *    IDL source for FRAGMGX.EXE
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */



extern RPC_IF_HANDLE __MIDL_itf_fragmgx_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fragmgx_0000_0000_v0_0_s_ifspec;


#ifndef __FragMgx_LIBRARY_DEFINED__
#define __FragMgx_LIBRARY_DEFINED__

/* library FragMgx */
/* [helpstring][lcid][version][uuid] */ 


EXTERN_C const IID LIBID_FragMgx;

#ifndef __IDefragManager_INTERFACE_DEFINED__
#define __IDefragManager_INTERFACE_DEFINED__

/* interface IDefragManager */
/* [helpstring][unique][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_IDefragManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("78C1E4AD-4235-4828-A872-98A21299DF81")
    IDefragManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE QueueFile( 
            /* [string][in] */ __RPC__in_string LPCWSTR FileName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDefragManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IDefragManager * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IDefragManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IDefragManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *QueueFile )( 
            __RPC__in IDefragManager * This,
            /* [string][in] */ __RPC__in_string LPCWSTR FileName);
        
        END_INTERFACE
    } IDefragManagerVtbl;

    interface IDefragManager
    {
        CONST_VTBL struct IDefragManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDefragManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDefragManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDefragManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDefragManager_QueueFile(This,FileName)	\
    ( (This)->lpVtbl -> QueueFile(This,FileName) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDefragManager_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_DefragManager;

#ifdef __cplusplus

class DECLSPEC_UUID("5B2289D9-86CC-46B5-9735-8B9B865C9833")
DefragManager;
#endif
#endif /* __FragMgx_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


