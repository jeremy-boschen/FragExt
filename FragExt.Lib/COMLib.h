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
 
/* COMLib.h
 *    General lightweight COM library support
 */

/*

   EXE COM server lifetime:
      - RegisterClassObjects starts activations
      - RevokeClassObjects stops
      - CoReleaseServerProcess stops


 */

#pragma once

#ifndef __COMLIB_H__
#define __COMLIB_H__

#include <unknwn.h>
#include <winnt.h>

#ifdef _DEBUG
#include <DbgUtil.h>
#include <GuidDb.h>
#endif /* _DEBUG */

#include "FragLibp.h"

/*++

   COM Server Lifetime Management

 --*/

void
APIENTRY
CoLockExeServer(
   __in BOOLEAN Lock
);

void
APIENTRY
CoLockDllServer(
   __in BOOLEAN Lock
);

/*++!FOR LIBRARY USE ONLY!*/
extern volatile ULONG __CoServerState;

#define COSRVSTATE_SERVER_ACTIVE 0x80000000U
/*--!FOR LIBRARY USE ONLY!*/

/**
 * CoServerIsActive
 *    Indicates whether this library is accepting external calls. For EXE servers,
 *    this also indicates whether COM is routing activation requests to the instance.
 */
#define CoServerIsActive( ) \
   ( 0 != (ULONG)(COSRVSTATE_SERVER_ACTIVE & __CoServerState) )

/**
 * CoSignalServer
 *    Used to communicate a state to the COM server
 */
#define COSVRSIGNAL_ACTIVE    1
#define COSVRSIGNAL_INACTIVE  2

void
APIENTRY
CoSignalServer( 
   __in ULONG Signal
);

/*++

   Server State Notifications
   
 --*/
typedef struct _COSVR_EVENT_DATA
{
   ULONG    EventID;
   union
   {
      ULONG LockCount;
   };
}COSVR_EVENT_DATA, *PCOSVR_EVENT_DATA;
typedef const struct _COSVR_EVENT_DATA* PCCOSVR_EVENT_DATA;

typedef
void
(CALLBACK* PCOSERVEREVENTCALLBACK)(
   __in PCCOSVR_EVENT_DATA EventData
);

#define COSVR_EVENT_ACTIVATED    1
#define COSVR_EVENT_DEACTIVATED  2

/* 
 * Locking/unlocking, EventData = external reference count
 */
#define COSVR_EVENT_LOCKED       3
#define COSVR_EVENT_UNLOCKED     4

/*++!FOR LIBRARY USE ONLY!*/
extern PCOSERVEREVENTCALLBACK __pCoServerEventCallback;
/*--!FOR LIBRARY USE ONLY!*/

__inline
void
CoSetServerEventCallback(
   __in PCOSERVEREVENTCALLBACK Callback
)
{  
   __pCoServerEventCallback = Callback;
}

/*
 * Global in-call reference count
 *
 * - Update via CoEnterExternalCall/CoLeaveExternalCall
 * - Poll via CoIsExternalCallInProgress/CoGetExternalCallCount
 */

/*++!FOR LIBRARY USE ONLY!*/
extern volatile ULONG __CoExternalCallCount;

#define CoIncrementExternalCallCount( ) \
   InterlockedIncrement(reinterpret_cast<volatile LONG*>(&__CoExternalCallCount))

#define CoDecrementExternalCallCount( ) \
   InterlockedDecrement(reinterpret_cast<volatile LONG*>(&__CoExternalCallCount))
/*--!FOR LIBRARY USE ONLY!*/

/**
 * CoIsExternalCallInProgress
 *    Returns TRUE while an external client is executing a call in this server
 */
#define CoIsExternalCallInProgress( ) \
   (0 != __CoExternalCallCount)

/**
 * CoGetExternalCallCount
 *    Retrieves the active external call count
 *
 * Note:
 *    This value is not accurate and is to be used as a debugging aid only
 */
#define CoGetExternalCallCount( ) \
   (__CoExternalCallCount)

/* CoEnterExternalCall
 *    Called on method entry to determine if the server is accepting external 
 *    calls and to update the global call count if so
 *
 * Note:
 *    Each call to this function must be paired with a call to CoLeaveExternalCall
 */
