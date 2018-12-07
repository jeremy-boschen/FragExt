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
 
/* ServiceCtl.h
 *    Service routine implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "COMLib.h"
#include "ServiceCtl.h"
/*++
   WPP
  --*/
#include "ServiceCtl.tmh"

/*++
   Private declarations
 --*/

/*
 * FragSvx Service SID:
 *    S-1-5-80-3825820650-217860745-2438786872-82588861-3636731892
 */
DEFINE_SID(FragSvxServiceSid, SECURITY_NT_AUTHORITY, SECURITY_SERVICE_ID_BASE_RID, 3825820650, 217860745, 2438786872, 82588861, 3636731892);
   
/* 
 * Service control variables
 */
SERVICE_STATUS         gServiceStatus  = {SERVICE_WIN32_OWN_PROCESS, SERVICE_STOPPED, 0, NO_ERROR, 0, 0, 0};
SERVICE_STATUS_HANDLE  ghServiceHandle = NULL;
HANDLE                 ghShutdownEvent = NULL;

#define IsServiceStopped( ) \
   (SERVICE_STOPPED == gServiceStatus.dwCurrentState)

#define IsServiceRunning( ) \
   (SERVICE_RUNNING == gServiceStatus.dwCurrentState)


/*
 * Incremental gServiceStatus.dwCheckPoint counter
 */
volatile LONG gServiceCheckPoint = 0;

/*
 * Most recent control code received via SvxServiceHandlerEx()
 */
volatile DWORD gdwServiceControl = 0;

/*
 * Status of shutdown initiation
 */
volatile LONG gServiceShutdownInitiated = 0;

#define GetLastServiceControl( ) \
   (gdwServiceControl)

#define IsSystemShutdownInProgress( ) \
   (SERVICE_CONTROL_SHUTDOWN == gdwServiceControl)

/* 
 * FragSvx service name 
 */
#define FRAGSVX_SERVICENAME \
   L"fragsvx"

/*
 * Service thread background maintenance timeout, in seconds
 */
#define FRAGSVX_MAINTENANCE_INTERVAL \
   (30 * 1000)

/*
 * CLSID_GlobalOptions seems to be missing from OLE32.LIB and UUID.LIB, so
 * we declare it here with a unique name. It's used by SvxInitializeCOMSecurity
 */
class __declspec(uuid("0000034B-0000-0000-C000-000000000046")) __CLSID_GlobalOptions;

VOID 
WINAPI 
SvxServiceMain( 
   __in DWORD dwArgc, 
   __in LPTSTR* lpszArgv 
);

DWORD 
WINAPI 
SvxServiceHandlerEx(
   __in DWORD dwControl,
   __in DWORD dwEventType,
   __in LPVOID lpEventData,
   __in LPVOID lpContext
);

DWORD
SvxUpdateServiceStatusEx(
   __in DWORD dwServiceState,
   __in DWORD dwWin3ExitCode
);

DWORD
SvxInitializeCOM(
);

void
SvxUninitializeCOM(
);

DWORD
SvxInitializeCOMSecurity(
);

DWORD
SvxUpdateServiceStatus(
   __in DWORD dwServiceState
);

void
SvxSignalServiceInitiateShutdown(
);

void
CALLBACK
SvxCoServerEventCallback(
   __in PCCOSVR_EVENT_DATA EventData
);

void
SvxPerformMaintenanceTasks(
);

VOID
CALLBACK
SvxCoMaintenanceTaskRoutine(
   __in ULONG_PTR Reserved
);

/*
 * Registered service maintenance task handlers
 */
PSERVICEMAINTENANCETASKROUTINE gMaintenanceTaskRoutine[] = {
   &SvxCoMaintenanceTaskRoutine
};


DWORD
SvxCreateServiceEvent(
   __in BOOL bManualReset,
   __in BOOL bInitialState,
   __out PHANDLE phEvent
);

/*++

   IMPLEMENTATIONS

 --*/

