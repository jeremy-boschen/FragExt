
#define _INC_COMDEF
#include <windows.h>
#include <fragsvx.h>
#include <stdio.h>
#include <Sddl.h>
#define WINERROR( e ) (0 != (e))

#include <RWLock.h>
#include <GuidDB.h>
#include <DbgUtil.h>

#include <utility.h>
#include <SeUtil.h>
#include <PathUtil.h>

#include <NTVersion.h>

#include <CacheList.h>

#if 0
typedef struct _SE_CHECKLIST_ENTRY
{
   ULONG                   Flags;
   union {
      LPCWSTR              PrivilegeName;
      PCGENERIC_SID        TemplateSid;
      WELL_KNOWN_SID_TYPE  WellKnownSid;
   };
}SE_CHECKLIST_ENTRY, *PSE_CHECKLIST_ENTRY;
typedef const struct _SE_CHECKLIST_ENTRY* PCSE_CHECKLIST_ENTRY;

typedef struct _SE_CHECKLIST
{
   SE_CHECKLIST_ENTRY   Entry[];
}SE_CHECKLIST, *PSE_CHECKLIST;
typedef const struct _SE_CHECKLIST* PCSE_CHECKLIST;

#define SECM_IMPERSONATE_MASK          0x0000000FU
#define SECM_IMPERSONATE_COM           0x00000001U
#define SECM_IMPERSONATE_RPC           0x00000002U
#define SECM_IMPERSONATE_PIPE          0x00000003U

#define SECM_ENTRYTYPE_MASK            0x000000FFU
#define SECM_ENTRYTYPE_PRIVILEGE       0x00000001U
#define SECM_ENTRYTYPE_TEMPLATESID     0x00000002U
#define SECM_ENTRYTYPE_WELLKNOWNSID    0x00000003U

#define SECM_ENTRYTYPE_DISALLOW        0x00000010U

#define SECM_ENTRYFLAG_MASK            0xFFFFFF00U
#define SECM_ENTRYFLAG_REPORT          0x00000100U
#define SECM_ENTRYFLAG_RAISE           0x00000200U
#define SECM_ENTRYFLAG_ASSERT          0x00000400U

#define SECM_ENTRYFLAG_DUMPSTACK       0x00000800U

#define BEGIN_SECHECK_SID_MAP( Flags ) \
   static PCSE_CHECKLIST _SeCheckGetSidMap( ) \ { \
      static const SE_CHECKLIST __SidMap = \ { \ { 

#define SECHECK_ENTRY_PRIVILEGE( Privilege, Flags ) \ {SECM_ENTRYTYPE_PRIVILEGE|(static_cast<ULONG>(Flags) & ~SECM_ENTRYTYPE_MASK), reinterpret_cast<LPCWSTR>(Privilege)},

#define SECHECK_ENTRY_TEMPLATESID( Sid, Flags ) \ {SECM_ENTRYTYPE_TEMPLATESID|(static_cast<ULONG>(Flags) & ~SECM_ENTRYTYPE_MASK), reinterpret_cast<LPCWSTR>(Sid)},

#define SECHECK_ENTRY_WELLKNOWNSID( Sid, Flags ) \ {SECM_ENTRYTYPE_WELLKNOWNSID|(static_cast<ULONG>(Flags) & ~SECM_ENTRYTYPE_MASK), reinterpret_cast<LPCWSTR>(Sid)},

#define END_SECHECK_SID_MAP( ) \ {0, reinterpret_cast<LPCWSTR>(NULL)} \
         } \
      }; \
      return ( &(__SidMap) ); \
   } 

///////////////////////

