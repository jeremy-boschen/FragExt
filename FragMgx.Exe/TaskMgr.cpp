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

/* TaskMgr.cpp
 *    
 * Copyright (C) 2004-2009 Jeremy Boschen
 */
 
#include "Stdafx.h"
#include "TaskMgr.h"
#include <PathUtil.h>

//TODO:Can CTaskManager::_dwState be a LONG?


template < class T, class Base >
class ATL_NO_VTABLE CFxObjectRootEx : public Base
{
protected:
   typedef struct _FX_SID
   {
      DWORD                   Flags;
      WELL_KNOWN_SID_TYPE  SidType;
      struct
      {
         SID_IDENTIFIER_AUTHORITY   Authority;
         DWORD                      SubAuthority;
      };
   }FX_SID, *PFX_SID;

#ifdef _FX_COM_SECURITY_CHECKS
   static void _LookupAccountSid( PSID pSid, LPTSTR pszAccountName, size_t cchAccountName )
   {
      SID_NAME_USE   SidType;
      DWORD          cchName;
      DWORD          cchDomain;
      TCHAR          chName[256+1]    = {0};
      TCHAR          chDomain[256+1]  = {0};

      cchName   = sizeof(chName) / sizeof(chName[0]);
      cchDomain = sizeof(chDomain) / sizeof(chDomain[0]);

      if ( LookupAccountSid(NULL, pSid, chName, &cchName, chDomain, &cchDomain, &SidType) )
      {
         StringCchPrintf(pszAccountName,
                         cchAccountName,
                         _T("%s\\%s"),
                         chDomain,
                         chName);
      }
      else
      {
         StringCchCopy(pszAccountName,
                       cchAccountName,
                       _T("<UNKNOWN>"));
      }
   }

   static void _CheckImpersonationUserName( )
   {
      HANDLE hProcessToken = NULL;
      if ( OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken) )
      {
         HANDLE hThreadToken = NULL;         
            
         if ( !OpenThreadToken(GetCurrentThread(),
                            TOKEN_QUERY,
                            TRUE,
                            &hThreadToken) )
         {
            if ( ERROR_NO_TOKEN == GetLastError() )
            {
               if ( SUCCEEDED(CoImpersonateClient()) )
               {
                  OpenThreadToken(GetCurrentThread(),
                                  TOKEN_QUERY,
                                  TRUE,
                                  &hThreadToken);

                  CoRevertToSelf();
               }
            }
         }

         if ( hThreadToken )
         {
            PTOKEN_USER pThreadUser  = (PTOKEN_USER)GetTokenInformationData(hThreadToken, TokenUser, NULL);
            PTOKEN_USER pProcessUser = (PTOKEN_USER)GetTokenInformationData(hProcessToken, TokenUser, NULL);

            if ( pThreadUser && pProcessUser && !EqualSid(pThreadUser->User.Sid, pProcessUser->User.Sid) )
            {
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

   static void _CheckImpersonationUserType( )
   {
      HANDLE               hThreadToken;

      BOOL                 bIsMember;
      
      PSID                 pSid;
      UCHAR                SidBuf[SECURITY_MAX_SID_SIZE];
      DWORD                cbSid;
      LPTSTR               pszSidName;
      TCHAR                chAccountName[512+1];

      size_t               idx;
      WELL_KNOWN_SID_TYPE  SidTypeList[] =
      {
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
                            &hThreadToken) )
      {
         if ( ERROR_NO_TOKEN == GetLastError() )
         {
            if ( SUCCEEDED(CoImpersonateClient()) )
            {
               OpenThreadToken(GetCurrentThread(),
                               TOKEN_QUERY,
                               TRUE,
                               &hThreadToken);

               CoRevertToSelf();
            }
         }
      }

      if ( hThreadToken )
      {
         for ( idx = 0; idx < (sizeof(SidTypeList) / sizeof(SidTypeList[0])); idx++ )
         {
            cbSid = sizeof(SidBuf);

            if ( CreateWellKnownSid(SidTypeList[idx],
                                    NULL,
                                    pSid,
                                    &cbSid) )
            {
               bIsMember = FALSE;

               if ( CheckTokenMembership(hThreadToken,
                                         pSid,
                                         &bIsMember) && bIsMember )
               {
                  if ( !ConvertSidToStringSid(pSid,
                                              &pszSidName) )
                  {
                     pszSidName = NULL;
                  }

                  _LookupAccountSid(pSid, chAccountName, sizeof(chAccountName) / sizeof(chAccountName[0]));

                  ATLTRACE2(atlTraceQI, 0, _T("***\n\tWARNING - Impersonating credentials contains a dangerous SID[%s], Account[%s]\n\n"), pszSidName ? pszSidName : _T("<NAME NOT MAPPED>"), chAccountName);

                  if ( pszSidName )
                  {
                     LocalFree(pszSidName);
                  }
               }
            }
         }

         CloseHandle(hThreadToken);
      }
   }

   static void _RunImpersonationChecks( )
   {
      _CheckImpersonationUserName();
      _CheckImpersonationUserType();
   }

public:
   STDMETHOD_(ULONG, AddRef)( ) 
   {
      _RunImpersonationChecks();
      return ( Base::AddRef() );
   }

   STDMETHOD_(ULONG, Release)()
   {
      _RunImpersonationChecks();
      return ( Base::Release() );
   }

   STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) throw()
   {
      _RunImpersonationChecks();
      return ( Base::QueryInterface(iid, ppvObject) );
   }
#endif /* _FX_COM_SECURITY_CHECKS */
};

/**********************************************************************

   CTaskManager : ATL

 **********************************************************************/
HRESULT CTaskManager::FinalConstruct( ) throw()
{
   //CComObject<CTest>* pTest;

   /* Success */
   return ( S_OK );
}

void CTaskManager::FinalRelease( ) throw()
{
   ATLASSERT(!_hDefragThread);
   
   _FlushDefragQueue(false);
}

/**********************************************************************

   CTaskManager : IDefragQueue

 **********************************************************************/
HRESULT STDMETHODCALLTYPE CTaskManager::Initialize( ) throw()
{
   DWORD dwRet;

   ATLASSERT(TMS_UNINITIALIZED == _dwState);

   _hSuspend = CreateEvent(NULL,
                           FALSE,
                           FALSE,
                           NULL);

   if ( !_hSuspend )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }
   