DWORD
SvxMain( )
{
   DWORD                dwRet;
   SERVICE_TABLE_ENTRY  ServiceTable[] = { 
      {FRAGSVX_SERVICENAME, &SvxServiceMain}, 
      {NULL,                NULL}
   };

   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Entered SvxMain, starting dispatcher\n");

   /*
    * Kick off the service thread. This call won't return until the service
    * is shutdown
    */
   if ( !StartServiceCtrlDispatcher(ServiceTable) ) {
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_ERROR,
              SvxService,
              L"StartServiceCtrlDispatcher failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __EXIT;
   }

   /* Success */
   dwRet = NO_ERROR;

__EXIT:
   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Exiting SvxMain, dwRet = %!WINERROR!\n",
           dwRet);

    /* Success / Failure */
   return ( dwRet );
}

VOID 
WINAPI 
SvxServiceMain( 
   __in DWORD dwArgc, 
   __in LPTSTR* lpszArgv 
)
{
   DWORD    dwRet;

   DWORD    dwWait;

   BOOLEAN  bCOMInitialized;
   BOOLEAN  bBackgroundPriority;
   
   UNREFERENCED_PARAMETER(dwArgc);
   UNREFERENCED_PARAMETER(lpszArgv);

   /* Initialize locals */
   bCOMInitialized     = FALSE;
   bBackgroundPriority = FALSE;
   
   FpTrace(TRACE_LEVEL_VERBOSE, 
           SvxService,
           L"SvxServiceMain, dwArgc = %u, lpszArgv = %p, starting service initialization phase\n",
           dwArgc,
           lpszArgv);
   
   /* 
    * Register the control handler right away so we can start posting status updates
    */
   ghServiceHandle = RegisterServiceCtrlHandlerEx(FRAGSVX_SERVICENAME,
                                                  &SvxServiceHandlerEx,
                                                  NULL);

   if ( !ghServiceHandle ) {
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_ERROR,
              SvxService,
              L"RegisterServiceCtrlHandlerEx failed for " FRAGSVX_SERVICENAME L", dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }
   
   /*
    * Notify the SCM that we're starting up..
    */
   dwRet = SvxUpdateServiceStatus(SERVICE_START_PENDING);
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxService,
              L"SvxUpdateServiceStatus failed for state %!SVCSTATE!, dwRet = %!WINERROR!, exiting\n",
              SERVICE_START_PENDING,
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   /* 
    * Create the service shutdown event that will be signaled when we receive a
    * SERVICE_STOP control. This is signaled from SvxSignalServiceInitiateShutdown
    */
   dwRet = SvxCreateServiceEvent(TRUE, /*manual reset*/
                                 FALSE,/*nonsignalted*/
                                 &ghShutdownEvent);

   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxService,
              L"CreateEvent failed for ghShutdownEvent, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Initialize COM support
    */
   dwRet = SvxInitializeCOM();
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxService,
              L"SvxInitializeCOM failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      goto __CLEANUP;
   }

   bCOMInitialized = TRUE;

   /*
    * Everything is up and running, so inform the SCM that we can now receive controls
    */
   dwRet = SvxUpdateServiceStatus(SERVICE_RUNNING);
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxService,
              L"SvxUpdateServiceStatus failed for %!SVCSTATE!, dwRet = %!WINERROR!, exiting\n",
              SERVICE_RUNNING,
              dwRet);

      goto __CLEANUP;
   }

   /* 
    * Now that we're up and running, let's switch our priority to background mode if
    * the platform supports it
    */
   if ( IsNTDDIVersion(g_NTVersion,
                       NTDDI_VISTA) ) {
      if ( SetPriorityClass(GetCurrentProcess(),
                            PROCESS_MODE_BACKGROUND_BEGIN) ) {
         bBackgroundPriority = TRUE;
      }

      if ( !bBackgroundPriority ) {
         dwRet = GetLastError();

         FpTrace(TRACE_LEVEL_WARNING,
                 SvxService,
                 L"SetPriorityClass failed for PROCESS_MODE_BACKGROUND_BEGIN, dwRet = %!WINERROR!, continuing\n",
                 dwRet);
      }
   }

   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Service initilization phase complete. Starting wait phase.\n");

   /*
    * And now we wait to receive the signal that we should exit, or periodically perform
    * some service maintenance tasks
    */
   for ( ;; ) {
      dwWait = WaitForSingleObjectEx(ghShutdownEvent,
                                     FRAGSVX_MAINTENANCE_INTERVAL,
                                     TRUE);


      if ( WAIT_OBJECT_0 == dwWait ) {
         FpTrace(TRACE_LEVEL_VERBOSE,
                  SvxService,
                  L"WaitForMultipleObjectsEx returned, ghShutdownEvent signaled, exiting\n");

         /*
          * ghShutdownEvent has been signaled, so we're done
          */
         break;
      }
      else if ( WAIT_IO_COMPLETION == dwWait ) {
         /*
          * Do nothing, and resume the wait
          */
         FpTrace(TRACE_LEVEL_VERBOSE,
                 SvxService,
                 L"WaitForMultipleObjectsEx returned, dwWait = WAIT_IO_COMPLETION, continuing\n");
      }
      else if ( WAIT_TIMEOUT == dwWait ) {
         /*
          * Run some maintenance tasks..
          */
         SvxPerformMaintenanceTasks();
      }
      else {
         dwRet = GetLastError();

         FpTrace(TRACE_LEVEL_ERROR,
                 SvxService,
                 L"WaitForSingleObject returned an unhandled wait status, dwWait = %08lx, dwRet = %!WINERROR!, exiting\n",
                 dwWait,
                 dwRet);

         /* Failure */
         goto __CLEANUP;
      }
   }

   /* Success */
   dwRet = NO_ERROR;

