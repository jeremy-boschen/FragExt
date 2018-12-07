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

/* SecUtil.h
 *   Various security related utilities included in FRAGEXT.LIB
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

HANDLE
APIENTRY
GetThreadToken(
   __in HANDLE ThreadHandle,
   __in DWORD DesiredAccess 
);

HANDLE
APIENTRY
GetEffectiveThreadToken(
   __in HANDLE ThreadHandle,
   __in DWORD DesiredAccess 
);

PVOID
APIENTRY
GetTokenInformationData(
   __in HANDLE TokenHandle,
   __in TOKEN_INFORMATION_CLASS TokenInformationClass,
   __out_opt PDWORD ReturnLength
);

VOID
APIENTRY
FreeTokenInformationData(
   __in PVOID Data
);

BOOLEAN
APIENTRY
SetTokenPrivilege(
   __in HANDLE TokenHandle,
   __in LUID PrivilegeValue,
   __in BOOLEAN Enable,
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
);

BOOLEAN
APIENTRY
SetTokenPrivilege( 
   __in HANDLE TokenHandle, 
   __in_z LPCWSTR PrivilegeName, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState 
);

BOOLEAN 
APIENTRY
SetThreadTokenPrivilege( 
   __in HANDLE ThreadHandle, 
   __in_z LPCWSTR PrivilegeName, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
);

BOOLEAN 
APIENTRY
SetThreadTokenPrivilege( 
   __in HANDLE ThreadHandle, 
   __in LUID PrivilegeValue, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
);

BOOLEAN
APIENTRY
SetProcessTokenPrivilege(
   __in HANDLE ProcessHandle, 
   __in_z LPCWSTR PrivilegeName, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
);

BOOLEAN
APIENTRY
SetProcessTokenPrivilege(
   __in HANDLE ProcessHandle, 
   __in LUID PrivilegeValue, 
   __in BOOLEAN Enable, 
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
);

BOOLEAN
APIENTRY
SetEffectiveTokenPrivilege(
   __in_z LPCWSTR PrivilegeName,
   __in BOOLEAN Enable,
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
);

BOOLEAN
APIENTRY
SetEffectiveTokenPrivilege(
   __in_z LUID PrivilegeValue,
   __in BOOLEAN Enable,
   __deref_out_opt PTOKEN_PRIVILEGES PreviousState
);

inline
BOOLEAN 
IsEqualLUID( 
   PLUID pLuid1, 
   PLUID pLuid2 
)
{
   if ( (pLuid1->LowPart == pLuid2->LowPart) && (pLuid1->HighPart == pLuid2->HighPart) )
   {
      return ( TRUE );
   }

   return ( FALSE );
}


/*++ 
   Token information wrappers 
 --*/
SECURITY_IMPERSONATION_LEVEL
APIENTRY
GetTokenImpersonationLevel(
   __in HANDLE TokenHandle
);

SECURITY_IMPERSONATION_LEVEL
APIENTRY
GetThreadTokenImpersonationLevel(
   __in HANDLE ThreadHandle
);

BOOL
APIENTRY
IsThreadImpersonating(
   __in HANDLE ThreadHandle
);

#define VALID_TOKEN_TYPE( TokenType ) \
   (((TokenType) == TokenPrimary) || ((TokenType) == TokenImpersonation))

TOKEN_TYPE
APIENTRY
GetTokenType(
   __in HANDLE TokenHandle
);

TOKEN_TYPE
APIENTRY
GetThreadTokenType(
   __in HANDLE ThreadHandle
);

/*++
   RemoveTokenPrivileges
      Removes or disables a list of privileges in a token, leaving unlisted
      ones untouched
  --*/
BOOLEAN 
APIENTRY
RemoveTokenPrivileges( 
   __in HANDLE TokenHandle, 
   __in_z LPCWSTR PermittedPrivileges[], 
   __in DWORD PermittedPrivilegesCount
);

/*++
   RestrictTokenPrivileges
      Removes or disables all privileges in a token which aren't listed in
      the provided array
  --*/
