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

/* FragQuep.h
 *    FragQue private implementation interfaces
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#pragma once

#include "FragQue.h"
#include <Utility.h>

/**********************************************************************

   Private

   The per-user Queue is a directory in the user's application data
   folder where file records are written. Each file record is a record
   in the queue. The queue file contains only the priority and the full
   path of the file to defragment.

 **********************************************************************/

typedef struct _QUEUE_CONTEXT
{
   /* */
   volatile ULONG    ReferenceCount;
#ifdef _WIN64
   ULONG             Alignment;
#endif /* _WIN64 */
   /* Token used for impersonating the thread that initialized the context */
   HANDLE            ImpersonationToken;
   /* The path of the file queue directory. This always has a trailing \ */
   WCHAR             FileQueuePath[MAX_PATH];
}QUEUE_CONTEXT, *PQUEUE_CONTEXT;

#define HandleToQueueContext( QueueCtx ) \
   reinterpret_cast<PQUEUE_CONTEXT>(QueueCtx)

#define QueueContextToHandle( QueueCtx ) \
   reinterpret_cast<HANDLE>(QueueCtx)

#define IsValidQueueContextHandle( QueueCtx ) \
   ((BOOL)(NULL != (QueueCtx)))

typedef struct _QUEUE_RECORD
{
   /* Size of the record, in bytes */
   DWORD       Size;
   /* Length of the file, in characters, not including the terminating null character */
   DWORD       FileNameLength;
   /* Time the file was added to the queue */
   LONGLONG    LastModified;
   /* The file name. The FileName member is always null terminated in memory, but not
    * in a queue file */
   WCHAR       FileName[1];
}QUEUE_RECORD, *PQUEUE_RECORD;

typedef struct _NOTIFY_CONTEXT
{
   volatile ULONG          ReferenceCount;
#ifdef _WIN64
   ULONG                   Alignment;
#endif /* _WIN64 */
   PQUEUE_CONTEXT          QueueCtx;
   HANDLE                  NotificationEvent;
   PQUEUENOTIFICATIONPROC  NotificationCallback;
   PVOID                   NotificationParameter;
}NOTIFY_CONTEXT, *PNOTIFY_CONTEXT;

typedef struct _THREAD_CTX
{
   PVOID          Parameter;
   HMODULE        ModuleLock;
   volatile LONG  InitDoneEvent;
   DWORD          InitStatusCode;
}THREAD_CTX, *PTHREAD_CTX;

typedef struct _NOTIFY_THREAD_CTX : THREAD_CTX
{
   LONGLONG LastModified;
}NOTIFY_THREAD_CTX, *PNOTIFY_THREAD_CTX;

enum
{
   WORKTYPE_WRITERECORD,
   WORKTYPE_DELETERECORD,
   WORKTYPE_DELETEQUEUEFILE
};

typedef struct _WI_WRITERECORD
{
   WCHAR FileName[1];
}WI_WRITERECORD, *PWI_WRITERECORD;

typedef struct _WI_DELETERECORD
{
   WCHAR FileName[1];
}WI_DELETERECORD, *PWI_DELETERECORD;

typedef struct _WI_DELETEQUEUEFILE
{
   WCHAR FileName[1];
}WI_DELETEQUEUEFILE, *PWI_DELETEQUEUEFILE;

typedef struct _QUEUE_WORKITEM
{
   LIST_ENTRY              Entry;
   PQUEUE_CONTEXT          QueueCtx;
   ULONG                   WorkType;
   union
   {
      WI_WRITERECORD       WriteRecord;
      WI_DELETERECORD      DeleteRecord;
      WI_DELETEQUEUEFILE   DeleteQueueFile;
   };
}QUEUE_WORKITEM, *PQUEUE_WORKITEM;

#define HandleToNotificationContext( NotifyInfo ) \
   reinterpret_cast<PNOTIFY_CONTEXT>(NotifyInfo)

#define NotificationContextToHandle( NotifyInfo ) \
   reinterpret_cast<HANDLE>(NotifyInfo)

#define IsValidQueueNotificationContext( NotifyInfo ) \
   ((BOOL)(NULL != (NotifyInfo)))


enum
{
   /* Maximum length, in characters, of QUEUE_RECORD::FileName, not including the terminating null character */
   FQI_FILENAME_CCH_MAX = FQ_FILENAME_MAX_CCH,
   /* Maximum length, in bytes, of QUEUE_RECORD::FileName, not including the terminating null character */
   FQI_FILENAME_CB_MAX = FQI_FILENAME_CCH_MAX * sizeof(WCHAR),
   /* Minimum size for a QUEUE_RECORD structure. This includes the mandatory, terminating null character */
   FQI_QUEUE_RECORD_CB_MIN = RTL_SIZEOF_THROUGH_FIELD(QUEUE_RECORD, FileName),
   /* Maximum size for a QUEUE_RECORD structure. This includes the mandatory, terminating null character */
   FQI_QUEUE_RECORD_CB_MAX = FQI_QUEUE_RECORD_CB_MIN + FQI_FILENAME_CB_MAX,
   /* Minimum size of the memory block allocated for each QUEUE_RECORD */
   FQI_QUEUE_RECORD_CB_ALLOC = AlignUp<FQI_QUEUE_RECORD_CB_MIN + MAX_PATH, 16>::AlignedSize,

