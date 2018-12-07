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
 
/* COMLib.cpp
 *    General COM library support
 */

#include "Stdafx.h"

#include <List.h>
#include <COMLib.h>

#include "SeUtil.h"

/*++
   WPP
 --*/
#include "COMLib.tmh"

/*++

Private declarations

 --*/
#ifndef WINERROR
   #define WINERROR( dw ) \
      (0 != (dw))
#endif /* WINERROR */

/*
 * Server event notifications
 */
PCOSERVEREVENTCALLBACK __pCoServerEventCallback;

/*
 * Lifetime data
 */
volatile ULONG __CoServerState       = 0U;
volatile ULONG __CoExternalCallCount = 0U;

#define COSRVSTATE_SERVER_DLLLOCK    0x3fffffffU
#define COSRVSTATE_CALLCX_ACTIVE     0x40000000U

#define IsCallCancellationActive( ) \
   ( 0 != (COSRVSTATE_CALLCX_ACTIVE & __CoServerState) )

/*
 * 
 */
LIST_ENTRY       gCancelList     = {0};
CRITICAL_SECTION gCancelListLock = {0};

typedef struct _COCALL_CTX
{
   LIST_ENTRY  Entry;
   DWORD       ThreadId;
   ULONG       Timeout    : 16;
   ULONG       Cancelled  : 1;
}COCALL_CTX, *PCOCALL_CTX;

/*++

   IMPLEMENTATION

 --*/

/**
 * CoSetServerActive
 *    Sets state bits in __CoServerState
 *
 * Note:
 *    - Calling CoRegisterModuleClassObjects causes COSRVSTATE_SERVER_ACTIVE to be set
 *      when REGCLS_SUSPENDED is set in dwFlags
 *    - Calling CoRevokeModuleClassObjects causes COSRVSTATE_SERVER_ACTIVE to be cleared
 *
 *    - Calling CoInitializeCallCancellation causes COSRVSTATE_CALLCX_ACTIVE to be set
 *    - Calling CoUninitializeCallCancellation or CoCancelRegisteredCalls causes
 *      COSRVSTATE_CALLCX_ACTIVE to be cleared
 */
__forceinline
void
CoSetServerState( 
   __in ULONG Feature,
   __in ULONG State
)
{
   ULONG StateOld;
   ULONG StateNew;

   /*
    * Ensure the state mask doesn't include the DLL lock bits
    */
   Feature &= COSRVSTATE_SERVER_DLLLOCK;

   do {
      StateOld = __CoServerState;
      StateNew = StateOld;

      /*
       * Clear the current value of the states in the mask
       */
      StateNew &= ~Feature;

      /*
       * Assign the new state value to the mask
       */
      StateNew |= State; 
   }
   while ( (LONG)StateOld != InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&__CoServerState),
                                                        (LONG)StateNew,
                                                        (LONG)StateOld) );
}

/*++
   CoLockExeServer / CoLockDllServer
      Maintains the lifetime of the server via COM's server reference
      counting, and signals shutdown when all references are closed
 --*/
void
APIENTRY
CoLockExeServer(
   __in BOOLEAN Lock
)
{
   ULONG             cCoSpLockCount;
   COSVR_EVENT_DATA  EData;

   ZeroMemory(&EData,
              sizeof(COSVR_EVENT_DATA));

   if ( Lock ) {
      cCoSpLockCount  = CoAddRefServerProcess();
      EData.EventID   = COSVR_EVENT_LOCKED;
      EData.LockCount = cCoSpLockCount;
   }
   else {
      cCoSpLockCount  = CoReleaseServerProcess();
      EData.EventID   = COSVR_EVENT_UNLOCKED;
      EData.LockCount = cCoSpLockCount;
      
      if ( 0 == cCoSpLockCount ) {
         /*
          * For an exe server only, transition the server to an inactive state as there will be no more activations 
          * coming in because COM is now blocking them
          */
         CoSetServerState(COSRVSTATE_SERVER_ACTIVE,
                          0);
      }
   }

   /*
    * If an event callback is registered, call it now
    */
   if ( __pCoServerEventCallback ) {
      (*__pCoServerEventCallback)(&EData);
   }

   FpTrace(TRACE_LEVEL_VERBOSE,
           FpCOMLibrary,
           L"CoLockExeServer, cCoSpLockCount ~= %u\n",
           cCoSpLockCount);
}