BOOLEAN
APIENTRY
RestrictTokenPrivileges( 
   __in HANDLE hToken, 
   __in_ecount(PermittedPrivilegesCount) LPCWSTR PermittedPrivileges[], 
   __in DWORD PermittedPrivilegesCount
);

DWORD
APIENTRY
SelfRelativeToAbsoluteSD(
   __in PSECURITY_DESCRIPTOR SelfRelativeSD, 
   __out PSECURITY_DESCRIPTOR AbsoluteSD,
   __inout DWORD* AbsoluteSDSize
);

/*++

   Generic Pointer Encoding/Decoding
 
      These functions do very simple pointer encoding using the application __security_cookie.
 --*/

PVOID
APIENTRY
SeEncodePointer( 
   PVOID Pointer 
);

PVOID 
APIENTRY
SeDecodePointer( 
   PVOID Pointer 
);

template < typename T >
T*
SeEncodePointerT(
   __in T* pT
)
{
   return ( reinterpret_cast<T*>(SeEncodePointer(pT)) );
}

template < typename T >
T*
SeDecodePointerT(
   __in T* pT
)
{
   return ( reinterpret_cast<T*>(SeDecodePointer(pT)) );
}

template < typename T >
class EncodedPointerT
{
public:
   typedef T   __type;
   typedef T*  __ptype;


   EncodedPointerT( ) 
   {
      _pT = NULL;
      _pR = NULL;
   }

   void SetPointer( T* p )
   {
      _pT = SeEncodePointerT(p);
      _pR = _CreateReferencePointer(p);
   }

   T* GetPointer( )
   {
      T* p;

      if ( _pT )
      {
         p = SeDecodePointerT(p);
         if ( _CreateReferencePointer(p) == _pR )
         {
            return ( p );
         }

         /* 
          * Raise a STATUS_HEAP_CORRUPTION exception. It's not exactly what we have, but close enough 
          */
         RaiseException(0xC0000374U,
                        EXCEPTION_NONCONTINUABLE,
                        0,
                        NULL);
      }

      return ( NULL );
   }

protected:
   T* _pT;
   T* _pR;

   T* _CreateReferencePointer( T* p )
   {
      return ( reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(p) ^ reinterpret_cast<unintptr_t>(this)) );
   }

private:
   /* NOT SUPPORTED */
   EncodedPointerT( const EncodedPointerT& );
   EncodedPointerT& operator=( const EncodedPointerT& );
};

/*++

SID, ACE, ACL, SECURITY_DESCRIPTOR Helpers

How to use the SID_TEMPLATE helpers..
 
1) Define SID_TEMPLATE objects with the DEFINE_SID() macro, or use one of the 
   predefined ones if they are declared. If using the pseudo SIDS, declare them
   exactly as defined below, or use the pre-declared ones if possible.
      
      DEFINE_SID(MyCurrentUserSid, SIDTEMPLATE_AUTHORITY, SIDTEMPLATE_TOKEN_USER_RID);
      DEFINE_SID(MyNetworkSid,     SECURITY_NT_AUTHORITY, SECURITY_NETWORK_RID);
      DEFINE_SID(MyLocalAdminsSid, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);

   Note that the difference between using the pre-declared ones and declaring a
   SID_TEMPLATE locally is that the pre-declared ones end up being allocated and
   initialized in a global program section, while declaring one in a function uses 
   the function's stack space for allocation and initializes it each time the
   function is called.

2) Build the ACE_ENTRY list for a DACL and/or a SACL. Note that the order in which each ACE is listed is the
   the order in which that ACE will be placed in the ACL, so make sure the order is correct. As a general rule,
   make sure all DENY type ACE's are listed before ALLOW type ACE's.

   ACE_ENTRY MySaclList[] =
   {
      DEFINE_ACE(SYSTEM_AUDIT_ACE_TYPE, SUCCESSFUL_ACCESS_ACE_FLAG|CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE, FILE_WRITE_DATA, &MyNetworkSid)
   };

   ACE_ENTRY MyDaclList[] =
   {
      DEFINE_ACE(ACCESS_DENIED_ACE_TYPE,  CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE, NTFS_FILE_FULL_PERMISSIONS, &MyNetworkSid),
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE, NTFS_FILE_FULL_PERMISSIONS, &MyLocalAdminsSid),
      DEFINE_ACE(ACCESS_ALLOWED_ACE_TYPE, CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE, NTFS_FILE_FULL_PERMISSIONS, &MyCurrentUserSid)
   };

3) Build the SECURITY_DESCRIPTOR
      
   PSECURITY_DESCRIPTOR pSD = NULL;
   CreateTemplateSecurityDescriptorT(&MyCurrentUserSid, &MyCurrentUserSid, 
                                     MySaclList, ARRAYSIZE(MySaclList), 
                                     MyDaclList, ARRAYSIZE(MyDaclList),
                                     SE_DACL_PROTECTED,
                                     &pSD);
      
4) Use the SECURITY_DESCRIPTOR
      
   SECURITY_ATTRIBUTES sa = {sizeof(sa), pSD, FALSE};
   // Make sure SE_SECURITY_NAME privilege is enabled when applying an SD that uses an SACL
   CreateDirectoryW(L"C:\\MyDirectory", &sa);

5) Cleanup

   FreeTemplateSecurityDescriptor(pSD);
 
  --*/