template < class T >
class CSeCheckT
{
protected:
   static
   void
   APIENTRY
   SeCheckPerformTokenAnalysis(
      __in ULONG Flags,
      __in_opt HANDLE TokenHandle,
      __in_opt PVOID ImpersonationHandle
   ) {
      HANDLE                        hThreadToken;
      BOOLEAN                       bImpersonating;

      PSID                          pSid;
      DWORD                         cbSid;
      BYTE                          SidWorkSpace[SECURITY_MAX_SID_SIZE];
      
      PSID                          pDomainSid;
      BYTE                          DomainWorkSpace[SECURITY_MAX_SID_SIZE];

      PCSE_CHECKLIST                pSecMap;
      PCSE_CHECKLIST_ENTRY          pMapEntry;
      DWORD                         dwFlags;
      
      DWORD                         dwRet;
      BOOL                          bIsTokenMember;

      PTOKEN_GROUPS_AND_PRIVILEGES  pTokenInformation;

      /*
       * Define a template SID that we can use to retrieve the user's domain SID
       */
      DEFINE_SID(TokenUserNtDomain, 
                 SIDTEMPLATE_AUTHORITY, 
                 SIDTEMPLATE_TOKEN_USER_DOMAIN_RID);

      /* Initialize locals.. */
      hThreadToken      = NULL;
      bImpersonating    = FALSE;
      pTokenInformation = NULL;
      
      /*
       * Get the SE_CHECKLIST which controls the validations to perform
       */
      pSecMap = T::_SeCheckGetSidMap();

      if ( !TokenHandle ) {
         /*
          * Check if there's already an impersonation token and use that if
          * it exists
          */
         hThreadToken = GetThreadToken(GetCurrentThread(),
                                       TOKEN_READ);

         if ( !hThreadToken ) {
            /*
             * This thread isn't impersonating yet, so do the impersonation
             * specified in the SE_CHECKLIST
             */
            switch ( Flags & SECM_IMPERSONATE_MASK ) {
               case SECM_IMPERSONATE_COM:
                  if ( SUCCEEDED(CoImpersonateClient()) ) {
                     bImpersonating = TRUE;
                  }
                  break;

               case SECM_IMPERSONATE_RPC:
                  if ( RPC_S_OK == RpcImpersonateClient(ImpersonationHandle) ) {
                     bImpersonating = TRUE;
                  }
                  break;

               case SECM_IMPERSONATE_PIPE:
                  if ( ImpersonateNamedPipeClient(ImpersonationHandle) ) {
                     bImpersonating = TRUE;
                  }
                  break;

               default:
                  /* Invalid impersonation type */
                  _ASSERT(FALSE);
                  break;
            }

            if ( !bImpersonating ) {
               /* Failure - No token to validate */
               goto __CLEANUP;
            }

            /*
             * We're now impersonating some client, so lets get that token
             */
            hThreadToken = GetThreadToken(GetCurrentThread(),
                                          TOKEN_READ);

            switch ( Flags & SECM_IMPERSONATE_MASK ) {
               case SECM_IMPERSONATE_COM:
                  CoRevertToSelf();
                  break;

               case SECM_IMPERSONATE_RPC:
                  RpcRevertToSelfEx(ImpersonationHandle);
                  break;

               case SECM_IMPERSONATE_PIPE:
                  RevertToSelf();
                  break;

               DEFAULT_UNREACHABLE;
            }

            if ( !hThreadToken ) {
               /* 
                * Well something went wrong, so bail
                */
               goto __CLEANUP;
            }
         }

         /*
          * We have a token to validate..
          */
         TokenHandle = hThreadToken;
      }

      /*
       * Try to get the domain SID for the token
       */
      ZeroMemory(&(DomainWorkSpace[0]),
                 sizeof(DomainWorkSpace));

      pDomainSid = reinterpret_cast<PSID>(&(DomainWorkSpace[0]));

      dwRet = BuildTemplateSid(&(TokenUserNtDomain),
                               pDomainSid,
                               sizeof(DomainWorkSpace));

      if ( WINERROR(dwRet) ) {
         /*
          * We'll assume the token is not a domain SID
          */
         pDomainSid = NULL;
      }

      /*
       * Load the token data we'll need to check
       */
      pTokenInformation = (PTOKEN_GROUPS_AND_PRIVILEGES)GetTokenInformationData(TokenHandle,
                                                  TokenGroupsAndPrivileges,
                                                  NULL);

      if ( !pTokenInformation ) {
         /* Failure */
         dwRet = GetLastError();

         /*
          * Nothing we can do now..
          */
         goto __CLEANUP;
      }

      /*
       * Walk the entry list and perform each check
       */
      for ( pMapEntry = &(pSecMap->Entry[0]); pMapEntry->PrivilegeName; pMapEntry++ ) {
         dwFlags = (pMapEntry->Flags & SECM_ENTRYFLAG_MASK);

         switch ( pMapEntry->Flags & SECM_ENTRYTYPE_MASK ) {
            case SECM_ENTRYTYPE_PRIVILEGE:
               break;

            case SECM_ENTRYTYPE_TEMPLATESID:
               pSid  = reinterpret_cast<PSID>(&(SidWorkSpace[0]));
               cbSid = sizeof(SidWorkSpace);

               dwRet = BuildTemplateSid(pMapEntry->TemplateSid,
                                        pSid,
                                        cbSid);

               if ( !WINERROR(dwRet) ) {
                  bIsTokenMember = FALSE;

                  if ( CheckTokenMembership(TokenHandle,
                                            pSid,
                                            &bIsTokenMember) && bIsTokenMember ) {
                     SeCheckAlert(TokenHandle,
                                  pSid,
                                  dwFlags);
                  }
               }
               break;

            case SECM_ENTRYTYPE_WELLKNOWNSID:
               pSid  = reinterpret_cast<PSID>(&(SidWorkSpace[0]));
               cbSid = sizeof(SidWorkSpace);
               
               if ( CreateWellKnownSid(pMapEntry->WellKnownSid,
                                       pDomainSid,
                                       pSid,
                                       &cbSid) ) {
                  bIsTokenMember = FALSE;

                  if ( CheckTokenMembership(TokenHandle,
                                            pSid,
                                            &bIsTokenMember) && bIsTokenMember ) {
                     SeCheckAlert(TokenHandle,
                                  pSid,
                                  dwFlags);
                  }
               }

               break;

            DEFAULT_UNREACHABLE;
         }
      }

__CLEANUP:
      if ( pTokenInformation ) {
         FreeTokenInformationData(pTokenInformation);
      }

      if ( hThreadToken ) {
         CloseHandle(hThreadToken);
      }
   }

   static
   BOOLEAN
   SeCheckValidateTokenSid(
      __in PTOKEN_GROUPS_AND_PRIVILEGES TokenInformation,
      __in PSID Sid,
      __in ULONG Flags
   ) {
      DWORD                cSidCount;
      PSID_AND_ATTRIBUTES  pSidAttributes;
      
      BOOLEAN              bExists;

      cSidCount      = TokenInformation->SidCount;
      pSidAttributes = TokenInformation->Sids;
      bExists        = FALSE;

      /*
       * Check the restricted SIDs first
       */
      while ( cSidCount-- ) {
         if ( !FlagOn(pSidAttributes->Attributes, SE_GROUP_USE_FOR_DENY_ONLY) ) {
            if ( EqualSid(Sid,
                          pSidAttributes->Sid) ) {
               bExists = TRUE;
               break;
            }
         }
      }

      if ( bExists ) {

      }
   }

