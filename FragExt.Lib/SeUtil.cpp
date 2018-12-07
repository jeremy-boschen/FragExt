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

/* SecUtil.cpp
 *   Various security related utility wrappers
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include "SeUtil.h"
#include "MemUtil.h"

/*++

   PRIVATE DECLARATIONS

 --*/

/*++
   Shared Declarations
  --*/
#ifndef TypeAlignSize
   #define TypeAlignSize( Type, Size ) \
      ( (((ULONG)(Size)) + (ULONG)(sizeof(Type) - 1)) & ~(ULONG)(sizeof(Type) - 1) )
#endif /* TypeAlignSize */

#ifndef PtrAlignSize
   #define PtrAlignSize( Size ) TypeAlignSize(PVOID, Size)
#endif /* PtrAlignSize */

#ifndef FlagOn
   #define FlagOn( _F, _S ) (0 != ((_F) & (_S)))
#endif /* FlagOn */

#ifndef BooleanFlagOn
   #define BooleanFlagOn( _F, _SF ) (0 != ((_F) & (_S)) ? TRUE : FALSE)
#endif /* BooleanFlagOn */

#ifndef WINERROR
   #define WINERROR( ErrorCode ) \
      (NO_ERROR != ErrorCode)
#endif /* WINERROR */

/*++ 
   SID Template Declarations
  --*/
typedef struct _GENERIC_ACE
{
   ACE_HEADER  Header;
   ACCESS_MASK AccessMask;
   DWORD       SidStart;
}GENERIC_ACE, *PGENERIC_ACE;

/*++

   PUBLIC 

 --*/

PVOID
APIENTRY
GetTokenInformationData(
   __in HANDLE TokenHandle,
   __in TOKEN_INFORMATION_CLASS TokenInformationClass,
   __out_opt PDWORD ReturnLength
)
{
   DWORD dwRet;

   DWORD cbAllocate;
   PVOID pTokenBuffer;

   /* Initialize outputs */
   if ( ReturnLength )
   {
      (*ReturnLength) = 0;
   }

   /* Start with a decent sized buffer */
   cbAllocate = 128;

   /* Get the token information */
   for ( ;; )
   {
      pTokenBuffer = malloc(cbAllocate);
      if ( !pTokenBuffer )
      {
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         /* Failure */
         return ( FALSE );
      }

      ZeroMemory(pTokenBuffer,
                 cbAllocate);
      
      if ( GetTokenInformation(TokenHandle,
                               TokenInformationClass,
                               pTokenBuffer,
                               cbAllocate,
                               &cbAllocate) )
      {
         if ( ReturnLength )
         {
            (*ReturnLength) = cbAllocate;
         }

         /* Success */
         return ( pTokenBuffer );        
      }
      
      dwRet = GetLastError();

      /* Cleanup our current buffer */
      free(pTokenBuffer);
      pTokenBuffer = NULL;
     
      /* 
       * The only error we handle is ERROR_INSUFFICIENT_BUFFER. If that's not the case,
       * then we bail
       */
      if ( ERROR_INSUFFICIENT_BUFFER != dwRet )
      {
         SetLastError(dwRet);
         /* Failure */
         break;
      }
   }

   /* Failure */
   return ( NULL );
}

VOID
APIENTRY
FreeTokenInformationData(
   __in PVOID Data
)
{
   free(Data);
}

HANDLE
APIENTRY
GetThreadToken(
   __in HANDLE ThreadHandle,
   __in DWORD DesiredAccess 
)
{
   HANDLE hToken;
   
   hToken = NULL;

   /*
    * If the thread is impersonating, use the thread token otherwise revert
    * to the process token
    */
   if ( !OpenThreadToken(ThreadHandle,
                         DesiredAccess, 
                         TRUE, 
                         &hToken) )
   {     
      /* Failure */
      return ( NULL );
   }

   /* Success */
   return ( hToken );
}

HANDLE
APIENTRY
GetEffectiveThreadToken(
   __in HANDLE ThreadHandle,
   __in DWORD DesiredAccess 
)
{
   HANDLE hToken;
   
   hToken = NULL;

   /*
    * If the thread is impersonating, use the thread token otherwise revert
    * to the process token
    */
   if ( !OpenThreadToken(ThreadHandle,
                         DesiredAccess, 
                         TRUE, 
                         &hToken) )
   {
      if ( !OpenProcessToken(GetCurrentProcess(),
                             DesiredAccess, 
                             &hToken) )
      {
         /* Failure */
         return ( NULL );
      }
   }

   /* Success */
   return ( hToken );
}

BOOLEAN
APIENTRY
SetTokenPrivilege(
   __in HANDLE TokenHandle,
   __in LUID PrivilegeValue,
   __in BOOLEAN Enable,
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
)
{
   DWORD            dwRet;

   TOKEN_PRIVILEGES TokenPrivilege;
   DWORD            cbPreviousState;
   
   TokenPrivilege.PrivilegeCount           = 1;
   TokenPrivilege.Privileges[0].Luid       = PrivilegeValue;   
   TokenPrivilege.Privileges[0].Attributes = (Enable ? SE_PRIVILEGE_ENABLED : 0);

   if ( PreviousState )
   {
      cbPreviousState = sizeof(*PreviousState) + (sizeof(LUID_AND_ATTRIBUTES) * PreviousState->PrivilegeCount) - sizeof(LUID_AND_ATTRIBUTES);
   }
   else
   {
      cbPreviousState = 0;
   }

   if ( !AdjustTokenPrivileges(TokenHandle,
                               FALSE,
                               &TokenPrivilege, 
                               cbPreviousState, 
                               PreviousState, 
                               &cbPreviousState) )
   {
      /* Failure */
      return ( FALSE );
   }

   dwRet = GetLastError();

   return ( ERROR_SUCCESS == dwRet ? TRUE : FALSE );
}

BOOLEAN
APIENTRY
SetTokenPrivilege( 
   __in HANDLE TokenHandle, 
   __in_z LPCWSTR PrivilegeName, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState 
)
{
   LUID  Luid;
   
   if ( !LookupPrivilegeValue(NULL, 
                              PrivilegeName, 
                              &Luid ) )
   {
      /* Failure */
      return ( FALSE ); 
   }

   return ( SetTokenPrivilege(TokenHandle,
                              Luid,
                              Enable,
                              PreviousState) );
}

BOOLEAN 
APIENTRY
SetThreadTokenPrivilege( 
   __in HANDLE ThreadHandle, 
   __in_z LPCWSTR PrivilegeName, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
)
{
   BOOLEAN  bRet;
   HANDLE   hToken;

   /* Initialize locals.. */
   hToken = NULL;

   if ( !OpenThreadToken(ThreadHandle, 
                         TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, 
                         TRUE, 
                         &hToken) )
   {
      /* Failure */
      return ( FALSE );
   }

   bRet = SetTokenPrivilege(hToken, 
                            PrivilegeName, 
                            Enable, 
                            PreviousState);
   
   CloseHandle(hToken);

   /* Success / Failure */
   return ( bRet );
}

BOOLEAN 
APIENTRY
SetThreadTokenPrivilege( 
   __in HANDLE ThreadHandle, 
   __in LUID PrivilegeValue, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
)
{
   BOOLEAN  bRet;
   HANDLE   hToken;

   /* Initialize locals.. */
   hToken = NULL;

   if ( !OpenThreadToken(ThreadHandle, 
                         TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, 
                         TRUE, 
                         &hToken) )
   {
      /* Failure */
      return ( FALSE );
   }

   bRet = SetTokenPrivilege(hToken, 
                            PrivilegeValue, 
                            Enable, 
                            PreviousState);
   
   CloseHandle(hToken);

   /* Success / Failure */
   return ( bRet );
}

BOOLEAN
APIENTRY
SetProcessTokenPrivilege(
   __in HANDLE ProcessHandle, 
   __in_z LPCWSTR PrivilegeName, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
)
{
   BOOLEAN  bRet;
   HANDLE   hToken;

   /* Initialize locals.. */
   hToken = NULL;

   if ( !OpenProcessToken(ProcessHandle, 
                          TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, 
                          &hToken) )
   {
      /* Failure */
      return ( FALSE );
   }

   bRet = SetTokenPrivilege(hToken, 
                            PrivilegeName, 
                            Enable, 
                            PreviousState);

   CloseHandle(hToken);

   /* Success / Failure */
   return ( bRet );
}

BOOLEAN
APIENTRY
SetProcessTokenPrivilege(
   __in HANDLE ProcessHandle, 
   __in LUID PrivilegeValue, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
)
{
   BOOLEAN  bRet;
   HANDLE   hToken;

   /* Initialize locals.. */
   hToken = NULL;

   if ( !OpenProcessToken(ProcessHandle, 
                          TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, 
                          &hToken) )
   {
      /* Failure */
      return ( FALSE );
   }

   bRet = SetTokenPrivilege(hToken, 
                            PrivilegeValue, 
                            Enable, 
                            PreviousState);

   CloseHandle(hToken);

   /* Success / Failure */
   return ( bRet );
}

