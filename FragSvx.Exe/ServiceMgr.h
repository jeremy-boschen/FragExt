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
 
/* ServiceMgr.h
 *    CFSxServiceManager declarations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include "FragSvx.h"

#include "COMLib.h" // move to Stdafx.h

class __declspec(novtable) 
CFSxServiceManager : public CCoUnknownT<CCoMultiThreadModel>,
                     public CCoClassT< CCoHeapObjectT<CFSxServiceManager>, CCoExeServerLockModel >,
                     public IServiceProvider
{
public:
   BEGIN_QI_MAP(CFSxServiceManager)
      QI_ENTRY(IServiceProvider)
   END_QI_MAP()

   /* IServiceProvider */
public:
   STDMETHODIMP 
   QueryService( 
      REFGUID guidService,
      REFIID riid,
      void** ppvObject
   );
   
   /* CFSxServiceManager */
private:
   HRESULT
   InternalQueryService( 
      REFGUID guidService,
      REFIID riid,
      void** ppvObject
   );
};

DECLARE_CLASS_OBJECT(__uuidof(FSxServiceManager), CFSxServiceManager);

#if 0
/**
 * IFSxServiceManagerImpl
 *    Internal interface for exposing the inheritence tree of CFSxServiceManagerImpl
 */
class ATL_NO_VTABLE __declspec(uuid("7DEA4F88-B45B-481F-A0CF-AF80DA611A81"))
IFSxServiceManagerImpl : public IServiceProvider,
                         public IFSxFileDefragmenter
{
public:
};

/**
 * CFSxServiceManagerImpl
 *    Singleton class which does the real work for CServiceModule and CFSxServiceManager
 */
class ATL_NO_VTABLE CFSxServiceManagerImpl : public CComObjectRootEx<CComMultiThreadModel>,
                                             public IServiceProvider
{
   /* ATL */
public:
   DECLARE_NO_REGISTRY()

   DECLARE_NOT_AGGREGATABLE(CFSxServiceManagerImpl)

   BEGIN_COM_MAP(CFSxServiceManagerImpl)
      COM_INTERFACE_ENTRY(IServiceProvider)
      COM_INTERFACE_ENTRY(IFSxFileDefragmenter)
   END_COM_MAP()

   DECLARE_PROTECT_FINAL_CONSTRUCT()

   HRESULT FinalConstruct( ) throw();
   void FinalRelease( ) throw();

   /* IServiceProvider */
public:
   STDMETHODIMP 
   QueryService( 
      __RPC_in REFGUID guidService, 
      __RPC_in REFIID riid, 
      __RPC_out void** ppv 
   ) throw();

   /* CFSxServiceManagerImpl */
protected:
   /* Limit creation to CComObjectXXX */
   CFSxServiceManagerImpl( ) throw();

private:
};

/* Global reference to the single CFSxServiceManagerImpl instance. Initialized by
 * CFSxServiceManagerImpl::FinalConstruct() and cleared by CFSxServiceManagerImpl::FinalRelease()
 *
 * IMPORTANT:
 *    This pointer IS NOT AddRef()'d
 *    This pointer is encoded via SeEncodePointer()
 */
__declspec(selectany) IFSxServiceManagerImpl* __pServiceManager = NULL;

/**
 * CFSxServiceManager
 *    Exported FSxServiceManager coclass. COM will create instances of these, but
 *    each one will simply forward to the single CFSxServiceManagerImpl instance
 *    assigned in __pServiceManager
 */
class ATL_NO_VTABLE CFSxServiceManager : public CComObjectRootEx<CComMultiThreadModel>,
                                         public CComCoClass<CFSxServiceManager, &CLSID_FSxServiceManager>,
                                         public IServiceProvider
{
   /* ATL */
public:
   DECLARE_NO_REGISTRY()

   DECLARE_NOT_AGGREGATABLE(CFSxServiceManager)

   BEGIN_COM_MAP(CFSxServiceManager)
      COM_INTERFACE_ENTRY(IServiceProvider)
   END_COM_MAP()

   HRESULT FinalConstruct( );
   void FinalRelease( );
   
   /* IServiceProvider */
public:
   STDMETHOD(QueryService)( REFGUID guidService, REFIID riid, void** ppv ) throw();

   /* CFSxServiceManager */
protected:
   /* Limit creation to CComObjectXXX */
   CFSxServiceManager( ) throw();
};

OBJECT_ENTRY_AUTO(CLSID_FSxServiceManager, CFSxServiceManager)
#endif //0