   static
   void
   SeCheckAlert(
      __in HANDLE TokenHandle,
      __in PSID Sid,
      __in ULONG Flags
   ) {
      if ( FlagOn(Flags, SECM_ENTRYFLAG_REPORT) ) {
      }

      if ( FlagOn(Flags, SECM_ENTRYFLAG_RAISE) ) {
      }

      if ( FlagOn(Flags, SECM_ENTRYFLAG_ASSERT) ) {
      }
   }
};

////////////////////

class CTest : public CSeCheckT<CTest>
{
public:
/*
   BEGIN_SECHECK_SID_MAP(SECM_IMPERSONATE_COM)
      SECHECK_ENTRY_WELLKNOWNSID(WinAccountAdministratorSid, SECM_ENTRYTYPE_DISALLOW)
      SECHECK_ENTRY_PRIVILEGE(SE_TCB_NAME, SECM_ENTRYTYPE_DISALLOW)
   END_SECHECK_SID_MAP()
*/

   void CheckIt() {
      //SeCheckPerformTokenAnalysis(SECM_IMPERSONATE_COM, NULL, NULL);
   }
};

/////////////////////////

class CFSxFileDefragmenterCallback : public IFSxDefragmentFileCallback, private CTest
{
public:
   CFSxFileDefragmenterCallback( 
   ) {
      _cReferenceCount = 0;
   }

   HRESULT 
   STDMETHODCALLTYPE 
   QueryInterface( 
      REFIID riid,
      __RPC__deref_out void** ppvObject
   ) {
      WCHAR chGUID[GDB_CCH_MAXGUIDNAME];
      
      if ( !ppvObject ) {
         return ( E_POINTER );
      }

      if ( NO_ERROR == LookupNameOfGuidW(riid,
                                         chGUID,
                                         _countof(chGUID)) ) {
         DbgPrintfW(_L(__FUNCTION__) L"!%s\n",
                    chGUID);
      }

      if ( IsEqualGUID(riid, __uuidof(IUnknown)) ) {
         AddRef();
         (*ppvObject) = static_cast<IUnknown*>(this);
         return ( S_OK );
      }

      if ( IsEqualGUID(riid, __uuidof(IFSxDefragmentFileCallback)) ) {
         AddRef();
         (*ppvObject) = static_cast<IFSxDefragmentFileCallback*>(this);
         return ( S_OK );
      }

      (*ppvObject) = NULL;
      return ( E_NOINTERFACE );
   }

   ULONG 
   STDMETHODCALLTYPE 
   AddRef( 
   ) {
      return ( (ULONG)InterlockedIncrement((volatile LONG*)&_cReferenceCount) );
   }

   ULONG 
   STDMETHODCALLTYPE 
   Release( 
   ) {
      if ( SUCCEEDED(CoImpersonateClient()) ) {
         CheckIt();
         CoRevertToSelf();
      }

      ULONG cReferenceCount;
      cReferenceCount = (ULONG)InterlockedDecrement((volatile LONG*)&_cReferenceCount);
      if ( 0 == cReferenceCount ) {
         delete this;
      }

      return ( cReferenceCount );
   }

   HRESULT STDMETHODCALLTYPE OnDefragmentFileUpdate( 
      __RPC__in_string LPCWSTR FileName,
      ULONG PercentComplete,
      ULONG_PTR CallbackParameter
   )  {
      UNREFERENCED_PARAMETER(FileName);
      UNREFERENCED_PARAMETER(PercentComplete);
      UNREFERENCED_PARAMETER(CallbackParameter);

      RpcRaiseException(RPC_X_ENUM_VALUE_OUT_OF_RANGE);

      // test cancel support in fragsvx
      static int i = 0;
      if ( ++i > 5 ) {
         //Sleep(30000);
      }

      return ( S_OK );
   }

   static 
   HRESULT
   CreateInstance(
      REFIID riid,
      void** ppvObject
   ) {
      HRESULT                       hr;
      CFSxFileDefragmenterCallback* pCallback;

      pCallback = new CFSxFileDefragmenterCallback();
      if ( !pCallback ) {
         (*ppvObject) = NULL;
         return ( E_OUTOFMEMORY );
      }

      hr = pCallback->QueryInterface(riid,
                                     ppvObject);
      if ( FAILED(hr) ) {
         (*ppvObject) = NULL;
         delete pCallback;
      }

      return ( hr );
   }

   ULONG _cReferenceCount;
};

