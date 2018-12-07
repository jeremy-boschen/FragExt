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
 
/* Module.cpp
 *    Module implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 05/24/2009 - Created
 */

#include "Stdafx.h"
#include "Module.h"
#include "ServiceMgr.h"

#include <DbgUtil.h>
#include <SecUtil.h>

#include "WinLogonRpc.h"
#include "WinStationRpc.h"


/**********************************************************************

	CServiceModule : CAtlServiceModuleExT

 **********************************************************************/
HRESULT CServiceModule::Start( int nShowCmd ) throw()
{
   if ( m_bService )
	{
      SERVICE_TABLE_ENTRY ServiceTable[] =
      {
	      {m_szServiceName, _ServiceMain},
	      {NULL,            NULL}
      };
			
      if ( 0 == ::StartServiceCtrlDispatcher(ServiceTable) )
      {
         m_status.dwWin32ExitCode = GetLastError();
      }
   }
   else
   {	
      m_status.dwWin32ExitCode = Run(nShowCmd);
   }
   
   return ( HRESULT_FROM_WIN32(m_status.dwWin32ExitCode) );
}

void CServiceModule::ServiceMain( DWORD dwArgc, LPTSTR* lpszArgv ) throw()
{  
   _AtlServiceModuleExT::ServiceMain(dwArgc,
                                     lpszArgv);
}

HRESULT CServiceModule::Run( int nShowCmd ) throw()
{
   HRESULT   hr;
   IUnknown* pUnkServiceManager;
   BOOL      bBackgroundSupported;

   /* Initialize locals */
   hr                 = E_FAIL;
   pUnkServiceManager = NULL;

   if ( m_bService )
   {
      SetServiceStatusEx(SERVICE_START_PENDING, 
                         1, 
                         5000);
   }

   /* Security has to be intialized before CFSxServiceManagerImpl is created, so do it here... */
   hr = _AtlServiceModuleExT::PreMessageLoop(nShowCmd);
   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Create the global instance of CFSxServiceManagerImpl. The constructor will assign the
    * __pServiceManager variable which is used by the rest of the project. This function
    * controls the lifetime of the object with its reference on pUnkServiceManager, and the
    * global __pServiceManager variable is just a typed alias */
   hr = ATLEx::CreateObjectInstance<CFSxServiceManagerImpl>(__uuidof(IUnknown), 
                                                            reinterpret_cast<void**>(&pUnkServiceManager),
                                                            NULL);

   if ( FAILED(hr) )
   {
      LogEventEx(hr, 
                 L"Failed to initialize FragExt service manager.", 
                 EVENTLOG_ERROR_TYPE);

      /* Failure */
      goto __CLEANUP;
   }

   if ( m_bService )
   {
      SetServiceStatusEx(SERVICE_RUNNING, 
                         0, 
                         0);
   }

   /* We'll put the entire process into background mode when it's supported */
   bBackgroundSupported = IsVersionNT(6, 0, 0, 0);

   /* Now that we're up and running, let's switch our priority to background mode */
   if ( bBackgroundSupported )
   {
      SetPriorityClass(GetCurrentProcess(),
                       PROCESS_MODE_BACKGROUND_BEGIN);
   }

   /* Forward to the base class's implementation */
   _AtlServiceModuleExT::RunMessageLoop();

   /* Leave background mode */
   if ( bBackgroundSupported )
   {
      SetPriorityClass(GetCurrentProcess(),
                       PROCESS_MODE_BACKGROUND_END);
   }

   if ( m_bService )
   {
      SetServiceStatusEx(SERVICE_STOP_PENDING, 
                         1, 
                         5000);
   }

   hr = _AtlServiceModuleExT::PostMessageLoop();
   
__CLEANUP:
   if ( pUnkServiceManager )
   {
      pUnkServiceManager->Release();
   }

   /* Success / Failure */
   return ( hr );
}

/*
   if <= win2k
      if SessionId == 0
         if SecpGetCurrentUserToken("WinSta0", &hToken)
            return hToken
         
      if WinStationQueryInformationW(NULL, SessionId, WinStationUserToken, WINSTATIONUSERTOKEN{GetCurrentProcessId(),GetCurrentThreadId(),NULL}, sizeof(WINSTATIONUSERTOKEN), &RetLength)
         return WINSTATIONUSERTOKEN.hToken

      return failure

   >= winxp
      if WTSQueryUserToken(SessionId, &hToken)
         return hToken

   return failure



 */