/*
 * Standard access masks for full permissions of common NT securable objects
 */
#define NTFS_FILE_FULL_PERMISSIONS \
   (FILE_READ_DATA|FILE_WRITE_DATA|FILE_APPEND_DATA|FILE_READ_EA|FILE_WRITE_EA|FILE_EXECUTE|FILE_DELETE_CHILD|FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|DELETE|READ_CONTROL|WRITE_DAC|WRITE_OWNER|SYNCHRONIZE)

#define REG_KEY_FULL_PERMISSIONS \
   (KEY_QUERY_VALUE|KEY_SET_VALUE|KEY_CREATE_SUB_KEY|KEY_ENUMERATE_SUB_KEYS|KEY_NOTIFY|KEY_CREATE_LINK|DELETE|READ_CONTROL|WRITE_DAC|WRITE_OWNER)

/*
 * Pseudo SIDs
 *
 * Special SID templates which the creation routines will automatically replace with SIDs 
 * that can only be determined at runtime
 *
 * For all token based SIDs, the thread token is used if present, otherwise the process
 * token is used. 
 */

#define SIDTEMPLATE_AUTHORITY                {4,4,4,4,4,4}

#define SIDTEMPLATE_TOKEN_USER_RID           (0xFFFFFFFFU)
#define SIDTEMPLATE_TOKEN_USER_DOMAIN_RID    (0xFFFFFFFEU)

#define SIDTEMPLATE_TOKEN_OWNER_RID          (0xFFFFFFFDU)
#define SIDTEMPLATE_TOKEN_GROUP_RID          (0xFFFFFFFCU)

#define SIDTEMPLATE_TOKEN_LOGON_SESSION_RID  (0xFFFFFFFBU)

#define SIDTEMPLATE_MAX_RID                  (0xFFFFFFFFU)
#define SIDTEMPLATE_MIN_RID                  (0xFFFFFFFBU)

template < UCHAR MaxSubAuthorityCount >
struct SID_TEMPLATE
{
   UCHAR                      SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY   IdentifierAuthority;
   DWORD                      SubAuthority[MaxSubAuthorityCount];
};

typedef SID_TEMPLATE<1>          GENERIC_SID;
typedef SID_TEMPLATE<1>*         PGENERIC_SID;
typedef const SID_TEMPLATE<1>*   PCGENERIC_SID;

#define GetSizeOfSidTemplate( MaxSubAuthorityCount ) \
   ( (sizeof(GENERIC_SID) - sizeof(DWORD)) + (MaxSubAuthorityCount * sizeof(DWORD)) )

namespace RidList
{
   C_ASSERT(SID_MAX_SUB_AUTHORITIES == 15);

   typedef DWORD RID;