__forceinline
BOOLEAN
CoEnterExternalCall( ) 
{
   CoIncrementExternalCallCount();

   if ( !CoServerIsActive() ) {
      CoDecrementExternalCallCount();
      /* Failure */
      return ( FALSE );
   }

   /* Success */
   return ( TRUE );
}

/* CoLeaveExternalCall
 *    Decrements the global external call count previous incremented by
 *    a successful call to CoEnterExternalCall
 */
#define CoLeaveExternalCall( ) \
   CoDecrementExternalCallCount()

/*++

   COM Object Support

 --*/
struct CCoMultiThreadModel
{
   static ULONG Increment( volatile ULONG* p ) throw() { 
      return ( static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(p))) );
   } 

   static ULONG Decrement( volatile ULONG* p ) throw() { 
      return ( static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<volatile LONG*>(p))) );
   }
};

struct CCoSingleThreadModel
{
   static ULONG Increment( volatile ULONG* p ) throw() { 
      return ( ++(*p) );
   } 

   static ULONG Decrement( volatile ULONG* p ) throw() { 
      return ( --(*p) );
   }
};

struct CCoNoLockModel
{
   static ULONG Increment( volatile ULONG* ) throw() { 
      return ( 1 );
   } 

   static ULONG Decrement( volatile ULONG* ) throw() { 
      return ( 1 );
   }
};

struct CCoExeServerLockModel
{
   static void LockServer( )  throw() {
      CoLockExeServer(TRUE);
   }

   static void UnlockServer( ) throw() {
      CoLockExeServer(FALSE);
   }
};

struct CCoExeServerNoLockModel
{
   static void LockServer( )  throw() {
   }

   static void UnlockServer( ) throw() {
   }
};


struct CCoDllServerLockModel
{
   static void LockServer( )  throw() {
      CoLockDllServer(TRUE);
   }

   static void UnlockServer( ) throw() {
      CoLockDllServer(FALSE);
   }
};

struct CCoDllServerNoLockModel
{
   static void LockServer( )  throw() {
   }

   static void UnlockServer( ) throw() {
   }
};

template < class ThreadModelTraits = CCoMultiThreadModel >
class __declspec(novtable) CCoUnknownT
{
public:
   ULONG     
   InternalAddRef( ) throw() {
      return ( ThreadModelTraits::Increment(&_ReferenceCount) );
   }

   ULONG 
   InternalRelease( ) throw() {
      return ( ThreadModelTraits::Decrement(&_ReferenceCount) );
   }

   HRESULT
   InternalConstruct( ) throw() {
      return ( S_OK );
   }

   void
   InternalDestruct( ) throw() {
   }

protected:
   CCoUnknownT( ) throw() {
      _ReferenceCount = 0;
   }

   volatile ULONG _ReferenceCount;
};

typedef struct _QI_MAP_ENTRY
{
   const IID*  Interface;
   ULONG       Offset;
}QI_MAP_ENTRY, *PQI_MAP_ENTRY;
typedef const struct _QI_MAP_ENTRY* PCQI_MAP_ENTRY;

#define _OFFSETOFINTERFACE( _Base, _Interface ) \
   (ULONG)( (ULONG_PTR)( static_cast<_Base*>( ( (_Interface*)sizeof(PVOID) ) ) ) - sizeof(PVOID) )

/**
 * BEGIN_QI_MAP / QI_ENTRY / END_QI_MAP
 *    COM QueryInterface support
 */
#define BEGIN_QI_MAP( _Class ) \
   typedef _Class __QIMAPClassType; \
   static PCQI_MAP_ENTRY _GetQIMap( ) { \
      static const struct _QI_MAP_ENTRY __QIMap[] = { \
         {&__uuidof(IUnknown), _OFFSETOFINTERFACE(__QIMAPClassType, IUnknown) },

#define QI_ENTRY( _Interface ) \
         {&__uuidof(_Interface), _OFFSETOFINTERFACE(__QIMAPClassType, _Interface) },

#define END_QI_MAP( ) \
         {NULL, 0xffffffffU} \
      }; \
      return ( __QIMap ); \
   } \
   HRESULT InternalQueryInterface( REFIID riid, void** ppvObject ) { \
      PCQI_MAP_ENTRY pQIMap; \
      IUnknown*        pObject; \
      if ( !ppvObject ) { \
         return ( E_POINTER ); \
      } \
      for ( pQIMap = __QIMAPClassType::_GetQIMap(); pQIMap->Interface; pQIMap++ ) { \
         if ( InlineIsEqualGUID(*(pQIMap->Interface), riid) ) { \
            pObject = reinterpret_cast<IUnknown*>( reinterpret_cast<ULONG_PTR>(this) + pQIMap->Offset ); \
            pObject->AddRef(); \
            (*ppvObject) = reinterpret_cast<void*>(pObject); \
            return ( S_OK ); \
         } \
      } \
      (*ppvObject) = NULL; \
      return ( E_NOINTERFACE ); \
   } 

