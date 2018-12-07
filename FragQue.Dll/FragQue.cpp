/* FragExt - Shell extension for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2008 Jeremy Boschen. All rights reserved.
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

/* FragQue.cpp
 *    FragQue.dll implementation
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#include "Stdafx.h"
#include "FragQuep.h"

#include <List.h>
#include <psapi.h>
#include <DbgUtil.h>
#include <PathUtil.h>
using namespace PathUtil;

/**
 * Global work queue data 
 **/
HANDLE            WorkNotificationEvent = NULL;
CRITICAL_SECTION  WorkQueueLock         = {0};
LIST_ENTRY        WorkQueue             = {NULL, NULL};
volatile LONG     WorkThreadInitialized = 0;

/**********************************************************************

   Public - Exported functions

 **********************************************************************/

/**
 * FqInitialize
 */
DWORD QUEUEAPI FqInitialize( PHANDLE phQueueCtx ) throw()
{
   DWORD          dwRet;
   HANDLE         hToken;
   PQUEUE_CONTEXT pQueueCtx;

   /* Initialize outputs.. */
   (*phQueueCtx) = NULL;

   /* Initialize locals */
   hToken    = NULL;
   pQueueCtx = NULL;

   /* Build the context token based on the thread that is executing this function */
   dwRet = FqiCreateQueuempersonationToken(GetCurrentThread(),
                                           &hToken);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   __try
   {
      /* Allocate an initialize the new context */
      pQueueCtx = reinterpret_cast<PQUEUE_CONTEXT>(FqiAlloc(sizeof(QUEUE_CONTEXT)));
      if ( !pQueueCtx )
      {
         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      pQueueCtx->ReferenceCount     = 1;
      pQueueCtx->ImpersonationToken = hToken;
      
      /* Initialize the file queue for the user identified by the specified token.. */
      dwRet = FqiInitializeFileQueue(pQueueCtx);

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      /* Assign the context to the caller */
      (*phQueueCtx) = QueueContextToHandle(pQueueCtx);
      
      /* Clear the locals so cleanup code knows they're know owned by the caller. When they're
       * not cleared, this function will do a manual cleanup of the QUEUE_CONTEXT. After this,
       * cleanup of the QUEUE_CONTEXT happens only via FqiReleaseQueueContext() */
      hToken    = NULL;
      pQueueCtx = NULL;
   }
   __finally
   {
      if ( hToken )
      {
         CloseHandle(hToken);
      }

      if ( pQueueCtx )
      {
         FqiFree(pQueueCtx);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

/**
 * FqUninitialize
 */
VOID QUEUEAPI FqUninitialize( HANDLE hQueueCtx ) throw()
{
   PQUEUE_CONTEXT pQueueCtx;

   if ( !IsValidQueueContextHandle(hQueueCtx) )
   {
      _dTrace(L"IsValidQueueContextHandle() failed\n");
      /* Failure */
      return;
   }

   /* Initialize locals.. */
   pQueueCtx = HandleToQueueContext(hQueueCtx);

   /* Release the implicit reference added in FqInitialize(). This may or may not destroy
    * the context. Work items may own it, notifications may own it, etc. */
   FqiReleaseQueueContext(pQueueCtx);
}

/**
 * FqInsertFile
 */
DWORD QUEUEAPI FqInsertFile( __in HANDLE hQueueCtx, __in_z LPCWSTR pwszFileName ) throw()
{
   DWORD           dwRet;
   PQUEUE_CONTEXT  pQueueCtx;
   PQUEUE_WORKITEM pWorkItem;

   /* Validate parameters.. */
   if ( !IsValidQueueContextHandle(hQueueCtx) )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }
   
   dwRet     = NO_ERROR;
   pQueueCtx = HandleToQueueContext(hQueueCtx);
   pWorkItem = NULL;

   FqiReferenceQueueContext(pQueueCtx);

   __try   
   {
      pWorkItem = FqiBuildWorkItem(pQueueCtx,
                                   WORKTYPE_WRITERECORD,
                                   pwszFileName);

      if ( !pWorkItem )
      {
         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      dwRet = FqiQueueWorkItem(pWorkItem);
      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      /* The work queue owns the work item now */
      pWorkItem = NULL;
   }
   __finally
   {
      if ( pWorkItem )
      {
         FqiDestroyWorkItem(pWorkItem);
      }

      FqiReleaseQueueContext(pQueueCtx);
   }

   /* Success / Failure */
   return ( dwRet );
}

/**
 * FqRemoveFile
 */
DWORD QUEUEAPI FqRemoveFile( __in HANDLE hQueueCtx, __in_z LPCWSTR pwszFileName ) throw()
{
   DWORD             dwRet;
   PQUEUE_CONTEXT    pQueueCtx;
   PQUEUE_WORKITEM   pWorkItem;

   /* Validate parameters.. */
   if ( !IsValidQueueContextHandle(hQueueCtx) )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }
   
   dwRet     = NO_ERROR;
   pQueueCtx = HandleToQueueContext(hQueueCtx);
   pWorkItem = NULL;

   FqiReferenceQueueContext(pQueueCtx);

   __try   
   {
      pWorkItem = FqiBuildWorkItem(pQueueCtx,
                                   WORKTYPE_DELETERECORD,
                                   pwszFileName);

      if ( !pWorkItem )
      {
         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      dwRet = FqiQueueWorkItem(pWorkItem);
      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      /* The work queue owns the work item now */
      pWorkItem = NULL;
   }
   __finally
   {
      if ( pWorkItem )
      {
         FqiDestroyWorkItem(pWorkItem);
      }

      FqiReleaseQueueContext(pQueueCtx);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD QUEUEAPI FqEnumerateQueue( __in HANDLE hQueueCtx, __in PENUMQUEUEPROC pCallback, __in_opt PVOID pParameter, __in_opt PLONGLONG pLastModified ) throw()
{
   DWORD          dwRet;
   PQUEUE_CONTEXT pQueueCtx;
   BOOL           bImpersonating;

   /* Validate parameters... */
   if ( !IsValidQueueContextHandle(hQueueCtx) || !pCallback )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   dwRet     = NO_ERROR;
   pQueueCtx = HandleToQueueContext(hQueueCtx);
   FqiReferenceQueueContext(pQueueCtx);

   bImpersonating = FALSE;   

   __try
   {
      if ( !SetThreadToken(NULL,
                           pQueueCtx->ImpersonationToken) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      bImpersonating = TRUE;

      dwRet = FqiEnumerateFileQueue(pQueueCtx,
                                    pCallback,
                                    pParameter,
                                    pLastModified);
   }
   __finally
   {
      if ( bImpersonating )
      {
         RevertToSelf();
      }

      FqiReleaseQueueContext(pQueueCtx);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD QUEUEAPI FqRegisterQueueNotification( __in HANDLE hQueueCtx, __in PQUEUENOTIFICATIONPROC pCallback, __in_opt PVOID pParameter, __in_opt PLONGLONG pLastModified, __out PHANDLE phNotifyCtx ) throw()
{
   DWORD             dwRet;
   PQUEUE_CONTEXT    pQueueCtx;
   PNOTIFY_CONTEXT   pNotifyCtx;
   LONGLONG          LastModified;

   /* Validate parameters.. */
   if ( !IsValidQueueContextHandle(hQueueCtx) || !pCallback || !phNotifyCtx )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals.. */
   dwRet      = NO_ERROR;
   pQueueCtx  = HandleToQueueContext(hQueueCtx);
   pNotifyCtx = NULL;
   
   /* If the caller didn't pass a starting delta, use the current system time */
   if ( !pLastModified )
   {
      FqGetLastModifiedTime(&LastModified);
      pLastModified = &LastModified;
   }

   __try
   {
      FqiReferenceQueueContext(pQueueCtx);

      dwRet = FqiStartQueueNotificationThread(pQueueCtx,
                                              pCallback,
                                              pParameter,
                                              pLastModified,
                                              &pNotifyCtx);

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }
         
      (*phNotifyCtx) = NotificationContextToHandle(pNotifyCtx);
   }
   __finally
   {
      FqiReleaseQueueContext(pQueueCtx);
   }   

   /* Success / Failure */
   return ( dwRet );
}

VOID QUEUEAPI FqUnregisterQueueNotification( HANDLE hNotify ) throw()
{
   PNOTIFY_CONTEXT pNotifyCtx;

   if ( !IsValidQueueNotificationContext(hNotify) )
   {
      /* Failure */
      return;
   }

   /* Initialize locals.. */
   pNotifyCtx = HandleToNotificationContext(hNotify);

   FqiShutdownQueueNotificationThread(pNotifyCtx);
}

VOID QUEUEAPI FqGetLastModifiedTime( __out PLONGLONG pLastModified ) throw()
{
   HANDLE         hThread;
   DWORD_PTR      dwAffinity;
   LARGE_INTEGER  iCounter;

   hThread    = GetCurrentThread();
   dwAffinity = SetThreadAffinityMask(hThread,
                                      1);

   iCounter.QuadPart = 0;
   QueryPerformanceCounter(&iCounter);

   (*pLastModified) = (iCounter.QuadPart);

   if ( 0 != dwAffinity )
   {
      SetThreadAffinityMask(hThread,
                            dwAffinity);
   }                                      
}

/**********************************************************************

   Private

 **********************************************************************/

/**
 * DllMain
 *    Entry point of the DLL. We require DLL_PROCESS_ATTACH & DLL_PROCESS_DETACH events
 *    for initialization and cleanup of work queue data.
 */
BOOL WINAPI DllMain( HINSTANCE /*hInstance*/, DWORD fdwReason, LPVOID /*lpReserved*/ ) throw()
{
   BOOL bRet;

   /* The only thing we can fail is DLL_PROCESS_ATTACH */
   bRet = TRUE;

   switch ( fdwReason )
   {
      case DLL_PROCESS_ATTACH:
         InitializeListHead(&WorkQueue);

         bRet = InitializeCriticalSectionAndSpinCount(&WorkQueueLock,
                                                      0x80000400);

         if ( !bRet )
         {
            /* Failure */
            break;
         }

         WorkNotificationEvent = CreateEvent(NULL,
                                             FALSE,
                                             FALSE,
                                             NULL);

         if ( !WorkNotificationEvent )
         {
            /* Failure */
            bRet = FALSE;
         }

         break;

      case DLL_THREAD_ATTACH:
         break;

      case DLL_THREAD_DETACH:
         break;

      case DLL_PROCESS_DETACH:
         if ( WorkNotificationEvent )
         {
            CloseHandle(WorkNotificationEvent);
         }

         DeleteCriticalSection(&WorkQueueLock);
         /* Success */
         break;
   }

   return ( bRet );
}

/**********************************************************************

   WorkQueue

   This library uses exclusive file locks, so there's a chance that
   clients can end up creating a deadlock by executing some sequence
   like the following..
      
   FqEnumerateQueue(Callback) : Acquires shared-read lock
      -> Callback(File)
         -> FqRemoveFile(File) : Tries to acquire exclusive write lock,
         but cannot because the FqEnumerateQueue() is holding a shared-
         read lock. This will cause a deadlock.

   To overcome this, all operations that require an exclusive file lock
   are posted to a single worker thread. The worker thread is created
   during the first request for either of these operations, and exists
   until either there are no requests for a period of time, or until the
   program exits.

 **********************************************************************/

/**
 * FqiInitializeWorkerThread
 */
DWORD FqiInitializeWorkerThread( ) throw()
{
   DWORD       dwRet;
   LONG        iOwner;
   HMODULE     hModule;
   HANDLE      hThread;
   THREAD_CTX  ThreadCtx;

   /* Initialize locals */
   dwRet   = NO_ERROR;
   hThread = NULL;
   hModule = NULL;
   
   do
   {
      iOwner = InterlockedCompareExchange(&WorkThreadInitialized,
                                          1,
                                          0);

      if ( 0 == iOwner )
      {
         /* We're the thread that's creating the work thread.. */

         /* Lock this DLL in memory.. */
         hModule = LoadLibrary(L"FRAGQUE.DLL");
         if ( !hModule )
         {
            dwRet = GetLastError();

            InterlockedExchange(&WorkThreadInitialized,
                                0);

            /* Failure */
            break;
         }

         ThreadCtx.Parameter     = NULL;
         ThreadCtx.ModuleLock    = hModule;
         ThreadCtx.InitDoneEvent = 0;
         
         hThread = CreateThread(NULL,
                                0,
                                &FqiWorkerThreadRoutine,
                                &ThreadCtx,
                                0,
                                NULL);

         if ( !hThread )
         {
            /* Failure */
            dwRet = GetLastError();
            
            /* Reset the initialized state so another thread can try to do it */
            InterlockedExchange(&WorkThreadInitialized,
                                0);

            /* Failure */
            break;
         }

         /* The thread owns hModule now.. */
         hModule = NULL;

         /* Wait for the thread to finish initializing.. */
         while ( 0 == ThreadCtx.InitDoneEvent )
         {
            Sleep(30);
         }

         dwRet = ThreadCtx.InitStatusCode;
            
         /* Success / Failure */
         InterlockedExchange(&WorkThreadInitialized,
                             NO_ERROR != dwRet ? 0 : 2);

         /* Success / Failure */
         break;
      }
      else if ( 1 == iOwner )
      {
         /* Another thread is trying to initialize so just sit back and wait for it to finish */
         Sleep(500);
      }
   }
   while ( iOwner != 2 );

   /* Cleanup whatever is left lying around */
   if ( hThread )
   {
      CloseHandle(hThread);
   }

   if ( hModule )
   {
      FreeLibrary(hModule);
   }
            
   /* Success / Failure */
   return ( dwRet );
}

/**
 * FqiWorkerThreadRoutine
 */
DWORD WINAPI FqiWorkerThreadRoutine( LPVOID pParam ) throw()
{
   DWORD             dwRet;
   PTHREAD_CTX       pThreadCtx;
   HMODULE           hModule;
   DWORD             dwWait;
   BOOL              bExit;
   PQUEUE_WORKITEM   pWorkItem;

   dwRet      = NO_ERROR;
   pThreadCtx = reinterpret_cast<PTHREAD_CTX>(pParam);
   hModule    = pThreadCtx->ModuleLock;

#ifdef _DEBUG
   SetThreadName(GetCurrentThreadId(),
                 __FUNCTION__);
#endif /* _DEBUG */

   __try
   {
      /* Notify the caller that we've finished initializing */
      pThreadCtx->InitStatusCode = NO_ERROR;
      InterlockedExchange(&(pThreadCtx->InitDoneEvent),
                          1);
      pThreadCtx = NULL;

      #pragma warning ( suppress : 4127 )
      while ( true )
      {
         dwWait = WaitForSingleObjectEx(WorkNotificationEvent,
                                        FQI_WORKQUEUE_TIMEOUT,
                                        TRUE);

         if ( WAIT_OBJECT_0 == dwWait )
         {
            EnterCriticalSection(&WorkQueueLock);
            {
               while ( !IsListEmpty(&WorkQueue) )
               {
                  pWorkItem = CONTAINING_RECORD(RemoveHeadList(&WorkQueue),
                                                QUEUE_WORKITEM,
                                                Entry);

                  /* We don't need to hold the lock while we're executing the work item */
                  LeaveCriticalSection(&WorkQueueLock);
                  FqiExecuteWorkItem(pWorkItem);
                  EnterCriticalSection(&WorkQueueLock);
               }
            }
            LeaveCriticalSection(&WorkQueueLock);
         }
         else if ( WAIT_TIMEOUT == dwWait )
         {
            bExit = FALSE;
            /* If there are no work items on the queue, then we'll try to shut this thread down */
            EnterCriticalSection(&WorkQueueLock);
            {
               if ( IsListEmpty(&WorkQueue) )
               {
                  InterlockedExchange(&WorkThreadInitialized,
                                      0);

                  /* There are no items on the queue */
                  bExit = TRUE;
               }
            }
            LeaveCriticalSection(&WorkQueueLock);

            if ( bExit )
            {
               __leave;
            }
         }
         else if ( WAIT_IO_COMPLETION == dwWait )
         {
            /* Nothing to do.. */
         }
         else
         {
            /* Allow some other thread to try and spin up a new worker thread. We may be abandoning
             * items on the work queue, but oh well.. */
            InterlockedExchange(&WorkThreadInitialized,
                                0);

            /* Some fatal error */
            __leave;
         }
      }
   }
   __finally
   {      
      FreeLibraryAndExitThread(hModule,
                               dwRet);
   }
}

/**
 * FqiExecuteWorkItem
 */
VOID FqiExecuteWorkItem( PQUEUE_WORKITEM pWorkItem ) throw()
{
   DWORD dwRet;
   BOOL  bImpersonating;
   
   /* Initialize locals */
   bImpersonating = FALSE;

   __try
   {
      if ( !SetThreadToken(NULL,
                           pWorkItem->QueueCtx->ImpersonationToken) )
      {
         /* Failure */
         __leave;
      }

      bImpersonating = TRUE;
      
      if ( WORKTYPE_WRITERECORD == pWorkItem->WorkType )
      {
         dwRet = FqiWriteFileQueueFile(pWorkItem->QueueCtx,
                                       pWorkItem->WriteRecord.FileName,
                                       NULL);

         if ( ERROR_FILE_CORRUPT == dwRet )
         {
            /* The queue file is corrupted, so blow it away */
            FqiDeleteQueueFile(pWorkItem->QueueCtx,
                               pWorkItem->WriteRecord.FileName);
         }
      }
      else if ( WORKTYPE_DELETERECORD == pWorkItem->WorkType )
      {
         dwRet = FqiDeleteFileQueueFile(pWorkItem->QueueCtx,
                                        pWorkItem->DeleteRecord.FileName);

         if ( ERROR_FILE_CORRUPT == dwRet )
         {
            /* The queue file is corrupted, so blow it away */
            FqiDeleteQueueFile(pWorkItem->QueueCtx,
                               pWorkItem->WriteRecord.FileName);
         }
      }
      else if ( WORKTYPE_DELETEQUEUEFILE == pWorkItem->WorkType )
      {
         FqiDeleteQueueFile(pWorkItem->QueueCtx,
                            pWorkItem->DeleteQueueFile.FileName);
      }
   }
   __finally
   {
      if ( bImpersonating )
      {
         RevertToSelf();
      }
      
      FqiDestroyWorkItem(pWorkItem);
   }
}

/**
 * FqiQueueWorkItem
 */
DWORD FqiQueueWorkItem( PQUEUE_WORKITEM pWorkItem ) throw()
{
   DWORD dwRet;

   EnterCriticalSection(&WorkQueueLock);
   {
      dwRet = FqiInitializeWorkerThread();
      if ( NO_ERROR == dwRet )
      {      
         InsertTailList(&WorkQueue,
                        &(pWorkItem->Entry));
      }
   }
   LeaveCriticalSection(&WorkQueueLock);

   if ( NO_ERROR == dwRet )
   {
      /* Notify the thread that there are items on the queue.. */
      SetEvent(WorkNotificationEvent);
   }

   /* Success / Failure */
   return ( dwRet );
}

/**
 * FqiBuildWorkItem
 */
PQUEUE_WORKITEM FqiBuildWorkItem( PQUEUE_CONTEXT pQueueCtx, ULONG eWorkType, LPCWSTR pwszFileName ) throw()
{
   HRESULT           hr;
   size_t            cbName;
   size_t            cbAlloc;
   PQUEUE_WORKITEM   pWorkItem;

   _ASSERTE((WORKTYPE_WRITERECORD == eWorkType) || (WORKTYPE_DELETERECORD == eWorkType));

   /* We limit the filename length to the max */
   hr = StringCbLength(pwszFileName,
                       FQI_FILENAME_CB_MAX,
                       &cbName);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( NULL );
   }
   
   /* This cannot overflow because of the restriction we place on the filename
    * length via FQI_FILENAME_CB_MAX */
   cbAlloc = FqiRoundUp(cbName + sizeof(QUEUE_WORKITEM), 
                        MEMORY_ALLOCATION_ALIGNMENT);

   pWorkItem = reinterpret_cast<PQUEUE_WORKITEM>(FqiAlloc(cbAlloc));
   if ( !pWorkItem )
   {
      /* Failure */
      return ( NULL );
   }

   /* Lock the queue context for the work item */
   FqiReferenceQueueContext(pQueueCtx);

   pWorkItem->Entry.Flink = NULL;
   pWorkItem->Entry.Blink = NULL;
   pWorkItem->QueueCtx    = pQueueCtx;
   pWorkItem->WorkType    = eWorkType;

   if ( WORKTYPE_WRITERECORD == eWorkType )
   {
      CopyMemory(&(pWorkItem->WriteRecord.FileName[0]),
                 pwszFileName,
                 cbName);
      pWorkItem->WriteRecord.FileName[cbName / sizeof(WCHAR)] = L'\0';
   }
   else if ( WORKTYPE_DELETERECORD == eWorkType )
   {
      CopyMemory(&(pWorkItem->DeleteRecord.FileName[0]),
                 pwszFileName,
                 cbName);
      pWorkItem->DeleteRecord.FileName[cbName / sizeof(WCHAR)] = L'\0';
   }

   /* Success */
   return ( pWorkItem );
}

/**
 * FqiDestroyWorkItem
 */
VOID FqiDestroyWorkItem( PQUEUE_WORKITEM pWorkItem ) throw()
{
   FqiReleaseQueueContext(pWorkItem->QueueCtx);
   FqiFree(pWorkItem);
}

/**********************************************************************

   FileQueue


 **********************************************************************/

DWORD FqiInitializeFileQueue( PQUEUE_CONTEXT pQueueCtx ) throw()
{
   DWORD dwRet;
   
   /* Get the file queue path for the user identified by the specified token */
   dwRet = FqiGetFileQueuePath(pQueueCtx,
                               FQI_CREATE,
                               pQueueCtx->FileQueuePath,
                               _countof(pQueueCtx->FileQueuePath));

   /* There's really nothing else to do here.. */

   /* Success / Failure */
   return ( dwRet );
}

DWORD FqiStartQueueNotificationThread( PQUEUE_CONTEXT pQueueCtx, PQUEUENOTIFICATIONPROC pCallback, PVOID pParameter, PLONGLONG pLastModified, PNOTIFY_CONTEXT* ppNotifyCtx  ) throw()
{
   DWORD             dwRet;   
   HANDLE            hEvent;
   HANDLE            hThread;
   HMODULE           hModule;
   PNOTIFY_CONTEXT   pNotifyCtx;
   NOTIFY_THREAD_CTX ThreadCtx;
   
   /* Initialize locals.. */
   dwRet      = NO_ERROR;   
   hEvent     = NULL;
   hThread    = NULL;
   hModule    = NULL;
   pNotifyCtx = NULL;
      
   __try
   {   
      /* Build the notification packet */
      hEvent = CreateEvent(NULL,
                           TRUE,
                           FALSE,
                           NULL);

      if ( !hEvent )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      pNotifyCtx = reinterpret_cast<PNOTIFY_CONTEXT>(FqiAlloc(sizeof(NOTIFY_CONTEXT)));
      if ( !pNotifyCtx )
      {
         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      /* Add a reference on the QueueCtx for the notification context */
      FqiReferenceQueueContext(pQueueCtx);

      /* 1 reference for this function, and 1 for the caller. The thread will add its own */
      pNotifyCtx->ReferenceCount         = 2;
      pNotifyCtx->QueueCtx               = pQueueCtx;
      pNotifyCtx->NotificationEvent      = hEvent;
      pNotifyCtx->NotificationCallback   = pCallback;
      pNotifyCtx->NotificationParameter  = pParameter;
      /* The notification packet now owns hEvent */
      hEvent = NULL;
            
      hModule = LoadLibrary(L"FRAGQUE.DLL");
      if ( !hModule )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      ThreadCtx.Parameter      = pNotifyCtx;
      ThreadCtx.ModuleLock     = hModule;
      ThreadCtx.InitDoneEvent  = 0;
      ThreadCtx.InitStatusCode = NO_ERROR;
      ThreadCtx.LastModified   = (*pLastModified);

      /* Kick off the thread.. */
      hThread = CreateThread(NULL,
                             0,
                             &FqiNotificationThreadRoutine,
                             &ThreadCtx,
                             0,
                             NULL);

      if ( !hThread )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      /* The thread owns the module count now */
      hModule = NULL;

      /* Wait for the thread to signal the startup event */
      while ( 0 == ThreadCtx.InitDoneEvent )
      {
         Sleep(30);
      }

      dwRet = ThreadCtx.InitStatusCode;

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      /* Everything is cool. Assign the notification packet to the caller */
      (*ppNotifyCtx) = pNotifyCtx;
   }
   __finally
   {
      if ( hModule )
      {
         FreeLibrary(hModule);
      }

      if ( hThread )
      {
         CloseHandle(hThread);
      }

      if ( hEvent )
      {
         CloseHandle(hEvent);
      }

      if ( pNotifyCtx )
      {
         FqiReleaseNotificationContext(pNotifyCtx);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

VOID FqiShutdownQueueNotificationThread( PNOTIFY_CONTEXT pNotifyCtx ) throw()
{
   _ASSERTE(NULL != pNotifyCtx->NotificationEvent);

   /* Signal the notification thread so it will shutdown */
   SetEvent(pNotifyCtx->NotificationEvent);

   /* Release the implicit reference added in FqiStartQueueNotificationThread(). This may or
    * may not be the last one if the thread exited already */
   FqiReleaseNotificationContext(pNotifyCtx);
}

/**
 * FqiNotificationThreadRoutine
 *    Thread callback routine for watching a queue directory and posting notifications
 *    about updates to a client
 */
DWORD WINAPI FqiNotificationThreadRoutine( LPVOID pParam ) throw()
{
   DWORD                dwRet;
   PNOTIFY_THREAD_CTX   pThreadCtx;
   HMODULE              hModule;
   PNOTIFY_CONTEXT      pNotifyCtx;
   HANDLE               rghWait[2];
   LONGLONG             StartTime;
   LONGLONG             LastModified;
   DWORD                dwWait;
   BOOL                 bFirstRun;

   /* Initialize locals */
   dwRet       = NO_ERROR;
   pThreadCtx  = reinterpret_cast<PNOTIFY_THREAD_CTX>(pParam);
   hModule     = pThreadCtx->ModuleLock;
   pNotifyCtx  = reinterpret_cast<PNOTIFY_CONTEXT>(pThreadCtx->Parameter);
   rghWait[0]  = pNotifyCtx->NotificationEvent;
   rghWait[1]  = NULL;
   StartTime   = pThreadCtx->LastModified;
   bFirstRun   = TRUE;
      
#ifdef _DEBUG
   SetThreadName(GetCurrentThreadId(),
                 __FUNCTION__);
#endif /* _DEBUG */

   /* Add a reference on the notification context for the lifetime of this thread.. */
   FqiReferenceNotificationContext(pNotifyCtx);

   __try
   {
      /* Open a watch on the directory.. */
      rghWait[1] = FindFirstChangeNotification(pNotifyCtx->QueueCtx->FileQueuePath,
                                               FALSE,
                                               FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE);

      if ( !rghWait[1] )
      {
         /* Failure */
         dwRet = GetLastError();                  
      }

      /* Notify the caller that we've finished our initilization */
      pThreadCtx->InitStatusCode = dwRet;
      InterlockedExchange(&(pThreadCtx->InitDoneEvent),
                          1);

      /* This is now unavailable to us.. */
      pThreadCtx = NULL;

      /* If we failed to acquire the change handle, bail */
      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      #pragma warning( suppress : 4127 )
      while ( true )
      {
         /* We always run this at idle priority. We do it here, in case a client changes it when
          * we enter their context */
         SetThreadPriority(GetCurrentThread(),
                           THREAD_PRIORITY_IDLE);
   
         if ( bFirstRun )
         {
            /* This is the first iteration of this loop. We guarantee callers that we'll at least scan
             * the directory once so we can capture any backdated records */
            dwWait    = WAIT_OBJECT_0 + 1;
            bFirstRun = FALSE;
         }
         else
         {
            dwWait = WaitForMultipleObjects(_countof(rghWait),
                                            rghWait,
                                            FALSE,
                                            INFINITE);
         }

         if ( (WAIT_OBJECT_0 + 0) == dwWait )
         {
            /* The shutdown event was set, so we just bail out */
            break;
         }
         else if ( (WAIT_OBJECT_0 + 1) == dwWait )
         {
            /* Something changed in the directory, so do the refresh and post the updates
             * to the client */

            /* The files enumerated are only tracked if their LastModified time is newer than the
             * delta we provide. We need to save the time before we do the enumeration though, or
             * there may be a gap between when FqiEnumerateFileQueue() returns and when we get to
             * update the time delta. So we save it before we do the enumeration. This means that
             * there may be some duplicate records next time, but it's up to clients to manage that */
            LastModified = StartTime - (100 * 1000);
            FqGetLastModifiedTime(&StartTime);

            /* Run the enumeration under the impersonation stored in the queue context */
            if ( SetThreadToken(NULL,
                                pNotifyCtx->QueueCtx->ImpersonationToken) )
            {
               __try
               {
                  dwRet = FqiEnumerateFileQueue(pNotifyCtx->QueueCtx,
                                                &FqiNotificationThreadEnumQueueRoutine,
                                                pNotifyCtx,
                                                &LastModified);
               }
               __finally
               {
                  RevertToSelf();
               }
            }
         }
         else
         {
            /* Some error */
            dwRet = GetLastError();
            /* Failure */
            break;
         }

         if ( !FindNextChangeNotification(rghWait[1]) )
         {
            dwRet = GetLastError();
            /* Failure */
            break;
         }
      }
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
      dwRet = GetExceptionCode();
   }

   if ( rghWait[1] )
   {
      FindCloseChangeNotification(rghWait[1]);
   }

   FqiReleaseNotificationContext(pNotifyCtx);
   
   FreeLibraryAndExitThread(hModule,
                            dwRet);
}

BOOL QUEUEAPI FqiNotificationThreadEnumQueueRoutine( __in HANDLE hQueueCtx, __in_z LPCWSTR pwszFileName, __in PLONGLONG pLastModified, __in_opt PVOID pParameter ) throw()
{
   PNOTIFY_CONTEXT pNotifyCtx;

   /* Initialize locals */
   pNotifyCtx = reinterpret_cast<PNOTIFY_CONTEXT>(pParameter);

   /* Check if this thread has the last reference on the queue context. If that's the case, then
    * the client must have unregistered itself so we shouldn't post any other notifications to them */
   if ( 1 == pNotifyCtx->ReferenceCount )
   {
      /* Cancel the enumeration */
      return ( FALSE );
   }
   
   pNotifyCtx->NotificationCallback(hQueueCtx,
                                    NotificationContextToHandle(pNotifyCtx),
                                    pwszFileName,
                                    pLastModified,
                                    pNotifyCtx->NotificationParameter);

   /* Continue the enumeration */
   return ( TRUE );
}

DWORD FqiGetFileQueuePath( PQUEUE_CONTEXT pQueueCtx, DWORD dwFlags, LPWSTR pwszBuf, size_t cchBuf ) throw()
{
   DWORD    dwRet;
   HRESULT  hr;
   int      idx;
   LPCWSTR  rgDir[] =
   {
      L"jBoschen",
      L"FragExt",
      L"FileQueue"
   };

   _ASSERTE(cchBuf >= MAX_PATH);

   /* Get the common application data folder, and ensure the sub-
    * directories for the file queue exist. The security on the
    * sub-directories will inherit from common appdata */
   hr = SHGetFolderPath(NULL,
                        CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,
                        pQueueCtx->ImpersonationToken,
                        SHGFP_TYPE_CURRENT,
                        pwszBuf);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( ERROR_DIRECTORY );
   }

   dwRet = NO_ERROR;

   for ( idx = 0; idx < _countof(rgDir); idx++ )
   {
      if ( !PathAppend(pwszBuf,
                        cchBuf,
                        rgDir[idx]) )
      {
         dwRet = ERROR_BAD_PATHNAME;
         /* Failure */
         break;
      }

      /* If the caller doesn't want to create the directory or it already exists, just
       * continue the loop */
      if ( FqiFlagOn(dwFlags,
                     FQI_DONOTCREATE) )
      {
         continue;
      }

      if ( PathDirectoryExists(pwszBuf) )
      {
         continue;
      }

      if ( !CreateDirectory(pwszBuf,
                            NULL) )
      {
         dwRet = GetLastError();
         /* Another thread could have created this already while were trying to, so it's cool
          * if we get this error. Any other is failure */
         if ( ERROR_ALREADY_EXISTS != dwRet )
         {
            /* Failure */
            break;
         }

         dwRet = NO_ERROR;
      }
   }

   if ( NO_ERROR == dwRet )
   {
      /* Ensure the path we store in the QUEUE_CONTEXT always has a trailing \ */
      if ( !PathAddBackslash(pwszBuf,
                              cchBuf) )
      {
         /* Failure */
         dwRet = ERROR_INSUFFICIENT_BUFFER;
      }
   }         


   /* Success / Failure */
   return ( dwRet );   
}

DWORD FqiGetQueueNameForFileName( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName, LPWSTR pwszBuf, size_t cchBuf ) throw()
{
   DWORD dwRet;
   ULONG iHash;

   /* Build the filename hash for the file */
   iHash = FqiCreateFileNameHash(pwszFileName);

   if ( FAILED(StringCchPrintf(pwszBuf,
                               cchBuf,
                               L"%s%08lX",
                               pQueueCtx->FileQueuePath,
                               iHash)) )
   {
      dwRet = ERROR_INSUFFICIENT_BUFFER;
      /* Failure */
      return ( dwRet );
   }

   /* Success */
   return ( NO_ERROR );
}

ULONG FqiCreateFileNameHash( LPCWSTR pwszFileName ) throw()
{
   WCHAR ch;
   ULONG iHash;

   iHash = 5381;

   while ( L'\0' != *pwszFileName )
   {
      ch    = (*pwszFileName++);
      iHash = ((iHash << 5) + iHash) + (((ch >= L'a') && (ch <= L'z')) ? ch - L'a' + L'A' : ch);
   }

#ifdef _DEBUG
   /* This will limit the number of files to 2, to stress the code that handles multiple records */
   return ( iHash & 0x00000001 );
#else /* DBG */
   return ( iHash );
#endif /* _DEBUG */
}

DWORD FqiWriteFileQueueFile( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName, PLONGLONG pLastModified ) throw()
{
   DWORD          dwRet;
   WCHAR          chPath[MAX_PATH];
   HANDLE         hFile;
   BOOL           bLocked;
   OVERLAPPED     Overlapped;
   LARGE_INTEGER  cbFile;
   PQUEUE_RECORD  pQueRecord;
   LARGE_INTEGER  iPointer;
   LONGLONG       LastModified;
   
   /* Initialize locals */
   hFile      = INVALID_HANDLE_VALUE;
   bLocked    = FALSE;
   pQueRecord = NULL;

   /* Get the hashed name for the specified file */
   dwRet = FqiGetQueueNameForFileName(pQueueCtx,
                                      pwszFileName,
                                      chPath,
                                      _countof(chPath));

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   hFile = CreateFile(chPath,
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                      NULL,
                      OPEN_ALWAYS,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);

   if ( INVALID_HANDLE_VALUE == hFile )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   __try
   {
      /* Lock the entire file for exclusive access. We specifiy that maximum possible
       * size here in case another thread has modified it since we opened it */
      ZeroMemory(&Overlapped,
                 sizeof(OVERLAPPED));

      bLocked = LockFileEx(hFile,
                           LOCKFILE_EXCLUSIVE_LOCK,
                           0,
                           0xFFFFFFFF,
                           0x7FFFFFFF,
                           &Overlapped);

      if ( !bLocked )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

#ifdef _DEBUG
      FqiValidateQueueFile(hFile);
#endif /* _DEBUG */

      /* It's now safe to get the file size and execute the correct code path.. */
      cbFile.QuadPart = 0;

      if ( !GetFileSizeEx(hFile,
                          &cbFile) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      /* If the caller didn't pass a timestamp, create one now. We do this after the file lock
       * is acquired so that it's likely to be unique */
      if ( !pLastModified )
      {
         FqGetLastModifiedTime(&LastModified);
         pLastModified = &LastModified;
      }

      while ( cbFile.QuadPart > 0 )
      {
         dwRet = FqiReadNextFileQueueFileRecord(hFile,
                                                &pQueRecord);

         if ( NO_ERROR != dwRet )
         {
            /* Failure */
            __leave;
         }

         /* Make sure the size of the record is valid for the file */
         if ( pQueRecord->Size > cbFile.QuadPart )
         {
            dwRet = ERROR_FILE_CORRUPT;
            /* Failure */
            __leave;
         }

         cbFile.QuadPart -= pQueRecord->Size;

         /* If the names match, then we're good to go */
         if ( 0 == lstrcmpi(pwszFileName,
                            pQueRecord->FileName) )
         {
            /* Move the file pointer to the start of this record, and rewrite it
             * into the file. Since they're exactly the same, we're not going to
             * overwrite any other records */
            iPointer.QuadPart = -static_cast<LONGLONG>(pQueRecord->Size);

            if ( !SetFilePointerEx(hFile,
                                   iPointer,
                                   NULL,
                                   FILE_CURRENT) )
            {
               dwRet = GetLastError();
               /* Failure */
               __leave;
            }

            pQueRecord->LastModified = (*pLastModified);
            /* Now write out the exact same record, but with the modified time, then bail */
            dwRet = FqiWriteNextFileQueueFileRecord(hFile,
                                                    pQueRecord);

            /* Success / Failure */
            __leave;
         }

         _dTrace(FQI_TRACE_WARN, L"Bucket collision:Bucket:%s, Target:%s, Collision:%s\n", chPath, pwszFileName, pQueRecord->FileName);
         
         /* Free the record so we can allocate a new one */
         FqiFreeQueueFileRecord(pQueRecord);
         pQueRecord = NULL;
      }

      /* If we get here, the file does not already have this record in it, so build
       * it up and write it out. The file pointer will be in the correct location  */
      pQueRecord = FqiBuildQueueFileRecord(pwszFileName);
      if ( !pQueRecord )
      {
         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      pQueRecord->LastModified = (*pLastModified);
      dwRet = FqiWriteNextFileQueueFileRecord(hFile,
                                              pQueRecord);

      /* If the write failed, FqiWriteNextFileQueueFileRecord will have reset the
       * file pointer to where it was before we called it, so we will truncate it */
      SetEndOfFile(hFile);
   }
   __finally
   {
#ifdef _DEBUG
      FqiValidateQueueFile(hFile);
#endif /* _DEBUG */

      /* If we locked the file, make sure we unlocked it */
      if ( bLocked )
      {
         _ASSERTE(INVALID_HANDLE_VALUE != hFile);

         ZeroMemory(&Overlapped,
                    sizeof(OVERLAPPED));

         UnlockFileEx(hFile,
                      0,
                      0xFFFFFFFF,
                      0x7FFFFFFF,
                      &Overlapped);
      }

      if ( INVALID_HANDLE_VALUE != hFile )
      {
         CloseHandle(hFile);
      }

      if ( pQueRecord )
      {
         FqiFreeQueueFileRecord(pQueRecord);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD FqiReadNextFileQueueFileRecord( HANDLE hFile, PQUEUE_RECORD* ppQueRecord ) throw()
{
   DWORD          dwRet;
   LARGE_INTEGER  cbFile;
   LARGE_INTEGER  iPointer;
   DWORD          cbRead;
   DWORD          cbToRead;
   QUEUE_RECORD   QueRecord;
   PQUEUE_RECORD  pQueRecord;

   /* Initialize outputs.. */
   (*ppQueRecord) = NULL;

   /* Intiialize locals */
   dwRet      = NO_ERROR;
   pQueRecord = NULL;

   /* Get the current file size and pointer, so we can verify the size we read out of
    * the file is accurate */
   if ( !GetFileSizeEx(hFile,
                       &cbFile) )
   {
      dwRet = GetLastError();
      
      _dTrace(FQI_TRACE_WARN, L"GetFileSizeEx() failed!0x%08lx\n", dwRet);
      /* Failure */
      return ( dwRet );
   }

   iPointer.QuadPart = 0;
   if ( !SetFilePointerEx(hFile,
                          iPointer,
                          &iPointer,
                          FILE_CURRENT) )
   {
      dwRet = GetLastError();

      _dTrace(FQI_TRACE_WARN, L"SetFilePointerEx() failed!0x%08lx\n", dwRet);
      /* Failure */
      return ( dwRet );
   }

   __try
   {
      /* Try to read the size of the record out of the file.. */
      ZeroMemory(&QueRecord,
                 sizeof(QUEUE_RECORD));

      cbRead   = 0;
      cbToRead = RTL_SIZEOF_THROUGH_FIELD(QUEUE_RECORD, LastModified);
      
      if ( !ReadFile(hFile,
                     reinterpret_cast<LPVOID>(&QueRecord),
                     cbToRead,
                     &cbRead,
                     NULL) )
      {
         dwRet = GetLastError();

         _dTrace(FQI_TRACE_WARN, L"ReadFile() failed!0x%08lx\n", dwRet);
         /* Failure */
         __leave;
      }

      if ( cbRead < cbToRead )
      {
         dwRet = ERROR_FILE_CORRUPT;
         /* Failure */
         __leave;
      }

      /* Make sure the size for this record is valid */
      if ( (QueRecord.Size < FQI_QUEUE_RECORD_CB_MIN) || (QueRecord.Size > FQI_QUEUE_RECORD_CB_MAX) )
      {
         _dTrace(FQI_TRACE_WARN, L"The record size is smaller or larger than expected values\n");

         dwRet = ERROR_FILE_CORRUPT;
         /* Failure */
         __leave;
      }
      else if ( QueRecord.FileNameLength != FqiCalculateQueueRecordFileNameLength(QueRecord.Size) )
      {
         _dTrace(FQI_TRACE_WARN, L"The file name length is does not match the expected length of the record\n");


         dwRet = ERROR_FILE_CORRUPT;
         /* Failure */
         __leave;
      }
      else if ( (iPointer.QuadPart + QueRecord.Size) > cbFile.QuadPart )
      {
         _dTrace(FQI_TRACE_WARN, L"The file does not contain enough data to account for record size\n");

         dwRet = ERROR_FILE_CORRUPT;
         /* Failure */
         __leave;
      }
      else if ( (iPointer.QuadPart + QueRecord.Size) < iPointer.QuadPart )
      {
         /* The size of the record in the file caused an overflow */
         _dTrace(FQI_TRACE_WARN, L"The size of the record caused an overflow\n");

         dwRet = ERROR_FILE_CORRUPT;
         /* Failure */
         __leave;
      }
      
      /* The size stored in the file includes the terminating null character */
      pQueRecord = FqiAllocQueueFileRecord(QueRecord.Size);
      if ( !pQueRecord )
      {
         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      /* Assign the header information.. */
      pQueRecord->FileNameLength = QueRecord.FileNameLength;
      pQueRecord->LastModified   = QueRecord.LastModified;

      /* Read the rest of the record from the file */
      cbRead   = 0;
      cbToRead = QueRecord.Size - RTL_SIZEOF_THROUGH_FIELD(QUEUE_RECORD, LastModified);
      if ( !ReadFile(hFile,
                     &(pQueRecord->FileName[0]),
                     cbToRead,
                     &cbRead,
                     NULL) )
      {
         dwRet = GetLastError();

         _dTrace(FQI_TRACE_WARN, L"ReadFile() failed!0x%08lx\n", dwRet);
         /* Failure */
         __leave;
      }

      /* Validate the data we read. The size and lengths must match up, and we'll check for a null
       * term character where it is supposed to be */
      if ( cbRead != cbToRead )
      {
         /* The read failed for some reason. Since we already verified the size, this is some other error */

         dwRet = ERROR_READ_FAULT;
         /* Failure */
         __leave;
      }
      else if ( L'\0' != pQueRecord->FileName[pQueRecord->FileNameLength] )
      {
         _dTrace(FQI_TRACE_WARN, L"The record is missing a terminating null character\n");
         
         dwRet = ERROR_FILE_CORRUPT;
         /* Failure */
         __leave;
      }

      /* Everything is cool. Assign the record to the caller and exit */
      (*ppQueRecord) = pQueRecord;
      pQueRecord     = NULL;

      /* Success */
      dwRet = NO_ERROR;
   }
   __finally
   {
      if ( pQueRecord )
      {
         FqiFree(pQueRecord);
      }

      if ( NO_ERROR != dwRet )
      {
         /* Return the file pointer to the position it was when this function started */
         SetFilePointerEx(hFile,
                          iPointer,
                          NULL,
                          FILE_BEGIN);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD FqiWriteNextFileQueueFileRecord( HANDLE hFile, PQUEUE_RECORD pQueRecord ) throw()
{
   DWORD          dwRet;
   LARGE_INTEGER  iPointer;
   DWORD          cbWritten;

   /* Get the current file pointer so we can reset if anything fails */
   iPointer.QuadPart = 0;
   if ( !SetFilePointerEx(hFile,
                          iPointer,
                          &iPointer,
                          FILE_CURRENT) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   /* Write out the record */
   cbWritten = 0;
   if ( !WriteFile(hFile,
                   reinterpret_cast<LPCVOID>(pQueRecord),
                   pQueRecord->Size,
                   &cbWritten,
                   NULL) )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   if ( cbWritten != pQueRecord->Size )
   {
      dwRet = ERROR_WRITE_FAULT;
      /* Failure */
      goto __CLEANUP;
   }

   /* Everything is cool.. */
   dwRet = NO_ERROR;

__CLEANUP:
   if ( NO_ERROR != dwRet )
   {
      /* Reset the size of the file */
      SetFilePointerEx(hFile,
                       iPointer,
                       NULL,
                       FILE_BEGIN);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD FqiDeleteFileQueueFile( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName ) throw()
{
   DWORD          dwRet;
   WCHAR          chPath[MAX_PATH];
   HANDLE         hFile;
   BOOL           bLocked;
   OVERLAPPED     Overlapped;
   LARGE_INTEGER  cbFile;
   PQUEUE_RECORD  pQueRecord;
   LPBYTE         pbData;
   LPBYTE         pbDataPointer;
   DWORD          cbData;
   LARGE_INTEGER  iPointer;
   DWORD          cbWritten;
   DWORD          cbToWrite;

   /* Initialize locals */
   hFile         = INVALID_HANDLE_VALUE;
   bLocked       = FALSE;
   pQueRecord    = NULL;
   pbData        = NULL;
   pbDataPointer = NULL;

   /* Get the hashed name for the specified file */
   dwRet = FqiGetQueueNameForFileName(pQueueCtx,
                                      pwszFileName,
                                      chPath,
                                      _countof(chPath));

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   /* Try to open the file. If it doesn't exist, then there's no record */
   hFile = CreateFile(chPath,
                      GENERIC_READ|GENERIC_WRITE,
                      FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);

   if ( INVALID_HANDLE_VALUE == hFile )
   {
      dwRet = GetLastError();
      if ( ERROR_FILE_NOT_FOUND == dwRet )
      {
         /* Success */
         dwRet = NO_ERROR;
      }

      /* Success / Failure */
      return ( dwRet );
   }

   __try
   {
      /* Lock the entire file for exclusive write access. We specifiy that maximum possible
       * size here in case another thread has modified it since we opened it */
      ZeroMemory(&Overlapped,
                 sizeof(OVERLAPPED));

      bLocked = LockFileEx(hFile,
                           LOCKFILE_EXCLUSIVE_LOCK,
                           0,
                           0xFFFFFFFF,
                           0x7FFFFFFF,
                           &Overlapped);

      if ( !bLocked )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

#ifdef _DEBUG
      FqiValidateQueueFile(hFile);
#endif /* _DEBUG */

      /* It's now safe to get the file size and execute the correct code path.. */
      cbFile.QuadPart = 0;

      if ( !GetFileSizeEx(hFile,
                          &cbFile) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      /* We don't support files larger than ULONG_MAX */
      if ( cbFile.HighPart > 0 )
      {
         dwRet = ERROR_FILE_CORRUPT;
         /* Failure */
         __leave;
      }

      /* Save the file size in case we have to allocate copy storage */
      cbData = cbFile.LowPart;

      while ( cbFile.QuadPart > 0 )
      {
         dwRet = FqiReadNextFileQueueFileRecord(hFile,
                                                &pQueRecord);

         if ( NO_ERROR != dwRet )
         {
            /* Failure */
            __leave;
         }

         /* Validate and adjust the tracking size */
         if ( pQueRecord->Size > cbFile.QuadPart )
         {
            dwRet = ERROR_FILE_CORRUPT;
            /* Failure */
            __leave;
         }

         cbFile.QuadPart -= pQueRecord->Size;

         /* If the names do not match, then we copy this record to our temp storage */
         if ( 0 != lstrcmpi(pwszFileName,
                            pQueRecord->FileName) )
         {
            /* The temp storage is lazy initialized, because the normal case is that a file
             * will only have a single record, so we'll just be deleting the file */
            if ( !pbData )
            {
               pbData = reinterpret_cast<LPBYTE>(FqiAlloc(cbData));
               if ( !pbData )
               {
                  dwRet = ERROR_OUTOFMEMORY;
                  /* Failure */
                  __leave;
               }

               pbDataPointer = pbData;
            }

            /* Copy the record to our temporary storage */
            CopyMemory(pbDataPointer,
                       pQueRecord,
                       pQueRecord->Size);
            pbDataPointer += pQueRecord->Size;
         }
         
         /* Free the record so we can allocate a new one */
         FqiFreeQueueFileRecord(pQueRecord);
         pQueRecord = NULL;
      }

      if ( !pbData )
      {
         /* There was only 1 record in the file, so we're deleting it. If another thread has opened the file
          * this will fail, which we'll allow since in that case another file may be trying to write to or
          * delete a record from it, so it must stick around */
         if ( !DeleteFile(chPath) )
         {
            dwRet = GetLastError();
            if ( ERROR_ACCESS_DENIED == dwRet )
            {
               /* Success */
               dwRet = NO_ERROR;
            }

            /* Success / Failure */
            __leave;
         }
      }
      else
      {
         /* Copy the temp data back to the file */
         iPointer.QuadPart = 0;
         if ( !SetFilePointerEx(hFile,
                                iPointer,
                                NULL,
                                FILE_BEGIN) )
         {
            dwRet = GetLastError();
            /* Failure */
            __leave;
         }

         cbToWrite = static_cast<DWORD>(pbDataPointer - pbData);
         cbWritten = 0;
         
         if ( !WriteFile(hFile,
                         reinterpret_cast<LPCVOID>(pbData),
                         cbToWrite,
                         &cbWritten,
                         NULL) )
         {
            dwRet = GetLastError();
            /* Failure */
            __leave;
         }

         if ( cbWritten != cbToWrite )
         {
            dwRet = ERROR_FILE_CORRUPT;
            /* Failure */
            __leave;
         }

         SetEndOfFile(hFile);
      }

      /* Everything wen't Ok */
      dwRet = NO_ERROR;
   }   
   __finally
   {
#ifdef _DEBUG
      FqiValidateQueueFile(hFile);
#endif /* _DEBUG */

      /* If we locked the file, make sure we unlocked it */
      if ( bLocked )
      {
         _ASSERTE(INVALID_HANDLE_VALUE != hFile);

         ZeroMemory(&Overlapped,
                    sizeof(OVERLAPPED));

         UnlockFileEx(hFile,
                      0,
                      0xFFFFFFFF,
                      0x7FFFFFFF,
                      &Overlapped);
      }

      if ( INVALID_HANDLE_VALUE != hFile )
      {
         CloseHandle(hFile);
      }

      if ( pQueRecord )
      {
         FqiFreeQueueFileRecord(pQueRecord);
      }

      if ( pbData )
      {
         FqiFree(pbData);
      }
   }
   

   /* Success / Failure */
   return ( dwRet );
}

DWORD FqiDeleteQueueFile( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName ) throw()
{
   DWORD dwRet;
   WCHAR chPath[MAX_PATH];
   BOOL  bRet;
   
   /* Get the hashed name for the specified file */
   dwRet = FqiGetQueueNameForFileName(pQueueCtx,
                                      pwszFileName,
                                      chPath,
                                      _countof(chPath));

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   bRet  = DeleteFile(chPath);   
   if ( !bRet )
   {
      dwRet = GetLastError();
   }

   return ( dwRet );
}

DWORD FqiEnumerateFileQueue( __in PQUEUE_CONTEXT pQueueCtx, __in PENUMQUEUEPROC pEnumQueueProc, __in_opt PVOID pParameter, __in_opt PLONGLONG pLastModified ) throw()
{
   DWORD             dwRet;
   WCHAR             chPath[MAX_PATH];
   HANDLE            hFind;
   WIN32_FIND_DATA   FindData;
   
   /* Initialize locals */
   hFind = INVALID_HANDLE_VALUE;
   
   /* Build the search string */
   if ( FAILED(StringCchCopy(chPath,
                             _countof(chPath),
                             pQueueCtx->FileQueuePath)) )
   {
      dwRet = ERROR_INSUFFICIENT_BUFFER;
      /* Failure */
      return ( dwRet );
   }

   if ( !PathAppend(chPath,
                     _countof(chPath),
                     L"*") )
   {
      dwRet = ERROR_INSUFFICIENT_BUFFER;
      /* Failure */
      return ( dwRet );
   }

   ZeroMemory(&FindData,
              sizeof(WIN32_FIND_DATA));

   hFind = FindFirstFile(chPath,
                         &FindData);

   if ( INVALID_HANDLE_VALUE == hFind )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   dwRet = NO_ERROR;

   __try
   {
      do
      {
         if ( !FqiIsDotDirectory(FindData.cFileName) )
         {
            /* Build the path to the file */
            if ( PathReplaceFileSpec(chPath,
                                     _countof(chPath),
                                     FindData.cFileName) )
            {
               dwRet = FqiEnumerateFileQueueFile(pQueueCtx,
                                                 chPath,
                                                 pEnumQueueProc,
                                                 pParameter,
                                                 pLastModified);
            }
         }
      }
      while ( FindNextFile(hFind,
                           &FindData) );

      /* Success */
      dwRet = NO_ERROR;
   }
   __finally
   {      
      FindClose(hFind);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD FqiEnumerateFileQueueFile( __in PQUEUE_CONTEXT pQueueCtx, __in_z LPCWSTR pwszFile, __in PENUMQUEUEPROC pEnumQueueProc, __in_opt PVOID pParameter, __in_opt PLONGLONG pLastModified ) throw()
{
   DWORD          dwRet;
   HANDLE         hFile;
   LARGE_INTEGER  cbFile;
   BOOL           bLocked;
   OVERLAPPED     Overlapped;
   PQUEUE_RECORD  pQueRecord;

   /* Initialize locals.. */
   hFile      = INVALID_HANDLE_VALUE;
   bLocked    = FALSE;
   pQueRecord = NULL;

   /* Open the file.. */
   hFile = CreateFile(pwszFile,
                      GENERIC_READ,
                      FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_FLAG_SEQUENTIAL_SCAN,
                      NULL);

   if ( INVALID_HANDLE_VALUE == hFile )
   {
      dwRet = GetLastError();
      
      _dTrace(FQI_TRACE_WARN, L"Failed to open file. Error:%08lx, Path:%s\n", dwRet, pwszFile);

      /* Failure */
      return ( dwRet );      
   }

   dwRet = NO_ERROR;

   __try
   {
      /* Lock the file for shared read access.. */
      ZeroMemory(&Overlapped,
                 sizeof(OVERLAPPED));

      if ( !LockFileEx(hFile,
                       0,
                       0,
                       0xFFFFFFFF,
                       0x7FFFFFFF,
                       &Overlapped) )
      {
         dwRet = GetLastError();

         _dTrace(FQI_TRACE_WARN, L"Failed to lock file for shared read access. Error:%08lx, Path:%s\n", dwRet, pwszFile);
         /* Failure */
         __leave;
      }

      bLocked = TRUE;

      /* Get the file size and start reading records */
      cbFile.QuadPart = 0;

      if ( !GetFileSizeEx(hFile,
                          &cbFile) )
      {
         dwRet = GetLastError();

         _dTrace(FQI_TRACE_WARN, L"Failed to determine file size. Error:%08lx, Path:%s\n", dwRet, pwszFile);
         /* Failure */
         __leave;
      }

      while ( cbFile.QuadPart > 0 )
      {
         dwRet = FqiReadNextFileQueueFileRecord(hFile,
                                                &pQueRecord);

         if ( NO_ERROR != dwRet )
         {
            _dTrace(FQI_TRACE_WARN, L"Failed to read file record. Error:%08lx, Path:%s\n", dwRet, pwszFile);
            /* Failure */
            __leave;
         }

         /* Validate and adjust the tracking size */
         if ( pQueRecord->Size > cbFile.QuadPart )
         {
            _dTrace(FQI_TRACE_WARN, L"File record appears to be corrupt. Remaining file size:%i64d, Record size:%d, Path:%s\n", cbFile.QuadPart, pQueRecord->Size, pwszFile);

            dwRet = ERROR_FILE_CORRUPT;
            /* Failure */
            __leave;
         }

         cbFile.QuadPart -= pQueRecord->Size;

         /* If the client didn't pass a LastModified delta, or the current record is newer
          * than the delta, forward the record to the client */
         _dTrace(FQI_TRACE_INFO, L"LastModified Delta=%I64d, LastModified Record=%I64d, File=%s\n", !pLastModified ? 0I64 : (*pLastModified), pQueRecord->LastModified, pQueRecord->FileName);

         if ( !pLastModified || (pQueRecord->LastModified >= (*pLastModified)) )
         {
            /* Pass the record to the caller.. */        
            if ( !(*pEnumQueueProc)(QueueContextToHandle(pQueueCtx),
                                    pQueRecord->FileName,
                                    &(pQueRecord->LastModified),
                                    pParameter) )
            {
               dwRet = ERROR_CANCELLED;
               /* Caller has asked to abort the enumeration */
               __leave;
            }
         }

         FqiFreeQueueFileRecord(pQueRecord);
         pQueRecord = NULL;
      }
   }
   __finally
   {
      if ( bLocked )
      {
         ZeroMemory(&Overlapped,
                    sizeof(OVERLAPPED));

         UnlockFileEx(hFile,
                      0,
                      0xFFFFFFFF,
                      0x7FFFFFFF,
                      &Overlapped);
      }

      if ( INVALID_HANDLE_VALUE != hFile )
      {
         CloseHandle(hFile);
      }

      if ( pQueRecord )
      {
         FqiFreeQueueFileRecord(pQueRecord);
      }
   }

   /* If we encountered a corrupt file, blow it away */
   if ( ERROR_FILE_CORRUPT == dwRet )
   {
      DeleteFile(pwszFile);
   }

   /* Success / Failure */
   return ( dwRet );
}

LPVOID FqiAlloc( size_t cbAlloc )
{
   void* pMem;

   pMem = malloc(cbAlloc);
   if ( pMem )
   {
      ZeroMemory(pMem,
                 cbAlloc);
   }

   /* Success / Failure */
   return ( pMem );
}

VOID FqiFree( LPVOID pMem )
{
   free(pMem);
}

PQUEUE_RECORD FqiAllocQueueFileRecord( DWORD cbSize ) throw()
{
   DWORD          cbAlloc;
   PQUEUE_RECORD  pQueRecord;

   _ASSERTE(cbSize <= FQI_QUEUE_RECORD_CB_MAX);

   cbAlloc    = max(cbSize, FQI_QUEUE_RECORD_CB_ALLOC);
   pQueRecord = reinterpret_cast<PQUEUE_RECORD>(FqiAlloc(cbAlloc));
   if ( pQueRecord )
   {
      pQueRecord->Size           = cbSize;
      pQueRecord->FileNameLength = 0;
   }

   /* Success / Failure */
   return ( pQueRecord );
}

PQUEUE_RECORD FqiBuildQueueFileRecord( LPCWSTR pwszFileName ) throw()
{
   HRESULT        hr;
   size_t         cbName;
   PQUEUE_RECORD  pQueRecord;

   /* Determine the size of the file name. We limit this size FQ_FILENAME_MAX_CB */
   hr = StringCbLength(pwszFileName,
                       FQ_FILENAME_MAX_CB,
                       &cbName);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( NULL );
   }

   /* We know this will not overflow because of the restriction placed
    * on the size via FQI_FILENAME_CB_MAX */
   pQueRecord = FqiAllocQueueFileRecord(static_cast<DWORD>(FQI_QUEUE_RECORD_CB_MIN + cbName));
   if ( pQueRecord )
   {
      /* The file name length, does not include the terminating null character */
      pQueRecord->FileNameLength = static_cast<DWORD>(cbName / sizeof(WCHAR));
      /* Copy the string, and make sure it's null terminated */
      CopyMemory(&(pQueRecord->FileName[0]),
                 pwszFileName,
                 cbName);
      pQueRecord->FileName[cbName / sizeof(WCHAR)] = L'\0';
   }

   /* Success / Failure */
   return ( pQueRecord );
}

VOID FqiFreeQueueFileRecord( PQUEUE_RECORD pQueRecord ) throw()
{
   if ( pQueRecord )
   {
      FqiFree(pQueRecord);
   }
}

DWORD FqiCreateQueuempersonationToken( HANDLE hThread, PHANDLE phToken ) throw()
{
   DWORD    dwRet;
   HANDLE   hThreadToken;
   HANDLE   hRestrictedToken;

   /* Initialize outputs.. */
   (*phToken);

   /* Initialize locals.. */
   hThreadToken     = NULL;
   hRestrictedToken = NULL;
   
   if ( !ImpersonateSelf(SecurityImpersonation) )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   __try
   {
      if ( !OpenThreadToken(hThread,
                            TOKEN_DUPLICATE|TOKEN_QUERY|TOKEN_IMPERSONATE,
                            TRUE,
                            &hThreadToken) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      if ( !CreateRestrictedToken(hThreadToken,
                                  DISABLE_MAX_PRIVILEGE,
                                  0,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  NULL,
                                  &hRestrictedToken) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }
   }
   __finally
   {
      RevertToSelf();

      if ( hThreadToken )
      {
         CloseHandle(hThreadToken);
      }
   }

   /* We only get here via the success path, so assign the token to the caller */
   (*phToken) = hRestrictedToken;

   /* Success */
   dwRet = NO_ERROR;

__CLEANUP:

   /* Success / Failure */
   return ( dwRet );
}

VOID FqiValidateQueueFile( HANDLE hFile ) throw()
{
   DWORD          dwRet;
   HANDLE         hMap;
   LPVOID         pMap;
   LARGE_INTEGER  cbFile;
   PUCHAR         pbData;
   DWORD          cbRecord;
   PQUEUE_RECORD  pQueRecord;
   size_t         cchNameLength;
   WCHAR          chFile[MAX_PATH];

   /* Initialize locals */
   dwRet = NO_ERROR;
   hMap  = NULL;
   pMap  = NULL;
      
   wcscpy(chFile,
          L"<unknown>");

   /* Determine the size of the file.. */
   if ( !GetFileSizeEx(hFile,
                       &cbFile) )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   if ( 0 == cbFile.QuadPart )
   {
      dwRet = NO_ERROR;
      /* Success */
      goto __CLEANUP;
   }

   hMap = CreateFileMapping(hFile,
                            NULL,
                            PAGE_READONLY,
                            0,
                            0,
                            NULL);

   if ( !hMap )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   pMap = MapViewOfFile(hMap,
                        FILE_MAP_READ,
                        0,
                        0,
                        0);

   if ( !pMap )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   /* Get the name of the file we just mapped */
   if ( 0 == GetMappedFileName(GetCurrentProcess(), 
                               pMap,
                               chFile,
                               _countof(chFile)) )
   {
      wcscpy(chFile,
             L"<unknown>");
   }

   pbData   = reinterpret_cast<PUCHAR>(pMap);
   cbRecord = 0;

   while ( cbFile.QuadPart > 0 )
   {
      if ( cbFile.QuadPart < FQI_QUEUE_RECORD_CB_MIN )
      {
         _dTrace(FQI_TRACE_ERROR, L"Remaining file size is less than the minimum possible record size!Remaining file size:%i64d, Minimum:%d\n", cbFile.QuadPart, FQI_QUEUE_RECORD_CB_MIN);
         dwRet = ERROR_FILE_CORRUPT;
         
         _ASSERTE(dwRet != ERROR_FILE_CORRUPT);
         /* Failure */
         break;
      }

      pQueRecord = reinterpret_cast<PQUEUE_RECORD>(pbData);

      if ( pQueRecord->Size < FQI_QUEUE_RECORD_CB_MIN )
      {
         _dTrace(FQI_TRACE_ERROR, L"Record size is less than the minimum possible record size!Record size:%d, Minimum:%d\n", pQueRecord->Size, FQI_QUEUE_RECORD_CB_MIN);
         dwRet = ERROR_FILE_CORRUPT;

         _ASSERTE(dwRet != ERROR_FILE_CORRUPT);
         /* Failure */
         break;
      }

      if ( pQueRecord->Size > cbFile.QuadPart )
      {
         _dTrace(FQI_TRACE_ERROR, L"Record size is larger than available file data!Record size:%d, Remaining file size:%i64d\n", pQueRecord->Size, cbFile.QuadPart);
         dwRet = ERROR_FILE_CORRUPT;

         _ASSERTE(dwRet != ERROR_FILE_CORRUPT);
         /* Failure */
         break;
      }

      if ( pQueRecord->Size > FQI_QUEUE_RECORD_CB_MAX )
      {
         _dTrace(FQI_TRACE_ERROR, L"Record size is larger than maximum allowable record size!Record size:%d, Maximum:%d\n", pQueRecord->Size, FQI_QUEUE_RECORD_CB_MAX);
         dwRet = ERROR_FILE_CORRUPT;

         _ASSERTE(dwRet != ERROR_FILE_CORRUPT);
         /* Failure */
         break;
      }

      /* Scan the file name for the expected length.  This can throw an exception if we end up scanning past 
       * the available file mapping, which will indicate that the filename is corrupt */
      dwRet         = NO_ERROR;
      cchNameLength = 0;

      __try
      {
         cchNameLength = wcslen(pQueRecord->FileName);
      }
      __except( EXCEPTION_EXECUTE_HANDLER )
      {
         dwRet = GetExceptionCode();
      }
      
      if ( NO_ERROR != dwRet )
      {
         _dTrace(FQI_TRACE_ERROR, L"Exception occured while scanning expected file data for end of filename!0x%08lx\n", dwRet);
         dwRet = ERROR_FILE_CORRUPT;

         _ASSERTE(dwRet != ERROR_FILE_CORRUPT);
         /* Failure */
         break;
      }

      if ( cchNameLength != pQueRecord->FileNameLength )
      {
         _dTrace(FQI_TRACE_ERROR, L"File name length does is inaccurate!Record filename length:%d, Actual:%i64u\n", pQueRecord->FileNameLength, static_cast<ULONGLONG>(cchNameLength));
         dwRet = ERROR_FILE_CORRUPT;

         _ASSERTE(dwRet != ERROR_FILE_CORRUPT);
         /* Failure */
         break;
      }
      
      /* Everything with this record looks OK, so move up to the next one.. */
      pbData += pQueRecord->Size;

      /* Subtract how much data should be remaining in the file to satisfy any remaining records */
      cbFile.QuadPart -= pQueRecord->Size;
   }

__CLEANUP:
   if ( pMap )
   {
      UnmapViewOfFile(pMap);
   }

   if ( hMap )
   {
      CloseHandle(hMap);
   }

   _dTrace(dwRet, L"Exiting with status!0x%08lx!File:%s\n", dwRet, chFile);
}