   /* Run states for NOTIFY_CONTEXT::RunState */
   FQI_RUNSTATE_REGISTERED    = 1,
   FQI_RUNSTATE_UNREGISTERED  = 2,

   /* Flags for FqiGetFileQueuePath */
   FQI_CREATE      = 0x00000001,
   FQI_DONOTCREATE = 0x00000002,

   FQI_TRACE_WARN  = 0,
   FQI_TRACE_ERROR = 1,
   FQI_TRACE_INFO  = 0,

   /* Timeout value for the worker thread */
#ifdef _DEBUG
   FQI_WORKQUEUE_TIMEOUT = (1 * 1000)
#else /* _DEBUG */
   FQI_WORKQUEUE_TIMEOUT = (10 * 1000)
#endif /* _DEBUG */
};

DWORD FqiInitializeWorkerThread( ) throw();
DWORD WINAPI FqiWorkerThreadRoutine( LPVOID pParam ) throw();
VOID FqiExecuteWorkItem( PQUEUE_WORKITEM pWorkItem ) throw();
DWORD FqiQueueWorkItem( PQUEUE_WORKITEM pWorkItem ) throw();
PQUEUE_WORKITEM FqiBuildWorkItem( PQUEUE_CONTEXT pQueueCtx, ULONG eWorkType, LPCWSTR pwszFileName ) throw();
VOID FqiDestroyWorkItem( PQUEUE_WORKITEM pWorkItem ) throw();

/**
 * FqiInitializeFileQueue
 *    Ensures the file queue directory exists
 */
DWORD FqiInitializeFileQueue( PQUEUE_CONTEXT pQueueCtx ) throw();

/**
 * FqiStartQueueNotificationThread / FqiShutdownQueueNotificationThread
 */
DWORD FqiStartQueueNotificationThread( PQUEUE_CONTEXT pQueueCtx, PQUEUENOTIFICATIONPROC pCallback, PVOID pParameter, PLONGLONG pLastModified, PNOTIFY_CONTEXT* ppNotifyCtx ) throw();
VOID FqiShutdownQueueNotificationThread( PNOTIFY_CONTEXT pNotifyCtx ) throw();

/**
 * FqiGetFileQueuePath
 *    Retrieves the fully qualified path of the file queue, for the user represented
 *    by the specified token
 */
DWORD FqiGetFileQueuePath( PQUEUE_CONTEXT pQueueCtx, DWORD dwFlags, LPWSTR pwszBuf, size_t cchBuf ) throw();

/**
 * FqiCreateQueueFileName
 *    Creates a unique file name for the specified 
 */
DWORD FqiGetQueueNameForFileName( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName, LPWSTR pwszBuf, size_t cchBuf ) throw();

/**
 * FqiCreateFileNameHash
 */
ULONG FqiCreateFileNameHash( LPCWSTR pwszFileName ) throw();

/**
 * FqiWriteFileQueueFile
 */
DWORD FqiWriteFileQueueFile( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName, PLONGLONG pLastModified ) throw();

/**
 * FqiDeleteFileQueueFile
 */
DWORD FqiDeleteFileQueueFile( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName ) throw();

/**
 * FqiDeleteQueueFile
 */
DWORD FqiDeleteQueueFile( PQUEUE_CONTEXT pQueueCtx, LPCWSTR pwszFileName ) throw();

/**
 * FqiReadFileQueueFileRecord
 */
DWORD FqiReadNextFileQueueFileRecord( HANDLE hFile, PQUEUE_RECORD* ppQueRecord ) throw();

/**
 * FqiWriteFileQueueFileRecord
 */
DWORD FqiWriteNextFileQueueFileRecord( HANDLE hFile, PQUEUE_RECORD pQueRecord ) throw();

/**
 * FqiEnumerateFileQueue
 */
DWORD FqiEnumerateFileQueue( __in PQUEUE_CONTEXT pQueueCtx, __in PENUMQUEUEPROC pEnumQueueProc, __in_opt PVOID pParameter, __in_opt PLONGLONG pLastModified ) throw();

/**
 *FqiEnumerateFileQueueFileRecords
 */
DWORD FqiEnumerateFileQueueFile( __in PQUEUE_CONTEXT pQueueCtx, __in_z LPCWSTR pwszFile, __in PENUMQUEUEPROC pEnumQueueProc, __in_opt PVOID pParameter, __in_opt PLONGLONG pLastModified ) throw();

/**
 * FqiAlloc/FqiFree
 *    Memory allocation/deallocation routines used by this library
 */
LPVOID FqiAlloc( size_t cbAlloc ) throw();
VOID FqiFree( LPVOID pMem ) throw();
#define FqiIsValidMemoryPointer _CrtIsValidHeapPointer