template < class T >
class CCoHeapObjectT : public T
{
   /* IUnknown */
public: 
   HRESULT
   STDMETHODCALLTYPE
   QueryInterface( 
      REFIID riid,
      __deref_out void** ppvObject
   ) {
      HRESULT  hr;
#ifdef _DEBUG
      wchar_t  chIIDName[GDB_CCH_MAXGUIDNAME];
#endif /* _DEBUG */

      hr = InternalQueryInterface(riid,
                                  ppvObject);

#ifdef _DEBUG
      if ( NO_ERROR == LookupNameOfGuidW(riid,
                                         chIIDName,
                                         _countof(chIIDName)) ) {
         DbgPrintfExW(DBGF_INFORMATION,
                      _LTEXT(__FUNCTION__) L"\tthis = 0x%p, %s, %s\n",
                      this,
                      SUCCEEDED(hr) ? L"success" : L"failure",
                      chIIDName);
      }
#endif /* _DEBUG */

      /* Success / Failure */
      return ( hr );
   }

   ULONG
   STDMETHODCALLTYPE
   AddRef( ) {
      ULONG ReferenceCount;

      ReferenceCount = InternalAddRef();

#ifdef _DEBUG
      DbgPrintfExW(DBGF_INFORMATION,
                   _LTEXT(__FUNCTION__) L"\tthis = 0x%p, ReferenceCount = %-10d\n",
                   static_cast<T*>(this),
                   ReferenceCount);
#endif /* _DEBUG */

      return ( ReferenceCount );
   }

   ULONG
   STDMETHODCALLTYPE
   Release( ) {
      ULONG ReferenceCount;

      ReferenceCount = InternalRelease();
      if ( 0 == ReferenceCount ) {
         InternalDestruct();
         delete this;
      }

#ifdef _DEBUG
      DbgPrintfExW(DBGF_INFORMATION,
                   _LTEXT(__FUNCTION__) L"\tthis = 0x%p, ReferenceCount = %-10d\n",
                   static_cast<T*>(this),
                   ReferenceCount);
#endif /* _DEBUG */

      return ( ReferenceCount );
   }

   void* 
   __CRTDECL 
   operator new( 
      size_t _Size, 
      const char* _FileName,
      int _LineNumber 
   ) throw() {
#ifdef _DEBUG
      return ( _malloc_dbg(_Size, _NORMAL_BLOCK, _FileName, _LineNumber) );
#else
      UNREFERENCED_PARAMETER(_FileName);
      UNREFERENCED_PARAMETER(_LineNumber);
      return ( malloc(_Size) );
#endif
   }

   void* 
   __CRTDECL 
   operator new[]( 
      size_t _Size, 
      const char* _FileName,
      int _LineNumber 
   ) throw() {
#ifdef _DEBUG
      return ( _malloc_dbg(_Size, _NORMAL_BLOCK, _FileName, _LineNumber) );
#else
      UNREFERENCED_PARAMETER(_FileName);
      UNREFERENCED_PARAMETER(_LineNumber);
      return ( malloc(_Size) );
#endif
   }

   void 
   __CRTDECL 
   operator delete( 
      void* _Memory
   ) throw() {
#ifdef _DEBUG
      _free_dbg(_Memory,
                _NORMAL_BLOCK);
#else
      free(_Memory);
#endif
   }

   void 
   __CRTDECL 
   operator delete[](
      void* _Memory
   ) throw() {
#ifdef _DEBUG
      _free_dbg(_Memory,
                _NORMAL_BLOCK);
#else
      free(_Memory);
#endif
   }