extern "C" BOOLEAN WinStationQueryInformationW(
  __in   HANDLE hServer,
  __in   ULONG LogonId,
  __in   int WinStationInformationClass,
  __out  PVOID pWinStationInformation,
  __in   ULONG WinStationInformationLength,
  __out  PULONG pReturnLength
);

typedef struct _WINSTATIONUSERTOKEN {
  DWORD ProcessId;
  DWORD ThreadId;
  HANDLE UserToken;
} WINSTATIONUSERTOKEN,  *PWINSTATIONUSERTOKEN;

void _TestWinStation( )
{
   BOOLEAN bRet;

   WINSTATIONUSERTOKEN t;

   t.ProcessId = GetCurrentProcessId();
   t.ThreadId = GetCurrentThreadId();
   t.UserToken = NULL;

   ULONG cbRet = 0;
   bRet = WinStationQueryInformationW(NULL, (ULONG)-1, 14, &t, sizeof(t), &cbRet);
   if ( bRet )
   {
      if ( SetThreadToken(NULL, t.UserToken) )
      {
         SetThreadToken(NULL, NULL);
      }

      CloseHandle(t.UserToken);
   }

}

RPC_STATUS
TermSrvBind(
    IN LPCWSTR pszUuid,
    IN LPCWSTR pszProtocolSequence,
    IN LPCWSTR pszNetworkAddress,
    IN LPCWSTR pszEndPoint,
    IN LPCWSTR pszOptions,
    OUT RPC_BINDING_HANDLE *pHandle
    )
{
    RPC_STATUS Status;
    LPWSTR pszString = NULL;

    /*
     * Compose the binding string using the helper routine
     * and our protocol sequence, security options, UUID, and so on.
     */
    Status = RpcStringBindingCompose(
                 (LPWSTR)pszUuid,
                 (LPWSTR)pszProtocolSequence,
                 (LPWSTR)pszNetworkAddress,
                 (LPWSTR)pszEndPoint,
                 (LPWSTR)pszOptions,
                 &pszString
                 );

    if( Status != RPC_S_OK ) 
    {
        wprintf ( L"Error %d in RpcStringBindingCompose", Status );
        goto TS_EXIT_POINT;
    }

    /*
     * Now generate the RPC binding from the canonical RPC
     * binding string.
     */
    Status = RpcBindingFromStringBinding(
                 pszString,
                 pHandle
                 );

    if( Status != RPC_S_OK ) 
    {
        wprintf ( L"Error %d in RpcBindingFromStringBinding", Status );
        goto TS_EXIT_POINT;
    }

TS_EXIT_POINT:

    if ( NULL != pszString )
    {
        /*
         * Free the memory returned from RpcStringBindingCompose()
         */
        RpcStringFree( &pszString );
    }

    return( Status );
}

RPC_STATUS
TermSrvBindSecure(
    LPCWSTR pszUuid,
    LPCWSTR pszProtocolSequence,
    LPCWSTR pszNetworkAddress,
    LPCWSTR pszEndPoint,
    LPCWSTR pszOptions,
    RPC_BINDING_HANDLE *pHandle
    )
{
    RPC_STATUS Status;
    RPC_SECURITY_QOS qos;
    //LPWSTR wszServerSPN = NULL;

    *pHandle = NULL;

    Status = TermSrvBind(
                    pszUuid,
                    pszProtocolSequence,
                    pszNetworkAddress,
                    pszEndPoint,
                    pszOptions,
                    pHandle);

    if( Status != RPC_S_OK ) 
    {
        wprintf ( L"Error %d in TermSrvBind", Status );
        goto TS_EXIT_POINT;
    }

      return Status;

    qos.Capabilities = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
    qos.IdentityTracking = RPC_C_QOS_IDENTITY_DYNAMIC;
    qos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;
    qos.Version = RPC_C_SECURITY_QOS_VERSION;

    /*if( PrepareServerSPN( pszNetworkAddress, &wszServerSPN ))
    {
        Status = RpcBindingSetAuthInfoEx(
                        *pHandle,
                        wszServerSPN,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_AUTHN_GSS_NEGOTIATE,
                        NULL,
                        RPC_C_AUTHZ_NAME,
                        &qos);

        LocalFree(wszServerSPN);
    }
    else*/
    {
        Status = RpcBindingSetAuthInfoEx(
                        *pHandle,
                        (LPWSTR)pszNetworkAddress,
                        RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                        RPC_C_AUTHN_GSS_NEGOTIATE,
                        NULL,
                        RPC_C_AUTHZ_NAME,
                        &qos);
    }

    if ( RPC_S_OK != Status )
    {
        wprintf ( L"Error %d in RpcBindingSetAuthInfoEx", Status );
        goto TS_EXIT_POINT;
    }

TS_EXIT_POINT:

    if ( RPC_S_OK != Status &&
         NULL != *pHandle )
    {
        RpcBindingFree( pHandle );
    }

    return Status;
}