__CLEANUP:
   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Starting service cleanup phase, dwRet = %!WINERROR!\n",
           dwRet);

   /* 
    * Leave background mode if we're in it
    */
   if ( bBackgroundPriority ) {
      SetPriorityClass(GetCurrentProcess(),
                       PROCESS_MODE_BACKGROUND_END);
   }

   /*
    * Notify the SCM that we're stopping. We don't care whether this succeeds or not, or whether
    * we're already in this state
    */
   SvxUpdateServiceStatus(SERVICE_STOP_PENDING);
   
   /*
    * Cancel all pending COM calls which have registered for cancellation. If we're shutting down for 
    * a system shutdown then ignore the thread specific call timeouts set by registrees 
    */
   CoCancelRegisteredCalls(IsSystemShutdownInProgress() ? TRUE : FALSE);

   /*
    * Wait for any active calls to complete/abort as we don't want to abandon the process while
    * they're active.
    */
   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Starting poll for outstanding external client calls. ExternalCallCount = %u, continuing\n",
           CoGetExternalCallCount());

   while ( CoIsExternalCallInProgress() ) {
      SvxUpdateServiceStatus(SERVICE_STOP_PENDING);

      /*
       * Give the call some time to realize we're shutting down and abort. Outgoing calls, like
       * callbacks into clients could delay this until they're able to recover from cancellation
       * invoked above
       */
      Sleep(50);
   }   
   
   /*
    * Cleanup everything we initialized
    */
   if ( bCOMInitialized ) {
      SvxUninitializeCOM();
   }

   if ( ghShutdownEvent ) {
      CloseHandle(ghShutdownEvent);
      ghShutdownEvent = NULL;
   }

   /*
    * Setting the service status to stopped will cause the control dispatcher thread to return
    * at which point the process can exit, so we have to call this before that happens so the
    * process doesn't have a chance to die while its executing
    */
   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Exiting SvxServiceMain, service stopped, gServiceStatus.dwWin32ExitCode = %!WINERROR!, exiting\n",
           dwRet);

   /*
    * Notify the SCM that we're done and exit. This will cause StartServiceCtrlDispatcher()
    * to return from its wait. We're returning an exit code here, so we call the Ex version
    */
   SvxUpdateServiceStatusEx(SERVICE_STOPPED,
                            dwRet);

   /* Success / Failure */
   return;
}