   /* CCoHeapObjectT */
public:
   CCoHeapObjectT( ) throw() {
      InternalLockServer();
   }

   virtual ~CCoHeapObjectT( ) throw() {
      InternalUnlockServer();
   }

   /*
    * Create, return initial reference count
    */
   static 
   HRESULT
   STDMETHODCALLTYPE
   CreateObjectInstance(
      CCoHeapObjectT<T>** ppObjectT
   ) {
      HRESULT            hr;
      CCoHeapObjectT<T>* pObjectT;

      _ASSERTE(NULL != ppObjectT);

      (*ppObjectT) = NULL;
      
      pObjectT = new(__FILE__, __LINE__) CCoHeapObjectT<T>();
      if ( !pObjectT ) {
         return ( E_OUTOFMEMORY );
      }

      pObjectT->InternalAddRef(); {
         hr = pObjectT->InternalConstruct();
      }
      pObjectT->InternalRelease();
      
      if ( FAILED(hr) ) {
         /*
          * We don't call InternalDestruct() here because InternalConstruct() failed
          * and we assume the object's state is undefined
          */

         delete pObjectT;
         /* Failure */
         return ( hr );
      }

      (*ppObjectT) = pObjectT;
      /* Success */
      return ( S_OK );
   }

   static
   HRESULT
   STDMETHODCALLTYPE
   CreateObjectInstance(
      IUnknown* /*pUnkOuter*/,
      REFIID riid,
      void** ppvObject
   ) {
      HRESULT            hr;
      CCoHeapObjectT<T>* pObject;

      _ASSERTE(NULL != ppvObject);

      (*ppvObject) = NULL;

      hr = CreateObjectInstance(&pObject);
      if ( FAILED(hr) ) {
         /* Failure */
         return ( hr );
      }

      pObject->InternalAddRef(); {
         hr = pObject->QueryInterface(riid,
                                      ppvObject);
      }
      pObject->InternalRelease();

      if ( FAILED(hr) ) {
         _ASSERTE(NULL == (*ppvObject));

         /*
          * CreateObjectInstance() succeeded above, which means InternalConstruct()
          * also succeeded, so we know the object's state is initialized and must give
          * the object a chance to cleanup
          */
         pObject->InternalDestruct();

         delete pObject;
      }

      /* Success / Failure */
      return ( hr );
   }
};

template < class T, class ServerLockModelTraits >
class __declspec(novtable) CCoClassT
{
public:
   static 
   HRESULT
   STDMETHODCALLTYPE
   CreateCoClassInstance(
      CCoHeapObjectT<T>** ppObjectT
   ) {
      return ( T::CreateObjectInstance(ppObject) );
   }

   static
   HRESULT
   STDMETHODCALLTYPE
   CreateCoClassInstance(
      LPUNKNOWN pUnkOuter,
      REFIID riid,
      void** ppvObject
   ) {
      HRESULT hr;

      if ( pUnkOuter )  {
         /* Failure */
         return ( CLASS_E_NOAGGREGATION );
      }

      /* Create our object */
      hr = T::CreateObjectInstance(NULL,
                                   riid,
                                   ppvObject);

      /* Success / Failure */
      return ( hr );
   }

   /* Server lifetime */
protected:
   static void InternalLockServer( ) throw() {
      ServerLockModelTraits::LockServer();
   }

   static void InternalUnlockServer( ) throw() {
      ServerLockModelTraits::UnlockServer();
   }
};

typedef 
HRESULT 
(STDMETHODCALLTYPE* PCREATEINSTANCEROUTINE)( 
   LPUNKNOWN pUnkOuter, 
   REFIID riid, 
   void** ppvObject 
);

typedef struct _COCLASS_ENTRY
{
   union {
      const CLSID*         Clsid;
      void*                ObjectInstance;
   };
   PCREATEINSTANCEROUTINE  CreateInstanceRoutine;
   DWORD                   dwRegister;
}COCLASS_ENTRY;

#pragma section("CCM$__a", read, write, shared)
#pragma section("CCM$__m", read, write, shared)
#pragma section("CCM$__z", read, write, shared)