void
APIENTRY
CoLockDllServer(
   __in BOOLEAN Lock
)
{   
   ULONG             StateOld;
   ULONG             StateNew;
 
   do {
      StateOld = __CoServerState;
      StateNew = StateOld;

      /*
       * Clear the lock count so we can reset it
       */
      StateNew &= ~COSRVSTATE_SERVER_DLLLOCK;

      /*
       * Store the adjusted lock count
       */
      StateNew |= ( (StateOld & COSRVSTATE_SERVER_DLLLOCK) + (Lock ? 1 : -1) );
   }
   while ( (LONG)StateOld != InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&(__CoServerState)),
                                                        (LONG)StateNew,
                                                        (LONG)StateOld) );

   FpTrace(TRACE_LEVEL_VERBOSE,
           FpCOMLibrary,
           L"CoLockDllServer, __CoServerState ~= %u\n",
           (ULONG)(COSRVSTATE_SERVER_DLLLOCK & StateNew));
}

void
APIENTRY
CoSignalServer( 
   __in ULONG Signal
)
{
   ULONG             Feature;
   ULONG             State;

   COSVR_EVENT_DATA  EData;

   Feature = 0;
   State   = 0;

   ZeroMemory(&EData,
              sizeof(COSVR_EVENT_DATA));

   /*
    * Map the callers signal to the specified features and their state
    */
   switch ( Signal ) {
      case COSVRSIGNAL_ACTIVE: 
         Feature       = COSRVSTATE_SERVER_ACTIVE;
         State         = COSRVSTATE_SERVER_ACTIVE;   
         EData.EventID = COSVR_EVENT_ACTIVATED;
         break;

      case COSVRSIGNAL_INACTIVE:
         Feature       = COSRVSTATE_SERVER_ACTIVE;
         State         = 0;         
         EData.EventID = COSVR_EVENT_DEACTIVATED;
         break;

      default:
         _ASSERTE((COSVRSIGNAL_ACTIVE != Signal) && (COSVRSIGNAL_INACTIVE != Signal));
         __assume(0);
   }

   CoSetServerState(Feature,
                    State);

   /*
    * If an event callback is registered, call it now
    */
   if ( __pCoServerEventCallback && (0 != EData.EventID) ) {
      (*__pCoServerEventCallback)(&EData);                        
   }
}

/*++
  
CCoClassFactory
  
 --*/
template < class ThreadingModel, class ServerLockModel >
CCoClassFactory<ThreadingModel, ServerLockModel>::CCoClassFactory( ) throw()
{
   _CoClass = NULL;
}

template < class ThreadingModel, class ServerLockModel >
void
CCoClassFactory<ThreadingModel, ServerLockModel>::Initialize(
   const struct _COCLASS_ENTRY* CoClass
) throw()
{
   _CoClass = CoClass;
}

template < class ThreadingModel, class ServerLockModel >
void
CCoClassFactory<ThreadingModel, ServerLockModel>::Destroy( ) throw()
{
   InternalDestruct();
   _CoClass = NULL;
   delete this;
}

template < class ThreadingModel, class ServerLockModel >
STDMETHODIMP
CCoClassFactory<ThreadingModel, ServerLockModel>::CreateInstance( 
   IUnknown* pUnkOuter,
   REFIID riid,
   void** ppvObject
) 
{
   HRESULT hr;

   _ASSERTE(NULL != _CoClass);

   if ( !CoEnterExternalCall() ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"CoEnterExternalCall failed, CO_E_SERVER_STOPPING\n");

      /* Failure */
      return ( CO_E_SERVER_STOPPING );
   }

   hr = S_OK;

   __try {
      hr = InternalCreateInstance(pUnkOuter,
                                  riid,
                                  ppvObject);

      if ( FAILED(hr) ) {
         FpTrace(TRACE_LEVEL_ERROR,
                 FpCOMLibrary,
                 L"InternalCreateInstance failed, pUnkOuter = %p, riid = %!GUID!, ppvObject = %p\n",
                 pUnkOuter,
                 &riid,
                 ppvObject);
      }
   }
   __finally {
      CoLeaveExternalCall();
   }

   /* Success / Failure */
   return ( hr );
}