DWORD 
WINAPI 
SvxServiceHandlerEx(
   __in DWORD dwControl,
   __in DWORD dwEventType,
   __in LPVOID lpEventData,
   __in LPVOID lpContext
)
{
   DWORD dwRet;

   UNREFERENCED_PARAMETER(dwEventType);
   UNREFERENCED_PARAMETER(lpEventData);
   UNREFERENCED_PARAMETER(lpContext);

   switch ( dwControl ) {
      case SERVICE_CONTROL_STOP:
         __fallthrough;
      case SERVICE_CONTROL_SHUTDOWN:
         /* We only accept these two controls while the service is running */
         _ASSERTE(IsServiceRunning());

         /*
          * Save the control so the rest of the application can interrogate the 
          * last one received. This is only saved for the service controls our
          * app knows how to handle, excluding SERVICE_CONTROL_INTERROGATE as
          * that is only handled in this function
          */
         gdwServiceControl = dwControl;
   
         /*
          * Kick off the shutdown phase
          */
         SvxSignalServiceInitiateShutdown();
         
         dwRet = NO_ERROR;
         /* Success */
         break;

      case SERVICE_CONTROL_INTERROGATE:
         /*
          * This is the default control accepted by services, so it could be sent
          * prior to the initial call to set the service status and the controls
          * accepted during startup. It should be handled without any interaction
          * with global service state because it may not be initialized yet
          */
         dwRet = NO_ERROR;
         /* Success */
         break;

      /*
      case SERVICE_CONTROL_DEVICEEVENT:
         __fallthrough;
      case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
         __fallthrough;
      case SERVICE_CONTROL_POWEREVENT:
         __fallthrough;
       */
      default:
         dwRet = ERROR_CALL_NOT_IMPLEMENTED;

         FpTrace(TRACE_LEVEL_WARNING,
                 SvxService,
                 L"Unhandled service control, dwControl = %!SVCCTL!, dwRet = %!WINERROR!, continuing\n",
                 dwControl,
                 dwRet);

         /* Failure */
         break;
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
SvxUpdateServiceStatus(
   __in DWORD dwServiceState
)
{
   DWORD dwRet;

   if ( !ghServiceHandle ) {
      dwRet = ERROR_INVALID_HANDLE;
      /* Failure */
      goto __EXIT;
   }

   /*
    * Reinitialize the service status
    */
   gServiceStatus.dwCurrentState     = dwServiceState;
   gServiceStatus.dwControlsAccepted = 0;
   gServiceStatus.dwCheckPoint       = 0;
   gServiceStatus.dwWaitHint         = 0;
   
   switch ( dwServiceState ) {
      case SERVICE_STOPPED:
         break;

      case SERVICE_START_PENDING:
         __fallthrough;
      case SERVICE_STOP_PENDING:
         /*
          * This function can be called by several different threads, so we have to update the
          * check point atomically
          */
         gServiceStatus.dwCheckPoint = (DWORD)InterlockedIncrement(&gServiceCheckPoint);
         gServiceStatus.dwWaitHint   = 20000;
         break;

      case SERVICE_RUNNING:
         gServiceStatus.dwControlsAccepted |= SERVICE_CONTROL_INTERROGATE|SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
         break;
   
      default:
         /* If this fires then we've received a service state we haven't added support for */
         _ASSERTE(!(SERVICE_STOPPED == dwServiceState || SERVICE_START_PENDING == dwServiceState || SERVICE_STOP_PENDING == dwServiceState || SERVICE_RUNNING == dwServiceState));
         break;
   }

   if ( !SetServiceStatus(ghServiceHandle,
                          &gServiceStatus) ) {
      /* Failure */
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_ERROR,
              SvxService,
              L"SetServiceStatus failed, ghServiceHandle = %p, dwServiceState = %!SVCSTATE!, dwRet = %!WINERROR!, continuing\n",
              ghServiceHandle,
              dwServiceState,
              dwRet);
   }
   else {
      /* Success */
      dwRet = NO_ERROR;

      FpTrace(TRACE_LEVEL_VERBOSE,
              SvxService,
              L"Service status set, ghServiceHandle = %p, dwServiceState = %!SVCSTATE!, continuing\n",
              ghServiceHandle,
              dwServiceState);
   }

__EXIT:
   /* Success / Failure */
   return ( dwRet );
}

DWORD
SvxUpdateServiceStatusEx(
   __in DWORD dwServiceState,
   __in DWORD dwWin3ExitCode
)
{
   gServiceStatus.dwWin32ExitCode           = dwWin3ExitCode;
   gServiceStatus.dwServiceSpecificExitCode = 0;
   
   return ( SvxUpdateServiceStatus(dwServiceState) );
}

/**
 * SvxInitializeCOM
 *    Initialize all COM support required by the service
 *
 * Notes
 *    This function will perform cleanup for all failure paths so only
 *    call SvxUninitializeCOM when this function returns success
 */
DWORD
SvxInitializeCOM(
)
{
   DWORD             dwRet;

   HRESULT           hr;
   IGlobalOptions*   pGlobalOptions;

   /* Initialize locals */
   dwRet          = NO_ERROR;
   pGlobalOptions = NULL;
   
   /*
    * Setup the COM library callbacks before they have a chance to be called
    */
   CoSetServerEventCallback(&SvxCoServerEventCallback);
   
   /*
    * Initialize the cancellation used by bi-directional COM components we expose
    */
   hr = CoInitializeCallCancellation();
   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxCOM,
              L"CoInitializeCallCancellation failed, hr = %!HRESULT!, exiting\n",
              hr);

      /* Failure */
      goto __EXIT;
   }

   /* 
    * Start up COM runtime services
    */
   hr = CoInitializeEx(NULL,
                       COINIT_MULTITHREADED|COINIT_DISABLE_OLE1DDE);

   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxCOM,
              L"CoInitializeEx failed, hr = %!HRESULT!, exiting\n",
              hr);

      dwRet = static_cast<DWORD>(hr);
      /* Failure */
      goto __EXIT;
   }

   /* 
    * Set up our explicit DCOM security settings 
    */
   dwRet = SvxInitializeCOMSecurity();
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxCOM,
              L"SvxInitializeCOMSecurity failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /*
       * Do any cleanup that won't be done because SvxUninitializeCOM doesn't
       * get called for failure here
       */
      CoUninitialize();

      /* Failure */
      goto __EXIT;
   }

   /*
    * Try to turn off COM exception handling if possible
    */
   if ( IsNTDDIVersion(g_NTVersion,
                       NTDDI_WINXPSP1) ) {
      hr = CoCreateInstance(__uuidof(__CLSID_GlobalOptions), 
                            NULL, 
                            CLSCTX_INPROC_SERVER, 
                            __uuidof(IGlobalOptions), 
                            reinterpret_cast<void**>(&pGlobalOptions));

      if ( FAILED(hr) ) {
         FpTrace(TRACE_LEVEL_WARNING,
                 SvxCOM,
                 L"CoCreateInstance failed for CLSID_GlobalOptions, hr = %!HRESULT!, continuing\n",
                 hr);
      }
      else {
         hr = pGlobalOptions->Set(COMGLB_EXCEPTION_HANDLING,
                                  COMGLB_EXCEPTION_DONOT_HANDLE);

         /*
          * We're done with pGlobalOptions now, so get rid of it
          */
         pGlobalOptions->Release();

         if ( FAILED(hr) ) {
            FpTrace(TRACE_LEVEL_WARNING,
                    SvxCOM,
                    L"IGlobalOptions::Set failed for COMGLB_EXCEPTION_HANDLING, COMGLB_EXCEPTION_DONOT_HANDLE, hr = %!HRESULT!, continuing\n",
                    hr);
         }
      }
   }

   /*
    * If we were started directly by the SCM, and not by an activation request via
    * COM then add an additional reference on the service to keep it alive until
    * we receive another SCM request to shutdown.
    *
    * We do this based on the assumption that if we are started via the SCM then
    * it is due to some automatic/manual service start request. For this case we
    * want to remain active until we receive a similar request to shutdown. If
    * we didn't do this, then we could be shutdown when the last external reference
    * to a class object is released. 
    *
    * We have to do this before registering class objects because they could be
    * torn down before we had a chance to add this, which would trigger shutdown.
    */
   if ( !g_ControlFlags.IsCOMActivation ) {
      CoAddRefServerProcess();
   }

   /*
    * Register all class objects in the server. If this call succeeds, then
    * by the time it returns we may already have started receiving activations
    */
   hr = CoRegisterModuleClassObjects(CLSCTX_LOCAL_SERVER,
                                     REGCLS_MULTIPLEUSE);

   if ( FAILED(hr) ) {
      /*
       * Do any cleanup that won't be done because SvxUninitializeCOM doesn't
       * get called for failure here
       */
      if ( !g_ControlFlags.IsCOMActivation ) {
         CoReleaseServerProcess();
      }

      CoUninitialize();

      FpTrace(TRACE_LEVEL_ERROR,
              SvxCOM,
              L"RegisterClassObjects failed, hr = %!HRESULT!, exiting\n",
              hr);
      
      dwRet = static_cast<DWORD>(hr);
      /* Failure */
      goto __EXIT;
   }
   
   /* Success */
   dwRet = NO_ERROR;