extern "C"
{
   __declspec(selectany) __declspec(allocate("CCM$__a")) struct _COCLASS_ENTRY* __pCoClassEntryFirst = NULL;
   __declspec(selectany) __declspec(allocate("CCM$__z")) struct _COCLASS_ENTRY* __pCoClassEntryLast  = NULL;
};

#pragma comment(linker, "/merge:CCM=.data")

#if defined(_M_IX86)
   #define EXTERN_C_DECORATION_PREFIX "_"
#elif defined(_M_AMD64)
   #define EXTERN_C_DECORATION_PREFIX ""
#endif

#define DECLARE_CLASS_OBJECT( _Clsid, _Class ) \
   __declspec(selectany) struct _COCLASS_ENTRY __CoClassEntry##_Class = {&_Clsid, (PCREATEINSTANCEROUTINE)&_Class::CreateCoClassInstance, 0}; \
   extern "C" __declspec(allocate("CCM$__m")) __declspec(selectany) struct _COCLASS_ENTRY* const __pCoClassEntry##_Class = &__CoClassEntry##_Class; \
   __pragma(comment(linker, "/include:" EXTERN_C_DECORATION_PREFIX "__pCoClassEntry" #_Class))

/**
 * CCoClassFactory
 *    Class object used by RegisterClassObjects 
 */
template < class ThreadingModel, class ServerLockModelTraits >
class CCoClassFactory : public CCoUnknownT<ThreadingModel>,
                        public IClassFactory
{
   /* CComHeapObjectT */
public:
   BEGIN_QI_MAP(CCoClassFactory)
      QI_ENTRY(IClassFactory)
   END_QI_MAP()
   
protected:
   static void InternalLockServer( ) throw() {
      ServerLockModelTraits::LockServer();
   }

   static void InternalUnlockServer( ) throw() {
      ServerLockModelTraits::UnlockServer();
   }

   /* IClassFactory */
public:
   STDMETHODIMP
   CreateInstance( 
      IUnknown* pUnkOuter,
      REFIID riid,
      void** ppvObject
   );

   STDMETHODIMP
   LockServer( 
      BOOL fLock
   )
   {      
      _ASSERTE(CoServerIsActive());

      if ( fLock ) {
         ServerLockModelTraits::LockServer();
      } else {
         ServerLockModelTraits::UnlockServer();
      }

      /* Success */
      return ( S_OK );
   }

   /* CCoClassFactory */
protected:
   CCoClassFactory( 
   ) throw();
   
public:
   void
   Initialize(
      const struct _COCLASS_ENTRY* CoClass
   ) throw();

   void
   Destroy(
   ) throw();

private:
   const struct _COCLASS_ENTRY* _CoClass;

   HRESULT
   InternalCreateInstance(   
      IUnknown* pUnkOuter,
      REFIID riid,
      void** ppvObject
   );

   /* NOT-SUPPORTED */
   CCoClassFactory( const CCoClassFactory& );
   CCoClassFactory& operator =( const CCoClassFactory& );
};

/*
 * EXE class factories are destroyed via CoRevokeModuleClassObjects and do not maintain
 * a reference count on the server
 *
 * DLL class factories are destroyed via IUnknown::Release and do add a reference count
 * on the server
 */
typedef CCoHeapObjectT< CCoClassFactory<CCoNoLockModel, CCoExeServerNoLockModel> > CoExeClassFactory;
typedef CCoHeapObjectT< CCoClassFactory<CCoMultiThreadModel, CCoDllServerLockModel> > CoDllClassFactory;

/*++

   EXE Server Class Management

 --*/
HRESULT
APIENTRY
CoRegisterModuleClassObjects(
   __in DWORD dwClsContext,
   __in DWORD dwFlags
);

void
APIENTRY
CoRevokeModuleClassObjects(
);

/*++

   DLL Server Class Management

 --*/
HRESULT
APIENTRY
CoDllGetModuleClassObject(
   __in REFCLSID rclsid, 
   __in REFIID riid, 
   __deref_out LPVOID* ppv
);

HRESULT
APIENTRY
CoDllCanUnloadNow(
);

/*++

   Security Helpers

 --*/

HRESULT
APIENTRY
CoGetClientToken(
   __in DWORD DesiredAccess,
   __out PHANDLE TokenHandle
);

HRESULT 
APIENTRY
CoSetStaticCloaking(
   __in IUnknown* pProxy
);

template < class Q >
inline
HRESULT
CoCloakedQueryInterface(
   __in IUnknown* pUnknown,
   __out Q** ppObject
)
{
   HRESULT hr;
   Q*      pObject;

   /*
    * Set cloaking on the interface we're about to QI. This will cause DCOM to
    * use the current identity for QI/AddRef/Release calls as well
    */
   hr = CoSetStaticCloaking(pUnknown);

   if ( SUCCEEDED(hr) ) {
      hr = pUnknown->QueryInterface(__uuidof(Q), 
                                    reinterpret_cast<void**>(&pObject));

      if ( SUCCEEDED(hr) ) {
         /*
          * Set cloaking on the outgoing interface
          */
         hr = CoSetStaticCloaking(static_cast<IUnknown*>(pObject));
         if ( SUCCEEDED(hr) ) {
            (*ppObject) = pObject;
            /* Success */
            return ( S_OK );
         }

         /*
          * We failed to set cloaking on the outgoing interface and won't
          * be returning it, so free it
          */
         pObject->Release();
      }
   }

   /* Failure */
   return ( hr );
}

/*
 * CoGetLastError
 *    GetLastError and __HRESULT_FROM_WIN32 wrapper 
 */
__forceinline
HRESULT
CoGetLastError(
)
{
   DWORD dwRet;
   dwRet = GetLastError();

   return ( __HRESULT_FROM_WIN32(dwRet) );
}

/*++

   COM Call Cancellation

   Wrappers around COM call cancellation that allow threads to register in a global
   list of active calls that can be cancelled by another thread.

   - This is used by the service to cancel outbound calls that could be hung in 
     clients thereby preventing the service from shutting down

 --*/

typedef ULONG_PTR COCALLCTXID;

/* Called by bookeeping thread */
HRESULT
APIENTRY
CoInitializeCallCancellation(
);

void
APIENTRY
CoUninitializeCallCancellation(
);

void
APIENTRY
CoRundownAbandonedCallCancellations(
);

void
APIENTRY
CoCancelRegisteredCalls(
   __in BOOLEAN IgnoreThreadTimeout
);

/* Called by threads before/after synchronous outbound call */
HANDLE
APIENTRY
CoRegisterCallCancelCtx(
   __in USHORT Timeout
);

void
APIENTRY
CoUnregisterCallCancelCtx(
   __in HANDLE CancelCtx
);

inline
BOOLEAN
CoRpcIsFatalError(
   __in ULONG RpcStatus
)
{
   size_t             idx;
   static const ULONG rgStatus[] = {
      0xC0000005U, /*STATUS_ACCESS_VIOLATION*/
      0xC0000194U, /*STATUS_POSSIBLE_DEADLOCK*/
      0xC00000AAU, /*STATUS_INSTRUCTION_MISALIGNMENT*/
      0x80000002U, /*STATUS_DATATYPE_MISALIGNMENT*/
      0xC0000096U, /*STATUS_PRIVILEGED_INSTRUCTION*/
      0xC000001DU, /*STATUS_ILLEGAL_INSTRUCTION*/
      0x80000003U, /*STATUS_BREAKPOINT*/
      0xC00000FDU, /*STATUS_STACK_OVERFLOW*/
      0xC0000235U, /*STATUS_HANDLE_NOT_CLOSABLE*/
      0xC0000006U, /*STATUS_IN_PAGE_ERROR*/
      0xC0000420U, /*STATUS_ASSERTION_FAILURE*/
      0xC0000409U, /*STATUS_STACK_BUFFER_OVERRUN*/
      0x80000001U  /*STATUS_GUARD_PAGE_VIOLATION*/
   };

   for ( idx = 0; idx < ARRAYSIZE(rgStatus); idx++ ) {
      if ( rgStatus[idx] == RpcStatus ) {
         return ( TRUE );
      }
   }

   return ( FALSE );
}

inline
int
CoRpcExceptionFilter(
   __in ULONG RpcStatus
)
{
   if ( CoRpcIsFatalError(RpcStatus) ) {
      return ( EXCEPTION_CONTINUE_SEARCH );
   }

   return ( EXCEPTION_EXECUTE_HANDLER );
}

#endif /* __COMLIB_H__ */
