/* FragExt - Windows defragmenter
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
 
/* DbgUtil.cpp
 *    Debugging utility implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include <DbgUtil.h>
#include <GuidDb.h>


/*
 * Global DbgPrintfEx level. Calls to DbgPrintfEx with a level greater than
 * this value will not be processed. When it is equal to 0, no calls are
 * processed
 */
UCHAR g_DbgPrintFilter = 0;

/*++
   DbgPrint variants
      OutputDebugString wrapper with support for printf style formatting
 
 --*/
void 
__cdecl 
DbgPrintfExA( 
   __in UCHAR Level,
   __in_z LPCSTR Format, 
   ... 
)
{
   va_list args;
   char    chBuf[1024];

   if ( !(Level > g_DbgPrintFilter) )
   {
      va_start(args,
               Format);

      if ( SUCCEEDED(StringCbVPrintfA(chBuf,
                                      sizeof(chBuf),
                                      Format,
                                      args)) )
      {
         OutputDebugStringA(chBuf);
      }
      
      va_end(args);
   }
}

void 
__cdecl 
DbgPrintfExW( 
   __in UCHAR Level,
   __in_z LPCWSTR Format, 
   ... 
)
{
   va_list args;
   wchar_t chBuf[1024];

   if ( !(Level > g_DbgPrintFilter) )
   {
      va_start(args,
               Format);

      if ( SUCCEEDED(StringCbVPrintfW(chBuf,
                                      sizeof(chBuf),
                                      Format,
                                      args)) )
      {
         OutputDebugStringW(chBuf);
      }
      
      va_end(args);
   }
}

void
APIENTRY
SetDebuggerThreadName( 
   __in DWORD dwThreadID, 
   __in LPCSTR pszThreadName 
)
{
#pragma pack(push, 8)
   typedef struct _THREADNAMEINFO
   {
      DWORD    dwType;     /* Must be 0x1000. */
      LPCSTR   szName;     /* Pointer to name (in user addr space). */
      DWORD    dwThreadID; /* Thread ID (-1=caller thread). */
      DWORD    dwFlags;    /* Reserved for future use, must be zero. */
   }THREADNAMEINFO;
#pragma pack(pop)

   THREADNAMEINFO NameInfo;

   NameInfo.dwType     = 0x1000;
   NameInfo.szName     = pszThreadName;
   NameInfo.dwThreadID = dwThreadID;
   NameInfo.dwFlags    = 0;

   __try
   {
      /*
       * The MSVC debugger, if attached, will get the first-chance at this
       * and set the thread name. So we just consume it afterward
       */
      RaiseException(0x406D1388/*MS_VC_EXCEPTION*/, 
                     0, 
                     sizeof(NameInfo) / sizeof(ULONG_PTR), 
                     reinterpret_cast<ULONG_PTR*>(&NameInfo));
   }
   __except( 0x406D1388 == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
   {
   }
}

DWORD
CheckSubAuthorityTokenMembership(
   __in HANDLE TokenHandle,
   __in DWORD SubAuthority,
   __out PBOOL IsMember 
)
{
   DWORD                      dwRet;
   BOOL                       bRet;
   PSID                       pSid;
   SID_IDENTIFIER_AUTHORITY   SidAuthorityNT = SECURITY_NT_AUTHORITY;

   (*IsMember) = FALSE;
   pSid        = NULL;

   if ( AllocateAndInitializeSid(&SidAuthorityNT,
                                 1,
                                 SubAuthority,
                                 0, 0, 0, 0, 0, 0, 0,
                                 &pSid) )
   {
      bRet = CheckTokenMembership(TokenHandle,
                                  pSid,
                                  IsMember);

      FreeSid(pSid);

      if ( bRet )
      {
         return ( NO_ERROR );
      }
   }

   dwRet = GetLastError();
   /* Failure */
   return ( dwRet );
}

BOOLEAN
APIENTRY
IsServiceExecutionMode(
) 
{
   BOOL bIsMember;
   
   bIsMember = FALSE;

   /* Check for the SERVICE SID. Note that this will only be present in services started
    * with an account other than LOCAL SYSTEM */
   CheckSubAuthorityTokenMembership(NULL,
                                    SECURITY_SERVICE_RID,
                                    &bIsMember);

   if ( !bIsMember )
   {
      /* Check for the predefined service accounts */
      CheckSubAuthorityTokenMembership(NULL,
                                       SECURITY_LOCAL_SYSTEM_RID,
                                       &bIsMember);

      if ( !bIsMember )
      {
         CheckSubAuthorityTokenMembership(NULL,
                                          SECURITY_LOCAL_SERVICE_RID,
                                          &bIsMember);

         if ( !bIsMember )
         {
            CheckSubAuthorityTokenMembership(NULL,
                                             SECURITY_NETWORK_SERVICE_RID,
                                             &bIsMember);
         }      
      }
   }
 
   /* Success / Failure */
   return ( static_cast<BOOLEAN>(bIsMember) );
}

/**
 * IsUnrestrictedProcess
 *    Checks if the current process is allowed to load this module.
 *
 * Parameters
 *    rgModules
 *       Pointer to an array of module names.
 *    cModules
 *       Number of elements in the array pointed to by rgModules
 *
 * Return Value
 *    Returns TRUE if any of the modules in the list are loaded in
 *    the current process, FALSE otherwise.
 *
 * Remarks
 *    None
 */
BOOLEAN
APIENTRY
IsUnrestrictedProcess( 
   LPCTSTR* rgModules, 
   size_t cModules 
)
{   
   typedef 
   BOOL 
   (WINAPI* PCHECKREMOTEDEBUGGERPRESENT)(
      __in HANDLE, 
      __out PBOOL
   );

   BOOL                        bDebuggerPresent;
   PCHECKREMOTEDEBUGGERPRESENT pCheckRemoteDebuggerPresent;

   /* Check for a local debugger.. */
   if ( IsDebuggerPresent() )
   {
      /* Success */
      return ( TRUE );
   }
   
   /* Check for a remote debugger.. */
   pCheckRemoteDebuggerPresent = reinterpret_cast<PCHECKREMOTEDEBUGGERPRESENT>(GetProcAddress(GetModuleHandle(L"KERNEL32.DLL"), 
                                                                                              "CheckRemoteDebuggerPresent"));

   if ( pCheckRemoteDebuggerPresent )
   {
      bDebuggerPresent = FALSE;

      if ( pCheckRemoteDebuggerPresent(GetCurrentProcess(),
                                       &bDebuggerPresent) )
      {
         if ( bDebuggerPresent )
         {
            /* Success */
            return ( TRUE );
         }
      }
   }

   /* Check if any of the supplied modules are loaded in the process */
   while ( cModules-- )
   {        
      /* Check for the module by attempting to retrieve its HMODULE */
      if ( GetModuleHandle(rgModules[cModules]) )
      {
         /* Success */
         return ( TRUE );
      }
   }
 
   /* Failure */
   return ( FALSE );
}