__EXIT:

   /* Success / Failure */
   return ( dwRet );
}

void
SvxUninitializeCOM(
)
{
   /*
    * Tear down all COM related support. If anything fails we don't
    * care because we're about to bail anyway
    */
   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Tearing down class objects and uninitializing COM\n");

   CoRevokeModuleClassObjects();

   CoUninitializeCallCancellation();

   CoUninitialize();
}

DWORD
SvxInitializeCOMSecurity(
)
{
   HRESULT              hr;
   DWORD                dwRet;
   PSECURITY_DESCRIPTOR pCOMSecuritySD;

   ACE_ENTRY            ComSecurityDacl[] = {
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, 0, COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL, &NtInteractiveSid),
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, 0, COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL, &NtBuiltinAdministratorSid),
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, 0, COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL, &NtServiceSid),
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, 0, COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL, &NtLocalSystemSid)
   };

   /* Initialize locals */
   pCOMSecuritySD = NULL;

   dwRet = CreateTemplateSecurityDescriptor(&CreatorOwnerSid, 
                                            &CreatorGroupSid,
                                            NULL,
                                            0,
                                            ComSecurityDacl, 
                                            ARRAYSIZE(ComSecurityDacl),
                                            0,
                                            &pCOMSecuritySD);

   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxSecurity,
              L"CreateTemplateSecurityDescriptor failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */ 
      goto __CLEANUP;
   }

   hr = CoInitializeSecurity(pCOMSecuritySD,
                             -1, 
                             NULL, 
                             NULL, 
                             RPC_C_AUTHN_LEVEL_PKT, 
                             RPC_C_IMP_LEVEL_IDENTIFY,
                             NULL, 
                             EOAC_STATIC_CLOAKING|EOAC_DISABLE_AAA|EOAC_NO_CUSTOM_MARSHAL, 
                             NULL);

   if ( FAILED(hr) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxCOM,
              L"CoInitializeSecurity failed, hr = %!HRESULT!, exiting\n",
              hr);

      dwRet = static_cast<DWORD>(hr);
      /* Failure */
      goto __CLEANUP;      
   }