template < class ThreadingModel, class ServerLockModel >
HRESULT
CCoClassFactory<ThreadingModel, ServerLockModel>::InternalCreateInstance(   
   __in IUnknown* pUnkOuter,
   __in REFIID riid,
   __out void** ppvObject
)
{
   HRESULT hr;
   
   /*
    * Validate parameters..
    */
   if ( !ppvObject ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"Invalid parameter, ppvObject = NULL\n");

      /* Failure */
      return ( E_POINTER );
   }
   
   (*ppvObject) = NULL;

   /*
    * COM law.. If caller is requesting aggregation, they must also request IUnknown
    */
   if ( pUnkOuter && !InlineIsEqualGUID(__uuidof(IUnknown), 
                                        riid) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"Invalid parameters, pUnkOuter = %p, riid = %!GUID!\n",
              pUnkOuter,
              &riid);

      /* Failure */
      return ( CLASS_E_NOAGGREGATION );
   }

   /* Initialize not called? */
   _ASSERTE(NULL != _CoClass);

   hr = _CoClass->CreateInstanceRoutine(pUnkOuter,
                                        riid,
                                        ppvObject);

   /* Success / Failure */
   return ( hr );
}

HRESULT
APIENTRY
CoRegisterModuleClassObjects(
   __in DWORD dwClsContext,
   __in DWORD dwFlags
)
{
   HRESULT                 hr;
   
   struct _COCLASS_ENTRY** ppCoClassEntry;
   struct _COCLASS_ENTRY** ppCoClassEntryTail;
   
   CoExeClassFactory*      pCoClassFactory;

   /* Initialize locals.. */
   hr                 = S_OK;
   ppCoClassEntry     = &__pCoClassEntryFirst;
   ppCoClassEntryTail = &__pCoClassEntryLast;

   while ( ++ppCoClassEntry < ppCoClassEntryTail ) {
      if ( (*ppCoClassEntry)->CreateInstanceRoutine ) {
         /* Already registered the class object? */
         _ASSERTE(0 == (*ppCoClassEntry)->dwRegister);

         hr = CoExeClassFactory::CreateObjectInstance(&pCoClassFactory);
         if ( FAILED(hr) ) {
            FpTrace(TRACE_LEVEL_ERROR,
                    FpCOMLibrary,
                    L"CoRegisterModuleClassObjects!CoClassFactory::CreateObjectInstance failed, &pCoClassFactory = %p, hr = %!HRESULT!\n",
                    &pCoClassFactory,
                    hr);

            /* Failure */
            goto __CLEANUP;
         }

         pCoClassFactory->Initialize(*ppCoClassEntry);
         
         /*
          * Register this object with COM and save the registration ID for later. We force a suspended
          * registration so that we can safely cleanup on failure
          */
         hr = CoRegisterClassObject(*(*ppCoClassEntry)->Clsid,
                                    static_cast<IUnknown*>(pCoClassFactory),
                                    dwClsContext,
                                    dwFlags|REGCLS_SUSPENDED,
                                    &((*ppCoClassEntry)->dwRegister));

         if ( FAILED(hr) ) {
            _ASSERTE(0 == (*ppCoClassEntry)->dwRegister);

            /*
             * Since the object wasn't registered, it can't be destroyed via CoRevokeModuleClassObjects
             * so we need to do it here
             */
            pCoClassFactory->Destroy();

            FpTrace(TRACE_LEVEL_ERROR,
                    FpCOMLibrary,
                    L"CoRegisterClassObject failed, Clsid = %!GUID!, hr = %!HRESULT!\n",
                    (*ppCoClassEntry)->Clsid,
                    hr);

            /* Failure */
            goto __CLEANUP;
         }

         /*
          * Store object so we can delete it later
          */
         (*ppCoClassEntry)->ObjectInstance = pCoClassFactory;
      }
   }

   /*
    * If the caller supplied REGCLS_SUSPENDED then we don't do automatic resuming. We do this
    * so the caller can do any additional setup before allowing activations to come in
    */
   if ( !FlagOn(dwFlags, REGCLS_SUSPENDED) ) {
      /*
       * We got through all our class object registrations successfully. Transition the server
       * state to running, so CoEnterExternalCall can succeed, then resume all the objects to 
       * allow requests to come in. The server state has to be transitioned before resuming the
       * class objects because the class objects will reject any requests if the server is not
       * running.
       */
      CoSetServerState(COSRVSTATE_SERVER_ACTIVE,
                       COSRVSTATE_SERVER_ACTIVE);

      hr = CoResumeClassObjects();
      if ( FAILED(hr) ) {
         FpTrace(TRACE_LEVEL_ERROR,
                  FpCOMLibrary,
                  L"CoResumeClassObjects failed, hr = %!HRESULT!\n",
                  hr);
      }
   }

__CLEANUP:
   if ( FAILED(hr) ) {
      CoRevokeModuleClassObjects();
   }

   /* Success / Failure */
   return ( hr );
}