BOOLEAN
APIENTRY
SetEffectiveTokenPrivilege(
   __in_z LPCWSTR PrivilegeName,
   __in BOOLEAN Enable,
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
)
{
   BOOLEAN  bRet;
   HANDLE   hToken;

   /* Initialize locals..*/
   hToken = NULL;
   
   /*
    * Get the active token for the current thread
    */
   hToken = GetEffectiveThreadToken(GetCurrentThread(),
                                    TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY);

   if ( !hToken )
   {
      return ( FALSE );
   }

   bRet = SetTokenPrivilege(hToken,
                            PrivilegeName,
                            Enable,
                            PreviousState);

   CloseHandle(hToken);

   /* Success / Failure */
   return ( bRet );
}

BOOLEAN
APIENTRY
SetEffectiveTokenPrivilege(
   __in_z LUID PrivilegeValue,
   __in BOOLEAN Enable,
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
)
{
   BOOLEAN  bRet;
   HANDLE   hToken;

   /* Initialize locals..*/
   bRet   = FALSE;
   hToken = NULL;
   
   /*
    * Get the active token for the current thread
    */
   hToken = GetEffectiveThreadToken(GetCurrentThread(),
                                    TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY);

   if ( !hToken )
   {
      /* Failure */
      return ( FALSE );
   }

   bRet = SetTokenPrivilege(hToken,
                            PrivilegeValue,
                            Enable,
                            PreviousState);

   CloseHandle(hToken);

   /* Success / Failure */
   return ( bRet );
}

/*++ 
   Token information wrappers 
 --*/
SECURITY_IMPERSONATION_LEVEL
APIENTRY
GetTokenImpersonationLevel(
   __in HANDLE TokenHandle
)
{
   DWORD                        cbData;
   SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;

   if ( !GetTokenInformation(TokenHandle,
                             TokenImpersonationLevel,
                             &ImpersonationLevel,
                             sizeof(ImpersonationLevel),
                             &cbData) )
   {
      /* Failure - Return a bogus impersonation level that can be checked
       * via VALID_IMPERSONATION_LEVEL() */
      return ( static_cast<SECURITY_IMPERSONATION_LEVEL>(-1) );
   }

   /* Success */
   return ( ImpersonationLevel );
}

SECURITY_IMPERSONATION_LEVEL
APIENTRY
GetThreadTokenImpersonationLevel(
   __in HANDLE ThreadHandle
)
{
   HANDLE                        hToken;
   SECURITY_IMPERSONATION_LEVEL  ImpersonationLevel;

   if ( OpenThreadToken(ThreadHandle, 
                        TOKEN_QUERY, 
                        TRUE, 
                        &hToken) )
   {
      ImpersonationLevel = GetTokenImpersonationLevel(hToken);
      
      CloseHandle(hToken);

      /* Success */
      return ( ImpersonationLevel );
   }

   /* Failure - Return a bogus impersonation level that can be checked
    * via VALID_IMPERSONATION_LEVEL() */
   return ( static_cast<SECURITY_IMPERSONATION_LEVEL>(-1) );
}

BOOL
APIENTRY
IsThreadImpersonating(
   __in HANDLE ThreadHandle
)
{
   DWORD  dwRet;
   HANDLE hToken;

   if ( !OpenThreadToken(ThreadHandle,
                         0,
                         TRUE,
                         &hToken) )
   {
      dwRet = GetLastError();
      if ( ERROR_NO_TOKEN == dwRet )
      {
         return ( FALSE );
      }
   }

   CloseHandle(hToken);

   return ( TRUE );
}

TOKEN_TYPE
APIENTRY
GetTokenType(
   __in HANDLE TokenHandle
)
{
   DWORD      cbType;
   TOKEN_TYPE Type;

   if ( !GetTokenInformation(TokenHandle,
                             TokenType,
                             &Type,
                             sizeof(Type),
                             &cbType) )
   {
      return ( static_cast<TOKEN_TYPE>(-1) );
   }

   return ( Type );
}

TOKEN_TYPE
APIENTRY
GetThreadTokenType(
   __in HANDLE ThreadHandle
)
{
   HANDLE     hToken;
   TOKEN_TYPE Type;

   if ( OpenThreadToken(ThreadHandle, 
                        TOKEN_QUERY, 
                        TRUE, 
                        &hToken) )
   {
      Type = GetTokenType(hToken);
      
      CloseHandle(hToken);

      /* Success */
      return ( Type );
   }
   
   return ( static_cast<TOKEN_TYPE>(0) );
}

BOOLEAN 
APIENTRY
RemoveTokenPrivileges( 
   __in HANDLE TokenHandle, 
   __in_z LPCWSTR PrivilegesToRemove[], 
   __in DWORD PrivilegesToRemoveCount
)
{
   BOOL              bRet;

   DWORD             dwRet;

   DWORD             cbTokenPrivileges;
   PTOKEN_PRIVILEGES pTokenPrivileges;
   
   DWORD             Attributes;
   DWORD             idx;

   dwRet  = NO_ERROR;

   /*
    * Determine the size required to build the TOKEN_PRIVILEGES record
    */
   cbTokenPrivileges = sizeof(TOKEN_PRIVILEGES) - sizeof(LUID_AND_ATTRIBUTES) + (sizeof(LUID_AND_ATTRIBUTES) * PrivilegesToRemoveCount);
      
   pTokenPrivileges = reinterpret_cast<PTOKEN_PRIVILEGES>(malloc(cbTokenPrivileges));
   if ( !pTokenPrivileges )
   {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      /* Failure */
      return ( FALSE );
   }

   ZeroMemory(pTokenPrivileges,
              cbTokenPrivileges);

   if ( IsNTDDIVersion(g_NTVersion,
                       NTDDI_WINXPSP2) )
   {
      Attributes = SE_PRIVILEGE_REMOVED;
   }
   else
   {
      Attributes = 0;
   }
   
   pTokenPrivileges->PrivilegeCount = PrivilegesToRemoveCount;
   
   /*
    * Initialize the LUID_AND_ATTRIBUTES array
    */
   for ( idx = 0; idx < PrivilegesToRemoveCount; idx++ )
   {
      LookupPrivilegeValue(NULL,
                           PrivilegesToRemove[idx],
                           &(pTokenPrivileges->Privileges[idx].Luid));

      pTokenPrivileges->Privileges[idx].Attributes = Attributes;
   }

   bRet = AdjustTokenPrivileges(TokenHandle,
                                FALSE,
                                pTokenPrivileges,
                                0,
                                NULL,
                                NULL);

   /*
    * Cache any error so we can reset after cleanup. The last error is set on
    * both success and failure by AdjustTokenPrivileges
    */
   dwRet = GetLastError();

   free(pTokenPrivileges);
   pTokenPrivileges = NULL;
    
   SetLastError(dwRet);

   /* Success / Failure */
   return ( bRet ? TRUE : FALSE );
}