   char (&CountOf(RID))[1];
   char (&CountOf(RID,RID))[2];
   char (&CountOf(RID,RID,RID))[3];
   char (&CountOf(RID,RID,RID,RID))[4];
   char (&CountOf(RID,RID,RID,RID,RID))[5];
   char (&CountOf(RID,RID,RID,RID,RID,RID))[6];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID))[7];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID))[8];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID,RID))[9];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID,RID,RID))[10];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID))[11];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID))[12];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID))[13];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID))[14];
   char (&CountOf(RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID,RID))[15];
}; /* namespace RidList */

#define CountOfRid(...) \
   sizeof(RidList::CountOf(__VA_ARGS__))

#define DEFINE_SID( Name, IdentifierAuthority, ... ) \
   const SID_TEMPLATE<CountOfRid(__VA_ARGS__)> Name = {CountOfRid(__VA_ARGS__), IdentifierAuthority, {__VA_ARGS__}}

typedef struct _ACE_ENTRY
{
   UCHAR          AceType;
   UCHAR          AceFlags;
   ACCESS_MASK    AccessMask;
   PCGENERIC_SID  SidTemplate;
}ACE_ENTRY, *PACE_ENTRY;
typedef const ACE_ENTRY* PCACE_ENTRY;

#define DEFINE_ACE( AceType, AceFlags, AccessMask, SidTemplate ) \
   {AceType, AceFlags, AccessMask, reinterpret_cast<PCGENERIC_SID>(SidTemplate)}

/*++

Pseudo and well known SID templates

 --*/

/*
 * TokenUserSid
 *    SID of the user associated with the caller's effective thread token
 */
DEFINE_SID(TokenUserSid, SIDTEMPLATE_AUTHORITY, SIDTEMPLATE_TOKEN_USER_RID);

/*
 * TokenUserNtDomainAdmins
 *    SID that identifies the domain admins group of the domain associated with the caller's effective thread token.
 *
 *    This is an example of how to use SIDTEMPLATE_TOKEN_USER_DOMAIN_RID as a base for relative domain SIDs. 
 *
 *    Note that the domain SID can expand to multiple entries at runtime, so generation will fail if too many RIDs 
 *    are specified in the template. The total runtime count of sub authories cannot exceed SID_MAX_SUB_AUTHORITIES
 */
DEFINE_SID(TokenUserNtDomainAdmins, SIDTEMPLATE_AUTHORITY, SIDTEMPLATE_TOKEN_USER_DOMAIN_RID, DOMAIN_GROUP_RID_ADMINS);
 
/*
 * TokenOwnerSid
 *     Default owner SID for newly created objects associated with the caller's effective thread token
 */
DEFINE_SID(TokenOwnerSid, SIDTEMPLATE_AUTHORITY, SIDTEMPLATE_TOKEN_OWNER_RID);

/*
 * TokenPrimaryGroupSid
 *    Default primary group SID for newly created objects associated with the caller's effective thread token
 */
DEFINE_SID(TokenPrimaryGroupSid, SIDTEMPLATE_AUTHORITY, SIDTEMPLATE_TOKEN_GROUP_RID);

/*
 * TokenLogonSessionSid
 *    SID that identifies the logon session associated with the caller's effective thread token
 */
DEFINE_SID(TokenLogonSessionSid, SIDTEMPLATE_AUTHORITY, SIDTEMPLATE_TOKEN_LOGON_SESSION_RID);

/*
 * Common well-known SIDs
 */

DEFINE_SID(NullSid,                    SECURITY_NULL_SID_AUTHORITY,     SECURITY_NULL_RID);
DEFINE_SID(WorldSid,                   SECURITY_WORLD_SID_AUTHORITY,    SECURITY_WORLD_RID);
DEFINE_SID(LocalSid,                   SECURITY_LOCAL_SID_AUTHORITY,    SECURITY_LOCAL_RID);
DEFINE_SID(CreatorOwnerSid,            SECURITY_CREATOR_SID_AUTHORITY,  SECURITY_CREATOR_OWNER_RID);
DEFINE_SID(CreatorGroupSid,            SECURITY_CREATOR_SID_AUTHORITY,  SECURITY_CREATOR_GROUP_RID);