   dwRet = _InitializeDefragThread();
   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      goto __CLEANUP;
   }

   _dwState = TMS_INITIALIZED;

__CLEANUP:
   if ( NO_ERROR != dwRet )
   {
      /* Something failed, so we need to tear it all down */
      Uninitialize();
   }

   /* Success / Failure */
   return ( NO_ERROR != dwRet ? __HRESULT_FROM_WIN32(dwRet) : S_OK );
}

HRESULT STDMETHODCALLTYPE CTaskManager::Uninitialize( ) throw()
{
   /* Set the shutdown flag */
   InterlockedExchange(reinterpret_cast<volatile LONG*>(&_dwState),
                       TMS_TERMINATING);

   if ( _hSuspend )
   {
      /* Pause might have been called, so we force a resume */
      Resume();
   }

   /* Tear everything down.. */
   _UninitializeDefragThread();
   _FlushDefragQueue(false);

   if ( _hSuspend )
   {
      CloseHandle(_hSuspend);
      _hSuspend = NULL;
   }

   /* Return the instance to the clean state */
   _dwState = TMS_UNINITIALIZED;

   /* Success */
   return ( S_OK );
}

HRESULT STDMETHODCALLTYPE CTaskManager::SetQueueUpdateRoutine( __in_opt PQUEUEUPDATEPROC pCallback, __in_opt PVOID pParameter ) throw()
{
   AcquireSpinLock(&_QueueCallbackLock);
   {
      _pQueueCallback  = pCallback;
      _pQueueParameter = pParameter;
   }
   ReleaseSpinLock(&_QueueCallbackLock);

   /* Success */
   return ( S_OK );
}

HRESULT STDMETHODCALLTYPE CTaskManager::SetDefragUpdateRoutine( __in_opt PDEFRAGUPDATEPROC pCallback, __in_opt PVOID pParameter ) throw()
{
   AcquireSpinLock(&_DefragCallbackLock);
   {
      _pDefragCallback  = pCallback;
      _pDefragParameter = pParameter;
   }
   ReleaseSpinLock(&_DefragCallbackLock);

   /* Success */
   return ( S_OK );
}