/**
 * FqiAllocQueueFileRecord/FqiBuildQueueFileRecord/FqiFreeQueueFileRecord
 */
PQUEUE_RECORD FqiAllocQueueFileRecord( DWORD cbSize ) throw();
PQUEUE_RECORD FqiBuildQueueFileRecord( LPCWSTR pwszFileName ) throw();
VOID FqiFreeQueueFileRecord( PQUEUE_RECORD pQueRecord ) throw();

/**
 * FqiCreateQueuempersonationToken
 *    Helper function for creating the impersonation token used by the QUEUE_CONTEXT 
 */
DWORD FqiCreateQueuempersonationToken( HANDLE hThread, PHANDLE phToken ) throw();

/**
 * FqiNotificationThreadRoutine / FqiNotificationEnumQueueProc
 *    Thread procedure and callback for monitoring changes to a file queue
 */
DWORD WINAPI FqiNotificationThreadRoutine( LPVOID pParam ) throw();
BOOL QUEUEAPI FqiNotificationThreadEnumQueueRoutine( __in HANDLE hQueueCtx, __in_z LPCWSTR pwszFileName, __in PLONGLONG pLastModified, __in_opt PVOID pParameter ) throw();

/**
 *
 */
VOID FqiValidateQueueFile( HANDLE hFile ) throw();

__inline ULONG FqiReferenceQueueContext( PQUEUE_CONTEXT pQueueCtx ) throw()
{
   ULONG iRef;

   _ASSERTE(FqiIsValidMemoryPointer(pQueueCtx));

   iRef = static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(&(pQueueCtx->ReferenceCount))));
   /* Another thread has already dropped the reference count to 0. The code that creates the QUEUE_CONTEXT 
    * starts with an initial reference count of 1, so that calling code should _NEVER_ end up trying to 
    * reuse a context after the last reference has been dropped */
   _ASSERTE(1 != iRef);
   return ( iRef );
}

__inline ULONG FqiReleaseQueueContext( PQUEUE_CONTEXT pQueueCtx ) throw()
{
   ULONG iRef;

   _ASSERTE(FqiIsValidMemoryPointer(pQueueCtx));

   iRef = static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<volatile LONG*>(&(pQueueCtx->ReferenceCount))));
   if ( 0 == iRef )
   {
      CloseHandle(pQueueCtx->ImpersonationToken);
      FqiFree(pQueueCtx);
   }

   return ( iRef );
}

__inline size_t FqiCalculateQueueRecordFileNameLength( size_t RecordSize ) throw()
{
   return ( ((RecordSize - FQI_QUEUE_RECORD_CB_MIN) / sizeof(WCHAR)) );
}

__inline size_t FqiCalculateQueueRecordFileNameSize( size_t RecordSize ) throw()
{
   return ( RecordSize - FQI_QUEUE_RECORD_CB_MIN );
}

__inline ULONG FqiReferenceNotificationContext( PNOTIFY_CONTEXT pNotifyCtx ) throw()
{
   ULONG iRef;

   _ASSERTE(FqiIsValidMemoryPointer(pNotifyCtx));

   iRef = static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<volatile LONG*>(&(pNotifyCtx->ReferenceCount))));
   _ASSERTE(1 != iRef);
   return ( iRef );
}

__inline ULONG FqiReleaseNotificationContext( PNOTIFY_CONTEXT pNotifyCtx ) throw()
{
   ULONG iRef;

   _ASSERTE(FqiIsValidMemoryPointer(pNotifyCtx));

   iRef = static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<volatile LONG*>(&(pNotifyCtx->ReferenceCount))));
   if ( 0 == iRef )
   {
      CloseHandle(pNotifyCtx->NotificationEvent);
      FqiReleaseQueueContext(pNotifyCtx->QueueCtx);
      FqiFree(pNotifyCtx);
   }

   return ( iRef );
}

/**
 * Utility stuff
 **/
__inline size_t FqiRoundDown( size_t cbValue, size_t cbAlign ) throw()
{
   return ( cbValue & ~(cbAlign - 1) );
}

__inline size_t FqiRoundUp( size_t cbValue, size_t cbAlign ) throw()
{
   return ( (cbValue + (cbAlign - 1)) & ~(cbAlign - 1) );
}

__inline BOOL FqiFlagOn( DWORD dwFlags, DWORD dwMask ) throw()
{
   return ( 0 != (dwFlags & dwMask) ? TRUE : FALSE );
}

__inline BOOL FqiFlagOff( DWORD dwFlags, DWORD dwMask ) throw()
{
   return ( 0 == (dwFlags & dwMask) ? TRUE : FALSE );
}

__inline BOOL FqiIsDotDirectory( LPCWSTR pwszFile ) throw()
{
   if ( ((L'.' == pwszFile[0]) && (L'\0' == pwszFile[1])) )
   {
      return ( TRUE );
   }
   else if ( ((L'.' == pwszFile[0]) && (L'.'  == pwszFile[1])  && (L'\0' == pwszFile[2])) )
   {
      return ( TRUE );
   }

   return ( FALSE );
}