void
APIENTRY
CoRevokeModuleClassObjects(
)
{
   struct _COCLASS_ENTRY** ppCoClassEntry;
   struct _COCLASS_ENTRY** ppCoClassEntryTail;
   
   /*
    * Transition the server to an inactive state. It may already be in this state
    * but we'll do it anyway
    */
   CoSetServerState(COSRVSTATE_SERVER_ACTIVE,
                    0);

   ppCoClassEntry     = &__pCoClassEntryFirst;
   ppCoClassEntryTail = &__pCoClassEntryLast;
   
   while ( ++ppCoClassEntry < ppCoClassEntryTail ) {
      if ( 0 != (*ppCoClassEntry)->dwRegister ) {
         CoRevokeClassObject((*ppCoClassEntry)->dwRegister);
         (*ppCoClassEntry)->dwRegister = 0;

         reinterpret_cast<CoExeClassFactory*>((*ppCoClassEntry)->ObjectInstance)->Destroy();
      }
   }
}

HRESULT
CoDllGetModuleClassObject(
   __in REFCLSID rclsid, 
   __in REFIID riid, 
   __deref_out LPVOID* ppv
)
{
   HRESULT                 hr;
   
   struct _COCLASS_ENTRY** ppCoClassEntry;
   struct _COCLASS_ENTRY** ppCoClassEntryTail;
   
   CoDllClassFactory*      pCoClassFactory;
   
   /* Initialize locals.. */
   hr                 = CLASS_E_CLASSNOTAVAILABLE;
   ppCoClassEntry     = &__pCoClassEntryFirst;
   ppCoClassEntryTail = &__pCoClassEntryLast;

   while ( ++ppCoClassEntry < ppCoClassEntryTail ) {
      if ( (*ppCoClassEntry)->CreateInstanceRoutine ) {
         if ( IsEqualCLSID(rclsid,
                           *((*ppCoClassEntry)->Clsid)) ) {
            hr = CoDllClassFactory::CreateObjectInstance(&pCoClassFactory);
            if ( FAILED(hr) ) {
               FpTrace(TRACE_LEVEL_ERROR,
                       FpCOMLibrary,
                       L"CoDllGetModuleClassObject!CoDllClassFactory::CreateObjectInstance failed, pCoClassFactory = %p, hr = %!HRESULT!\n",
                       pCoClassFactory,
                       hr);

               /* Failure */
               break;
            }

            pCoClassFactory->Initialize(*ppCoClassEntry);

            /*
             * Place an initial live reference count on the object. If QI succeededs, it will be
             * at 2, if it fails, then the Release below will destroy the object
             */
            pCoClassFactory->AddRef(); {
               /*
                * Query the newly created class object for the interface the client is requesting
                */
               hr = pCoClassFactory->QueryInterface(riid,
                                                    ppv);
            }
            pCoClassFactory->Release();

            if ( FAILED(hr) ) {
               FpTrace(TRACE_LEVEL_ERROR,
                       FpCOMLibrary,
                       L"CoDllGetModuleClassObject!CoDllClassFactory::QueryInterface failed, pCoClassFactory = %p, hr = %!HRESULT!\n",
                       pCoClassFactory,
                       hr);
            }

            break;
         }
      }
   }

   /* Success / Failure */
   return ( hr );
}

HRESULT
APIENTRY
CoDllCanUnloadNow(
)
{
   return ( CoServerIsActive() ? S_OK : S_FALSE );
}

