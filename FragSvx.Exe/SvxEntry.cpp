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
 
/* SvxEntry.cpp
 *    Program entry point and initialization routines
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "COMLib.h"
#include "ServiceCtl.h"
/*++
   WPP
 *--*/
#include "SvxEntry.tmh"

/*++
   Global.h data declarations
 --*/
SVXFLAGS g_ControlFlags = {0};


/*++
   Private declarations
 --*/
DWORD
DetectPlatformVersion(
);

DWORD
InitializeProcessSecurity(
);

BOOL
ParseCommandLine(
   __in int ArgCount,
   __in_ecount(ArgCount) wchar_t* ArgList[]
);

/*++
   wmain
      Program entry point called by the C runtime
 --*/
extern "C" 
int 
__cdecl
wmain(
   int argc,
   wchar_t* argv[],
   wchar_t* envp[]
) 
{
   DWORD    dwRet;
   
   UNREFERENCED_PARAMETER(envp);

   /* Initialize locals */
   dwRet = ERROR_APP_INIT_FAILURE;

   /* Initialize WPP tracing.. */
   WPP_INIT_TRACING(FRAGSVX_TRACING_ID);

   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxStartup,
           L"Entered wmain, argc = %d, argv = %p, envp = %p, continuing with startup initilization\n",
           argc,
           argv,
           envp);

#ifdef _WAITFORDEBUGGER
#ifndef _DEBUG
   #pragma warning( "Non-debug build! Process will wait for debugger to attach!\n" )
#endif /* _DEBUG */
   if ( IsServiceExecutionMode() ) {
      FpTrace(TRACE_LEVEL_VERBOSE,
              SvxDebug,
              L"Running as service, waiting for debugger to attach\n");

      while ( !IsDebuggerPresent() ) {
         Sleep(50);
      }

      /*
       * Sleep for a brief time to allow the debugger to finalize attachment
       */
      Sleep(1000);

      FpTrace(TRACE_LEVEL_VERBOSE,
              SvxDebug,
              L"Debugger attached, continuing\n");
   }
#endif /* _WAITFORDEBUGGER */

   /* 
    * Turn on automatic checking of memory leaks
    */
   _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF/*|_CRTDBG_CHECK_ALWAYS_DF*/|_CRTDBG_LEAK_CHECK_DF|_CRTDBG_DELAY_FREE_MEM_DF);
 
   dwRet = DetectPlatformVersion();
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxStartup,
              L"DetectPlatformVersion failed, error = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   dwRet = InitializeProcessSecurity();
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxStartup,
              L"InitializeProcessSecurity failed, error = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      goto __CLEANUP;
   }

   if ( !ParseCommandLine(argc,
                          argv) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxStartup,
              L"ParseCommandLine failed, exiting\n");

      dwRet = ERROR_INVALID_PARAMETER;
      /* Failure */
      goto __CLEANUP;
   }

   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxStartup,
           L"Startup initilization complete, forwarding to SvxMain\n");

   /*
    * Forward to the service entry point
    */
   dwRet = SvxMain();

__CLEANUP:
   FpTrace(TRACE_LEVEL_VERBOSE, 
           SvxStartup, 
           L"wmain, dwRet = %!WINERROR!\n",
           dwRet);

   WPP_CLEANUP();

#ifdef _DEBUG
   _CrtDumpMemoryLeaks();
#endif /* _DEBUG */

   /* Success / Failure */
   return ( (int)dwRet );
}

DWORD
DetectPlatformVersion(
)
{
   DWORD dwRet;

   /*
    * Ensure that g_NTVersion from NTVersion.h was initialized properly, and that
    * the system we're running on is at least Windows XP
    */
   if ( !IsNTDDIVersion(g_NTVersion,
                        NTDDI_WINXP) ) {
      dwRet = ERROR_NOT_SUPPORTED;

      FpTrace(TRACE_LEVEL_ERROR,
              SvxStartup,
              L"IsNTDDIVersion failed, g_NTVersion = %08lx, exiting\n",
              g_NTVersion);

      /* Failure */
      return ( dwRet );
   }

   FpTrace(TRACE_LEVEL_VERBOSE,
           SvxStartup,
           L"Detected supported platform version, g_NTVersion = %08lx\n",
           g_NTVersion);

   /* Success */
   return ( NO_ERROR );
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
                 SvxSecurity,
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
              SvxSecurity,
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
              SvxSecurity,
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
   __in int ArgCount,
   __in_ecount_z(ArgCount) wchar_t* ArgList[]
)
{
   int            idx;
   const wchar_t* pszArg;
 
   /*
    * The C-runtime will pass the program path as the first command line argument,
    * and we don't care about it so we start our processing at the next argument
    */
   for ( idx = 1; idx < ArgCount; idx++ ) {
      pszArg = ArgList[idx];

      if ( (L'-' == (*pszArg)) || (L'/' == (*pszArg)) ) {
         pszArg++;
      }

      if ( 0 == _wcsicmp(pszArg,
                         L"COM") ) {
         g_ControlFlags.IsCOMActivation = 1;

         FpTrace(TRACE_LEVEL_VERBOSE,
                 SvxStartup,
                 L"Command line argument found, -COM, g_ControlFlags.IsCOMActivation = 1\n");
      }
#ifdef _DEBUG
      else {
         FpTrace(TRACE_LEVEL_WARNING,
                 SvxStartup,
                 L"Unknown command line argument specified, %ws, continuing\n",
                 pszArg);
      }
#endif /* _DEBUG */
   }

   /* Success */
   return ( TRUE );
}