DEFINE_SID(NtNetworkSid,               SECURITY_NT_AUTHORITY,           SECURITY_NETWORK_RID);
DEFINE_SID(NtInteractiveSid,           SECURITY_NT_AUTHORITY,           SECURITY_INTERACTIVE_RID);
DEFINE_SID(NtServiceSid,               SECURITY_NT_AUTHORITY,           SECURITY_SERVICE_RID);
DEFINE_SID(NtAnonymousLogonSid,        SECURITY_NT_AUTHORITY,           SECURITY_ANONYMOUS_LOGON_RID);
DEFINE_SID(NtAuthenticatedUserSid,     SECURITY_NT_AUTHORITY,           SECURITY_AUTHENTICATED_USER_RID);
DEFINE_SID(NtRestrictedCodeSid,        SECURITY_NT_AUTHORITY,           SECURITY_RESTRICTED_CODE_RID);
DEFINE_SID(NtTerminalServerSid,        SECURITY_NT_AUTHORITY,           SECURITY_TERMINAL_SERVER_RID);
DEFINE_SID(NtLocalSystemSid,           SECURITY_NT_AUTHORITY,           SECURITY_LOCAL_SYSTEM_RID);
DEFINE_SID(NtBuiltinAdministratorSid,  SECURITY_NT_AUTHORITY,           SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);

DWORD
APIENTRY
BuildTemplateSid(
   __in PCGENERIC_SID SidTemplate,
   __out_bcount(SidSize) PSID Sid,
   __in ULONG SidSize
);

template < typename SidTemplateT >
inline
DWORD
BuildTemplateSid(
   __in typename SidTemplateT* SidTemplate,
   __out_bcount(SidSize) PSID Sid,
   __in ULONG SidSize
)
{
   return ( BuildTemplateSid(reinterpret_cast<PCGENERIC_SID>(SidTemplate),
                             Sid,
                             SidSize) );
}

DWORD
APIENTRY
BuildTemplateAcl(
   __in_ecount(AceListCount) PCACE_ENTRY AceList,
   __in WORD AceListCount,
   __out_bcount(AclSize) PACL Acl,
   __in ULONG AclSize
);

DWORD
APIENTRY
CreateTemplateSecurityDescriptor(
   __in_opt PCGENERIC_SID OwnerSid,
   __in_opt PCGENERIC_SID GroupSid,
   __in_ecount_opt(SaclListCount) PCACE_ENTRY SaclList,
   __in WORD SaclListCount,
   __in_ecount_opt(DaclListCount) PCACE_ENTRY DaclList,
   __in WORD DaclListCount,
   __in WORD ControlFlags,
   __out PSECURITY_DESCRIPTOR* SecurityDescriptor
);

template < typename OwnerSidT, typename GroupSidT >
inline
DWORD 
CreateTemplateSecurityDescriptor(
   __in_opt typename OwnerSidT* OwnerSid,
   __in_opt typename GroupSidT* GroupSid,
   __in_ecount_opt(SaclListCount) PCACE_ENTRY SaclList,
   __in WORD SaclListCount,
   __in_ecount_opt(DaclListCount) PCACE_ENTRY DaclList,
   __in WORD DaclListCount,
   __in WORD ControlFlags,
   __out PSECURITY_DESCRIPTOR* SecurityDescriptor
)
{
   return ( CreateTemplateSecurityDescriptor(reinterpret_cast<PCGENERIC_SID>(OwnerSid), 
                                             reinterpret_cast<PCGENERIC_SID>(GroupSid), 
                                             SaclList, 
                                             SaclListCount, 
                                             DaclList, 
                                             DaclListCount, 
                                             ControlFlags, 
                                             SecurityDescriptor) );
}

void
APIENTRY
FreeTemplateSecurityDescriptor(
   __in PSECURITY_DESCRIPTOR SecurityDescriptor
);
