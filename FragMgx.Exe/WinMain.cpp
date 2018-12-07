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
 
/* WinMain.cpp
 *    Application entry
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 04/10/2005 - Created
 */

#include "Stdafx.h"

#include <AtlEx.h>
#include <SeUtil.h>

#include <sddl.h>

/**
 * _tWinMain
 *    Program entry point
 */
extern "C" int WINAPI _tWinMain( HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPTSTR lpCmdLine, int nShowCmd )
{
   int    iRet;
   HANDLE hToken;

   UNREFERENCED_PARAMETER(lpCmdLine);

   {
      SC_HANDLE hSCM = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
      if ( hSCM )
      {
         SC_HANDLE hService = OpenService(hSCM, L"FRAGSVX", SERVICE_START);
         if ( hService )
         {
            StartService(hService, 0, NULL);
            CloseServiceHandle(hService);
         }

         CloseServiceHandle(hSCM);
      }
   }
   
#ifdef _DEBUG
    /* Turn on automatic checking of memory leaks... */
   _CrtSetDbgFlag(((_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & 0x0000ffff)|_CRTDBG_CHECK_EVERY_128_DF)|(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF/*|_CRTDBG_DELAY_FREE_MEM_DF*/));
   /* Turn on abort/retry/cancel reporting for CRT assertion failures */
   _CrtSetReportMode(_CRT_ASSERT, _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_REPORT_MODE)|_CRTDBG_MODE_WNDW);
#endif /* _DEBUG */

   iRet   = -1;
   hToken = NULL;

   if ( OpenProcessToken(GetCurrentProcess(),
                         TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES,
                         &hToken) )
   {
      /*RestrictTokenPrivileges(hToken,
                              NULL,
                              0);*/

      CloseHandle(hToken);
      hToken = NULL;
   }

   iRet = _AtlModule.WinMain(nShowCmd);

#ifdef _DEBUG
   _CrtDumpMemoryLeaks();
#endif /* _DEBUG */

#ifdef _DEBUG
   /* Send a control to shutdown the service */
   {
      SC_HANDLE hSCM = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
      if ( hSCM )
      {
         SC_HANDLE hService = OpenService(hSCM, L"FRAGSVX", SERVICE_STOP);
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
   return ( iRet );
}