DWORD
WINAPI
DefragThreadProc(
   PVOID Parameter
)
{
   Parameter;
   HRESULT hr;     
   hr = CoInitializeEx(0, 
                       COINIT_MULTITHREADED);

   if ( SUCCEEDED(hr) ) {
      IFSxFileDefragmenter* pFileDefragmenter = NULL;
      hr = FSxManagerQueryService(__uuidof(FSxFileDefragmenter),
                                  __uuidof(IFSxFileDefragmenter),
                                  reinterpret_cast<void**>(&pFileDefragmenter));

      if ( SUCCEEDED(hr) ) {
         IFSxDefragmentFileCallback* pCallback = NULL;
         hr = CFSxFileDefragmenterCallback::CreateInstance(__uuidof(IFSxDefragmentFileCallback), 
                                                           reinterpret_cast<void**>(&pCallback));

         if ( SUCCEEDED(hr) ) {
            hr = pFileDefragmenter->SetCallbackInterface(pCallback);
            if ( SUCCEEDED(hr) ) {
               LPCWSTR pszFile;
               pszFile = L"D:\\Projects\\Programming\\Active\\FragExt.WDK\\FragSvx.Unit\\objchk_wnet_amd64\\amd64\\FragSvx.Unit.pdb";

#if 0
               if ( (int)Parameter <= 1 ) {
                  pszFile = L"\\\\.\\pipe\\FragSvx";
               }

               if ( (int)Parameter == 0 ) {
                  HANDLE hPipe;
                  hPipe   = CreateNamedPipe(pszFile, PIPE_ACCESS_DUPLEX|FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_NOWAIT, 1, 1, 1, 5000, NULL);
                  if ( hPipe ) {
                     HANDLE hEvent;
                     hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

                     if ( hEvent ) {
                        OVERLAPPED Overlapped = {0};
                        Overlapped.hEvent = hEvent;

                        DisconnectNamedPipe(hPipe);
                        if ( 0 == ConnectNamedPipe(hPipe, &Overlapped) ) {
                           WaitForSingleObject(hEvent, INFINITE);
                           DWORD cbTransferred = 0;
                           if ( GetOverlappedResult(hPipe, &Overlapped, &cbTransferred, TRUE) ) {
                              if ( ImpersonateNamedPipeClient(hPipe) ) {
                                 DebugBreak();
                                 RevertToSelf();
                              }
                           }
                        }

                        CloseHandle(hEvent);
                     }

                     CloseHandle(hPipe);
                  }
               }

               if ( (int)Parameter == 1 ) {
                  Sleep(5000);
               }
#endif //0
               hr = pFileDefragmenter->DefragmentFile(pszFile, 0, GetCurrentThreadId());               
            }

            pCallback->Release();
         }

         pFileDefragmenter->Release();
      } 

      CoUninitialize();
   }

   return ( 0 );
}

void
RunDefragTests(
)
{
   if ( SUCCEEDED(CoInitializeEx(0, COINIT_MULTITHREADED)) ) {
      HRESULT hr;      
      hr = CoInitializeSecurity(NULL,
                                -1, 
                                NULL, 
                                NULL, 
                                RPC_C_AUTHN_LEVEL_DEFAULT, 
                                RPC_C_IMP_LEVEL_IMPERSONATE,
                                NULL, 
                                EOAC_STATIC_CLOAKING|EOAC_DISABLE_AAA|EOAC_NO_CUSTOM_MARSHAL, 
                                NULL);

      if ( SUCCEEDED(hr) ) {
         int    idx;
         HANDLE rgThread[2];

         for ( idx = 0; idx < ARRAYSIZE(rgThread); idx++ ) {
            rgThread[idx] = CreateThread(NULL,
                                         0,
                                         DefragThreadProc,
                                         (PVOID)idx,
                                         0,
                                         NULL);
         }

         DWORD dwWait;
         dwWait = WaitForMultipleObjects(ARRAYSIZE(rgThread),
                                         &(rgThread[0]),
                                         TRUE,
                                         INFINITE);

         for ( idx = 0; idx < ARRAYSIZE(rgThread); idx++ ) {
            if ( NULL != rgThread[idx] ) {
               CloseHandle(rgThread[idx]);
            }
         }
      }

      CoUninitialize();
   }
}

void
RunRWLockTests(
)
{
   DWORD    dwRet;
   RWXLOCK  Lock;

   InitializeRWXLockLibrary();

   dwRet = InitializeRWXLock(&(Lock));
   if ( NO_ERROR != dwRet ) {
      /* Failure */
      return;
   }

   AcquireRWXLockExclusive(&(Lock)); {
   }
   ReleaseRWXLockExclusive(&(Lock));

   AcquireRWXLockShared(&(Lock)); {
   }
   ReleaseRWXLockShared(&(Lock));

   UninitializeRWXLock(&(Lock));
}

#if 0
template < class T, class Base >
class ATL_NO_VTABLE CFxObjectRootEx : public Base
{
protected:
   typedef struct _FX_SID {
      DWORD                   Flags;
      WELL_KNOWN_SID_TYPE  SidType;
      struct {
         SID_IDENTIFIER_AUTHORITY   Authority;
         DWORD                      SubAuthority;
      };
   }FX_SID, *PFX_SID;

#ifdef _FX_COM_SECURITY_CHECKS
   static void _LookupAccountSid( PSID pSid, LPTSTR pszAccountName, size_t cchAccountName ) {
      SID_NAME_USE   SidType;
      DWORD          cchName;
      DWORD          cchDomain;
      TCHAR          chName[256+1]    = {0};
      TCHAR          chDomain[256+1]  = {0};

      cchName   = sizeof(chName) / sizeof(chName[0]);
      cchDomain = sizeof(chDomain) / sizeof(chDomain[0]);

      if ( LookupAccountSid(NULL, pSid, chName, &cchName, chDomain, &cchDomain, &SidType) ) {
         StringCchPrintf(pszAccountName,
                         cchAccountName,
                         _T("%s\\%s"),
                         chDomain,
                         chName);
      }
      else {
         StringCchCopy(pszAccountName,
                       cchAccountName,
                       _T("<UNKNOWN>"));
      }
   }