__inline
ULONG 
RpcClientExceptionFilter(
   __in ULONG ExceptionCode
)
{
   const ULONG*       pExCode;
   static const ULONG rgExCode[] =
   {
      STATUS_ACCESS_VIOLATION, 
      STATUS_POSSIBLE_DEADLOCK,
      STATUS_INSTRUCTION_MISALIGNMENT,
      STATUS_DATATYPE_MISALIGNMENT,
      STATUS_PRIVILEGED_INSTRUCTION,
      STATUS_ILLEGAL_INSTRUCTION,
      STATUS_BREAKPOINT,
      STATUS_STACK_OVERFLOW,
      STATUS_HANDLE_NOT_CLOSABLE,
      STATUS_IN_PAGE_ERROR,
      STATUS_ASSERTION_FAILURE,
      STATUS_STACK_BUFFER_OVERRUN,
      STATUS_GUARD_PAGE_VIOLATION
   };

   for ( pExCode = &(rgExCode[0]); pExCode < &(rgExCode[_countof(rgExCode)]); pExCode++ )
   {
      /*
       * This is an exception that RPC will not send back to a client, so it's a client issue
       */
      if ( (*pExCode) == ExceptionCode )
      {
         return ( EXCEPTION_CONTINUE_SEARCH );
      }
   }

   return ( EXCEPTION_EXECUTE_HANDLER );
}


void _TestIcaApi()
{
   NTSTATUS Result;

   RPC_BINDING_HANDLE RpcTSHandle;

   SERVER_HANDLE ContextHandle = NULL;

   Result = TermSrvBindSecure(
      L"5ca4a760-ebb1-11cf-8611-00a0245420ed",
      L"ncalrpc",
      NULL,
      L"IcaApi",
      L"Security=Impersonation Dynamic False",
      &RpcTSHandle
     );

   if ( 0 != Result )
      return;

   BOOLEAN rc;
     //
     // Get a context handle from the server so it can
     // manage the connections state
     //
     RpcTryExcept {
       rc = RpcWinStationOpenServer( RpcTSHandle, &Result, &ContextHandle );
     }
     RpcExcept(__RpcExceptionFilter(RpcExceptionCode())) {
       Result = RpcExceptionCode();
       rc = FALSE;
       wprintf(L"ERR: RPC Exception %d\n",Result );
     }
     RpcEndExcept

   if ( ContextHandle )
   {
      RpcTryExcept {
         rc = RpcWinStationCloseServer( ContextHandle, &Result );
      }
      RpcExcept(__RpcExceptionFilter(RpcExceptionCode())) {
      }
      RpcEndExcept
   }   

   RpcBindingFree(&RpcTSHandle);
}