HRESULT STDMETHODCALLTYPE CTaskManager::InsertQueueFile( __in_z LPCWSTR pwszFileName ) throw()
{
   PQUEUEFILE pQueFile;
   
   pQueFile = _CreateQueueFile(pwszFileName,
                               static_cast<USHORT>(PathDirectoryExists(pwszFileName) ? TM_QUEUEFILE_ISDIRECTORY : 0));
   
   if ( !pQueFile )
   {
      /* Failure */
      return ( E_OUTOFMEMORY );
   }

   _InsertDefragQueueFile(pQueFile);

   /* Success */
   return ( S_OK );
}

HRESULT STDMETHODCALLTYPE CTaskManager::DeleteQueueFile( __in_z LPCWSTR pwszFileName ) throw()
{
   UNREFERENCED_PARAMETER(pwszFileName);

   /* Failure */
   return ( E_NOTIMPL );
}

HRESULT STDMETHODCALLTYPE CTaskManager::FlushQueue( ) throw()
{
   _FlushDefragQueue(true);
   return ( S_OK );
}

HRESULT STDMETHODCALLTYPE CTaskManager::Suspend( DWORD dwTimeout ) throw()
{
   InterlockedExchange(reinterpret_cast<volatile LONG*>(&_dwWaitTimeout),
                       static_cast<LONG>(dwTimeout));

   /* If Resume() was called without the defrag thread having gone into a
    * wait state, the event will still be signaled and any subsequent wait
    * will be immediate, so we need to manually ensure that it's non-signaled */
   ResetEvent(_hSuspend);

   /* Success */
   return ( S_OK );
}

HRESULT STDMETHODCALLTYPE CTaskManager::Resume( ) throw()
{
   InterlockedExchange(reinterpret_cast<volatile LONG*>(&_dwWaitTimeout),
                       0);

   /* Signal the event incase we're in a wait state */
   SetEvent(_hSuspend);

   /* Success */
   return ( S_OK );
}

/**********************************************************************

   CTaskManager : IFSxDefragmentFileCallback

 **********************************************************************/
HRESULT STDMETHODCALLTYPE CTaskManager::OnDefragmentFileUpdate( __RPC__in_string LPCWSTR /*pwszFileName*/, LONG iPercentComplete, LONG_PTR dwParameter ) throw()
{
   HRESULT    hr;
   PQUEUEFILE pQueFile;

   /* Initialize locals */
   pQueFile = reinterpret_cast<PQUEUEFILE>(dwParameter);

   /* Pass the notification on to the client */
   hr = _PostDefragUpdate(DC_DEFRAGMENTING,
                          NO_ERROR,
                          pQueFile->FileName,
                          iPercentComplete);

   /* Success / Failure */
   return ( hr );
}

/**********************************************************************

   CTaskManager : CTaskManager

 **********************************************************************/
CTaskManager::CTaskManager( ) throw()
{
   _hSuspend      = NULL;
   _dwState       = TMS_UNINITIALIZED;
   _dwWaitTimeout = 0;

   _pQueueCallback  = NULL;
   _pQueueParameter = NULL;
   InitializeSpinLock(&_QueueCallbackLock);

   _pDefragCallback  = NULL;
   _pDefragParameter = NULL;
   InitializeSpinLock(&_DefragCallbackLock);

   _hDefragThread = NULL;
   _hDefragSignal = NULL;
   InitializeListHead(&_DefragQueue);
   InitializeSpinLock(&_DefragQueueLock);

   _EnumDirState   = 0;
   _hEnumDirThread = NULL;
   _hEnumDirSignal = NULL;
   InitializeListHead(&_DirectoryQueue);
   InitializeSpinLock(&_DirectoryLock);
}

