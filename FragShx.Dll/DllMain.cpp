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
 
/* DllMain.cpp
 *    Implementation of DllMain w/ load restrictions
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

/* When the _LOADRESTRICTIONS_ON flag is defined, DllMain succeeds only
 * if one of the following conditions is met:
 *  1) The process is being debugged.
 *  2) One of the modules listed in _szUnrestrictedModules is loaded
 *     in the process.
 * 
 * This prevents apps like the shell from loading and locking the module,
 * preventing new builds. The _szUnrestrictedModules is needed so that
 * non-debug apps like REGSVR32 can load the module.
 * 
 * Defining _LOADRESTRICTIONS_OFF will disable this functionality.
 */
#ifndef _LOADRESTRICTIONS_OFF
   #ifdef NDEBUG
      #pragma message("Warning! _LOADRESTRICTIONS_OFF is not defined! DllMain may fail!")
   #endif
    
   #define _LOADRESTRICTIONS_ON
#endif

#ifdef _LOADRESTRICTIONS_ON
/**
 * _szUnrestrictedModules
 *    Modules which can load this library.
 */
LPCWSTR _szUnrestrictedModules[] =
{
    L"REGSVR32.EXE",
    L"DLLHOST.EXE",
    L"VERCLSID.EXE"
};
#endif /* _LOADRESTRICTIONS_ON */
 
/**
 * DllMain
 *    Library entry point called by the C/C++ runtime.
 *
 * Remarks
 *    This function must be extern "C" to link properly with the C/C++
 *    runtime.
 */
extern "C" BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{
   BOOL bRet;

   UNREFERENCED_PARAMETER(hInstance);

   /* Do load restrictions, if enabled */
#ifdef _LOADRESTRICTIONS_ON
   if ( (DLL_PROCESS_ATTACH == dwReason) && !IsUnrestrictedProcess(_szUnrestrictedModules, 
                                                                   _countof(_szUnrestrictedModules)) )
   {
#ifdef _DEBUG
      {
         WCHAR chProgram[MAX_PATH];
         WCHAR chLibrary[MAX_PATH];
       
         ZeroMemory(chProgram,
                    sizeof(chProgram));

         ZeroMemory(chLibrary,
                    sizeof(chLibrary));

         GetModuleFileName(NULL, 
                           chProgram, 
                           _countof(chProgram));

         GetModuleFileName(hInstance, 
                           chLibrary, 
                           _countof(chLibrary));

         DbgPrintf(L"Failing %s!%s!" _T(__FUNCTION__) _T("!Load restrictions\n"), chProgram, chLibrary);
      }
#endif /* _DEBUG */

      /* Failure - Prevent the DLL from being loaded */
      return ( FALSE );
   }
#endif /* _LOADRESTRICTIONS_ON */

   bRet = _AtlModule.DllMain(dwReason, 
                             lpReserved);

#ifdef _DEBUG
   switch ( dwReason )
   {
      case DLL_PROCESS_DETACH:
         _CrtDumpMemoryLeaks();
         break;
   }
#endif /* _DEBUG */

   /* Success / Failure */
   return ( bRet ); 
}
