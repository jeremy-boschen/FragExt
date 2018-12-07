

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* Compiler settings for fragsvx.idl:
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
#define __REQUIRED_RPCNDR_H_VERSION__ 475
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

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __fragsvx_h__
#define __fragsvx_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IFSxDefragmentFileCallback_FWD_DEFINED__
#define __IFSxDefragmentFileCallback_FWD_DEFINED__
typedef interface IFSxDefragmentFileCallback IFSxDefragmentFileCallback;
#endif 	/* __IFSxDefragmentFileCallback_FWD_DEFINED__ */


#ifndef __IFSxFileDefragmenter_FWD_DEFINED__
#define __IFSxFileDefragmenter_FWD_DEFINED__
typedef interface IFSxFileDefragmenter IFSxFileDefragmenter;
#endif 	/* __IFSxFileDefragmenter_FWD_DEFINED__ */


#ifndef __FSxServiceManager_FWD_DEFINED__
#define __FSxServiceManager_FWD_DEFINED__

#ifdef __cplusplus
typedef class FSxServiceManager FSxServiceManager;
#else
typedef struct FSxServiceManager FSxServiceManager;
#endif /* __cplusplus */

#endif 	/* __FSxServiceManager_FWD_DEFINED__ */


#ifndef __FSxFileDefragmenter_FWD_DEFINED__
#define __FSxFileDefragmenter_FWD_DEFINED__

#ifdef __cplusplus
typedef class FSxFileDefragmenter FSxFileDefragmenter;
#else
typedef struct FSxFileDefragmenter FSxFileDefragmenter;
#endif /* __cplusplus */

#endif 	/* __FSxFileDefragmenter_FWD_DEFINED__ */


#ifndef __IFSxDefragmentFileCallback_FWD_DEFINED__
#define __IFSxDefragmentFileCallback_FWD_DEFINED__
typedef interface IFSxDefragmentFileCallback IFSxDefragmentFileCallback;
#endif 	/* __IFSxDefragmentFileCallback_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


/* interface __MIDL_itf_fragsvx_0000_0000 */
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


/* FragSvx.idl
 *    IDL source for FRAGSVX.EXE
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */



extern RPC_IF_HANDLE __MIDL_itf_fragsvx_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fragsvx_0000_0000_v0_0_s_ifspec;

#ifndef __IFSxDefragmentFileCallback_INTERFACE_DEFINED__
#define __IFSxDefragmentFileCallback_INTERFACE_DEFINED__