CTaskManager::PQUEUEFILE CTaskManager::_CreateQueueFile( LPCWSTR pwszFileName, USHORT uFlags ) throw()
{
   HRESULT     hr;
   size_t      cbLength;
   size_t      cbAlloc;
   PQUEUEFILE  pQueFile;
   
   hr = StringCbLength(pwszFileName,
                       TM_FILENAME_MAX_CB,
                       &cbLength);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( NULL );
   }

   /* This cannot overflow */
   cbAlloc = (cbLength + sizeof(QUEUEFILE));
   if ( cbAlloc < TM_QUEUEFILE_MIN_CB )
   {
      cbAlloc = TM_QUEUEFILE_MIN_CB;
   }

   pQueFile = reinterpret_cast<PQUEUEFILE>(malloc(cbAlloc));
   if ( !pQueFile )
   {
      /* Failure */
      return ( NULL );
   }

   FillMemory(pQueFile,
              cbAlloc,
              MEMORY_FILL_CHAR);

   pQueFile->Flags = uFlags;

   /* We have 1 extra character in QUEUEFILE::FileName to account for the terminating null character */
   CopyMemory(&(pQueFile->FileName[0]),
              pwszFileName,
              cbLength);
   pQueFile->FileName[cbLength / sizeof(WCHAR)] = L'\0';

   /* Success */
   return ( pQueFile );
}

void CTaskManager::_DestroyQueueFile( PQUEUEFILE pQueFile ) throw()
{
   free(pQueFile);
}

void CTaskManager::_InsertDefragQueueFile( PQUEUEFILE pQueFile ) throw()
{
   AcquireSpinLock(&_DefragQueueLock);
   {
      InsertTailList(&_DefragQueue,
                     &(pQueFile->Entry));
   }
   ReleaseSpinLock(&_DefragQueueLock);
   
   /* Signal the defrag thread that there is work */
   SetEvent(_hDefragSignal);

   /* If we have a queue update callback to notify, send it the inserted message. These are
    * not intended to be serialized with the updates to the queue. */
   _PostQueueUpdate(QC_FILEINSERTED,
                    pQueFile->FileName);
}

void CTaskManager::_FlushDefragQueue( bool bPostUpdates ) throw()
{
   PQUEUEFILE  pQueFile;
   LIST_ENTRY  WorkQueue; 
   
   /* We don't want to post callbacks while holding the spinlock, so move the
    * work queue to a local copy and then process it */
   InitializeListHead(&WorkQueue);

   AcquireSpinLock(&_DefragQueueLock);
   {
      while ( !IsListEmpty(&_DefragQueue) )
      {
         InsertTailList(&WorkQueue,
                        RemoveHeadList(&_DefragQueue));
      }      
   }
   ReleaseSpinLock(&_DefragQueueLock);

   while ( !IsListEmpty(&WorkQueue) )
   {
      pQueFile = CONTAINING_RECORD(RemoveHeadList(&WorkQueue),
                                   QUEUEFILE,
                                   Entry);

      if ( bPostUpdates )
      {
         _PostDefragUpdate(DC_INITIALIZING,
                           ERROR_CANCELLED,
                           pQueFile->FileName,
                           0);

         _PostDefragUpdate(DC_COMPLETED,
                           ERROR_CANCELLED,
                           pQueFile->FileName,
                           0);
         
         _PostQueueUpdate(QC_FILEDELETED,
                          pQueFile->FileName);

      }

      free(pQueFile);
   }
}

DWORD CTaskManager::_InitializeDefragThread( ) throw()
{
   DWORD dwRet;

   ATLASSERT(TMS_UNINITIALIZED == _dwState);
   
   /* Create an event that will be used to signal the defrag thread
    * that it needs to perform some operation (shutdown, defrag, etc) */
   ATLASSERT(!_hDefragSignal);
   _hDefragSignal = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                NULL);

   if ( !_hDefragSignal )
   {
      dwRet = GetLastError();

      _dTrace(eTraceError, L"Failed to create signaling event for defrag thread!%#08lx\n", dwRet);
      /* Failure */
      goto __CLEANUP;
   }

   dwRet = TxCreateThread(&CTaskManager::_DefragThreadRoutine,
                          this,
                          &_hDefragThread,
                          NULL);

   if ( NO_ERROR != dwRet )
   {
      _dTrace(eTraceError, L"Failed to create defrag thread!%#08lx\n", dwRet);
      /* Failure */
      goto __CLEANUP;
   }

__CLEANUP:
   if ( NO_ERROR != dwRet )
   {
      _UninitializeDefragThread();
   }

   /* Success / Failure */
   return ( dwRet );
}