HRESULT CServiceModule::InitializeSecurity( ) throw()
{
   HRESULT              hr;
   DWORD                dwErr;
   PSECURITY_DESCRIPTOR pRelativeSD;
   PSECURITY_DESCRIPTOR pAbsoluteSD;

   /* Initialize locals */
   pRelativeSD = NULL;
   pAbsoluteSD = NULL;
   
   /* Construct a SECURITY_DESCRIPTOR that grants access to SYSTEM, SERVICE, Administrators and the Interactive User */
   if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(L"O:SYG:SYD:(A;;CCDCSW;;;IU)(A;;CCDCSW;;;BA)(A;;CCDCSW;;;SU)(A;;CCDCSW;;;SY)",
                                                             SDDL_REVISION_1,
                                                             &pRelativeSD,
                                                             NULL) )
   {
      dwErr = GetLastError();
      hr    = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

#if 0
   {
      /* This is the code that was used to generate the SD string used above */
      CDacl         dacl;
      CSecurityDesc secd;      
      
      dacl.AddAllowedAce(Sids::World(), COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL);      
      secd.SetDacl(dacl);
      secd.SetOwner(Sids::System());
      secd.SetGroup(Sids::System());
      secd.MakeSelfRelative();

      CString szSDDL;
      secd.ToString(&szSDDL);
      szSDDL += L"\n";
      OutputDebugString(szSDDL);
   }
#endif /* 0 */

   /* Convert the self-relative SD to an absolute one as is required by CoInitializeSecurity() */
   dwErr = MakeAbsoluteSDFromSelfRelativeSD(pRelativeSD,
                                            &pAbsoluteSD);
   if ( NO_ERROR != dwErr )
   {
      hr = __HRESULT_FROM_WIN32(dwErr);
      /* Failure */
      goto __CLEANUP;
   }

   hr = CoInitializeSecurity(pAbsoluteSD,
                             -1, 
                             NULL, 
                             NULL, 
                             RPC_C_AUTHN_LEVEL_PKT, 
                             RPC_C_IMP_LEVEL_IDENTIFY,
                             NULL, 
                             EOAC_STATIC_CLOAKING|EOAC_DISABLE_AAA|EOAC_NO_CUSTOM_MARSHAL, 
                             NULL);

#if 0
   while ( !IsDebuggerPresent() )
   {
      Sleep(50);
   }

   __debugbreak();

   _TestIcaApi();

   _TestWinStation();
   // XP+ uses WTSQueryUserToken(SessionId) exclusively
   //
   // Win2k tries, in order.. winlogonrpc, WinStationQueryInformationW   
   //

   
   //
   //VER_PLATFORM_WIN32_NT 
   if ( !IsVersionNT(6,0,0,0) )
   {
      RPC_STATUS           RpcStatus;
      RPC_BINDING_HANDLE   hBinding;

      RPC_SECURITY_QOS     SecurityQos;

      HANDLE               hToken;
      
      hBinding = NULL;
      hToken   = NULL;

      RpcStatus = RpcBindingFromStringBinding(L"ncacn_np:",
                                              &hBinding);

      if ( RPC_S_OK == RpcStatus )
      {
         SecurityQos.Version           = RPC_C_SECURITY_QOS_VERSION_1;
         SecurityQos.Capabilities      = 0;
         SecurityQos.IdentityTracking  = RPC_C_QOS_IDENTITY_DYNAMIC;
         SecurityQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;         

         RpcStatus = RpcBindingSetAuthInfoEx(hBinding,
                                             NULL,
                                             RPC_C_AUTHN_LEVEL_PKT_INTEGRITY,
                                             RPC_C_AUTHN_WINNT,
                                             NULL,
                                             RPC_C_AUTHZ_NONE,
                                             &SecurityQos);

         if ( RPC_S_OK == RpcStatus )
         {
            RpcTryExcept 
            {
               RpcStatus = SecpGetCurrentUserToken(hBinding, 
                                                   L"WinSta0", 
                                                   GetCurrentProcessId(), 
                                                   &hToken, 
                                                   TOKEN_ALL_ACCESS);

               _dTrace(1, L"SecpGetCurrentUserToken=%08lx, hToken=%08lx\n", RpcStatus, hToken);
               
               if ( RPC_S_OK == RpcStatus )
               {
                  if ( SetThreadToken(NULL, hToken) )
                  {
                     SetThreadToken(NULL, NULL);
                  }
                  CloseHandle(hToken);
               }
            }
            RpcExcept(( ( (RpcExceptionCode() != STATUS_ACCESS_VIOLATION) &&
                   (RpcExceptionCode() != STATUS_DATATYPE_MISALIGNMENT) &&
                   (RpcExceptionCode() != STATUS_PRIVILEGED_INSTRUCTION) &&
                   (RpcExceptionCode() != STATUS_BREAKPOINT) &&
                   (RpcExceptionCode() != STATUS_STACK_OVERFLOW) &&
                   (RpcExceptionCode() != STATUS_IN_PAGE_ERROR) &&
                   (RpcExceptionCode() != STATUS_GUARD_PAGE_VIOLATION)
                    )
                    ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )) 
            {
               _dTrace(1, L"SecpGetCurrentUserToken!Exception=%08lx\n", RpcExceptionCode());
            }
            RpcEndExcept
         }

         RpcBindingFree(&hBinding);
      }
   }
#endif //0

__CLEANUP:
   if ( pRelativeSD )
   {
      LocalFree(reinterpret_cast<HLOCAL>(pRelativeSD));
   }

   if ( pAbsoluteSD )
   {
      LocalFree(reinterpret_cast<HLOCAL>(pAbsoluteSD));
   }

   /* Success / Failure */
   return ( hr );
}

bool CServiceModule::ParseCommandLine( LPCTSTR lpCmdLine, HRESULT* pnRetCode ) throw()
{
#ifdef _DEBUG
   return ( _AtlServiceModuleExT::ParseCommandLine(lpCmdLine,
                                                   pnRetCode) );
#else /* _DEBUG */
   UNREFERENCED_PARAMETER(lpCmdLine);
   (*pnRetCode) = S_OK;
   return ( true );
#endif /* _DEBUG */
}