   static void _CheckImpersonationUserName( ) {
      HANDLE hProcessToken = NULL;
      if ( OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken) ) {
         HANDLE hThreadToken = NULL;         
            
         if ( !OpenThreadToken(GetCurrentThread(),
                            TOKEN_QUERY,
                            TRUE,
                            &hThreadToken) ) {
            if ( ERROR_NO_TOKEN == GetLastError() ) {
               if ( SUCCEEDED(CoImpersonateClient()) ) {
                  OpenThreadToken(GetCurrentThread(),
                                  TOKEN_QUERY,
                                  TRUE,
                                  &hThreadToken);

                  CoRevertToSelf();
               }
            }
         }

         if ( hThreadToken ) {
            PTOKEN_USER pThreadUser  = (PTOKEN_USER)GetTokenInformationData(hThreadToken, TokenUser, NULL);
            PTOKEN_USER pProcessUser = (PTOKEN_USER)GetTokenInformationData(hProcessToken, TokenUser, NULL);

            if ( pThreadUser && pProcessUser && !EqualSid(pThreadUser->User.Sid, pProcessUser->User.Sid) ) {
               TCHAR chThreadName[512+1]  = {0};
               TCHAR chProcessName[512+1] = {0};

               _LookupAccountSid(pThreadUser->User.Sid, chThreadName, sizeof(chThreadName) / sizeof(chThreadName[0]));
               _LookupAccountSid(pProcessUser->User.Sid, chProcessName, sizeof(chProcessName) / sizeof(chProcessName[0]));

               ATLTRACE2(atlTraceQI, 0, _T("***\n\tWARNING - Thread and process token users are not equal. Thread=%s, Process=%s\n\n"), chThreadName, chProcessName);
            }

            FreeTokenInformationData(pThreadUser);
            FreeTokenInformationData(pProcessUser);

            CloseHandle(hThreadToken);
         }

         CloseHandle(hProcessToken);
      }
   }

   static void _CheckImpersonationUserType( ) {
      HANDLE               hThreadToken;

      BOOL                 bIsMember;
      
      PSID                 pSid;
      UCHAR                SidBuf[SECURITY_MAX_SID_SIZE];
      DWORD                cbSid;
      LPTSTR               pszSidName;
      TCHAR                chAccountName[512+1];

      size_t               idx;
      WELL_KNOWN_SID_TYPE  SidTypeList[] = {
         WinServiceSid,
         WinLocalSystemSid,
         WinLocalServiceSid,
         WinNetworkServiceSid
      };

      hThreadToken = NULL;

      pSid         = (PSID)SidBuf;
      pszSidName   = NULL;

      if ( !OpenThreadToken(GetCurrentThread(),
                            TOKEN_QUERY,
                            TRUE,
                            &hThreadToken) ) {
         if ( ERROR_NO_TOKEN == GetLastError() ) {
            if ( SUCCEEDED(CoImpersonateClient()) ) {
               OpenThreadToken(GetCurrentThread(),
                               TOKEN_QUERY,
                               TRUE,
                               &hThreadToken);

               CoRevertToSelf();
            }
         }
      }

      if ( hThreadToken ) {
         for ( idx = 0; idx < (sizeof(SidTypeList) / sizeof(SidTypeList[0])); idx++ ) {
            cbSid = sizeof(SidBuf);

            if ( CreateWellKnownSid(SidTypeList[idx],
                                    NULL,
                                    pSid,
                                    &cbSid) ) {
               bIsMember = FALSE;

               if ( CheckTokenMembership(hThreadToken,
                                         pSid,
                                         &bIsMember) && bIsMember ) {
                  if ( !ConvertSidToStringSid(pSid,
                                              &pszSidName) ) {
                     pszSidName = NULL;
                  }

                  _LookupAccountSid(pSid, chAccountName, sizeof(chAccountName) / sizeof(chAccountName[0]));

                  ATLTRACE2(atlTraceQI, 0, _T("***\n\tWARNING - Impersonating credentials contains a dangerous SID[%s], Account[%s]\n\n"), pszSidName ? pszSidName : _T("<NAME NOT MAPPED>"), chAccountName);

                  if ( pszSidName ) {
                     LocalFree(pszSidName);
                  }
               }
            }
         }

         CloseHandle(hThreadToken);
      }
   }

   static void _RunImpersonationChecks( ) {
      _CheckImpersonationUserName();
      _CheckImpersonationUserType();
   }

public:
   STDMETHOD_(ULONG, AddRef)( )  {
      _RunImpersonationChecks();
      return ( Base::AddRef() );
   }

   STDMETHOD_(ULONG, Release)() {
      _RunImpersonationChecks();
      return ( Base::Release() );
   }

   STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw() {
      _RunImpersonationChecks();
      return ( Base::QueryInterface(iid, ppvObject) );
   }
#endif /* _FX_COM_SECURITY_CHECKS */
};

#endif //0

#include <process.h>
#ifndef _WIN64
#define _WIN64
#endif


#ifdef _WIN64
   #define _RotatePtr _rotr64
#else /* !_WIN64 */
   #define _RotatePtr _rotr
#endif /* _WIN64 */

PVOID 
APIENTRY
xSeEncodePointer( 
   PVOID Pointer 
)
{
   const uintptr_t uintptr_t_bitcount = (sizeof(uintptr_t) * CHAR_BIT);
   return ( (PVOID)_RotatePtr(__security_cookie ^ (uintptr_t)Pointer, (unsigned char)(__security_cookie & (uintptr_t)(uintptr_t_bitcount - 1))) );
}

PVOID 
APIENTRY
xSeDecodePointer( 
   PVOID Pointer 
)
{  
   const uintptr_t uintptr_t_bitcount = (sizeof(uintptr_t) * CHAR_BIT);
   return ( (PVOID)(__security_cookie ^ (uintptr_t)_RotatePtr((uintptr_t)Pointer, (unsigned char)(uintptr_t_bitcount - (__security_cookie & (uintptr_t_bitcount - 1))))) );
}

#include <msxml6.h>

typedef struct _PROGRAM_TARGET
{
   LPCWSTR  Version;
   LPCWSTR  PackageType;
   LPCWSTR  Url;
}PROGRAM_TARGET, *PPROGRAM_TARGET;

typedef struct _PROGRAM_VERSION_INFORMATION
{
   LPCWSTR        Name;
   LPCWSTR        Url;
   DWORD          Platform;
   DWORD          BuildType;
   PROGRAM_TARGET Targets[1];
}PROGRAM_VERSION_INFORMATION, *PPROGRAM_VERSION_INFORMATION;