void CTaskManager::_UninitializeDefragThread( ) throw()
{
   /* Set the class state so the defrag thread, if running, will know it
    * needs to shutdown */
   InterlockedExchange(reinterpret_cast<volatile LONG*>(&_dwState),
                       TMS_TERMINATING);

   if ( _hDefragSignal )
   {
      /* Let the thread know that it should begin its shutdown. We postpone
       * closing it until we're certain the thread has exited */
      SetEvent(_hDefragSignal);
   }

   if ( _hDefragThread )
   {
      /* Wait for the thread to exit. It may have already done so, and
       * we don't care about the result */
      WaitForSingleObject(_hDefragThread,
                          INFINITE);

      CloseHandle(_hDefragThread);
      _hDefragThread = NULL;
   }
   
   if ( _hDefragSignal )
   {
      CloseHandle(_hDefragSignal);
      _hDefragSignal = NULL;
   }
}

DWORD CTaskManager::_DefragThreadHandler( ) throw()
{
   DWORD dwRet;
   DWORD dwWait;
   
   /* Initialize locals */
   dwRet = NO_ERROR;

   while ( TMS_TERMINATING != _dwState )
   {
      /* Wait for the work signal.. */
      dwWait = WaitForSingleObject(_hDefragSignal,
                                   TM_WAIT_INTERVAL);

      /* Check the current state of the class, and bail if it is shutting down */
      if ( TMS_TERMINATING == _dwState )
      {
         dwRet = NO_ERROR;
         /* Success */
         break;
      }

      if ( (WAIT_OBJECT_0 == dwWait) || (WAIT_TIMEOUT == dwWait) )
      {
         /* We're going to empty the queue now, so we can reset the event.. */
         ResetEvent(_hDefragSignal);

         /* This thread has been signaled to do some work, other than shutting down,
          * so there must be items on the queue */
         _ProcessDefragQueue();
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

void CTaskManager::_ProcessDefragQueue( ) throw()
{
   BOOL                  bIsEmpty;
   HRESULT               hr;
   IFSxFileDefragmenter* pDefrag;
   PLIST_ENTRY           pEntry;
   PQUEUEFILE            pQueFile;
   BOOLEAN               bMultiple;
   
   /* Initialize locals.. */
   pEntry    = NULL;
   bMultiple = FALSE;
   pDefrag   = NULL;

   /* This function is called on a timeout cycle, and we don't want to connect to
    * the service if unnecessary, so make sure we have a reason to.. */
   AcquireSpinLock(&_DefragQueueLock);
   {
      bIsEmpty = IsListEmpty(&_DefragQueue);
   }
   ReleaseSpinLock(&_DefragQueueLock);

   if ( bIsEmpty )
   {
      /* Nothing to do... */
      return;
   }

   /* Connect to the defrag service.. */
   hr = FSxManagerQueryService(__uuidof(FSxFileDefragmenter),
                               __uuidof(IFSxFileDefragmenter),
                               reinterpret_cast<void**>(&pDefrag));

   if ( FAILED(hr) )
   {
      /* Failure */
      return;
   }

   __try
   {
      hr = pDefrag->SetCallbackInterface(static_cast<IFSxDefragmentFileCallback*>(this));
      if ( FAILED(hr) )
      {
         __leave;
      }
                                               
      while ( TMS_TERMINATING != _dwState )
      {
         AcquireSpinLock(&_DefragQueueLock);
         {
            if ( !IsListEmpty(&_DefragQueue) )
            {
               pEntry    = RemoveHeadList(&_DefragQueue);
               bMultiple = !IsListEmpty(&_DefragQueue);
            }
         }
         ReleaseSpinLock(&_DefragQueueLock);

         if ( !pEntry )
         {
            break;
         }

         pQueFile = CONTAINING_RECORD(pEntry,
                                      QUEUEFILE,
                                      Entry);
      
         __try
         {
            if ( TMS_TERMINATING == _dwState )
            {
               break;
            }

            /* If either FSxManagerQueryService() or CoSetProxyBlanket() failed, hr will have that
             * error and pDefrag will be NULL. We want to save that error and post it with the 
             * completion event for the file. Otherwise, we're going to try and defrag the file */
            if ( pDefrag )
            {
               /* Inform the client that we're starting a new file.. */
               hr = _PostDefragUpdate(DC_INITIALIZING,
                                      NO_ERROR,
                                      pQueFile->FileName,
                                      0);

               if ( TMS_TERMINATING == _dwState )
               {
                  break;
               }

               if ( SUCCEEDED(hr) )
               {
                  hr = pDefrag->DefragmentFile(pQueFile->FileName,
                                               reinterpret_cast<LONG_PTR>(pQueFile));
                  ATLASSERT(SUCCEEDED(hr) || (HRESULT_CODE(hr) == ERROR_CANCELLED) || (HRESULT_CODE(hr) == ERROR_DISK_FULL) || (HRESULT_CODE(hr) == ERROR_SHARING_VIOLATION));
               }
            }
                     
            if ( TMS_TERMINATING == _dwState )
            {
               break;
            }
            
            /* Post a final update to the client.. */
            _PostDefragUpdate(DC_COMPLETED,
                              static_cast<DWORD>(hr),
                              pQueFile->FileName,
                              100);
         }
         __finally
         {   
            /* If an exception wasn't thrown and this is a directory file that was added via ITaskManager::InsertQueueFile(), then
             * kick off the directory enum thread and put this entry up on its list. Otherwise, destroy it */
            if ( !AbnormalTermination() && FlagOn(pQueFile->Flags, TM_QUEUEFILE_ISDIRECTORY) && !FlagOn(pQueFile->Flags, TM_QUEUEFILE_ISAUTOFILE) && (NO_ERROR == _InitializeEnumDirThread()) )
            {
               AcquireSpinLock(&_DirectoryLock);
               {
                  InsertTailList(&_DirectoryQueue,
                                 &(pQueFile->Entry));
               }
               ReleaseSpinLock(&_DirectoryLock);

               SetEvent(_hEnumDirSignal);
            }
            else
            {
               _DestroyQueueFile(pQueFile);
            }
            
            pEntry = NULL;
         }

         /* If we know there are more entries in the list, pause
          * for a bit before processing the next */
         if ( bMultiple )
         {
            _DoWaitResume(TM_DELAY_INTERVAL);
         }
      }
   }
   __finally
   {
      if ( pDefrag )
      {
         pDefrag->Release();
      }
   }
}

DWORD CTaskManager::_InitializeEnumDirThread( ) throw()
{
   DWORD dwRet;
   LONG  State;
   
   do
   {
      dwRet = NO_ERROR;
      State = InterlockedCompareExchange(&_EnumDirState,
                                         1,
                                         0);

      if ( 0 == State )
      {
         _hEnumDirSignal = CreateEvent(NULL,
                                       TRUE,
                                       FALSE,
                                       NULL);

         if ( !_hEnumDirSignal )
         {
            InterlockedExchange(&_EnumDirState,
                                0);

            dwRet = GetLastError();
            /* Failure */
            break;
         }

         dwRet = TxCreateThread(&CTaskManager::_EnumDirThreadRoutine,
                                this,
                                &_hEnumDirThread,
                                NULL);

         InterlockedExchange(&_EnumDirState,
                             (NO_ERROR != dwRet ? 0 : 2));

         break;
      }
      else if ( 1 == State )
      {
         Sleep(36);
      }
   }
   while ( (2 != State) && (3 != State) );

   /* Success / Failure */
   return ( dwRet );
}

void CTaskManager::_UninitializeEnumDirThread( ) throw()
{
   /* Set the class state so the defrag thread, if running, will know it
    * needs to shutdown */
   InterlockedExchange(&_EnumDirState,
                       3);

   if ( _hEnumDirSignal )
   {
      /* Let the thread know that it should begin its shutdown. We postpone
       * closing it until we're certain the thread has exited */
      SetEvent(_hEnumDirSignal);
   }

   if ( _hEnumDirThread )
   {
      /* Wait for the thread to exit. It may have already done so, and
       * we don't care about the result */
      WaitForSingleObject(_hEnumDirThread,
                          INFINITE);

      CloseHandle(_hEnumDirThread);
      _hEnumDirThread = NULL;
   }
   
   if ( _hEnumDirSignal )
   {
      CloseHandle(_hEnumDirSignal);
      _hEnumDirSignal = NULL;
   }

   InterlockedExchange(&_EnumDirState,
                       0);
}

DWORD CTaskManager::_EnumDirThreadHandler( ) throw()
{
   DWORD dwRet;
   DWORD dwWait;
   
   /* Initialize locals */
   dwRet = NO_ERROR;

   while ( 3 != _EnumDirState )
   {
      /* Wait for the work signal.. */
      dwWait = WaitForSingleObject(_hEnumDirSignal,
                                   TM_WAIT_INTERVAL);

      /* Check the current state of the class, and bail if it is shutting down */
      if ( 3 == _EnumDirState )
      {
         dwRet = NO_ERROR;
         /* Success */
         break;
      }

      if ( (WAIT_OBJECT_0 == dwWait) || (WAIT_TIMEOUT == dwWait) )
      {
         /* We're going to empty the queue now, so we can reset the event.. */
         ResetEvent(_hEnumDirSignal);

         /* This thread has been signaled to do some work, other than shutting down,
          * so there must be items on the queue */
         _ProcessDirectoryQueue();
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

void CTaskManager::_ProcessDirectoryQueue( ) throw()
{
   BOOL        bIsEmpty;
   PLIST_ENTRY pEntry;
   PQUEUEFILE  pQueFile;
   BOOLEAN     bMultiple;
   
   /* Initialize locals.. */
   pEntry    = NULL;
   bMultiple = FALSE;

   /* This function is called on a timeout cycle, and we don't want to connect to
    * the service if unnecessary, so make sure we have a reason to.. */
   AcquireSpinLock(&_DirectoryLock);
   {
      bIsEmpty = IsListEmpty(&_DirectoryQueue);
   }
   ReleaseSpinLock(&_DirectoryLock);

   if ( bIsEmpty )
   {
      /* Nothing to do... */
      return;
   }

   __try
   {
      while ( 3 != _EnumDirState )
      {
         AcquireSpinLock(&_DirectoryLock);
         {
            if ( !IsListEmpty(&_DirectoryQueue) )
            {
               pEntry    = RemoveHeadList(&_DirectoryQueue);
               bMultiple = !IsListEmpty(&_DirectoryQueue);
            }
         }
         ReleaseSpinLock(&_DirectoryLock);

         if ( !pEntry )
         {
            break;
         }

         pQueFile = CONTAINING_RECORD(pEntry,
                                      QUEUEFILE,
                                      Entry);
      
         __try
         {
            if ( 3 == _EnumDirState )
            {
               break;
            }

            FpEnumDirectoryInformation(pQueFile->FileName,
                                       EncodeMaxTraverseDepth(255),
                                       &CTaskManager::_EnumDirectoryCallback,
                                       this);

         }
         __finally
         {
            _DestroyQueueFile(pQueFile);
            pEntry = NULL;
         }

         /* If we know there are more entries in the list, pause
          * for a bit before processing the next */
         if ( bMultiple )
         {
            _DoWaitResume(TM_DELAY_INTERVAL);
         }
      }
   }
   __finally
   {
   }
}

void CTaskManager::_DoWaitResume( DWORD dwForceDelay ) throw()
{
   HRESULT hr;
   DWORD   dwTimeout;
   DWORD   dwWaitIdx;
   
   /* We only want to do this wait once */
   dwTimeout = static_cast<DWORD>(InterlockedExchange(reinterpret_cast<volatile LONG*>(&_dwWaitTimeout),
                                                      0));

   dwTimeout = max(dwTimeout,
                   dwForceDelay);

   if ( dwTimeout > 0 )
   {
      hr = CoWaitForMultipleHandles(COWAIT_WAITALL|COWAIT_ALERTABLE,
                                    dwTimeout,
                                    1,
                                    &_hSuspend,
                                    &dwWaitIdx);
      ATLASSERT(SUCCEEDED(hr) || (RPC_S_CALLPENDING == hr));
   }
}

void CTaskManager::_PostQueueUpdate( QUEUEUPDATECODE eUpdateCode, __in_z LPCWSTR pwszFileName ) throw()
{
   PQUEUEUPDATEPROC pCallback;
   PVOID            pParameter;

   if ( TMS_TERMINATING == _dwState )
   {
      /* Aborting.. */
      return;
   }

   AcquireSpinLock(&_QueueCallbackLock);
   {
      pCallback  = _pQueueCallback;
      pParameter = _pQueueParameter;
   }
   ReleaseSpinLock(&_QueueCallbackLock);

   if ( pCallback )
   {
      (*pCallback)(eUpdateCode,
                   pwszFileName,
                   pParameter);
   }
}

HRESULT CTaskManager::_PostDefragUpdate( DEFRAGUPDATECODE eUpdateCode, DWORD dwStatus, __in_z LPCWSTR pwszFileName, LONG iPercentComplete ) throw()
{
   HRESULT           hr;
   PDEFRAGUPDATEPROC pCallback;
   PVOID             pParameter;

   if ( TMS_TERMINATING == _dwState )
   {
      /* Aborting.. */
      return ( __HRESULT_FROM_WIN32(ERROR_CANCELLED) );
   }

   AcquireSpinLock(&_DefragCallbackLock);
   {
      pCallback  = _pDefragCallback;
      pParameter = _pDefragParameter;
   }
   ReleaseSpinLock(&_DefragCallbackLock);

   if ( !pCallback )
   {
      hr = __HRESULT_FROM_WIN32(ERROR_CANCELLED);
   }
   else
   {
      do
      {
         /* Check the wait state, and suspend if necessary */
         _DoWaitResume(0);

         hr = (*pCallback)(eUpdateCode,
                           dwStatus,
                           pwszFileName,
                           iPercentComplete,
                           pParameter);
      }
      while ( (TMS_TERMINATING != _dwState) && (__HRESULT_FROM_WIN32(ERROR_RETRY) == hr) );
   }

   /* Success / Failure */
   return ( hr );
}

DWORD WINAPI CTaskManager::_DefragThreadRoutine( PVOID pParameter ) throw()
{
   DWORD          dwRet;
   CTaskManager*  pTaskMgr;
   
   /* Initialize locals */
   dwRet    = NO_ERROR;
   pTaskMgr = reinterpret_cast<CTaskManager*>(pParameter);

   /* Add a reference on the DefragQueue while this thread is running */
   pTaskMgr->AddRef();

   __try
   {
      /* We're cool to forward to the handler, so signal _InitializeDefragThread() 
       * that we're done */
      TxSetThreadInitComplete(NO_ERROR);

      /* Forward to the handler for all the real work */
      dwRet = pTaskMgr->_DefragThreadHandler();
   }
   __finally
   {
      pTaskMgr->Release();
   }

   return ( static_cast<unsigned>(dwRet) );
}

DWORD WINAPI CTaskManager::_EnumDirThreadRoutine( PVOID pParameter ) throw()
{
   DWORD          dwRet;
   CTaskManager*  pTaskMgr;
   
   /* Initialize locals */
   dwRet    = NO_ERROR;
   pTaskMgr = reinterpret_cast<CTaskManager*>(pParameter);

   /* Add a reference on the DefragQueue while this thread is running */
   pTaskMgr->AddRef();

   __try
   {
      /* We're cool to forward to the handler, so signal _InitializeDefragThread() 
       * that we're done */
      TxSetThreadInitComplete(NO_ERROR);

      /* Forward to the handler for all the real work */
      dwRet = pTaskMgr->_EnumDirThreadHandler();
   }
   __finally
   {
      pTaskMgr->Release();
   }

   return ( static_cast<unsigned>(dwRet) );
}

BOOL FRAGAPI CTaskManager::_EnumDirectoryCallback( __in PCENUMDIRECTORY_INFORMATION FileInfo, __in DWORD /*Depth*/, __in_opt PVOID Parameter ) throw()
{
   PQUEUEFILE    pQueFile;
   CTaskManager* pTaskMgr;

   pTaskMgr = reinterpret_cast<CTaskManager*>(Parameter);

   pQueFile = pTaskMgr->_CreateQueueFile(FileInfo->FilePath,
                                         static_cast<USHORT>((FlagOn(FileInfo->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) ? TM_QUEUEFILE_ISDIRECTORY : 0)|TM_QUEUEFILE_ISAUTOFILE));

   if ( !pQueFile )
   {
      /* Failure */
      return ( FALSE );
   }

   pTaskMgr->_InsertDefragQueueFile(pQueFile);

   /* Success */
   return ( TRUE );
}