HRESULT
APIENTRY
CoGetClientToken(
   __in DWORD DesiredAccess,
   __out PHANDLE TokenHandle
)
{
   HRESULT           hr;
   DWORD             dwRet;

   IServerSecurity*  pServerSecurity;
   BOOL              bImpersonating;

   /* Initialize outputs */
   (*TokenHandle) = NULL;

   /* Initialize locals */
   pServerSecurity = NULL;
   bImpersonating  = FALSE;


   /*
    * Get the COM call context so we can impersonate if we're not already
    */
   hr = CoGetCallContext(__uuidof(IServerSecurity),
                         reinterpret_cast<void**>(&pServerSecurity));

   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"CoGetCallContext failed, IID_IServerSecurity, hr = %!HRESULT!\n",
              hr);

      /* Failure */
      return ( hr );
   }

   if ( !pServerSecurity->IsImpersonating() ) {
      hr = pServerSecurity->ImpersonateClient();
      if ( FAILED(hr) ) {
         FpTrace(TRACE_LEVEL_ERROR,
                 FpCOMLibrary,
                 L"pServerSecurity->IsImpersonating failed, hr = %!HRESULT!\n",
                 hr);

         /* Failure */
         goto __CLEANUP;
      }

      bImpersonating = TRUE;
   }

   if ( !OpenThreadToken(GetCurrentThread(),
                         DesiredAccess,
                         TRUE,
                         TokenHandle) ) {
      dwRet = GetLastError();
      hr = __HRESULT_FROM_WIN32(dwRet);

      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"OpenThreadToken failed, DesiredAccess = %08lx, dwRet = %!WINERROR!\n",
              DesiredAccess,
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   /* Success */
   hr = S_OK;

__CLEANUP:
   if ( pServerSecurity ) {
      if ( bImpersonating ) {
         pServerSecurity->RevertToSelf();
      }

      pServerSecurity->Release();
   }

   /* Success / Failure */
   return ( hr );
}

HRESULT
APIENTRY
CoSetStaticCloaking(
   __in IUnknown* pProxy
)
{
   HRESULT           hr;

   IClientSecurity*  pClientSecurity;
   
   DWORD             dwAuthnSvc;
   DWORD             dwAuthzSvc;
   OLECHAR*          pszServerPrincName;
   DWORD             dwAuthnLevel;
   DWORD             dwCapabilities;

   /* Initialize locals */
   pClientSecurity    = NULL;
   pszServerPrincName = NULL;
   
   /* 
    * Retrieve the proxy managers IClientSecurity implementation.. 
    */
   hr = pProxy->QueryInterface(__uuidof(IClientSecurity),
                               reinterpret_cast<void**>(&pClientSecurity));

   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"pProxy->QueryInterface failed, pProxy = %p, IID_IClientSecurity, hr = %!HRESULT!\n",
              pProxy,
              hr);

      /* Failure */
      goto __CLEANUP;
   }
   
   /* 
    * Retrieve the current blanket settings so we can change only what we need to 
    */
   hr = pClientSecurity->QueryBlanket(pProxy,
                                      &dwAuthnSvc,
                                      &dwAuthzSvc,
                                      &pszServerPrincName,
                                      &dwAuthnLevel,
                                      NULL,
                                      NULL,
                                      &dwCapabilities);

   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"pClientSecurity->QueryBlanket failed, pClientSecurity = %p, pProxy = %p, hr = %!HRESULT!\n",
              pClientSecurity,
              pProxy,
              hr);

      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Force static cloaking and identify level impersonation
    */
   dwCapabilities &= ~(DWORD)EOAC_DYNAMIC_CLOAKING;
   dwCapabilities |=  (DWORD)EOAC_STATIC_CLOAKING;

   hr = pClientSecurity->SetBlanket(pProxy,
                                    dwAuthnSvc,
                                    dwAuthzSvc,
                                    pszServerPrincName,
                                    dwAuthnLevel,
                                    RPC_C_IMP_LEVEL_IDENTIFY,
                                    NULL,
                                    dwCapabilities);

   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpCOMLibrary,
              L"pClientSecurity->SetBlanket failed, pClientSecurity = %p, pProxy = %p, dwAuthnSvc = %u, dwAuthzSvc = %u, pszServerPrincName = %ws, dwAuthnLevel = %u, dwCapabilities = %08lx, hr = %!HRESULT!\n",
              pClientSecurity,
              pProxy,
              dwAuthnSvc,
              dwAuthzSvc,
              pszServerPrincName,
              dwAuthnLevel,
              dwCapabilities,
              hr);
   }
 