HRESULT CServiceModule::RegisterAppId( bool bService ) throw()
{
#ifdef _DEBUG
   HRESULT              hr;
   DWORD                dwErr;
   ULONG                cbLaunchPermSD;
   PSECURITY_DESCRIPTOR pLaunchPermSD;
   LONG                 lRet;
   HKEY                 hKey;
	
   hr = _AtlServiceModuleExT::RegisterAppId(bService);
   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   if ( !bService )
   {
      /* Success */
      return ( hr );
   }
   
   /* Add a LaunchPermission registry value that restricts usage to SYSTEM, SERVICE, Administrators and the InteractiveUser */
   if ( !ConvertStringSecurityDescriptorToSecurityDescriptor(L"O:SYG:SYD:(A;;CCDCSW;;;IU)(A;;CCDCSW;;;BA)(A;;CCDCSW;;;SU)(A;;CCDCSW;;;SY)",
                                                             SDDL_REVISION_1,
                                                             &pLaunchPermSD,
                                                             &cbLaunchPermSD) )
   {
      dwErr = GetLastError();
      /* Failure */
      return ( __HRESULT_FROM_WIN32(dwErr) );
   }
   
#if 0
   {
      CDacl         dacl;
      CSecurityDesc secd;

      dacl.AddAllowedAce(ATL::Sids::System(), COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL);
      dacl.AddAllowedAce(ATL::Sids::Service(), COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL);
      dacl.AddAllowedAce(ATL::Sids::Admins(), COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL);
	   dacl.AddAllowedAce(ATL::Sids::Interactive(), COM_RIGHTS_EXECUTE|COM_RIGHTS_EXECUTE_LOCAL|COM_RIGHTS_ACTIVATE_LOCAL);
      secd.SetDacl(dacl);
	   secd.SetOwner(Sids::System());
      secd.SetGroup(Sids::System());
      secd.MakeSelfRelative();
      
      CString szSDDL;
      secd.ToString(&szSDDL);
      szSDDL += L"\n";
      OutputDebugString(szSDDL);
   }
#endif /* 0 */

   lRet = RegOpenKeyEx(HKEY_CLASSES_ROOT, 
                       L"AppID\\{7C1A3EB5-37A0-4BC2-B0E5-F3C5BF1FCB5D}", 
                       0,
                       KEY_WRITE|KEY_QUERY_VALUE,
                       &hKey);

	if ( ERROR_SUCCESS != lRet )
   {
		hr = __HRESULT_FROM_WIN32(lRet);
      /* Failure */
      goto __CLEANUP;
   }

   lRet = RegSetValueEx(hKey,
                        L"LaunchPermission",
                        0,
                        REG_BINARY,
                        reinterpret_cast<const BYTE*>(pLaunchPermSD),
                        cbLaunchPermSD);

   if ( ERROR_SUCCESS != lRet )
   {
      hr = __HRESULT_FROM_WIN32(lRet);
      /* Failure */
      goto __CLEANUP;
   }

__CLEANUP:
   if ( pLaunchPermSD )
   {
      LocalFree(reinterpret_cast<HLOCAL>(pLaunchPermSD));
   }

   if ( hKey )
   {
      RegCloseKey(hKey);
   }

   /* Success / Failure */
   return ( hr );
#else /* _DEBUG */
   UNREFERENCED_PARAMETER(bService);
   return ( S_OK );
#endif /* _DEBUG */
}

DWORD CServiceModule::HandlerEx( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext ) throw()
{
   /* Service manager wants default behavior, so forward to the base class */
   return ( _AtlServiceModuleExT::HandlerEx(dwControl, 
                                            dwEventType, 
                                            lpEventData, 
                                            lpContext) );
}

/**********************************************************************

	CServiceModule : CServiceModule

 **********************************************************************/
CServiceModule::CServiceModule( ) throw()
{
   DWORD dwRet;

   /* Override the default ATL controls to allow for stop, param-change, shutdown */
   m_status.dwControlsAccepted = SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN;
 
   /* Reset the resource module and reload the service name, as CAtlServiceModuleT will
    * have failed to load it properly */   
   dwRet = LoadMUIModule();
   if ( NO_ERROR != dwRet )
   {
      CAtlBaseModule::m_bInitFailed = true;
   }
   else
   {
      _AtlBaseModule.SetResourceInstance(GetMUIInstance());

      LoadString(GetMUIInstance(),
                 IDS_SERVICENAME,
                 m_szServiceName,
                 _countof(m_szServiceName));
      ATLASSERT(L'\0' != *m_szServiceName);
   }
}

CServiceModule::~CServiceModule( ) throw()
{
   FreeMUIModule();
}