/* interface IFSxDefragmentFileCallback */
/* [helpstring][unique][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_IFSxDefragmentFileCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D26AF61A-609C-430F-9944-2F9DA30B0D63")
    IFSxDefragmentFileCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnDefragmentFileUpdate( 
            /* [string][in] */ __RPC__in_string LPCWSTR FileName,
            /* [in] */ ULONG PercentComplete,
            /* [in] */ ULONG_PTR CallbackParameter) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFSxDefragmentFileCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFSxDefragmentFileCallback * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFSxDefragmentFileCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFSxDefragmentFileCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnDefragmentFileUpdate )( 
            __RPC__in IFSxDefragmentFileCallback * This,
            /* [string][in] */ __RPC__in_string LPCWSTR FileName,
            /* [in] */ ULONG PercentComplete,
            /* [in] */ ULONG_PTR CallbackParameter);
        
        END_INTERFACE
    } IFSxDefragmentFileCallbackVtbl;

    interface IFSxDefragmentFileCallback
    {
        CONST_VTBL struct IFSxDefragmentFileCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFSxDefragmentFileCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFSxDefragmentFileCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFSxDefragmentFileCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFSxDefragmentFileCallback_OnDefragmentFileUpdate(This,FileName,PercentComplete,CallbackParameter)	\
    ( (This)->lpVtbl -> OnDefragmentFileUpdate(This,FileName,PercentComplete,CallbackParameter) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFSxDefragmentFileCallback_INTERFACE_DEFINED__ */


/* interface __MIDL_itf_fragsvx_0000_0001 */
/* [local] */ 

#define DFF_IGNORECONTIGUOUSFILES 0x00000001U
#define DFF_NOTBACKGROUNDPRIORITY 0x00000002U


extern RPC_IF_HANDLE __MIDL_itf_fragsvx_0000_0001_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fragsvx_0000_0001_v0_0_s_ifspec;

#ifndef __IFSxFileDefragmenter_INTERFACE_DEFINED__
#define __IFSxFileDefragmenter_INTERFACE_DEFINED__

/* interface IFSxFileDefragmenter */
/* [helpstring][unique][oleautomation][uuid][object] */ 


EXTERN_C const IID IID_IFSxFileDefragmenter;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("0A1CA5C0-00DF-43cf-A2BD-95118EB5196D")
    IFSxFileDefragmenter : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DefragmentFile( 
            /* [string][in] */ __RPC__in_string LPCWSTR FileName,
            /* [in] */ ULONG Flags,
            /* [in] */ ULONG_PTR CallbackParameter) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallbackInterface( 
            /* [in] */ __RPC__in_opt IUnknown *Callback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IFSxFileDefragmenterVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in IFSxFileDefragmenter * This,
            /* [in] */ __RPC__in REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in IFSxFileDefragmenter * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in IFSxFileDefragmenter * This);
        
        HRESULT ( STDMETHODCALLTYPE *DefragmentFile )( 
            __RPC__in IFSxFileDefragmenter * This,
            /* [string][in] */ __RPC__in_string LPCWSTR FileName,
            /* [in] */ ULONG Flags,
            /* [in] */ ULONG_PTR CallbackParameter);
        
        HRESULT ( STDMETHODCALLTYPE *SetCallbackInterface )( 
            __RPC__in IFSxFileDefragmenter * This,
            /* [in] */ __RPC__in_opt IUnknown *Callback);
        
        END_INTERFACE
    } IFSxFileDefragmenterVtbl;

    interface IFSxFileDefragmenter
    {
        CONST_VTBL struct IFSxFileDefragmenterVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IFSxFileDefragmenter_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IFSxFileDefragmenter_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IFSxFileDefragmenter_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IFSxFileDefragmenter_DefragmentFile(This,FileName,Flags,CallbackParameter)	\
    ( (This)->lpVtbl -> DefragmentFile(This,FileName,Flags,CallbackParameter) ) 

#define IFSxFileDefragmenter_SetCallbackInterface(This,Callback)	\
    ( (This)->lpVtbl -> SetCallbackInterface(This,Callback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IFSxFileDefragmenter_INTERFACE_DEFINED__ */



#ifndef __FragSvx_LIBRARY_DEFINED__
#define __FragSvx_LIBRARY_DEFINED__

/* library FragSvx */
/* [helpstring][lcid][version][uuid] */ 



EXTERN_C const IID LIBID_FragSvx;

EXTERN_C const CLSID CLSID_FSxServiceManager;

#ifdef __cplusplus

class DECLSPEC_UUID("FE0758EC-102C-4228-BB9F-0AA01DB604CF")
FSxServiceManager;
#endif

EXTERN_C const CLSID CLSID_FSxFileDefragmenter;

#ifdef __cplusplus

class DECLSPEC_UUID("64C92C98-3AE1-4795-8268-A114C0F2C61B")
FSxFileDefragmenter;
#endif
#endif /* __FragSvx_LIBRARY_DEFINED__ */

/* interface __MIDL_itf_fragsvx_0000_0002 */
/* [local] */ 


#ifndef _FSX_NO_FSXMANAGERQUERYSERVICE
/* FSxManagerQueryService
 *    Wraps CoCreateInstance/QueryService for FSxServiceManager
 */ 
__inline
HRESULT
FSxManagerQueryService(
   __in REFGUID guidService,
   __in REFIID riid,
   __deref_out void** ppService
)
{
   HRESULT            hr;
   IServiceProvider*  pService;
#ifndef _WIN64
   OSVERSIONINFO      VersionInfo;
   DWORD              dwClsContext;

   ZeroMemory(&VersionInfo,
              sizeof(VersionInfo));

   VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);

   GetVersionEx(&VersionInfo);

   dwClsContext = CLSCTX_LOCAL_SERVER|((VersionInfo.dwMajorVersion > 5) || ((VersionInfo.dwMajorVersion == 5) && (VersionInfo.dwMinorVersion >= 1)) ? CLSCTX_DISABLE_AAA : 0);
#endif /* _WIN64 */

   hr = CoCreateInstance(__uuidof(FSxServiceManager),
                         NULL,
#ifdef _WIN64
                         CLSCTX_LOCAL_SERVER|CLSCTX_DISABLE_AAA|CLSCTX_ACTIVATE_64_BIT_SERVER,
#else /* _WIN64 */
                         dwClsContext,
#endif /* _WIN64 */
                         __uuidof(IServiceProvider),
                         (void**)&pService);

   if ( SUCCEEDED(hr) )
   {
      hr = pService->QueryService(guidService,
                                  riid,
                                  ppService);

      pService->Release();
   }

   /* Success / Failure */
   return ( hr );
}
#endif /* _FSX_NO_FSXMANAGERQUERYSERVICE */



extern RPC_IF_HANDLE __MIDL_itf_fragsvx_0000_0002_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_fragsvx_0000_0002_v0_0_s_ifspec;

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