__CLEANUP:
   if ( pCOMSecuritySD ) {
      FreeTemplateSecurityDescriptor(pCOMSecuritySD);
   }

   /* Success / Failure */
   return ( dwRet );
}

void
SvxSignalServiceInitiateShutdown(
)
{
   ULONG cCoSpLockCount;

   /*
    * We only want to do all of this once, so we need to set a flag once it's started. This
    * function could be called twice, once via a user/system request to stop or shutdown, then
    * via SvxOnFinalCoServerUnlock as the service is asking any pending COM clients to exit
    * and those calls are dropping the COM server reference count via CoLockExeServer
    */
   if ( 0 != InterlockedExchange(&gServiceShutdownInitiated,
                                 1) )
   {
      /*
       * This function has already been called
       */
      return;
   }

   /*
    * As this call may not have been initiated by COMLib, we need to inform it that we
    * are transitioning to an inactive state so it can start blocking calls before we
    * have a chance to revoke the class objects or request cancellation
    */
   CoSignalServer(COSVRSIGNAL_INACTIVE);

   /*
    * If we added a COM server process reference then remove it
    */
   if ( !g_ControlFlags.IsCOMActivation ) {
      /*
       * Pull the reference on the COM process reference count that was added
       * to keep the service alive due to the manual start
       */
      cCoSpLockCount = CoReleaseServerProcess();

      FpTrace(TRACE_LEVEL_VERBOSE,
              SvxService,
              L"CoReleaseServerProcess called, cCoSpLockCount = %u\n",
              cCoSpLockCount);
   }

   /*
    * If the service state is anything but SERVICE_STOPPED, notify the SCM that we're shutting down
    */
   if ( !IsServiceStopped() ) {
      SvxUpdateServiceStatus(SERVICE_STOP_PENDING);
   }

   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxService,
           L"Signaling service shutdown\n");
      
   SetEvent(ghShutdownEvent);
}