HRESULT
HttpQueryProgramVersionInformation(
   __in_z LPCWSTR ProgramName,
   __in DWORD Platform,
   __in DWORD BuildType,
   __out PPROGRAM_VERSION_INFORMATION* ProgramVersionInformation
)
{
   //1. Create service URL
   //2. Create XMLHTTPRequestObject
}

#include <Winhttp.h>

#ifdef _DEBUG
   #define PGV_BUILDTYPE 1
#else /* _DEBUG */
   #define PGV_BUILDTYPE 0
#endif


DWORD
HttpQueryLatestProgramVersionEx(
   __in DWORD ProgramID,
   __in DWORD PlatformID,
   __in DWORD BuildTypeID,
   __out_ecount(cchLastestProgramVersion) LPWSTR LastestProgramVersion,
   __in size_t cchLastestProgramVersion
)
{  
   DWORD          dwRet;

   HRESULT        hr;

   HINTERNET      hSession;
   HINTERNET      hConnect;
   HINTERNET      hRequest;

   BOOL           bRet;
   
   WCHAR          chObjectName[96];

   DWORD          cbHeaderData;
   DWORD          dwHttpStatus;

   /* Initialize locals */
   dwRet     = NO_ERROR;
   hSession  = NULL;
   hConnect  = NULL;
   hRequest  = NULL;

   hSession = WinHttpOpen(L"mutexed.com Service Client/1.0",
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS,
                          0);

   if ( !hSession ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   hConnect = WinHttpConnect(hSession,
                             L"localhost",
                             INTERNET_DEFAULT_HTTP_PORT,
                             0);

   if ( !hConnect ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   ZeroMemory(chObjectName,
              sizeof(chObjectName));

   /*
    * Build the query string for the service
    */
   hr = StringCchPrintfW(chObjectName,
                         ARRAYSIZE(chObjectName),
                         L"/code/services/QueryLatestProgramVersion.aspx?ProgramID=%08lx&PlatformID=%08lx&BuildTypeID=%01lx",
                         ProgramID,
                         PlatformID,
                         BuildTypeID);

   if ( FAILED(hr) ) {
      dwRet = (DWORD)hr;
      /* Failure */
      goto __CLEANUP;
   }

   hRequest = WinHttpOpenRequest(hConnect,
                                 L"GET",
                                 chObjectName,
                                 NULL,
                                 NULL,
                                 NULL,
                                 WINHTTP_FLAG_REFRESH);

   if ( !hRequest ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   bRet = WinHttpSendRequest(hRequest,
                             WINHTTP_NO_ADDITIONAL_HEADERS,
                             0,
                             WINHTTP_NO_REQUEST_DATA,
                             0,
                             0,
                             NULL);

   if ( !bRet ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   bRet = WinHttpReceiveResponse(hRequest,
                                 NULL);

   if ( !bRet ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }
   
   /*
    * Everything is in the headers, so we're good to go now
    */

   /*
    * Verify that the request suceeded. 
    */
   dwHttpStatus = 0;
   cbHeaderData = sizeof(DWORD);

   bRet = WinHttpQueryHeaders(hRequest,
                              WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                              NULL,
                              &dwHttpStatus,
                              &cbHeaderData,
                              WINHTTP_NO_HEADER_INDEX);

   if ( !bRet ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   if ( HTTP_STATUS_OK != bRet ) {
      dwRet = ERROR_INVALID_DATA;
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Check for the version upgrade header
    */
   cbHeaderData = (DWORD)(cchLastestProgramVersion * sizeof(WCHAR));

   bRet = WinHttpQueryHeaders(hRequest,
                              WINHTTP_QUERY_CUSTOM,
                              L"X-Lastest-Version",
                              LastestProgramVersion,
                              &cbHeaderData,
                              WINHTTP_NO_HEADER_INDEX);

   if ( bRet ) {
      dwRet = ERROR_INVALID_DATA;
      /* Failure */
      goto __CLEANUP;
   }

   dwRet = GetLastError();

   if ( ERROR_WINHTTP_HEADER_NOT_FOUND == dwRet ) {
      dwRet = ERROR_INVALID_DATA;
      /* Failure */
      goto __CLEANUP;
   }

__CLEANUP:
   if ( hRequest ) {
      WinHttpCloseHandle(hRequest);
   }

   if ( hConnect ) {
      WinHttpCloseHandle(hConnect);
   }

   if ( hSession ) {
      WinHttpCloseHandle(hSession);
   }

   /* Success / Failure */
   return ( dwRet );
}
   

#include <Product.Version>
DWORD
APIENTRY
HttpQueryLatestProductProgramVersion(
   __out_ecount(cchLastestProgramVersion) LPWSTR LastestProgramVersion,
   __in size_t cchLastestProgramVersion
)
{
   DWORD dwRet;

   dwRet = HttpQueryLatestProgramVersionEx(PRODUCTPROGRAMID, 
                                           g_NTVersion, 
                                           PGV_BUILDTYPE,
                                           LastestProgramVersion, 
                                           cchLastestProgramVersion);
   
   return ( dwRet );
}

#endif //0

#include <WinLib.h>


class CTestWindow : public WindowInstanceT<CTestWindow, Single>,
                    public WindowMessageHandler
{
public:
   DECLARE_WNDCLASSINFO(L"MyWindow", CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, COLOR_WINDOW);

   BEGIN_MSG_ROUTER()
      ADAPTER_ENTRY(OnNCCreate,     MESSAGE_ADAPTER(WM_CREATE))
      ADAPTER_ENTRY(OnSysCommand,   SYSCOMMAND_ADAPTER(SC_CLOSE))

      ADAPTER_ENTRY(OnCommand,      COMMAND_ADAPTER(0, 100))
   END_MSG_ROUTER()

   LRESULT OnNCCreate( MSG_INFO& MsgInfo, BOOLEAN& b  ) {
      MsgInfo;
      ShowWindow(_hWnd, SW_NORMAL);
      b = FALSE;
      return 0;
   }

   LRESULT OnSysCommand( MSG_INFO MsgInfo, BOOLEAN& b ) {
      MsgInfo;
      DestroyWindow(_hWnd);
      PostQuitMessage(0);
      b = FALSE;

      return 0;
   }

   LRESULT OnCommand( MSG_INFO MsgInfo, BOOLEAN& b ) {
      MsgInfo;
      b;
      return 0;
   }
};

#if 0
class MyWindow : public CWndProcT<MyWindow>
{
public:
   MyWindow( ) throw() {
      _hWnd = (HWND)0x12345678;
      //InitializeWndProcThunk(this, &MyWindow::WndProc);
   }

   LRESULT
   CALLBACK
   WndProc(
      HWND hWnd,
      UINT uMsg,
      WPARAM wParam,
      LPARAM lParam
   )
   {
      switch ( uMsg ) {
         case WM_NCCREATE:
            _hWnd = hWnd;
            //DestroyWindow(hWnd);
            break;
      }

      return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }

   //DECLARE_WNDPROCTHUNK(MyWindow);
   
   HWND _hWnd;
};
class MyWindow2 : public CWndProcT<MyWindow2>
{
public:
   MyWindow2( ) throw() {
      _hWnd = (HWND)0x12345678;
      //InitializeWndProcThunk(this, &MyWindow::WndProc);
   }

   LRESULT
   CALLBACK
   WndProc(
      HWND hWnd,
      UINT uMsg,
      WPARAM wParam,
      LPARAM lParam
   )
   {
      switch ( uMsg ) {
         case WM_NCCREATE:
            _hWnd = hWnd;
            //DestroyWindow(hWnd);
            break;
      }

      return DefWindowProc(hWnd, uMsg, wParam, lParam);
   }

   //DECLARE_WNDPROCTHUNK(MyWindow);
   
   HWND _hWnd;
};

#endif
int 
__cdecl
wmain( 
   int argc, 
   wchar_t *argv[], 
   wchar_t *envp[] 
)
{
   UNREFERENCED_PARAMETER(argc);
   UNREFERENCED_PARAMETER(argv);
   UNREFERENCED_PARAMETER(envp);
   
   {
      CTestWindow cWnd;

      POINT pt = {50, 50};
      SIZE sz = {300, 500};
      HWND h = cWnd.CreateT(NULL, L"HI!", WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, WS_EX_APPWINDOW|WS_EX_WINDOWEDGE, pt, sz, NULL, NULL);

      MSG msg = {0};
      while ( GetMessage(&msg, NULL, 0, 0) > 0 ) {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
      h = NULL;
   }

#if 0
   {
      MyWindow myWin;
      //WNDPROC  pProc = CreateWndProcThunk(&myWin, &MyWindow::WndProc);
      //DestroyWndProcThunk(pProc);
      //pProc = CreateWndProcThunk(&myWin, &MyWindow::WndProc);
      //WNDPROC pProc = InitializeWndProcThunk(&myWin, &MyWindow::WndProc);

      WNDCLASS wc = {
         0, (WNDPROC)myWin, 0, 0, NULL, NULL, NULL, NULL, NULL, L"MyWindowType"
      };

      RegisterClass(&wc);

      HWND hw = CreateWindow(L"MyWindowType", L"YO!", WS_VISIBLE, 100, 100, 100, 100, NULL, NULL, NULL, 0);
      if ( hw ) {
         DestroyWindow(hw);
      }

      UnregisterClass(L"MyWindowType", NULL);

      MyWindow2 myWin2;
      wc.lpfnWndProc = (WNDPROC)myWin2;
      RegisterClass(&wc);
      hw = CreateWindow(L"MyWindowType", L"YO!", WS_VISIBLE, 100, 100, 100, 100, NULL, NULL, NULL, 0);
   }
#endif
   {
      HANDLE h = NULL;
      DWORD d = CreateCacheList(0, 4096 * 20, 2800, &h);
      if ( NO_ERROR == d ) {
         PVOID p = PopCacheListEntry(h);
         if ( p ) {
            PushCacheListEntry(h, p);
         }
         DestroyCacheList(h);
      }
   }

#if 0
   {
      LPCWSTR rgPath[] = {
         L".",
         L".EXT",
         L".EXT:STREAM:$DATA",
         L".:STREAM:$DATA",
         L".FILE.",
         L".FILE.EXT",
         L".FILE.EXT:STREAM:$DATA",
         L".FILE.EXT::$DATA",
                  
         L".\\",
         L".\\DIRECTORY\\",
         L".\\DIRECTORY:STREAM:$DATA\\",
         L".\\DIRECTORY.EXT\\",
         L".\\DIRECTORY.EXT:STREAM:$DATA\\",

         L"\\",
         L"\\DIRECTORY\\",
         L"\\DIRECTORY:STREAM:$DATA\\",
         L"\\DIRECTORY.EXT\\",
         L"\\DIRECTORY.EXT:STREAM:$DATA\\",
         
         L"C:\\DIRECTORY\\",
         L"C:\\DIRECTORY:STREAM:$DATA\\",
         L"C:\\DIRECTORY.EXT\\",
         L"C:\\DIRECTORY.EXT:STREAM:$DATA\\",
         
         L"C:\\DIRECTORY.EXT\\FILENAME",
         L"C:\\DIRECTORY.EXT\\FILENAME.EXT",
         L"C:\\DIRECTORY.EXT\\FILENAME.EXT:STREAM:$DATA",
         L"C:\\DIRECTORY.EXT\\FILENAME:STREAM:$DATA",
                  
         L"C:F",
         L"C:F.EXT",
         L"C:F.EXT:STREAM:$DATA",
         L"C:F:STREAM:$DATA",

         L".\\C:F",
         L".\\C:F.EXT",
         L".\\C:F.EXT:STREAM:$DATA",
         L".\\C:F:STREAM:$DATA",

         L"C:DIRECTORY.EXT\\FILENAME",
         L"C:DIRECTORY.EXT\\FILENAME.EXT",
         L"C:DIRECTORY.EXT\\FILENAME.EXT:STREAM:$DATA",
         L"C:DIRECTORY.EXT\\FILENAME:STREAM:$DATA",

         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY\\",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY:STREAM:$DATA\\",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY.EXT\\",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY.EXT:STREAM:$DATA\\",
         
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY\\FILENAME",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY\\FILENAME:STREAM:$DATA",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY\\FILENAME.EXT",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY\\FILENAME.EXT:STREAM:$DATA",

         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY.EXT\\FILENAME",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY.EXT\\FILENAME:STREAM:$DATA",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY.EXT\\FILENAME.EXT",
         L"\\\\?\\Volume{00000000-0000-0000-0000-000000000000}\\DIRECTORY.EXT\\FILENAME.EXT:STREAM:$DATA",
      };

      for ( int i = 0; i < ARRAYSIZE(rgPath); i++ ) {
         PATH_INFORMATION PathInformation = {0};
         BOOLEAN bRet = PathCchParseInformation(rgPath[i], PATHCCH_MAXLENGTH, &PathInformation);
         if ( !bRet ) {
            DbgPrintfEx(0, L"FAILED FOR:%s\n", rgPath[i]);
         }
         else {
            WCHAR chBuf[MAX_PATH] = {0};
            bRet = PathCchBuildPath(&PathInformation, chBuf, ARRAYSIZE(chBuf));
            //DbgPrintfEx(0, L"%s\n%s\n", rgPath[i], chBuf);

            DbgPrintfEx(0,
                        _T("Original=%s\n")
                        _T("Rebuilt =%s\n")
                        _T("PATH_INFORMATION::Type                    =%d\n")
                        _T("PATH_INFORMATION::IsSystemParsingDisabled =%d\n")
                        _T("PATH_INFORMATION::BasePathLength          =%d\n")
                        _T("PATH_INFORMATION::BasePath                =%.*s\n")
                        _T("PATH_INFORMATION::VolumeLength            =%d\n")
                        _T("PATH_INFORMATION::Volume                  =%.*s\n")
                        _T("PATH_INFORMATION::ShareLength             =%d\n")
                        _T("PATH_INFORMATION::Share                   =%.*s\n")
                        _T("PATH_INFORMATION::ParentPathLength        =%d\n")
                        _T("PATH_INFORMATION::ParentPath              =%.*s\n")
                        _T("PATH_INFORMATION::FinalNameLength         =%d\n")
                        _T("PATH_INFORMATION::FinalName               =%.*s\n")
                        _T("PATH_INFORMATION::ExtensionLength         =%d\n")
                        _T("PATH_INFORMATION::Extension               =%.*s\n")
                        _T("PATH_INFORMATION::StreamLength            =%d\n")
                        _T("PATH_INFORMATION::Stream                  =%.*s\n")
                        _T("\n"),
                        rgPath[i],
                        chBuf,
                        (UINT)PathInformation.Type,
                        (UINT)PathInformation.IsSystemParsingDisabled,
                        (UINT)PathInformation.BasePathLength,
                        (UINT)PathInformation.BasePathLength,
                        PathInformation.BasePath,
                        (UINT)PathInformation.VolumeLength,
                        (UINT)PathInformation.VolumeLength,
                        PathInformation.Volume,
                        (UINT)PathInformation.ShareLength,
                        (UINT)PathInformation.ShareLength,
                        PathInformation.Share,
                        (UINT)PathInformation.ParentPathLength,
                        (UINT)PathInformation.ParentPathLength,
                        PathInformation.ParentPath,
                        (UINT)PathInformation.FinalNameLength,
                        (UINT)PathInformation.FinalNameLength,
                        PathInformation.FinalName,
                        (UINT)PathInformation.ExtensionLength,
                        (UINT)PathInformation.ExtensionLength,
                        PathInformation.Extension,
                        (UINT)PathInformation.StreamLength,
                        (UINT)PathInformation.StreamLength,
                        PathInformation.Stream);
         }
      }
   }
#endif //0

   //RunRWLockTests();   
   //RunDefragTests();
 
   //ImpersonateSelf(SecurityImpersonation);
   //CTest ct;
   //ct.CheckIt();
   //RevertToSelf();
   
   {
      //WCHAR chVersion[64] = {0};
      //DWORD dwRet = HttpQueryLatestProductProgramVersion(chVersion, ARRAYSIZE(chVersion));
      //dwRet = 0;
   }

   return ( 0 );
}

extern "C" int __stdcall WndProcMember(void* This, void* hWnd, unsigned int uMsg, unsigned __int64 wParam, unsigned __int64 lParam )
{
   WCHAR chbuf[256] = {0};

   StringCchPrintf(chbuf, 256, L"%p %p %u %p %p", This, hWnd, uMsg, wParam, lParam);
   OutputDebugString(chbuf);

   return 0;
}