BOOLEAN
APIENTRY
RestrictTokenPrivileges( 
   __in HANDLE TokenHandle, 
   __in_ecount(PermittedPrivilegesCount) LPCWSTR PermittedPrivileges[], 
   __in DWORD PermittedPrivilegesCount
)
{
   BOOL              bRet;

   DWORD             dwRet;

   DWORD             cbTokenPrivileges;
   PTOKEN_PRIVILEGES pTokenPrivileges;

   size_t            cbLuid;
   PLUID             rgLuid;

   DWORD             Attributes;

   DWORD             idx;
   DWORD             jdx;
   BOOLEAN           bAllowPrivilege;

   /* Initialize locals.. */
   bRet             = FALSE;
   dwRet            = NO_ERROR;
   pTokenPrivileges = NULL;
   rgLuid           = NULL;

   /* 
    * Get the current token privileges. We'll start with an average sized buffer
    */
   cbTokenPrivileges = 128;

   for ( ;; )
   {
      pTokenPrivileges = reinterpret_cast<PTOKEN_PRIVILEGES>(malloc((size_t)cbTokenPrivileges));
      if ( !pTokenPrivileges )
      {
         dwRet = ERROR_NOT_ENOUGH_MEMORY;         
         /* Failure */
         goto __CLEANUP;
      }

      ZeroMemory(pTokenPrivileges,
                 cbTokenPrivileges);

      if ( GetTokenInformation(TokenHandle,
                               TokenPrivileges,
                               pTokenPrivileges,
                               cbTokenPrivileges,
                               &cbTokenPrivileges) )
      {
         /*
          * We retrieved the data so break out
          */
         dwRet = NO_ERROR;

         /* Success */         
         break;
      }

      dwRet = GetLastError();

      if ( ERROR_INSUFFICIENT_BUFFER != dwRet )
      {
         /* 
          * Some failure other than one we're prepared to handle, so bail 
          */
         
         /* Failure */
         goto __CLEANUP;
      }

      /*
       * Free the current buffer and try again with the size returned
       * by GetTokenInformation
       */
      free(pTokenPrivileges);
      pTokenPrivileges = NULL;
   }

   /* 
    * Build the LUID array. We add an extra one so we can add SE_CHANGE_NOTIFY_NAME, which is
    * always required. This will be the list privileges that we allow in the token. Anything
    * not in the list will either be removed or disabled
    */
   _ASSERTE(((size_t)PermittedPrivilegesCount + 1) < ((size_t)~0 / sizeof(LUID)));

   cbLuid = ((size_t)PermittedPrivilegesCount + 1) * sizeof(LUID);

   rgLuid = reinterpret_cast<PLUID>(malloc(cbLuid));
   if ( !rgLuid )
   {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   ZeroMemory(rgLuid,
              cbLuid);

   /* 
    * The first element is the extra we accounted for, and is always SE_CHANGE_NOTIFY_NAME 
    */
   LookupPrivilegeValue(NULL,
                        SE_CHANGE_NOTIFY_NAME,
                        &(rgLuid[0]));

   /* 
    * Now lookup the rest of the names the caller provided 
    */
   for ( idx = 0; idx < PermittedPrivilegesCount; idx++ )
   {
      /*
       * Note that the offset into rgLuid starts at the second element
       * because the first is for SE_CHANGE_NOTIFY_NAME. This was accounted
       * for in the allocation above
       */
      if ( !LookupPrivilegeValue(NULL,
                                 PermittedPrivileges[idx],
                                 &(rgLuid[idx + 1])) )
      {
         dwRet = GetLastError();

         if ( ERROR_NO_SUCH_PRIVILEGE != dwRet )
         {
            /*
             * Some error we're not prepared to handle
             */

            /* Failure */
            goto __CLEANUP;
         }

         /*
          * The privilege might not exist on this platform. We don't want to bail
          * so we'll just give a unique value to the LUID which will not be set
          * in the token, so our checks below will still work correctly
          */
         if ( !AllocateLocallyUniqueId(&(rgLuid[idx + 1])) )
         {
            
            /*
             * Something we can't handle fixing, so bail
             */

            dwRet = GetLastError();
            /* Failure */
            goto __CLEANUP;
         }
      }
   }

   /* 
    * Update the PermittedPrivilegesCount to include the one we added 
    * for SE_CHANGE_NOTIFY_NAME
    */
   PermittedPrivilegesCount += 1;

   /*
    * Determine if we're on a platform than can remove privileges, or only
    * disable them 
    */
   if ( IsNTDDIVersion(g_NTVersion,
                       NTDDI_WINXPSP2) )
   {
      Attributes = SE_PRIVILEGE_REMOVED;
   }
   else
   {
      Attributes = 0;
   }

   /* 
    * Enumerate the token privileges and remove any that aren't listed in
    * in rgLuid
    */
   for ( idx = 0; idx < pTokenPrivileges->PrivilegeCount; idx++ )
   {
      /*
       * Assume it's not present in rgLuid
       */
      bAllowPrivilege = FALSE;

      for ( jdx = 0; jdx < PermittedPrivilegesCount; jdx++ )
      {
         if ( IsEqualLUID(&(pTokenPrivileges->Privileges[idx].Luid),
                          &(rgLuid[jdx])) )
         {
            /* 
             * This one is OK 
             */
            bAllowPrivilege = TRUE;
            break;
         }
      }

      if ( bAllowPrivilege )
      {
         /*
          * Disable the privilege and clear the removed flag. Code that requires
          * the privilege to be enabled will have to enable it prior to use. 
          *
          * Most, but not all, system calls will do this
          */
         pTokenPrivileges->Privileges[idx].Attributes &= ~(SE_PRIVILEGE_REMOVED|SE_PRIVILEGE_ENABLED);
      }
      else
      {
         pTokenPrivileges->Privileges[idx].Attributes = Attributes;
      }
   }

   /* 
    * Now apply what we've adjusted and exit 
    */
   bRet = AdjustTokenPrivileges(TokenHandle,
                                FALSE,
                                pTokenPrivileges,
                                cbTokenPrivileges,
                                NULL,
                                NULL);
   
   /*
    * The last error is set when AdjustTokenPrivileges succeeds or fails
    */
   dwRet = GetLastError();

__CLEANUP:
   if ( rgLuid )
   {
      free(rgLuid);
   }

   if ( pTokenPrivileges )
   {
      free(pTokenPrivileges);
   }

   /*
    * Restore the last error
    */
   SetLastError(dwRet);

   /* Success / Failure */
   return ( bRet ? TRUE : FALSE );
}

DWORD 
APIENTRY
SelfRelativeToAbsoluteSD(
   __in PSECURITY_DESCRIPTOR SelfRelativeSD, 
   __out PSECURITY_DESCRIPTOR AbsoluteSD,
   __inout DWORD* AbsoluteSDSize
)
{
   /* This routine assumes the SD is self-relative */

   DWORD    dwRet;

   DWORD    cbSD;
   PACL     pDacl;
   DWORD    cbDacl;
   PACL     pSacl;
   DWORD    cbSacl;
   PSID     pOwner;
   DWORD    cbOwner;
   PSID     pGroup;
   DWORD    cbGroup;
   
   PUCHAR   pbBuffer;
   DWORD    cbRequired;

   /* Initialize outputs */
   cbSD = cbDacl = cbSacl = cbOwner = cbGroup = 0;
   
   /* Get the required size of all the pieces */
   MakeAbsoluteSD(SelfRelativeSD,
                  NULL,&cbSD,
                  NULL,&cbDacl,
                  NULL,&cbSacl,
                  NULL,&cbOwner,
                  NULL,&cbGroup);

   dwRet = GetLastError();
   if ( ERROR_INSUFFICIENT_BUFFER != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   cbRequired = cbSD + cbDacl + cbSacl + cbOwner + cbGroup;
   if ( (*AbsoluteSDSize) < cbRequired )
   {
      (*AbsoluteSDSize) = cbRequired;
      /* Failure */
      return ( ERROR_INSUFFICIENT_BUFFER );
   }
   
   ZeroMemory(AbsoluteSD,
              cbRequired);

   /* Set the pointers to the offsets within the AbsoluteSD buffer for each part of
    * the security descriptor */
   pbBuffer = reinterpret_cast<PUCHAR>(AbsoluteSD);
   
   pDacl = (!cbDacl  ? NULL : reinterpret_cast<PACL>(pbBuffer + cbSD));
   _ASSERTE((PUCHAR)pDacl <= (pbBuffer + (*AbsoluteSDSize)));

   pSacl = (!cbSacl  ? NULL : reinterpret_cast<PACL>(pbBuffer + cbSD + cbDacl));
   _ASSERTE((PUCHAR)pSacl <= (pbBuffer + (*AbsoluteSDSize)));

   pOwner = (!cbOwner ? NULL : reinterpret_cast<PSID>(pbBuffer + cbSD + cbDacl + cbSacl));
   _ASSERTE((PUCHAR)pOwner <= (pbBuffer + (*AbsoluteSDSize)));
   
   pGroup = (!cbGroup ? NULL : reinterpret_cast<PSID>(pbBuffer + cbSD + cbDacl + cbSacl + cbOwner));
   _ASSERTE((PUCHAR)pGroup <= (pbBuffer + (*AbsoluteSDSize)));

   if ( !MakeAbsoluteSD(SelfRelativeSD,
                        AbsoluteSD, &cbSD,
                        pDacl,      &cbDacl,
                        pSacl,      &cbSacl,
                        pOwner,     &cbOwner,
                        pGroup,     &cbGroup) )
   {
      dwRet = GetLastError();      
      /* Failure */
      return ( dwRet );
   }

   _ASSERTE(IsValidSecurityDescriptor(AbsoluteSD));

   /* Success */
   return ( NO_ERROR );
}

/*++

   Generic Pointer Encoding/Decoding
 
      These functions do very simple pointer encoding using the application __security_cookie.
 --*/

#ifdef _WIN64
   #define _rotr_ptr _rotr64
#else /* !_WIN64 */
   #define _rotr_ptr _rotr
#endif /* _WIN64 */

#ifndef UINTPTR_T_BIT
   #define UINTPTR_T_BIT (sizeof(uintptr_t) * CHAR_BIT)
#endif /* UINTPTR_T_BIT */

PVOID 
APIENTRY
SeEncodePointer( 
   PVOID Pointer 
)
{
   return ( (PVOID)_rotr_ptr(__security_cookie ^ (uintptr_t)Pointer, (unsigned char)(__security_cookie & (uintptr_t)(UINTPTR_T_BIT - 1))) );
}

PVOID 
APIENTRY
SeDecodePointer( 
   PVOID Pointer 
)
{  
   return ( (PVOID)(__security_cookie ^ (uintptr_t)_rotr_ptr((uintptr_t)Pointer, (unsigned char)(UINTPTR_T_BIT - (__security_cookie & (UINTPTR_T_BIT - 1))))) );
}

/*++

   SID Templates

  --*/

#define IsSupportedAceType( AceType ) \
   ( \
    (ACCESS_ALLOWED_ACE_TYPE == (UCHAR)(AceType)) || \
    (ACCESS_DENIED_ACE_TYPE  == (UCHAR)(AceType)) || \
    (SYSTEM_AUDIT_ACE_TYPE   == (UCHAR)(AceType)) ? TRUE : FALSE \
   )

BOOL
IsPseudoSidTemplate(
   __in PCGENERIC_SID SidTemplate
)
{
   SID_IDENTIFIER_AUTHORITY SidTemplateAuthority = SIDTEMPLATE_AUTHORITY;

   if ( 0 == memcmp(&(SidTemplateAuthority),
                    &(SidTemplate->IdentifierAuthority),
                    sizeof(SID_IDENTIFIER_AUTHORITY)) )
   {
      
      if ( (SidTemplate->SubAuthority[0] >= SIDTEMPLATE_MIN_RID) &&
           (SidTemplate->SubAuthority[0] <= SIDTEMPLATE_MAX_RID) )
      {
         return ( TRUE );
      }
   }

   return ( FALSE );
}

/*++
   GetSidDomainSubAuthorityCount
      Retrieves the number of sub-authorities for an NT domain account SID
 --*/
UCHAR
GetSidDomainSubAuthorityCount(
   __in PSID Sid
)
{
   DWORD                    SubAuthority;
   SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
      
   /*
    * Make sure the SID is owned by the NT authority
    */
   if ( 0 != memcmp(GetSidIdentifierAuthority(Sid),
                    &(NtAuthority),
                    sizeof(SID_IDENTIFIER_AUTHORITY)) )
   {
      SetLastError(ERROR_NON_ACCOUNT_SID);
      /* Failure */
      return ( 0 );
   }

   /*
    * NT domain SIDs start with a SECURITY_NT_NON_UNIQUE RID
    */
   SubAuthority = *(GetSidSubAuthority(Sid, 0));

   if ( SECURITY_NT_NON_UNIQUE == SubAuthority )
   {  
      /* Success */
      return ( 1/*SECURITY_NT_NON_UNIQUE*/ + SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT );
   }

   if ( SECURITY_BUILTIN_DOMAIN_RID == SubAuthority )
   {
      /* Success */
      return ( 1/*SECURITY_BUILTIN_DOMAIN_RID*/ );
   }

   SetLastError(ERROR_NON_DOMAIN_SID);     
   /* Failure */
   return ( 0 );
}

/*++
   IsNTDomainSid
      Verifies that a SID is an NT domain SID or a BUILTIN NT domain SID
  --*/
BOOL
IsNTDomainSid(
   __in PSID Sid
)
{
   if ( 0 != GetSidDomainSubAuthorityCount(Sid) )
   {
      /* Success */
      return ( TRUE );
   }

   /* Failure */
   return ( FALSE );
}

/*++
   CopyNTDomainSid
      Copies the NT domain portion of a SID, if present, to another SID
  --*/
BOOLEAN
CopyNTDomainSid(
   __out_bcount(DomainSidSize) PSID DomainSid,
   __inout DWORD* DomainSidSize,
   __in PSID Sid
)
{
   DWORD cbCallerDomainSidSize;
   DWORD cbRequiredDomainSidSize;

   UCHAR cDomainSubAthorities;
   UCHAR idx;
   
   /*
    * Make sure the SID can hold the domain SID.
    *
    * Examples:
    *    Domain SID:
    *    S-1-5-21-1010101010-1010101010-1010101010
    *      | | |  |
    *      | | |  +--> Domain RIDs (SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT)
    *      | | |
    *      | | +--> Required (SECURITY_NT_NON_UNIQUE)
    *      | |
    *      | +--> Required, SID authority (SECURITY_NT_AUTHORITY)
    *      |
    *      +--> Required, SID revision (SID_REVISION)
    *
    *    BUILTIN Domain SID:
    *    S-1-5-32
    *      | | |
    *      | | +--> Required (SECURITY_BUILTIN_DOMAIN_RID)
    *      | |
    *      | +--> Required, SID authority (SECURITY_NT_AUTHORITY)
    *      |
    *      +--> Required, SID revision (SID_REVISION)
    */
   cDomainSubAthorities = GetSidDomainSubAuthorityCount(Sid);
   if ( 0 == cDomainSubAthorities )
   {
      _ASSERTE(IsNTDomainSid(Sid));
      /* Failure */
      return ( FALSE );
   }

   /*
    * Cache the caller's buffer size so we can overwrite it
    */
   cbCallerDomainSidSize = (*DomainSidSize);
   
   /*
    * Determine the required size of the buffer to hold the domain SID and store
    * it in the caller's return length. Then make sure the caller provided a
    * large enough buffer
    */
   (*DomainSidSize) = cbRequiredDomainSidSize = GetSidLengthRequired(cDomainSubAthorities);

   if ( cbCallerDomainSidSize < cbRequiredDomainSidSize )
   {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      /* Failure */
      return ( FALSE );
   }

   if ( !InitializeSid(DomainSid,
                       GetSidIdentifierAuthority(Sid),
                       cDomainSubAthorities) )
   {
      /* Failure */
      return ( FALSE );
   }

   /*
    * Copy the NT domain RIDs
    */
   for ( idx = 0; idx < cDomainSubAthorities; idx++ )
   {
      *(GetSidSubAuthority(DomainSid, idx)) = *(GetSidSubAuthority(Sid, idx));
   }

   /* Success */
   return ( TRUE );
}

/*++
   GetPseudoTokenSid
      Translates a pseudo-sid to the actual matching SID contained in a token
 --*/
DWORD
GetPseudoTokenSid(
   __in DWORD PseudoSidRid,
   __out_bcount_part_opt(*SidSize, *SidSize) PSID Sid,
   __inout DWORD* SidSize
)
{
   DWORD                   dwRet;

   HANDLE                  hToken;

   PTOKEN_OWNER            pTokenOwner;
   PTOKEN_PRIMARY_GROUP    pTokenGroup;
   PTOKEN_USER             pTokenUser;
   PTOKEN_GROUPS           pTokenGroups;

   PVOID                   pWorkSpace;
   DWORD                   cbWorkSpace;
   UCHAR                   WorkSpace[256];
   
   TOKEN_INFORMATION_CLASS TokenInformationClass;

   DWORD                   cbCallerSidSize;
   DWORD                   cbRequiredSidSize;

   DWORD                   idx;

   /* Initialize locals.. */
   hToken      = NULL;
   pWorkSpace  = WorkSpace;
   cbWorkSpace = sizeof(WorkSpace);   

   /*
    * Map our RID to the proper token information class
    */
   switch ( PseudoSidRid )
   {
      case SIDTEMPLATE_TOKEN_USER_RID:
         __fallthrough;
      case SIDTEMPLATE_TOKEN_USER_DOMAIN_RID:
         TokenInformationClass = TokenUser;
         break;

      case SIDTEMPLATE_TOKEN_OWNER_RID:
         TokenInformationClass = TokenOwner;
         break;

      case SIDTEMPLATE_TOKEN_GROUP_RID:
         TokenInformationClass = TokenPrimaryGroup;
         break;

      case SIDTEMPLATE_TOKEN_LOGON_SESSION_RID:
         TokenInformationClass = TokenGroups;
         break;

      default:
         _ASSERT(FALSE);

         dwRet = ERROR_INVALID_PARAMETER;
         /* Failure */
         goto __CLEANUP;
   }

   /* 
    * Get the effective token of the caller
    */
   hToken = GetEffectiveThreadToken(GetCurrentThread(),
                                    TOKEN_QUERY);

   if ( !hToken )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   /* 
    * For all the types, we try to use our small local buffer but if it's not
    * large enough we'll allocate one of necessary size and use it
    */
   for ( ;; )
   {
      /*
       * First call will try to use the local workspace, second will try to
       * use dynamically allocated workspace
       */
      ZeroMemory(pWorkSpace,
                 cbWorkSpace);

      if ( GetTokenInformation(hToken,
                               TokenInformationClass,
                               pWorkSpace,
                               cbWorkSpace,
                               &cbWorkSpace) )
      {
         break;
      }

      dwRet = GetLastError();

      if ( ERROR_INSUFFICIENT_BUFFER != dwRet )
      {
         /* 
          * Some failure other than one we're prepared to handle, so bail 
          */
         
         /* Failure */
         goto __CLEANUP;
      }

      if ( pWorkSpace != WorkSpace )
      {
         /*
          * GetTokenInformation failed twice now, so just bail
          */

         dwRet = ERROR_INVALID_DATA;
         /* Failure */
         goto __CLEANUP;
      }

      /*
       * Don't allow the compiler/CPU to move the previous read because we rely
       * on it completing first
       */
      _ForceMemoryReadCompletion();

      _ASSERTE(cbWorkSpace > sizeof(WorkSpace));

      pWorkSpace = malloc(cbWorkSpace);
      if ( !pWorkSpace )
      {
         dwRet = ERROR_NOT_ENOUGH_MEMORY;
         /* Failure */
         goto __CLEANUP;
      }
   }

   /*
    * Cache the callers buffer size so we can overwrite it
    */
   cbCallerSidSize = (*SidSize);

   /*
    * We default to success, and only set dwRet on failure in the
    * code paths that follow
    */
   dwRet = NO_ERROR;

   if ( PseudoSidRid == SIDTEMPLATE_TOKEN_USER_RID )
   {
      pTokenUser = reinterpret_cast<PTOKEN_USER>(pWorkSpace);

      /*
       * Get the size of this SID and store it in the callers return length, then
       * make sure the caller provided a large enough buffer
       */
      (*SidSize) = cbRequiredSidSize = GetLengthSid(pTokenUser->User.Sid);

      if ( cbCallerSidSize < cbRequiredSidSize )
      {
         /* Failure */
         dwRet = ERROR_INSUFFICIENT_BUFFER;
      }
      else
      {
         /*
          * Caller's buffer can hold the SID, so copy it over
          */
         if ( !CopySid(cbCallerSidSize,
                       Sid,
                       pTokenUser->User.Sid) )
         {            
            /* Failure */
            dwRet = GetLastError();
         }
      }

      /* Success / Failure */
   }
   else if ( PseudoSidRid == SIDTEMPLATE_TOKEN_USER_DOMAIN_RID )
   {
      pTokenUser = reinterpret_cast<PTOKEN_USER>(pWorkSpace);

      /*
       * Attempt to copy the user's domain SID to the caller's buffer. CopyNTDomainSid
       * will set the proper error code if this fails and assign the buffer size as well
       */
      if ( !CopyNTDomainSid(Sid,
                            SidSize,
                            pTokenUser->User.Sid) )
      {
         /* Failure */
         dwRet = GetLastError();
      }

      /* Success / Failure */
   }
   else if ( PseudoSidRid == SIDTEMPLATE_TOKEN_OWNER_RID )
   {
      pTokenOwner = reinterpret_cast<PTOKEN_OWNER>(pWorkSpace);

      /*
       * Get the size of this SID and store it in the callers return length, then
       * make sure the caller provided a large enough buffer
       */
      (*SidSize) = cbRequiredSidSize = GetLengthSid(pTokenOwner->Owner);

      if ( cbCallerSidSize < cbRequiredSidSize )
      {
         /* Failure */
         dwRet = ERROR_INSUFFICIENT_BUFFER;
      }
      else
      {
         /*
          * Caller's buffer can hold the SID, so copy it over
          */
         if ( !CopySid(cbCallerSidSize,
                       Sid,
                       pTokenOwner->Owner) )
         {            
            /* Failure */
            dwRet = GetLastError();
         }
      }

      /* Success / Failure */
   }
   else if ( PseudoSidRid == SIDTEMPLATE_TOKEN_GROUP_RID )   
   {
      pTokenGroup = reinterpret_cast<PTOKEN_PRIMARY_GROUP>(pWorkSpace);

      /*
       * Get the size of this SID and store it in the callers return length, then
       * make sure the caller provided a large enough buffer
       */
      (*SidSize) = cbRequiredSidSize = GetLengthSid(pTokenGroup->PrimaryGroup);

      if ( cbCallerSidSize < cbRequiredSidSize )
      {
         /* Failure */
         dwRet = ERROR_INSUFFICIENT_BUFFER;
      }
      else
      {
         /*
          * Caller's buffer can hold the SID, so copy it over
          */
         if ( !CopySid(cbCallerSidSize,
                       Sid,
                       pTokenGroup->PrimaryGroup) )
         {            
            /* Failure */
            dwRet = GetLastError();
         }
      }

      /* Success / Failure */
   }
   else if ( PseudoSidRid == SIDTEMPLATE_TOKEN_LOGON_SESSION_RID )
   {
      pTokenGroups = reinterpret_cast<PTOKEN_GROUPS>(pWorkSpace);

      /*
       * Default to the SID not being found
       */
      dwRet = ERROR_NOT_FOUND;

      /*
       * Enumerate the groups in the list and find the one for the logon SID
       */
      for ( idx = 0; idx < pTokenGroups->GroupCount; idx++ )
      {
         if ( SE_GROUP_LOGON_ID == (SE_GROUP_LOGON_ID & pTokenGroups->Groups[idx].Attributes) )
         {
            /*
             * Get the size of this SID and store it in the callers return length, then
             * make sure the caller provided a large enough buffer
             */
            (*SidSize) = cbRequiredSidSize = GetLengthSid(pTokenGroups->Groups[idx].Sid);

            if ( cbCallerSidSize < cbRequiredSidSize )
            {
               /* Failure */
               dwRet = ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
               /*
                * Caller's buffer can hold the SID, so copy it over
                */
               if ( !CopySid(cbCallerSidSize,
                             Sid,
                             pTokenGroups->Groups[idx].Sid) )
               {                  
                  /* Failure */
                  dwRet = GetLastError();
               }
               else
               {
                  /* Success */
                  dwRet = NO_ERROR;
               }
            }

            /*
             * There should only be one logon id, so just bust out at this point whether
             * we succeeded or not
             */

            /* Success / Failure */
            break;
         }
      }
   }

__CLEANUP:
   /*
    * Free the workspace only if it's a dynamically allocated one
    */
   if ( pWorkSpace != WorkSpace )
   {
      free(pWorkSpace);
      pWorkSpace = NULL;
   }

   if ( hToken )
   {
      CloseHandle(hToken);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
GetSizeOfSidRequired(
   __in PCGENERIC_SID SidTemplate
)
{
   UCHAR SubAuthorityCount;

   if ( IsPseudoSidTemplate(SidTemplate) )
   {     
      /*
       * This function is called frequently so to avoid doing a lookup on the 
       * token for each call, we return the size required to hold the largest 
       * possible SID for that type
       */
      switch ( SidTemplate->SubAuthority[0] )
      {
         case SIDTEMPLATE_TOKEN_USER_RID:
            __fallthrough;
         case SIDTEMPLATE_TOKEN_OWNER_RID:
            __fallthrough;
         case SIDTEMPLATE_TOKEN_GROUP_RID:         
            SubAuthorityCount = SID_MAX_SUB_AUTHORITIES;
            break;

         case SIDTEMPLATE_TOKEN_USER_DOMAIN_RID:
            /*
             * For domain relative SIDs we need to add whatever remains in the template. BUILTIN
             * domain SIDs will be less, so we use the greater size which is the domain SID size
             */
            SubAuthorityCount = 1/*SECURITY_NT_NON_UNIQUE*/ + SECURITY_NT_NON_UNIQUE_SUB_AUTH_COUNT + SidTemplate->SubAuthorityCount - 1;
            
            _ASSERTE(SubAuthorityCount <= SID_MAX_SUB_AUTHORITIES);
            SubAuthorityCount = min(SubAuthorityCount, SID_MAX_SUB_AUTHORITIES);
            break;

         case SIDTEMPLATE_TOKEN_LOGON_SESSION_RID:
            SubAuthorityCount = (1/*SECURITY_LOGON_IDS_RID*/ + SECURITY_LOGON_IDS_RID_COUNT);
            break;

         default:
            _ASSERTE(IsPseudoSidTemplate(SidTemplate));
            __assume(0);
      }
   }
   else
   {
      SubAuthorityCount = SidTemplate->SubAuthorityCount;
   }

   _ASSERTE(SubAuthorityCount <= SID_MAX_SUB_AUTHORITIES);
   return ( GetSidLengthRequired(SubAuthorityCount) );
}

BOOL
IsValidAceTypeFlags(
   __in UCHAR AceType,
   __in UCHAR AceFlags
)
{
   /* 
    * Every ACE type supports the inherit flags, so if those are
    * all that is set, they're valid
    */
   AceFlags &= ~(UCHAR)VALID_INHERIT_FLAGS;

   if ( 0 != AceFlags )
   {
      /* 
       * Some other flags are set, and right now the only other type
       * we support that has additional flags, is the audit type. So
       * if it is that type then clear those flags. If we add more 
       * types that support additional flags, then we just check each
       * and clear those flags. If after all that we end up with no
       * flags set, we're valid
       */
      if ( SYSTEM_AUDIT_ACE_TYPE == AceType )
      {
         AceFlags &= ~(UCHAR)(SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG);
      }

      if ( 0 != AceFlags )
      {
         /* Failure */
         return ( FALSE );
      }
   }

   /* Success */
   return ( TRUE );
}

DWORD
GetSizeOfAceRequired(
   __in PCACE_ENTRY AceEntry
)
{
   _ASSERTE(IsSupportedAceType(AceEntry->AceType));

   DWORD cbAce;

   /* 
    * All the ACE types we support are the same size, so we use the generic
    * type. We also subtract the extra DWORD which holds the first part of 
    * trailing SID
    */
   cbAce = sizeof(GENERIC_ACE) - 
           sizeof(DWORD)       +
           GetSizeOfSidRequired(AceEntry->SidTemplate);

   return ( cbAce );
}

DWORD
GetSizeOfAclRequired(
   __in_ecount(AceListCount) PCACE_ENTRY AceList,
   __in WORD AceListCount
)
{
   DWORD cbAcl;
   DWORD cbAce;
   
   /* 
    * We start with the ACL header, which for any ACL is always present
    */
   cbAcl = sizeof(ACL);

   /* 
    * Walk the list of ACE_ENTRY records and add up the size of each ACE
    */
   while ( AceListCount-- )
   {
      _ASSERTE(IsSupportedAceType(AceList->AceType));

      /* 
       * If we don't support the AceType, skip it
       */
      if ( IsSupportedAceType(AceList->AceType) )
      {
         cbAce = GetSizeOfAceRequired(AceList);

         /*
          * If there's room for the ACE, add its size
          */
         if ( (DWORD)(cbAcl + cbAce) > cbAcl )
         {            
            /*
             * Add the size of this ACE to the running total
             */
            cbAcl += cbAce;
         }
      }

      /* Get the next entry entry to check */
      AceList += 1;
   }

   return ( cbAcl );
}

/*++
   GetTemplateSDRequiredSize
      Calculates the maximum required size, in bytes, of a template security descriptor
 --*/
DWORD
GetTemplateSDRequiredSize(
   __in_opt PCGENERIC_SID OwnerSid,
   __out DWORD* OwnerSidSize,
   __in_opt PCGENERIC_SID GroupSid,
   __out DWORD* GroupSidSize,
   __in_ecount_opt(SaclListCount) PCACE_ENTRY SaclList,
   __in WORD SaclListCount,
   __out DWORD* SaclSize,
   __in_ecount_opt(DaclListCount) PCACE_ENTRY DaclList,
   __in WORD DaclListCount,
   __out DWORD* DaclSize
)
{
   DWORD cbSD;

   /* 
    * We default to the larger of the two SD types, which is the absolute type
    */
   cbSD = sizeof(SECURITY_DESCRIPTOR);

   /*
    * If an argument is present, calculate its required size and assign that to 
    * the caller.
    */
   if ( OwnerSid )
   {
      cbSD += (*OwnerSidSize) = GetSizeOfSidRequired(OwnerSid);
   }

   if ( GroupSid )
   {
      cbSD += (*GroupSidSize) = GetSizeOfSidRequired(GroupSid);
   }

   if ( SaclList )
   {
      cbSD += (*SaclSize) = GetSizeOfAclRequired(SaclList,
                                                 SaclListCount);
   }

   if ( DaclList )
   {
      cbSD += (*DaclSize) = GetSizeOfAclRequired(DaclList,
                                                 DaclListCount);
   }

   return ( cbSD );
}

DWORD
APIENTRY
BuildTemplateSid(
   __in PCGENERIC_SID SidTemplate,
   __out_bcount(SidSize) PSID Sid,
   __in DWORD SidSize
)
{
   DWORD dwRet;
   
   UCHAR idx;

   PSID  pSid;
   BYTE  SidWorkSpace[SECURITY_MAX_SID_SIZE];

   PSID  pDomainSid;
   DWORD cbDomainSidSize;
   BYTE  DomainWorkSpace[SECURITY_MAX_SID_SIZE];

   UCHAR cSubAuthorityCount;   
   
   DWORD cbRequired;
   
   /* Initialize locals.. */
   pDomainSid         = NULL;
   cSubAuthorityCount = 0;

   cbRequired = GetSizeOfSidRequired(SidTemplate);

   if ( cbRequired > SidSize )
   {
      /* Failure */
      return ( ERROR_INSUFFICIENT_BUFFER );
   }

   if ( cbRequired > (DWORD)SECURITY_MAX_SID_SIZE )
   {
      /* Failure */
      return ( ERROR_INVALID_SID );
   }
      
   /*
    * If this is one of the pseudo SID templates, get the SID from
    * the token
    */
   if ( IsPseudoSidTemplate(SidTemplate) )
   {
      if ( SIDTEMPLATE_TOKEN_USER_DOMAIN_RID == SidTemplate->SubAuthority[0] )
      {
         /*
          * For the user domain RID, we allow appending further RIDs to what we
          * can extract from the user SID. So for this one, we need to get the
          * domain SID then fall through and copy the remaining sub-authorties
          */
         pDomainSid      = reinterpret_cast<PSID>(DomainWorkSpace);
         cbDomainSidSize = sizeof(DomainWorkSpace);

         dwRet = GetPseudoTokenSid(SIDTEMPLATE_TOKEN_USER_DOMAIN_RID,
                                   pDomainSid,
                                   &cbDomainSidSize);

         if ( WINERROR(dwRet) )
         {
            /* Failure */
            return ( dwRet );
         }

         /*
          * Determine how many sub authories are in the domain SID so we
          * know where to start copying the remaining ones to
          */
         cSubAuthorityCount = *(GetSidSubAuthorityCount(pDomainSid));

         /*
          * Ensure that the SID can accomodate the remaining sub authorities in the
          * SID template. This is necessary because the RID for the pseudo domain SID
          * is only 1 sub authority in the template, but multiple sub authorities when
          * expanded at runtime.
          *
          * We add an extra to SID_MAX_SUB_AUTHORITIES to account for the pseudo one
          * in the sid-template sub-authority
          */
         if ( (UCHAR)(cSubAuthorityCount + SidTemplate->SubAuthorityCount) > (UCHAR)(SID_MAX_SUB_AUTHORITIES + 1) )
         {
            dwRet = ERROR_INVALID_PARAMETER;
            /* Failure */
            return ( dwRet );
         }
      }
      else
      {
         /*
          * Otherwise we can just get the SID directly from the token
          */
         dwRet = GetPseudoTokenSid(SidTemplate->SubAuthority[0],
                                   Sid,
                                   &SidSize);

         /* Success / Failure */
         return ( dwRet );
      }
   }

   /*
    * We're building a SID from the template. Create it in the local
    * buffer first, then copy it to the caller when its done
    */
   pSid = reinterpret_cast<PSID>(SidWorkSpace);

   if ( pDomainSid )
   {
      /*
       * This must be a domain sid, so copy the domain portion first
       */
      if ( !CopySid(sizeof(SidWorkSpace),
                    pSid,
                    pDomainSid) )
      {
         dwRet = GetLastError();
         /* Failure */
         return ( dwRet );
      }

      /*
       * Walk the list of RIDs and set each one in the SID. Note that we start
       * after the entry which is known to be SIDTEMPLATE_TOKEN_USER_DOMAIN_RID
       */
      for ( idx = 1; idx < SidTemplate->SubAuthorityCount; idx++ )
      {
         *(GetSidSubAuthority(pSid, idx + cSubAuthorityCount)) = SidTemplate->SubAuthority[idx];
      }
   }
   else
   {
      if ( !InitializeSid(pSid,
                          const_cast<PSID_IDENTIFIER_AUTHORITY>(&(SidTemplate->IdentifierAuthority)),
                          SidTemplate->SubAuthorityCount) )
      {
         dwRet = GetLastError();
         /* Failure */
         return ( dwRet );
      }

      /*
       * Walk the list of RIDs and set each one in the SID
       */
      for ( idx = 0; idx < SidTemplate->SubAuthorityCount; idx++ )
      {
         *(GetSidSubAuthority(pSid, idx)) = SidTemplate->SubAuthority[idx];
      }
   }

   if ( !CopySid(SidSize,
                 Sid,
                 pSid) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }
   
   /* Success */
   return ( NO_ERROR );
}

DWORD
APIENTRY
BuildTemplateAcl(
   __in_ecount(AceListCount) PCACE_ENTRY AceList,
   __in WORD AceListCount,
   __out_bcount(AclSize) PACL Acl,
   __in DWORD AclSize
)
{
   DWORD          dwRet;

   PGENERIC_ACE   pAce;

   UCHAR          AceType;
   UCHAR          AceFlags;
   DWORD          AceSize;

   BYTE*          AclTail;

   PSID           pSidWorkSpace;
   BYTE           SidWorkSpace[SECURITY_MAX_SID_SIZE];

   /*
    * Initialize locals
    */
   pSidWorkSpace = reinterpret_cast<PSID>(SidWorkSpace);

   /*
    * Setup the ACL header..
    */
   if ( !InitializeAcl(Acl,
                       AclSize,
                       ACL_REVISION) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }
  
   /* 
    * Check if this is an empty ACL, which if it is then we're done. An empty ACL is
    * one with no ACE elements that denies all access to the object
    */
   if ( !AceList )
   {
      /* Success */
      return ( NO_ERROR );
   }

   /*
    * Calculate the tail of the ACL which we'll use for overrun checks
    */
   AclTail = reinterpret_cast<BYTE*>(Acl) + (size_t)Acl->AclSize;

   /* 
    * Walk the list of ACE_ENTRY records in the list, from start to finish and
    * build each in the list.
    *
    * Note that this means the ACE's in the ACL will be in the same order which
    * they are in the ACE_ENTRY list. Callers must therefore properly order the
    * list to create a properly ordered ACL. As a general rule, all DENY type
    * ACEs should preceed ALLOW type ACEs
    */

   /* 
    * Get the address of the first ACE we'll be creating in the ACL.
    */
   pAce = reinterpret_cast<PGENERIC_ACE>(reinterpret_cast<ULONG_PTR>(Acl) + sizeof(ACL));
   
   while ( AceListCount-- )
   {
      _ASSERTE(IsSupportedAceType(AceList->AceType));

      /* 
       * If we don't support the AceType, skip it
       */
      AceType = AceList->AceType;

      if ( IsSupportedAceType(AceType) )
      {
         /*
          * Build the SID for this ACE so we can determine its actual size
          */
         dwRet = BuildTemplateSid(AceList->SidTemplate,
                                  pSidWorkSpace,
                                  sizeof(SidWorkSpace));
         
         if ( WINERROR(dwRet) )
         {
            _ASSERTE(WINERROR(dwRet));
            /* Failure */
            return ( dwRet );
         }

         /* 
          * Calculate the actual size of the ACE and make sure that it's valid, 
          * and that we have room left in the ACL to add it
          */
         AceSize = sizeof(GENERIC_ACE) -
                   sizeof(DWORD)       +
                   GetLengthSid(pSidWorkSpace);

         /*
          * Make sure our ACE will fit in what's left of our ACL by calculating
          * how much remains. We know the size remaining fits in a DWORD because
          * the size difference is always less than MAXWORD
          */
         if ( AceSize > (DWORD)(reinterpret_cast<ULONG_PTR>(AclTail) - reinterpret_cast<ULONG_PTR>(pAce)) )
         {
            _ASSERTE(FALSE);
            /* Failure */
            return ( ERROR_INSUFFICIENT_BUFFER );
         }

         /* 
          * Make sure the flags are valid
          */
         AceFlags = AceList->AceFlags;

         if ( !IsValidAceTypeFlags(AceType,
                                   AceFlags) )
         {
            _ASSERTE(IsValidAceTypeFlags(AceType, AceFlags));
            /* Failure */
            return ( ERROR_INVALID_DATA );         
         }

         /* 
          * The header is good, so set it up. AceSize will never be larger than MAXWORD
          */
         pAce->Header.AceType  = AceType;
         pAce->Header.AceFlags = AceFlags;
         pAce->Header.AceSize  = (WORD)AceSize;
         pAce->AccessMask      = AceList->AccessMask;

         /*
          * Now append the SID to the ACE. If this fails, we just bail and the ACL
          * is still in a valid state because we haven't updated the AceCount. The
          * destination size is the full ACE size excluding the header and embedded
          * DWORD that gets overwritten with the start of the SID
          */
         if ( !CopySid(AceSize - (sizeof(GENERIC_ACE) - sizeof(DWORD)),
                       reinterpret_cast<PSID>(&(pAce->SidStart)),
                       pSidWorkSpace) )
         {
            dwRet = GetLastError();
            /* Failure */
            return ( dwRet );
         }

         /* 
          * Update the ACL count to reflect the one we just added
          */
         Acl->AceCount += 1;
      
         /* 
          * Move the ACE pointer to the next entry in the ACL 
          */
         pAce = reinterpret_cast<PGENERIC_ACE>(reinterpret_cast<ULONG_PTR>(pAce) + AceSize);
      }

      /* 
       * Get the next ACE_ENTRY record to build
       *
       * !THIS IS POINTER MATH!
       */
      AceList += 1;
   }

   _ASSERTE(IsValidAcl(Acl));

   /* Success */
   return ( NO_ERROR );
}

DWORD
APIENTRY
CreateTemplateSecurityDescriptor(
   __in_opt PCGENERIC_SID OwnerSid,
   __in_opt PCGENERIC_SID GroupSid,
   __in_ecount_opt(SaclListCount) PCACE_ENTRY SaclList,
   __in WORD SaclListCount,
   __in_ecount_opt(DaclListCount) PCACE_ENTRY DaclList,
   __in WORD DaclListCount,
   __in SECURITY_DESCRIPTOR_CONTROL ControlFlags,
   __out PSECURITY_DESCRIPTOR* SecurityDescriptor
)
{
   DWORD                            dwRet;

   PSECURITY_DESCRIPTOR             pSD;
   PSID                             pSid;
   PACL                             pAcl;

   size_t                           cbBuffer;

   DWORD                            cbSD;
   DWORD                            cbOwner;
   DWORD                            cbGroup;
   DWORD                            cbSacl;
   DWORD                            cbDacl;

   DWORD                            cbAlignedSD;
   DWORD                            cbAlignedOwner;
   DWORD                            cbAlignedGroup;
   DWORD                            cbAlignedSacl;
   DWORD                            cbAlignedDacl;

   SECURITY_DESCRIPTOR_CONTROL      SDControlFlags;

#ifdef _SEUTIL_NOINLINESELFRELATIVESDCONVERSION
   DWORD                            cbSelfRelative;
   PSECURITY_DESCRIPTOR             pSelfRelativeSD;
#else /* _SEUTIL_NOINLINESELFRELATIVESDCONVERSION */
   PISECURITY_DESCRIPTOR_RELATIVE   pISDRelative;
   PISECURITY_DESCRIPTOR            pISDAbsolute;
#endif /* _SEUTIL_NOINLINESELFRELATIVESDCONVERSION */

   /* Initialize outputs */
   (*SecurityDescriptor) = NULL;

   /* Initialize locals */
   pSD     = NULL;
   cbSD    = sizeof(SECURITY_DESCRIPTOR);
   cbOwner = cbGroup = cbDacl = cbSacl = 0;

   GetTemplateSDRequiredSize(OwnerSid,
                             &cbOwner,
                             GroupSid,
                             &cbGroup,
                             SaclList,
                             SaclListCount,
                             &cbSacl,
                             DaclList,
                             DaclListCount,
                             &cbDacl);

   /*
    * Align all the sizes on pointer boundaries so they're not misaligned
    * when we build the SD in a common buffer
    */
    cbAlignedSD    = PtrAlignSize(cbSD);
    cbAlignedOwner = PtrAlignSize(cbOwner);
    cbAlignedGroup = PtrAlignSize(cbGroup);
    cbAlignedSacl  = PtrAlignSize(cbSacl);
    cbAlignedDacl  = PtrAlignSize(cbDacl);

    cbBuffer = cbAlignedSD    + 
               cbAlignedOwner +
               cbAlignedGroup +
               cbAlignedSacl  +
               cbAlignedDacl;

   /* 
    * Allocate a buffer for all the components of the SD 
    */
   pSD = reinterpret_cast<PSECURITY_DESCRIPTOR>(malloc(cbBuffer));
   if ( !pSD )
   {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   ZeroMemory(pSD,
              cbBuffer);

   /* Start building each piece in the SD. We lay out each part as follows.
    *
    *       -------------------------------
    *       |     SECURITY_DESCRIPTOR     |
    *       |         [OWNER SID]         |
    *       |         [GROUP SID]         |
    *       |           [SACL]            |
    *       |           [DACL]            |
    *       -------------------------------
    * 
    * If a piece is missing, the next part moves into its place. This is 
    * similar to a self-relative SD but it is still an absolute one. We
    * build it this way so that we only have one buffer to cleanup
    */

   if ( !InitializeSecurityDescriptor(pSD,
                                      SECURITY_DESCRIPTOR_REVISION) )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   /* 
    * Build the owner SID, if present 
    */
   if ( OwnerSid )
   {
      pSid = reinterpret_cast<PSID>(reinterpret_cast<ULONG_PTR>(pSD) + cbAlignedSD);

      /* 
       * Build the SID in place then assign it to the SD
       */
      dwRet = BuildTemplateSid(OwnerSid,
                               pSid,
                               cbOwner);

      if ( WINERROR(dwRet) )
      {
         /* Failure */
         goto __CLEANUP;
      }

      if ( !SetSecurityDescriptorOwner(pSD,
                                       pSid,
                                       BooleanFlagOn(ControlFlags, SE_OWNER_DEFAULTED)) )
      {
         dwRet = GetLastError();  
         /* Failure */
         goto __CLEANUP;
      }
   }

   /* 
    * Build the group SID, if present 
    */
   if ( GroupSid )
   {
      pSid = reinterpret_cast<PSID>(reinterpret_cast<ULONG_PTR>(pSD) + cbAlignedSD + cbAlignedOwner);

      /* 
       * Build the SID in place then assign it to the SD
       */
      dwRet = BuildTemplateSid(GroupSid,
                               pSid,
                               cbGroup);

      if ( WINERROR(dwRet) )
      {
         /* Failure */
         goto __CLEANUP;
      }

      if ( !SetSecurityDescriptorGroup(pSD,
                                       pSid,
                                       BooleanFlagOn(ControlFlags, SE_GROUP_DEFAULTED)) )
      {
         dwRet = GetLastError();  
         /* Failure */
         goto __CLEANUP;
      }
   }

   /* 
    * Build the SACL if the caller passed us an ACE_ENTRY list for one
    */
   if ( SaclList )
   {
      pAcl = reinterpret_cast<PACL>(reinterpret_cast<ULONG_PTR>(pSD) + cbAlignedSD + cbAlignedOwner + cbAlignedGroup);

      /* 
       * Build the SACL in place
       */
      dwRet = BuildTemplateAcl(SaclList,
                               SaclListCount,
                               pAcl,
                               cbSacl);

      if ( WINERROR(dwRet) )
      {
         /* Failure */
         goto __CLEANUP;
      }

      /* 
       * Set a bit in the control flags so our code below knows that we want to set the
       * SACL
       */
      ControlFlags |= SE_SACL_PRESENT;
   }
   else
   {
      pAcl = NULL;
   }
   
   /*
    * If we built an SACL, or if the caller has requested that we assign a NULL SACL, then
    * do so now
    */
   if ( FlagOn(ControlFlags, SE_SACL_PRESENT) )
   {
      if ( !SetSecurityDescriptorSacl(pSD,
                                      TRUE,
                                      pAcl,
                                      BooleanFlagOn(ControlFlags, SE_SACL_DEFAULTED)) )
      {
         dwRet = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }
   }

   /* 
    * Build the DACL, if present
    */
   if ( DaclList )
   {
      pAcl = reinterpret_cast<PACL>(reinterpret_cast<ULONG_PTR>(pSD) + cbAlignedSD + cbAlignedOwner + cbAlignedGroup + cbAlignedSacl);

      /* 
       * Build the SACL in place.. 
       */
      dwRet = BuildTemplateAcl(DaclList,
                               DaclListCount,
                               pAcl,
                               cbDacl);

      if ( WINERROR(dwRet) )
      {
         /* Failure */
         goto __CLEANUP;
      }

      /* 
       * Set a bit in the control flags so our code below knows that we want to set
       * a DACL
       */
      ControlFlags |= SE_DACL_PRESENT;
   }
   else
   {
      pAcl = NULL;
   }
   
   /*
    * If we built a DACL, or if the caller has requested that we assign a NULL DACL, then
    * do so now
    */
   if ( FlagOn(ControlFlags, SE_DACL_PRESENT) )
   {
      if ( !SetSecurityDescriptorDacl(pSD,
                                      TRUE,
                                      pAcl,
                                      BooleanFlagOn(ControlFlags, SE_DACL_DEFAULTED)) )
      {
         dwRet = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }
   }

   /* 
    * Set any control flags that aren't set by the assignments we've already made. As we
    * created the SD they are off by default so the only thing we can be doing is turning 
    * them on 
    */
   SDControlFlags = (SECURITY_DESCRIPTOR_CONTROL)((DWORD)ControlFlags & (DWORD)(SE_SELF_RELATIVE|SE_OWNER_DEFAULTED|SE_GROUP_DEFAULTED|SE_SACL_PROTECTED|SE_DACL_PROTECTED));

   if ( FlagOn(SDControlFlags, (SE_OWNER_DEFAULTED|SE_GROUP_DEFAULTED|SE_SACL_PROTECTED|SE_DACL_PROTECTED)) )
   {
      if ( !SetSecurityDescriptorControl(pSD,
                                         SDControlFlags,
                                         SDControlFlags) )
      {
         dwRet = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }
   }

   _ASSERTE(IsValidSecurityDescriptor(pSD));

   /*
    * If the caller has requested a self-relative SD, convert it now.
    */
   if ( FlagOn(ControlFlags, SE_SELF_RELATIVE) )
   {
#ifdef _SEUTIL_NOINLINESELFRELATIVESDCONVERSION
      cbSelfRelative = 0;

      /*
       * Determine the size required to hold the self-relative SD. We let the system
       * calculate this because the sizes we have could be larger than necessary
       */
      if ( !MakeSelfRelativeSD(pSD,
                               NULL,
                               &cbSelfRelative) )
      {
         dwRet = GetLastError();

         if ( ERROR_INSUFFICIENT_BUFFER != dwRet )
         {
            /* Failure */
            goto __CLEANUP;
         }

         pSelfRelativeSD = reinterpret_cast<PSECURITY_DESCRIPTOR>(malloc(cbSelfRelative));
         if ( !pSelfRelativeSD )
         {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            /* Failure */
            goto __CLEANUP;
         }

         if ( !MakeSelfRelativeSD(pSD,
                                  pSelfRelativeSD,
                                  &cbSelfRelative) )
         {
            dwRet = GetLastError();
            /* Failure */
            goto __CLEANUP;
         }

         /*
          * We're going to discard the absolute SD now since we'll be returning the
          * self-relative copy
          */
         free(pSD);
         
         /*
          * Assign the self-relative SD to the SD we're returning
          */
         pSD = pSelfRelativeSD;
      }
#else /* _SEUTIL_NOINLINESELFRELATIVESDCONVERSION */
      pISDAbsolute = reinterpret_cast<PISECURITY_DESCRIPTOR>(pSD);
      pISDRelative = reinterpret_cast<PISECURITY_DESCRIPTOR_RELATIVE>(pSD);

      /*
       * The Revision, Sbz1 and Control occupy the same position in both absolute
       * and relative SDs, so they don't need to be moved. The Control does need
       * to be updated to signify that the SD is self-relative though.
       *
       * The Owner, Group, Sacl and Dacl are all aligned on pointer boundries
       * so there may be some waisted space, but not much
       */
      pISDRelative->Control |= SE_SELF_RELATIVE;
      pISDRelative->Owner    = cbAlignedSD;
      pISDRelative->Group    = cbAlignedSD + cbAlignedOwner;
      pISDRelative->Sacl     = cbAlignedSD + cbAlignedOwner + cbAlignedGroup;
      pISDRelative->Dacl     = cbAlignedSD + cbAlignedOwner + cbAlignedGroup + cbAlignedSacl;

      /*
       * Clear everything between the end of the relative SD and the end of the
       * absolute SD
       */
      ZeroMemory(reinterpret_cast<PUCHAR>(pSD) + sizeof(SECURITY_DESCRIPTOR_RELATIVE),
                 cbAlignedSD - sizeof(SECURITY_DESCRIPTOR_RELATIVE));

#endif /* _SEUTIL_NOINLINESELFRELATIVESDCONVERSION */
      _ASSERTE(IsValidSecurityDescriptor(pSD));
   }

   /*
    * Everything worked out, so now assign the SD to the caller
    */
   (*SecurityDescriptor) = pSD;

   /* 
    * Clear this so we don't free it in cleanup 
    */
   pSD = NULL;

   /* Success */
   dwRet = NO_ERROR;

__CLEANUP:
   if ( pSD )
   {
      free(pSD);
      pSD = NULL;
   }

   /* Failure */
   return ( dwRet );
}

void
APIENTRY
FreeTemplateSecurityDescriptor(
   __in PSECURITY_DESCRIPTOR SecurityDescriptor
)
{
   free(SecurityDescriptor);
}