__CLEANUP:
   if ( pszServerPrincName ) {
      CoTaskMemFree(pszServerPrincName);
   }

   if ( pClientSecurity ) {
      pClientSecurity->Release();
   }

   /* Success / Failure */
   return ( hr );
}

/*++

   COM Call Cancellation

 --*/

HRESULT
APIENTRY
CoInitializeCallCancellation(
)
{
   HRESULT  hr;
   DWORD    dwRet;

   dwRet = NO_ERROR;

   __try {
      InitializeCriticalSection(&gCancelListLock);
   }
   __except( STATUS_NO_MEMORY == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
      dwRet = GetExceptionCode();
   }

   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpCOMLibrary,
              L"InitializeCriticalSection failed, dwRet = %!STATUS!, exiting\n",
              dwRet);

      hr = HRESULT_FROM_NT(dwRet);
      /* Failure */
      return ( hr );
   }

   InitializeListHead(&gCancelList);

   /*
    * Everything is setup, so set the server state bit for this feature
    */
   CoSetServerState(COSRVSTATE_CALLCX_ACTIVE,
                    COSRVSTATE_CALLCX_ACTIVE);
   
   /* Success */
   return ( S_OK );
}

void
APIENTRY
CoUninitializeCallCancellation(
)
{
   _ASSERTE(IsListEmpty(&(gCancelList)));

   CoSetServerState(COSRVSTATE_CALLCX_ACTIVE,
                    0);

   DeleteCriticalSection(&gCancelListLock);
}

void
APIENTRY
CoRundownAbandonedCallCancellations(
)
{
   PLIST_ENTRY pEntry;
   PCOCALL_CTX pCoCallCtx;

   HANDLE      hThread;

   if ( TryEnterCriticalSection(&gCancelListLock) ) {
      __try {
         /*
          * Process each entry in the cancel list
          */
         for ( pEntry = gCancelList.Flink; pEntry != &gCancelList; pEntry = pEntry->Flink ) {
            pCoCallCtx = CONTAINING_RECORD(pEntry,
                                           COCALL_CTX,
                                           Entry);

            /*
             * Check if the thread still exists
             */
            hThread = OpenThread(THREAD_QUERY_INFORMATION,
                                 FALSE,
                                 pCoCallCtx->ThreadId);

            if ( hThread ) {
               /*
                * The thread is still alive, so the entry is valid
                */
               CloseHandle(hThread);
            }
            else {
               /*
                * The thread that registered this entry has died and will never remove it
                * from the list, so we'll do it instead
                */
               RemoveEntryList(pEntry);

               free(pCoCallCtx);
               pCoCallCtx = NULL;
            }
         }
      }
      __finally {
         LeaveCriticalSection(&gCancelListLock);
      }
   }
}

void
APIENTRY
CoCancelRegisteredCalls(
   __in BOOLEAN IgnoreThreadTimeout
)
{
   HRESULT     hr;
   
   PLIST_ENTRY pEntry;
   PCOCALL_CTX pCoCallCtx;

   /*
    * Turn off this feature as we're about to tear it down
    */
   CoSetServerState(COSRVSTATE_CALLCX_ACTIVE,
                    0);

   EnterCriticalSection(&(gCancelListLock)); {
      /*
       * Process each entry in the cancel list. The thread is responsible for freeing
       * the entries
       */
      for ( pEntry = gCancelList.Flink; pEntry != &gCancelList; pEntry = pEntry->Flink ) {
         pCoCallCtx = CONTAINING_RECORD(pEntry,
                                        COCALL_CTX,
                                        Entry);
         
         /*
          * Mark the entry as cancelled
          */
         pCoCallCtx->Cancelled = 1;

         /*
          * Cancel calls on this thread now allowing a timeout if possible, otherwise
          * force it to cancel immediately
          */
         hr = CoCancelCall(pCoCallCtx->ThreadId,
                           IgnoreThreadTimeout ? 0 : pCoCallCtx->Timeout);

         if ( FAILED(hr) ) {
            FpTrace(TRACE_LEVEL_WARNING,
                    FpCOMLibrary,
                    L"CoCancelCall failed, pCoCallCtx->ThreadId = %08lx, pCoCallCtx->Timeout = %u, IgnoreThreadTimeout = %u, hr = %!HRESULT!, continuing\n",
                    pCoCallCtx->ThreadId,
                    (ULONG)pCoCallCtx->Timeout,
                    (ULONG)IgnoreThreadTimeout,
                    hr);
         }
      }
   }
   LeaveCriticalSection(&(gCancelListLock));
}