/*
 * This will be called once by the COM library when the global COM refernece count reaches zero. If
 * the service is being shutdown manually or via a system shutdown even, then this will likely not
 * be triggered
 */
void
CALLBACK
SvxCoServerEventCallback(
   __in PCCOSVR_EVENT_DATA EventData
)
{
   switch ( EventData->EventID ) {
      case COSVR_EVENT_DEACTIVATED:
         SvxSignalServiceInitiateShutdown();
         break;
   }
}

void
SvxPerformMaintenanceTasks(
)
{
   size_t idx;

   for (idx = 0; idx < ARRAYSIZE(gMaintenanceTaskRoutine); idx++) {
      (*gMaintenanceTaskRoutine[idx])(NULL);
   }
}

VOID
CALLBACK
SvxCoMaintenanceTaskRoutine(
   __in ULONG_PTR Reserved
)
{
   UNREFERENCED_PARAMETER(Reserved);

   /*
    * Check for dead threads on the COM cancellation list
    */
   CoRundownAbandonedCallCancellations();
}

DWORD
SvxCreateServiceEvent(
   __in BOOL bManualReset,
   __in BOOL bInitialState,
   __out PHANDLE phEvent
)
{
   HANDLE               hEvent;

   DWORD                dwRet;
   PSECURITY_DESCRIPTOR pEventSD;
   SECURITY_ATTRIBUTES  SecAttributes;

#ifndef EVENT_QUERY_STATE
   #define EVENT_QUERY_STATE 0x0001
#endif /* EVENT_QUERY_STATE */

   ACE_ENTRY            EventSDDacl[] = {
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, 0, EVENT_QUERY_STATE|EVENT_MODIFY_STATE|DELETE|SYNCHRONIZE, &NtBuiltinAdministratorSid),
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, 0, EVENT_QUERY_STATE|EVENT_MODIFY_STATE|DELETE|SYNCHRONIZE, &NtServiceSid),
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, 0, EVENT_QUERY_STATE|EVENT_MODIFY_STATE|DELETE|SYNCHRONIZE, &NtLocalSystemSid)
   };

   /* Initialize outputs */
   (*phEvent) = NULL;

   /* Initialize locals */
   pEventSD = NULL;

   dwRet = CreateTemplateSecurityDescriptor(&NtLocalSystemSid, 
                                            &NtLocalSystemSid,
                                            NULL,
                                            0,
                                            EventSDDacl, 
                                            ARRAYSIZE(EventSDDacl),
                                            0,
                                            &pEventSD);

   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxSecurity,
              L"CreateTemplateSecurityDescriptor failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */ 
      goto __CLEANUP;
   }

   SecAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
   SecAttributes.lpSecurityDescriptor = pEventSD;
   SecAttributes.bInheritHandle       = FALSE;

   hEvent = CreateEvent(&SecAttributes,
                        bManualReset,
                        bInitialState,
                        NULL);

   if ( !hEvent ) {
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_ERROR,
              SvxCOM,
              L"CreateEvent failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __CLEANUP;      
   }

   /*
    * Everything went OK, so assign the event to the caller
    */
   (*phEvent) = hEvent;

   /* Success */
   dwRet = NO_ERROR;

__CLEANUP:
   if ( pEventSD ) {
      FreeTemplateSecurityDescriptor(pEventSD);
   }

   /* Success / Failure */
   return ( dwRet );
}
