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
 
/* MgxEntry.cpp
 *    Program entry point and initialization routines
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include "MgxModule.h"

/*++
   WPP
 *--*/
#include "MgxEntry.tmh"

/*++
   Global.h data declarations
 --*/
MGXFLAGS g_ControlFlags = {0};

BOOL
DetectPlatformVersion(
);

DWORD
InitializeProcessSecurity(
);

BOOL
ParseCommandLine(
   __in_z LPCWSTR CommandLine
);

/*++
   wmain
      Program entry point called by the C runtime
 --*/
extern "C" 
int 
__cdecl
wWinMain(
   __in HINSTANCE hInstance,
   __in_opt HINSTANCE hPrevInstance,
   __in LPWSTR lpCmdLine,
   __in int nShowCmd
) 
{
   DWORD    dwRet;
   
   UNREFERENCED_PARAMETER(hInstance);
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);
   UNREFERENCED_PARAMETER(nShowCmd);

   /* Initialize locals */
   dwRet = ERROR_APP_INIT_FAILURE;

   /* Initialize WPP tracing.. */
   WPP_INIT_TRACING(FRAGMGX_TRACING_ID);

   FpTrace(TRACE_LEVEL_VERBOSE,
           FpStartup,
           L"Entered wWinMain, hInstance = %p, hPrevInstance = %p, lpCmdLine = %p, nShowCmd = %i, continuing with startup initilization\n",
           hInstance,
           hPrevInstance,
           lpCmdLine,
           nShowCmd);

#ifdef _WAITFORDEBUGGER
#ifndef _DEBUG
   #pragma warning( "Non-debug build! Process will wait for debugger to attach!\n" )
#endif /* _DEBUG */
   FpTrace(TRACE_LEVEL_VERBOSE,
            FpDebug,
            L"Waiting for debugger to attach\n");

   while ( !IsDebuggerPresent() ) {
      Sleep(50);
   }

   /*
    * Sleep for an brief time to allow the debugger to finalize attachment
    */
   Sleep(500);

   FpTrace(TRACE_LEVEL_VERBOSE,
            FpDebug,
            L"Debugger attached, continuing\n");
#endif /* _WAITFORDEBUGGER */

   /* 
    * Turn on automatic checking of memory leaks
    */
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF/*|_CRTDBG_CHECK_ALWAYS_DF*/|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_DELAY_FREE_MEM_DF);
 
   if ( !DetectPlatformVersion() ) {
      dwRet = GetLastError();
      
      FpTrace(TRACE_LEVEL_ERROR,
              FpStartup,
              L"DetectPlatformVersion failed, error = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   dwRet = InitializeProcessSecurity();
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpStartup,
              L"InitializeProcessSecurity failed, error = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   if ( !ParseCommandLine(lpCmdLine) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpStartup,
              L"ParseCommandLine failed, exiting\n");

      dwRet = ERROR_INVALID_PARAMETER;
      /* Failure */
      goto __CLEANUP;
   }

   FpTrace(TRACE_LEVEL_VERBOSE,
           FpStartup,
           L"Startup initilization complete, forwarding to MgxMain\n");

   /*
    * Forward to the module entry point
    */
   dwRet = MgxModuleMain();

__CLEANUP:
   FpTrace(TRACE_LEVEL_VERBOSE, 
           FpStartup, 
           L"wWinMain, dwRet = %!WINERROR!\n",
           dwRet);

   WPP_CLEANUP();

#ifdef _DEBUG
   _CrtDumpMemoryLeaks();
#endif /* _DEBUG */
      
#ifdef _DEBUG
   /*++DEBUG 

      Send a control to shutdown the service so rebuilds can overwrite it
   
   --DEBUG*/
   {
      SC_HANDLE hSCM = OpenSCManagerW(NULL, SERVICES_ACTIVE_DATABASEW, SC_MANAGER_CONNECT);
      if ( hSCM )
      {
         SC_HANDLE hService = OpenServiceW(hSCM, L"FRAGSVX", SERVICE_STOP);
         if ( hService )
         {
            SERVICE_STATUS sc;
            ControlService(hService, SERVICE_CONTROL_STOP, &sc);
            CloseServiceHandle(hService);
         }

         CloseServiceHandle(hSCM);
      }
   }
#endif /* _DEBUG */

   /* Success / Failure */
   return ( (int)dwRet );
}

BOOL
DetectPlatformVersion(
)
{   
   DWORD dwRet;

   /*
    * Ensure that g_NTVersion from NTVersion.h was initialized properly, and that
    * the system we're running on is at least Windows XP
    *
    * FRAGEXT_NTDDI_MINIMUM is passed in project.mk
    */
   if ( !IsNTDDIVersion(g_NTVersion,
                        FRAGEXT_NTDDI_MINIMUM) ) {
      dwRet = ERROR_NOT_SUPPORTED;

      FpTrace(TRACE_LEVEL_ERROR,
              FpStartup,
              L"IsNTDDIVersion failed, g_NTVersion = %08lx, exiting\n",
              g_NTVersion);

      /* Failure */
      return ( FALSE );
   }

   FpTrace(TRACE_LEVEL_VERBOSE,
           FpStartup,
           L"Detected supported platform version, g_NTVersion = %08lx\n",
           g_NTVersion);

   /* Success */
   return ( TRUE );
}

DWORD
InitializeProcessSecurity(
)
{
   DWORD    dwRet;

   BOOLEAN  bRet;
   HANDLE   hToken;
   LPCWSTR  rgszPrivileges[] = {SE_MANAGE_VOLUME_NAME, SE_IMPERSONATE_NAME};

   /* Initialize locals.. */
   dwRet  = NO_ERROR;
   hToken = NULL;

   /*
    * Remove the current directory from the search path for DLL loading
    */
   SetDllDirectory(L"");

#ifndef _WIN64
   /*
    * Turn on heap protection if available. This is ony neccessary for 32-bit builds,
    * and is only available in Vista and up
    */
   if ( IsNTDDIVersion(g_NTVersion,
                       NTDDI_VISTA) ) {
      if ( !HeapSetInformation(NULL,
                               HeapEnableTerminationOnCorruption,
                               NULL,
                               0) ) {
         dwRet = GetLastError();

         FpTrace(TRACE_LEVEL_WARNING,
                 FpStartup,
                 L"HeapSetInformation failed, error = %!WINERROR!, exiting  \n",
                 dwRet);

         /* Failure */
         goto __CLEANUP;
      }
   }
#endif /* _WIN64 */
   
   /*
    * Reduce the set of privileges we run with to those listed in rgszPrivileges
    * for the entire process
    */
   if ( !OpenProcessToken(GetCurrentProcess(), 
                          TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, 
                          &hToken) ) {
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_WARNING,
              FpStartup,
              L"OpenProcessToken failed, error = %!WINERROR!, exiting  \n",
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   bRet = RestrictTokenPrivileges(hToken,
                                  rgszPrivileges,
                                  ARRAYSIZE(rgszPrivileges));

   if ( !bRet ) {
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_WARNING,
              FpStartup,
              L"RestrictTokenPrivileges failed, error = %!WINERROR!, continuing\n",
              dwRet);
   }

__CLEANUP:
   if ( hToken ) {
      CloseHandle(hToken);
   }

   /* Success / Failure */
   return ( dwRet );
}

BOOL
ParseCommandLine(
   __in_z LPCWSTR CommandLine
)
{
   _ASSERTE(NULL != CommandLine);

   UNREFERENCED_PARAMETER(CommandLine);

   /* Success */
   return ( TRUE );
}