HANDLE
APIENTRY
CoRegisterCallCancelCtx(
   __in USHORT Timeout
)
{
   HRESULT     hr;
   
   PCOCALL_CTX pCoCallCtx;
   HANDLE      hCancelCtx;
   
   /* Initialize locals */
   pCoCallCtx = NULL;
   hCancelCtx = INVALID_HANDLE_VALUE;
      
   if ( !IsCallCancellationActive() ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpCOMLibrary,
              L"IsCallCancellationActive returned false, exiting\n");

      /* Failure - CoCancelRegisteredCalls already called */
      goto __CLEANUP;
   }

   /*
    * Allocate an entry for this thread. As this is only to be used for outbound calls, the
    * memory usage and addtional time added by using the heap doesn't add much to the total
    * processing time of the caller. This also isolates the caller's stack from cleanup of
    * the list later on
    */
   pCoCallCtx = reinterpret_cast<PCOCALL_CTX>(malloc(sizeof(COCALL_CTX)));
   if ( !pCoCallCtx ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpCOMLibrary,
              L"malloc failed, sizeof(COCALL_CTX) = %u, exiting\n",
              (ULONG)sizeof(COCALL_CTX));

      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Setup the context for the caller
    */
   pCoCallCtx->ThreadId  = GetCurrentThreadId();
   pCoCallCtx->Cancelled = 0;
   pCoCallCtx->Timeout   = Timeout;

   /*
    * Try to register COM call cancellation for this thread
    */
   hr = CoEnableCallCancellation(NULL);
   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpCOMLibrary,
              L"CoEnableCallCancellation failed, hr = %!HRESULT!, exiting\n",
              hr);

      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Setup the call context handle for the caller
    */
   hCancelCtx = reinterpret_cast<HANDLE>(pCoCallCtx);

   /*
    * Everything is all good to go, so tack the cancel context onto the global list
    */
   EnterCriticalSection(&(gCancelListLock)); {
      InsertTailList(&(gCancelList),
                     &(pCoCallCtx->Entry));
   }
   LeaveCriticalSection(&(gCancelListLock));

   /*
    * Clear the local value so we don't free it below
    */
   pCoCallCtx = NULL;

__CLEANUP:
   if ( pCoCallCtx ) {
      free(pCoCallCtx);
      pCoCallCtx = NULL;
   }

   /* Success / Failure */
   return ( hCancelCtx );
}

void
APIENTRY
CoUnregisterCallCancelCtx(
   __in HANDLE CancelCtx
)
{
   HRESULT     hr;
   PCOCALL_CTX pCoCallCtx;
   
   /* 
    * Check if registration succeeded for this thread 
    */
   if ( INVALID_HANDLE_VALUE == CancelCtx ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpCOMLibrary,
              L"CoUnregisterCallCancelCtx called on invalid cancel context handle, exiting\n");

      /* Success */
      return;
   }

   /*
    * Decode the cancel context handle
    */
   pCoCallCtx = reinterpret_cast<PCOCALL_CTX>(CancelCtx);
   
   /* Valid context object? */
   _ASSERTE(_CrtIsValidHeapPointer(pCoCallCtx));
   
   /* Same thread calling unregister? */
   _ASSERTE(GetCurrentThreadId() == pCoCallCtx->ThreadId);

   /*
    * Take the entry off the global list
    */
   EnterCriticalSection(&(gCancelListLock)); {
      RemoveEntryList(&(pCoCallCtx->Entry));
   }
   LeaveCriticalSection(&(gCancelListLock));
   
   /*
    * Return the entry back to the heap
    */
   free(pCoCallCtx);
   pCoCallCtx = NULL;
   
   /*
    * Match up the call to disable cancellation on this thread 
    */
   hr = CoDisableCallCancellation(NULL);
   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpCOMLibrary,
              L"CoDisableCallCancellation failed, hr = %!HRESULT!\n",
              hr);
   }

   /* Success */
   return;
}