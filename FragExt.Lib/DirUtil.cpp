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

/* DirUtil.cpp
 *    Directory utility functions
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include "FragLibp.h"

#include <DirUtil.h>

#include <List.h>
#include <NTApi.h>
#include <SpinLock.h>
#include <StreamUtil.h>
#include <ThreadUtil.h>

#include "CacheList.h"

extern "C"
{

char
_InterlockedOr8(
   char volatile* Destination,
   char Value
);
#pragma intrinsic(_InterlockedOr8)

}; /* extern "C" */


/*++
   WPP
 *--*/
#include "DirUtil.tmh"

/* Worker thread execution flags.. */
#define FPI_STATE_ABORT                0x01
#define FPI_STATE_CLEANUP              0x02

#define FPI_PRIMARYTHREADWAITTIMEOUT   250
#define FPI_WORKERTHREADTHRESHOLD      2024
#define FPI_WORKERTHREADMAX            4


#define FPI_DIRECTORYPATH_MAX_CCH      (2 * (MAX_PATH + 36))
#define FPI_DIRECTORYPATH_MAX_CB       (FPI_DIRECTORYPATH_MAX_CCH * sizeof(WCHAR))

#define FPI_DIRECTORYINFOBUFFER_CB     (sizeof(FILE_DIRECTORY_INFORMATION) + FPI_DIRECTORYPATH_MAX_CB)

#define FPI_DIRECTORYRECORD_CB         544
#define FPI_DIRECTORYRECORDCACHE_CB    (32 * 1024 * 1024)

typedef struct _DIRECTORY_RECORD
{
   union 
   {
      /* 
       * Entry for ENUMDIRECTORY_CTX::DirectoryQueue/SubDirectoryQueue
       */
      LIST_ENTRY        Entry;

      /*
       * Entry placeholder for ENUMDIRECTORY_CTX::DirectoryRecordCache. This 
       * isn't used directly, but the CACHELIST requires space for it, so it
       * is explicitly declared here
       */
      CACHELIST_ENTRY   Reserved;
   };

   /* 
    * Zero based level from the root enumeration directory, used to limit traversal 
    */
   USHORT      Depth;

   /*
    * Length of the directory path, in characters
    */
   USHORT      DirectoryPathLength;
   /*
    * Null-terminated directory path
    */
   WCHAR       DirectoryPath[1];
}DIRECTORY_RECORD, *PDIRECTORY_RECORD;
typedef const struct _DIRECTORY_RECORD* PCDIRECTORY_RECORD;

typedef struct _ENUMDIRECTORY_CTX
{
   /*
    * Caller parameters 
    */
   PENUMDIRECTORYROUTINE   Callback;
   PVOID                   CallbackParameter;   
   DWORD                   Flags;
   
   /* 
    * Current number of directories on the list for determining when to
    * spawn a worker thread if enabled, and for debugging.
    */
   USHORT                  DirectoryCount;

   /* 
    * Enumeration state flags 
    */
   volatile UCHAR          State;
   UCHAR                   Reserved;

   /*
    * Cache-list for standard sized DIRECTORY_RECORD entries
    */
   HANDLE                  DirectoryRecordCache;
   
   /*
    * LIFO queue of directories to be processed 
    */
   LIST_ENTRY              DirectoryQueue;

   union
   {
      LIST_ENTRY           SubDirectoryQueue;

      struct
      {
         /* 
          * Lock for updating DirectoryQueue and DirectoryCount when using
          * worker threads
          */
         volatile SPINLOCK DirectoryLock;

         /* 
          * Manual reset event that is signaled when there are no worker
          * threads enumerating
          */
         volatile HANDLE   WorkerExit;

         /* 
          * Lock for setting/resetting WorkerExit and updating WorkerCount 
          */
         volatile SPINLOCK WorkerLock;

         /* 
          * Current number of active worker threads. When this reaches
          * zero, WorkerExit will be signaled 
          */
         volatile LONG     WorkerCount;
      };
   };
}ENUMDIRECTORY_CTX, *PENUMDIRECTORY_CTX;

typedef const struct _ENUMDIRECTORY_CTX* PCENUMDIRECTORY_CTX;

typedef struct _ENUMSTREAM_CTX
{
   /*
    * Active directory enumeration context
    */
   PENUMDIRECTORY_CTX            EnumCtx;

   /*
    * Current file/stream entry record. This will be modified by stream enumeration
    */
   PDIRECTORYENTRY_INFORMATION   EntryInformation;

   /*
    * Current directory being processed
    */
   PCDIRECTORY_RECORD            Record;
}ENUMSTREAM_CTX, *PENUMSTREAM_CTX;
typedef const struct _ENUMSTREAM_CTX* PCENUMSTREAM_CTX;

/*++
   General Helpers
 --*/

inline
USHORT 
DecodeMaxTraversalDepth( 
   __in DWORD Flags
)
{
   return ( static_cast<USHORT>((Flags & EDF_TRAVERSEDEPTHCOUNTMASK) >> EDF_TRAVERSEDEPTHCOUNTSHIFT) );
}

inline
void
CopyLargeIntegerToFileTime(
   __in PLARGE_INTEGER Time,
   __out FILETIME* FileTime
)
{
   FileTime->dwLowDateTime  = Time->LowPart;
   FileTime->dwHighDateTime = Time->HighPart;
}

inline
BOOLEAN
IsEnumerationError(
   __in DWORD ErrorCode
)
{
   return ( (ErrorCode != NO_ERROR)             &&
            (ErrorCode != ERROR_FILE_NOT_FOUND) &&
            (ErrorCode != ERROR_ACCESS_DENIED)  &&
            (ErrorCode != ERROR_NO_MORE_FILES)  &&
            (ErrorCode != ERROR_CANCELLED) );
}

/*++
   ENUMDIRECTORY_CTX Support
 --*/
 
DWORD
InitializeEnumDirectoryContext( 
   __inout PENUMDIRECTORY_CTX EnumCtx, 
   __in DWORD Flags, 
   __in PENUMDIRECTORYROUTINE Callback, 
   __in PVOID CallbackParameter
);

void
CleanupEnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx
);

DWORD
InitializeNextFreeListRegion(
   __inout PENUMDIRECTORY_CTX EnumCtx
);

DWORD 
EnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx 
);

/*++
   DIRECTORY_RECORD Support
 --*/
__declspec(restrict) __checkReturn
PDIRECTORY_RECORD 
CreateDirectoryRecord( 
   __in PENUMDIRECTORY_CTX EnumCtx,
   __in_z_opt LPCWSTR DirectoryPath,
   __in USHORT Depth
);

void
FreeDirectoryRecord(
   __in PENUMDIRECTORY_CTX EnumCtx,
   __in PDIRECTORY_RECORD DirectoryRecord
);

inline
void 
PushEnumDirectoryRecord( 
   __in PENUMDIRECTORY_CTX EnumCtx, 
   __in PDIRECTORY_RECORD Record
)
{
   if ( FlagOn(EnumCtx->Flags, EDF_THREADEDENUMERATION) )
   {
      AcquireSpinLock(&(EnumCtx->DirectoryLock));
      {
         InsertTailList(&(EnumCtx->DirectoryQueue),
                        &(Record->Entry));

         EnumCtx->DirectoryCount += 1;
      }
      ReleaseSpinLock(&(EnumCtx->DirectoryLock));
   }
   else
   {
      /* New directories are only added to SubDirectoryQueue */
      InsertTailList(&(EnumCtx->SubDirectoryQueue),
                     &(Record->Entry));

      EnumCtx->DirectoryCount += 1;
   }
}

inline
void
MergeEnumDirectoryRecordQueues(
   __inout PENUMDIRECTORY_CTX EnumCtx
)
{
   _ASSERTE(!FlagOn(EnumCtx->Flags, EDF_THREADEDENUMERATION));

   /* This method merges the DirectoryQueue and SubDirectoryQueue lists
    * by inserting SubDirectoryQueue in front of DirectoryQueue. This 
    * keeps DirectoryQueue sorted so that we always take the next 
    * directory in the original hiearchy. For example..
    *
    * Caller requests enumeration of C:\
    * 
    * First enum of C:\ fills out SubDirectoryQueue like..
    *    C:\A, C:\B
    * Next the lists are merged, and DirectoryQueue ends up as..
    *    C:\A, C:\B
    * Next enum picks up C:\A and fills out SubDirectoryQueue as..
    *    C:\A.1, C:\A.2, C:\A.3
    * Lists are merged, and DirectoryQueue ends up as..
    *    C:\A.1, C:\A.2, C:\A.3, C:\B 
    * Next enum picks up C:\A.1 and fills out SubDirectoryQueue as..
    *    C:\A.1.1, C:\A.1.2, C:\A.1.3
    * List are merged, and DirectoryQueue ends up as..
    *    C:\A.1.1, C:\A.1.2, C:\A.1.3, C:\A.2, C:\A.3, C:\B 
    *
    * So as sub-directories are enumerated, their order is preserved and
    * the next sub-directory processed will be the first one found in the
    * directory currently being processed.
    */
   if ( !IsListEmpty(&(EnumCtx->SubDirectoryQueue)) )
   {
      MergeHeadList(&(EnumCtx->DirectoryQueue),
                    FlushList(&(EnumCtx->SubDirectoryQueue)));
   }
}

inline
PDIRECTORY_RECORD 
PopEnumDirectoryRecord( 
   __in PENUMDIRECTORY_CTX EnumCtx 
)
{
   PDIRECTORY_RECORD  pRecord;

   pRecord = NULL;

   if ( FlagOn(EnumCtx->Flags, EDF_THREADEDENUMERATION) )
   {
      AcquireSpinLock(&(EnumCtx->DirectoryLock));
      {
         if ( !IsListEmpty(&(EnumCtx->DirectoryQueue)) )
         {
            pRecord = CONTAINING_RECORD(RemoveHeadList(&(EnumCtx->DirectoryQueue)),
                                        DIRECTORY_RECORD,
                                        Entry);

            EnumCtx->DirectoryCount -= 1;
         }
      }
      ReleaseSpinLock(&(EnumCtx->DirectoryLock));
   }
   else
   {
      /* Directories are removed from DirectoryQueue. The only way they get here is
       * when FpEnumDirectoryInformation adds the first one, or the lists are merged 
       * via MergeEnumDirectoryRecordQueues() */
      if ( !IsListEmpty(&(EnumCtx->DirectoryQueue)) )
      {
         pRecord = CONTAINING_RECORD(RemoveHeadList(&(EnumCtx->DirectoryQueue)),
                                     DIRECTORY_RECORD,
                                     Entry);

         EnumCtx->DirectoryCount -= 1;
      }
   }

   return ( pRecord );
}

DWORD 
FindFilesDirectoryRecord( 
   __in PENUMDIRECTORY_CTX EnumCtx, 
   __in PDIRECTORY_RECORD Record 
);

BOOLEAN
ProcessEntryClientCallback(
   __in PENUMDIRECTORY_CTX EnumCtx,
   __inout PDIRECTORYENTRY_INFORMATION EntryInformation,
   __in PCDIRECTORY_RECORD Record
);

/*++
   Worker Thread Support
 --*/
DWORD 
CreateEnumDirectoryWorker( 
   __in PENUMDIRECTORY_CTX pEnumCtx 
);

DWORD 
WINAPI
EnumDirectoryThreadRoutine( 
   __in LPVOID pParameter 
);

void
ReleaseEnumDirectoryWorker( 
   PENUMDIRECTORY_CTX pEnumCtx 
);

inline 
BOOLEAN 
IsEnumDirectoryWorkerRequired( 
   __in PCENUMDIRECTORY_CTX EnumCtx 
)
{
   if ( FlagOn(EnumCtx->Flags, EDF_THREADEDENUMERATION) )
   {
      if ( (EnumCtx->DirectoryCount > ((EnumCtx->WorkerCount + 1) * FPI_WORKERTHREADTHRESHOLD)) )
      {
         if ( (EnumCtx->WorkerCount < FPI_WORKERTHREADMAX) )
         {
            return ( TRUE );
         }
      }
   }

   return ( FALSE );
}


/*++
   NTFS ADS Support
 --*/

BOOLEAN
APIENTRY
EnumFileStreamRoutine( 
   __in ULONG StreamCount, 
   __in ULONGLONG StreamSize, 
   __in ULONGLONG StreamSizeOnDisk,
   __in_z LPCWSTR StreamName, 
   __in_opt PVOID CallbackParameter
);

/*++
   File Information Support
 --*/

DWORD
OpenDirectoryFile(
   __in_z LPCWSTR DirectoryPath,
   __out PHANDLE DirectoryHandle
);

DWORD
QueryDirectoryAttributeInformation(
   __in_z LPCWSTR DirectoryPath,
   __out_bcount(InformationLength) PDIRECTORYENTRY_INFORMATION DirectoryInformation,
   __in ULONG InformationLength
);

DWORD
QueryDirectoryFileInformation(
   __in HANDLE FileHandle,
   __in FILE_INFORMATION_CLASS FileInformationClass,
   __out_bcount(Length) PVOID FileInformation,
   __in ULONG Length,
   __out_opt PULONG ReturnLength,
   __in BOOLEAN ReturnSingleEntry,
   __in_opt LPCWSTR FileName,
   __in BOOLEAN RestartScan
);

/*++

   PUBLIC EXPORTS
   
--*/

DWORD 
APIENTRY
FpEnumDirectoryInformation( 
   __in_z LPCWSTR DirectoryPath, 
   __in DWORD Flags, 
   __in PENUMDIRECTORYROUTINE Callback, 
   __in_opt PVOID CallbackParameter 
)
{
   DWORD             dwRet;

   ENUMDIRECTORY_CTX EnumCtx;

   PDIRECTORY_RECORD pDirectory;
   
   DWORD             dwWait;
   
   _ASSERTE(NULL != DirectoryPath);
   _ASSERTE(NULL != Callback);

   /* Initialize locals */
   dwRet      = NO_ERROR;
   pDirectory = NULL;
   
   InitializeMemory(&EnumCtx,
                    sizeof(EnumCtx));

   /* 
    * Setup the enum context. This will be shared by any enumeration thread, including this one
    */
   dwRet = InitializeEnumDirectoryContext(&EnumCtx,
                                          Flags,
                                          Callback,
                                          CallbackParameter);

   if ( WINERROR(dwRet) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpDirectory,
              L"InitializeEnumDirectoryContext failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   /* 
    * Build the initial directory record for the list.
    */
   pDirectory = CreateDirectoryRecord(&EnumCtx,
                                      DirectoryPath,
                                      0);

   if ( !pDirectory )
   {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;

      FpTrace(TRACE_LEVEL_WARNING,
              FpDirectory,
              L"CreateDirectoryRecord failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   /* 
    * Beyond this point we have resources to cleanup 
    */
   __try
   {
      /* 
       * Put the initial directory on the list. As this is the first, we do it manually to
       * avoid the merging code 
       */
      InsertHeadList(&(EnumCtx.DirectoryQueue),
                     &(pDirectory->Entry));

      EnumCtx.DirectoryCount = 1;

      FpTrace(TRACE_LEVEL_VERBOSE,
              FpDirectory,
              L"EnumCtx initialized, starting enumeration. DirectoryPath %ws, Flags = %08lx, Callback = %p, CallbackParameter = %p\n",
              DirectoryPath,
              Flags,
              Callback,
              CallbackParameter);

      for ( ;; )
      {
         
         FpTrace(TRACE_LEVEL_VERBOSE,
                 FpDirectory,
                 L"Resuming enumeration. EnumCtx.State ~= %04lx, EnumCtx.DirectoryCount ~= %u\n",
                 EnumCtx.State,
                 static_cast<ULONG>(EnumCtx.DirectoryCount));
         /*
          * If we're not in an abort state, we'll enum the context. We may be here while in
          * an abort state and waiting for workers to exit. In that case, we just continue
          * waiting for all workers to exit
          */
         if ( !FlagOn(EnumCtx.State, FPI_STATE_ABORT) )
         {
            /*
             * Kick off the scan. As this runs, new threads may fire up to handle the
             * workload, if the option is enabled
             */
            dwRet = EnumDirectoryContext(&EnumCtx);
         }

         /* 
          * If there are more directories to process, and we haven't bailed on an error
          */

         if ( !FlagOn(EnumCtx.Flags, EDF_THREADEDENUMERATION) )
         {
            FpTrace(TRACE_LEVEL_VERBOSE,
                    FpDirectory,
                    L"Non-threaded enumeration, complete. Exiting\n");

            /* 
             * This is the only thread running, so the queue is either empty or we bailed
             * for an abort or error
             */
            break;
         }

         FpTrace(TRACE_LEVEL_VERBOSE,
                 FpDirectory,
                 L"Waiting for worker thread completion. EnumCtx.WorkerCount ~= %u\n",
                 EnumCtx.WorkerCount);

         /*
          * Worker threads are potentialy running, so wait for them to finish
          */
         dwWait = WaitForSingleObject(EnumCtx.WorkerExit,
                                      FPI_PRIMARYTHREADWAITTIMEOUT);

         /* 
          * If the handle was signaled, or an error occured, we bail 
          */
         if ( WAIT_TIMEOUT != dwWait )
         {
            _InterlockedOr8(reinterpret_cast<volatile char*>(&(EnumCtx.State)),
                            FPI_STATE_ABORT);

            if ( WAIT_OBJECT_0 != dwWait )
            {
               dwRet = GetLastError();
               
               FpTrace(TRACE_LEVEL_WARNING,
                       FpDirectory,
                       L"WaitForSingleObject failed, dwWait = %08lx, dwRet = %!WINERROR!\n",
                       dwWait,
                       dwRet);
            }

            
            /* Exiting while workers still running? */
            _ASSERTE(0 == EnumCtx.WorkerCount);

            /*
             * Give a slight pause to allow all worker threads to finish their cleanup too
             */
            Sleep(0);

            break;
         }

         /* 
          * We waited for a bit and some workers must still be running so we'll give
          * them a hand 
          */
      }
   }
   __finally
   {
      CleanupEnumDirectoryContext(&EnumCtx);
   }

   /* Success / Failure */
   return ( dwRet );
}

/*++

   PRIVATE

 --*/

/*++
   ENUMDIRECTORY_CTX
 --*/

DWORD 
InitializeEnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx,
   __in DWORD Flags, 
   __in PENUMDIRECTORYROUTINE Callback, 
   __in PVOID CallbackParameter 
)
{
   DWORD    dwRet;

   HANDLE   hWorkerExit;
   HANDLE   hRecordCache;
   
   ULONG    CacheListFlags;

   
   /* Initialize locals */
   dwRet        = NO_ERROR;
   hWorkerExit  = NULL;
   hRecordCache = NULL;

   __try
   {
      InitializeListHead(&(EnumCtx->DirectoryQueue)); 

      if ( FlagOn(Flags, EDF_THREADEDENUMERATION) )
      {
         InitializeSpinLock(&(EnumCtx->DirectoryLock),
                            4000);

         hWorkerExit = CreateEvent(NULL,
                                   FALSE,
                                   TRUE,
                                   NULL);

         if ( !hWorkerExit )
         {
            dwRet = GetLastError();

            FpTrace(TRACE_LEVEL_WARNING,
                    FpDirectory,
                    L"CreateEvent failed, EnumCtx.WorkerExit, dwRet = %!WINERROR!\n",
                    dwRet);

            /* Failure */
            __leave;
         }

         InitializeSpinLock(&(EnumCtx->WorkerLock),
                            4000);

         EnumCtx->WorkerExit  = hWorkerExit;
         EnumCtx->WorkerCount = 0;

         CacheListFlags = 0;
      }
      else
      {
         InitializeListHead(&(EnumCtx->SubDirectoryQueue));

         /*
          * There's only one thread, so no use using a locking cache list
          */
         CacheListFlags = CACHELIST_NOLOCKLIST;
      }
      
      /*
       * Initialize the DIRECTORY_RECORD cache
       */
      dwRet = CreateCacheList(CacheListFlags,
                              FPI_DIRECTORYRECORDCACHE_CB,
                              FPI_DIRECTORYRECORD_CB,
                              &hRecordCache);

      if ( WINERROR(dwRet) )
      {
         /* Failure */
         __leave;
      }
      
      EnumCtx->Flags                = Flags;
      EnumCtx->Callback             = Callback;
      EnumCtx->CallbackParameter    = CallbackParameter;   
      
      EnumCtx->DirectoryCount       = 0;
      EnumCtx->State                = 0;

      EnumCtx->DirectoryRecordCache = hRecordCache;
      
      /*
       * Clear local aliases so they're not lost in cleanup
       */
      hRecordCache = NULL;
      hWorkerExit  = NULL;
   }
   __finally
   {
      if ( hRecordCache )
      {
         DestroyCacheList(hRecordCache);
      }

      if ( hWorkerExit )
      {
         CloseHandle(hWorkerExit);
      }
   }
   
   /* Success / Failure */
   return ( dwRet );
}

void 
CleanupEnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx
)
{
   PLIST_ENTRY           Entry;

   CACHELIST_STATISTICS CacheListStatistics;      

   /*
    * Set the cleanup state flag
    */
   _InterlockedOr8(reinterpret_cast<volatile char*>(&(EnumCtx->State)),
                   FPI_STATE_CLEANUP);

   while ( !IsListEmpty(&(EnumCtx->DirectoryQueue)) )
   {
      Entry = RemoveHeadList(&(EnumCtx->DirectoryQueue));

      FreeDirectoryRecord(EnumCtx,
                          CONTAINING_RECORD(Entry,
                                            DIRECTORY_RECORD,
                                            Entry));
   }

   if ( !FlagOn(EnumCtx->Flags, EDF_THREADEDENUMERATION) )
   {
      while ( !IsListEmpty(&(EnumCtx->SubDirectoryQueue)) )
      {
         Entry = RemoveHeadList(&(EnumCtx->SubDirectoryQueue));

         FreeDirectoryRecord(EnumCtx,
                             CONTAINING_RECORD(Entry,
                                               DIRECTORY_RECORD,
                                               Entry));
      }
   }
   else
   {
      if ( EnumCtx->WorkerExit )
      {
         CloseHandle(EnumCtx->WorkerExit);
      }
   }

   if ( WPP_LEVEL_FLAGS_ENABLED(TRACE_LEVEL_VERBOSE, FpCacheList) )
   {
      QueryCacheListStatistics(EnumCtx->DirectoryRecordCache,
                               &CacheListStatistics);

      FpTrace(TRACE_LEVEL_VERBOSE,
              FpCacheList,
              L"EnumCtx->DirectoryRecordCache:CACHELIST statistics, SizeOfCache = %u, SizeOfRegion = %u, SizeOfRecord = %u, MaximumRegions = %u, RegionsCommitted = %u, MaximumRecords = %u, RecordsCommitted = %u, RecordsAvailable = %u\n",
              CacheListStatistics.SizeOfCache,
              CacheListStatistics.SizeOfRegion,
              CacheListStatistics.SizeOfRecord,
              CacheListStatistics.MaximumRegions,
              CacheListStatistics.RegionsCommitted,
              CacheListStatistics.MaximumRecords,
              CacheListStatistics.RecordsCommitted,
              CacheListStatistics.RecordsAvailable);
   }
#if _DEBUG
   else
   {
      QueryCacheListStatistics(EnumCtx->DirectoryRecordCache,
                               &CacheListStatistics);
   }

   DbgPrintfExW(0,
                L"EnumCtx Statistics:\n"
                L"\tDirectoryRecordCache:\n"
                L"\t CacheList statistics:\n"
                L"\t - Cache reserved    = %u (%.2fKB)\n"
                L"\t - Cache committed   = %u (%.2fKB)\n"
                L"\t - Region size       = %u (%.2fKB)\n"
                L"\t - Record size       = %u\n"
                L"\t - Maximum regions   = %u\n"
                L"\t - Committed regions = %u\n"
                L"\t - Maximum records   = %u\n"
                L"\t - Committed records = %u\n"
                L"\t - Available records = %u\n",
                CacheListStatistics.SizeOfCache, 
                (static_cast<float>(CacheListStatistics.SizeOfCache) / 1024),
                CacheListStatistics.SizeOfRegion * CacheListStatistics.RegionsCommitted, 
                (static_cast<float>(CacheListStatistics.SizeOfRegion * CacheListStatistics.RegionsCommitted) / 1024),
                CacheListStatistics.SizeOfRegion, 
                (static_cast<float>(CacheListStatistics.SizeOfRegion) / 1024),
                CacheListStatistics.SizeOfRecord,
                CacheListStatistics.MaximumRegions,
                CacheListStatistics.RegionsCommitted,
                CacheListStatistics.MaximumRecords,
                CacheListStatistics.RecordsCommitted,
                CacheListStatistics.RecordsAvailable);
#endif /* _DEBUG */

   DestroyCacheList(EnumCtx->DirectoryRecordCache);
}

DWORD 
EnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx 
)
{
   DWORD             dwRet;
   PDIRECTORY_RECORD pRecord;

   dwRet   = NO_ERROR;
   pRecord = NULL;

   /*
    * While there are directories on the queue, pop one off and scan it
    */
   for ( ;; )
   {
      if ( FlagOn(EnumCtx->State, FPI_STATE_ABORT) )
      {
         break;
      }
      
      pRecord = PopEnumDirectoryRecord(EnumCtx);
      if ( !pRecord )
      {
         break;
      }

      __try
      {
         dwRet = FindFilesDirectoryRecord(EnumCtx,
                                          pRecord);
      }
      __finally
      {
         FreeDirectoryRecord(EnumCtx,
                             pRecord);

         pRecord = NULL;
      }

      if ( IsEnumerationError(dwRet) )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"FindFilesDirectoryRecord failed, dwRet = %!WINERROR!, exiting\n",
                 dwRet);

         /* Failure */
         break; 
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

/*++
   DIRECTORY_RECORD
 --*/

__declspec(restrict)
PDIRECTORY_RECORD 
CreateDirectoryRecord(
   __in PENUMDIRECTORY_CTX EnumCtx,
   __in_z_opt LPCWSTR DirectoryPath,
   __in USHORT Depth
)
{
   HRESULT           hr;
   
   size_t            cbLength;
   size_t            cchLength;

   size_t            cbAllocate;
   
   PVOID             pEntry;
   PDIRECTORY_RECORD pRecord;

   /* Determine the length of the path */
   hr = StringCbLengthW(DirectoryPath,
                        FPI_DIRECTORYPATH_MAX_CB,
                        &cbLength);

   if ( FAILED(hr) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpDirectory,
              L"StringCbLengthW(DirectoryPath) failed, hr = %!HRESULT!\n, exiting",
              hr);

      /* Failure */
      return ( NULL );
   }
   
   cchLength = (cbLength / sizeof(WCHAR));

   /* 
    * The DIRECTORY_RECORD includes the terminating null char 
    */
   cbAllocate = (cbLength + sizeof(DIRECTORY_RECORD));

   /* 
    * If the path doesn't already end in a \, add room for one 
    */
   if ( L'\\' != DirectoryPath[cchLength - 1] )
   {
      cbAllocate += sizeof(WCHAR);
   }

   /*
    * If a common record can hold this directory, and there is a free one, use it
    */
   pRecord = NULL;

   if ( cbAllocate <= FPI_DIRECTORYRECORD_CB )
   {
      pEntry = PopCacheListEntry(EnumCtx->DirectoryRecordCache);
      if ( pEntry )
      {
         cbAllocate = FPI_DIRECTORYRECORD_CB;
         pRecord    = reinterpret_cast<PDIRECTORY_RECORD>(pEntry);
      }
   }

   if ( !pRecord )
   {
      /*
       * Either there were no available common records or this directory is larger
       * than a common one. Either way, we'll fall back to the heap and try to
       * allocate one there
       */
      cbAllocate = AlignUp(cbAllocate,
                           MEMORY_ALLOCATION_ALIGNMENT);
      
      pRecord = reinterpret_cast<PDIRECTORY_RECORD>(malloc(cbAllocate));
      if ( !pRecord )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"malloc failed, cbAllocate = %I64u, exiting\n",
                 cbAllocate);

         /* Failure */
         return ( NULL );
      }
   }

   InitializeMemory(pRecord,
                    cbAllocate);

   pRecord->Depth = Depth;

   /* 
    * Copy the directory path, append the trailing \ and the null terminator 
    */
   CopyMemory(&(pRecord->DirectoryPath[0]),
              DirectoryPath,
              cbLength);

   if ( L'\\' != pRecord->DirectoryPath[cchLength - 1] )
   {
      pRecord->DirectoryPath[cchLength] = L'\\';

      cchLength += 1;
   }

   pRecord->DirectoryPath[cchLength] = L'\0';

   /* Success / Failure */
   return ( pRecord );
}

void
FreeDirectoryRecord(
   __in PENUMDIRECTORY_CTX EnumCtx,
   __in PDIRECTORY_RECORD DirectoryRecord
)
{
   /*
    * If this record was allocated out of the DIRECTORY_RECORD cache, put it back onto it
    */
   if ( IsCacheListEntry(EnumCtx->DirectoryRecordCache,
                         DirectoryRecord) )
   {
      /*
       * If we're in the cleanup path, don't bother adding entries back to the free list
       * because we're just going to destroy the cache anyway
       */
      if ( !FlagOn(EnumCtx->State, FPI_STATE_CLEANUP) )
      {
         PushCacheListEntry(EnumCtx->DirectoryRecordCache,
                            DirectoryRecord);
      }
   }
   else 
   {
      free(DirectoryRecord);
   }
}

BOOLEAN
ProcessEntryClientCallback(
   __in PENUMDIRECTORY_CTX EnumCtx,
   __inout PDIRECTORYENTRY_INFORMATION EntryInformation,
   __in PCDIRECTORY_RECORD Record
)
{
   DWORD          dwRet;

   BOOLEAN        bIsDirectory;
   ENUMSTREAM_CTX EnumStreamCtx;

   USHORT         cchFilePathTail;

   if ( FlagOn(EnumCtx->State, FPI_STATE_ABORT) )
   {
      /* Abort */
      return ( FALSE );
   }

   /* 
    * Pass this one on to the caller 
    */
   if ( EnumCtx->Callback(EntryInformation,
                          Record->Depth,
                          EnumCtx->CallbackParameter) )
   {
      /*
       * If the caller is also wanting to see data streams, enumerate those now
       */
      if ( FlagOn(EnumCtx->Flags, EDF_INCLUDEDATASTREAMS) )
      {
         EnumStreamCtx.EnumCtx          = EnumCtx;
         EnumStreamCtx.EntryInformation = EntryInformation;
         EnumStreamCtx.Record           = Record;

         /*
          * Directories cannot be opened with a trailing \, so if there is one we need
          * to temporarily remove it
          */
         cchFilePathTail = (EntryInformation->FilePathLength - 1);
         bIsDirectory    = (L'\\' == EntryInformation->FilePath[cchFilePathTail]);

         if ( bIsDirectory )
         {
            EntryInformation->FilePath[cchFilePathTail] = L'\0';
            EntryInformation->FilePathLength            = cchFilePathTail;
            bIsDirectory                                = TRUE;
         }

         __try
         {
            dwRet = FpEnumFileStreamInformation(EntryInformation->FilePath,
                                                FSX_STREAM_NODEFAULTDATASTREAM,
                                                &EnumFileStreamRoutine,
                                                &EnumStreamCtx);
         }
         __finally
         {
            /*
             * If we modified the path, restore it
             */
            if ( bIsDirectory )
            {
               EntryInformation->FilePathLength                = cchFilePathTail + 1;
               EntryInformation->FilePath[cchFilePathTail]     = L'\\';
               EntryInformation->FilePath[cchFilePathTail + 1] = L'\0';
            }
         }
      }      
   }
   else
   {
      /*
       * The caller requested an abort, so set the state flag to notify any other 
       * working threads 
       */
      _InterlockedOr8(reinterpret_cast<volatile char*>(&(EnumCtx->State)),
                      FPI_STATE_ABORT);

      /* Abort */
      return ( FALSE );
   }

   /* Continue */
   return ( TRUE );
}

DWORD 
FindFilesDirectoryRecord(
   __in PENUMDIRECTORY_CTX EnumCtx,
   __in PDIRECTORY_RECORD Record
)
{
   DWORD                         dwRet;
   HRESULT                       hr;

   ULONG                         cbAllocate;

   BOOLEAN                       bWithinDepth;
   HANDLE                        hDirectory;
   PDIRECTORYENTRY_INFORMATION   pEntryInformation;
   PFILE_DIRECTORY_INFORMATION   pDirInformation;
   PFILE_DIRECTORY_INFORMATION   pDirInformationEntry;
   PDIRECTORY_RECORD             pSubDirectory;
   LPWSTR                        pwszFilePart;
   size_t                        cbRemaining;

   _ASSERTE(NULL != EnumCtx);
   _ASSERTE(NULL != Record);

   /* Initialize locals */
   dwRet             = NO_ERROR;
   bWithinDepth      = (Record->Depth < DecodeMaxTraversalDepth(EnumCtx->Flags) ? TRUE : FALSE);
   hDirectory        = INVALID_HANDLE_VALUE;
   pEntryInformation = NULL;
   pDirInformation   = NULL;
   
   /* 
    * Allocate an DIRECTORYENTRY_INFORMATION structure which will hold the longest possible 
    * file path we support
    */
   cbAllocate = (sizeof(DIRECTORYENTRY_INFORMATION) + FPI_DIRECTORYPATH_MAX_CB);
   cbAllocate = AlignUp(cbAllocate,
                        MEMORY_ALLOCATION_ALIGNMENT);

   pEntryInformation = reinterpret_cast<PDIRECTORYENTRY_INFORMATION>(malloc(cbAllocate));
   if ( !pEntryInformation )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpDirectory,
              L"malloc failed, cbAllocate = %I64u, exiting\n",
              cbAllocate);

      /* Failure */
      return ( ERROR_NOT_ENOUGH_MEMORY );      
   }

   /* 
    * Beyond this point we have resources to cleanup... 
    */

   __try
   {
      ZeroMemory(pEntryInformation,
                 cbAllocate);

      /* 
       * Retrieve the base information for this directory 
       */
      dwRet = QueryDirectoryAttributeInformation(Record->DirectoryPath,
                                                 pEntryInformation,
                                                 cbAllocate);

      if ( WINERROR(dwRet) )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"QueryDirectoryAttributeInformation failed, dwRet = %!WINERROR!, exiting\n",
                 dwRet);

         /* Failure */
         __leave;
      }

      /* 
       * Setup the base file path. This cannot fail becase we don't allow a DIRECTORY_RECORD to hold
       * a path any longer than FPI_DIRECTORYPATH_MAX_CB 
       */
      StringCbCopyExW(&(pEntryInformation->FilePath[0]),
                      FPI_DIRECTORYPATH_MAX_CB,
                      Record->DirectoryPath,
                      &pwszFilePart,
                      &cbRemaining,
                      0);

      pEntryInformation->FilePathLength = static_cast<USHORT>((FPI_DIRECTORYPATH_MAX_CB - cbRemaining) / sizeof(WCHAR));
      
      /* 
       * Post a callback for this directory 
       */
      if ( !ProcessEntryClientCallback(EnumCtx,
                                       pEntryInformation,
                                       Record) )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"ProcessEntryClientCallback returned FALSE, EnumCtx.Callback = %p, exiting\n",
                 EnumCtx->Callback);

         dwRet = ERROR_CANCELLED;
         /* Failure - Caller cancelled */
         __leave;
      }

      /*
       * Open the directory we'll be querying.. 
       */
      dwRet = OpenDirectoryFile(Record->DirectoryPath,
                                &hDirectory);

      if ( WINERROR(dwRet) )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"OpenDirectoryFile failed, dwRet = %!WINERROR!, exiting\n",
                 dwRet);

         /* Failure */
         __leave;
      }

      /* 
       * Allocate a buffer for our FILE_DIRECTORY_INFORMATION records. This should be fairly large
       * so that we don't have to do many round trips back down into the kernel, but not so large
       * that if we're running a lot of threads we start chewing up memory
       */
      cbAllocate = AlignUp(FPI_DIRECTORYINFOBUFFER_CB, 
                           MEMORY_ALLOCATION_ALIGNMENT);

      pDirInformation = reinterpret_cast<PFILE_DIRECTORY_INFORMATION>(malloc(cbAllocate));
      if ( !pDirInformation )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"malloc failed failed, cbAllocate = %u, exiting\n",
                 cbAllocate);

         dwRet = ERROR_NOT_ENOUGH_MEMORY;
         /* Failure */
         __leave;
      }

      ZeroMemory(pDirInformation,
                 cbAllocate);

      /* 
       * Retrieve the first set of files in the directory 
       */
      dwRet = QueryDirectoryFileInformation(hDirectory,
                                            FileDirectoryInformation,
                                            pDirInformation,
                                            cbAllocate,
                                            NULL,
                                            FALSE,
                                            NULL,
                                            TRUE);

      for ( ;; )
      {
         if ( WINERROR(dwRet) )
         {
            if ( ERROR_NO_MORE_FILES != dwRet )
            {
               FpTrace(TRACE_LEVEL_WARNING,
                       FpDirectory,
                       L"QueryDirectoryFileInformation failed, dwRet = %!WINERROR!, exiting\n",
                       dwRet);
            }

            /* Success / Failure */
            break;
         }

         for ( pDirInformationEntry = pDirInformation; pDirInformationEntry; pDirInformationEntry = RtlNextEntryFromOffset(pDirInformationEntry) )
         {
            /*
             * Check if any thread has set the abort flag
             */
            if ( FlagOn(EnumCtx->State, FPI_STATE_ABORT) )
            {
               dwRet = ERROR_CANCELLED;
               /* Failure - Caller cancelled */
               __leave;
            }

            if ( FlagOn(pDirInformationEntry->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) )
            {
               /* 
                * Skip over . & .. entries 
                */
               if ( ((pDirInformationEntry->FileNameLength == 2) && (L'.' == pDirInformationEntry->FileName[0])) ||
                    ((pDirInformationEntry->FileNameLength == 4) && (L'.' == pDirInformationEntry->FileName[0]) && (L'.' == pDirInformationEntry->FileName[1])) )
               {
                  continue;
               }
            }
            else 
            {
               /* 
                * If the caller only wants directories, and this isn't one, skip it 
                */
               if ( FlagOn(EnumCtx->Flags, EDF_DIRECTORIESONLY) )
               {
                  continue;
               }
            }
         
            /* 
             * Append the file name to the directory that's already in the structure. This will copy to
             * the character after the directory's trailing \, and no further than what was allocated 
             */
            hr = StringCbCopyNW(pwszFilePart,
                                cbRemaining,
                                pDirInformationEntry->FileName,
                                pDirInformationEntry->FileNameLength);

            /*
             * Make sure the system hasn't given us a file name that is longer than what we support
             */
            if ( FAILED(hr) )
            {
               FpTrace(TRACE_LEVEL_WARNING,
                       FpDirectory,
                       L"StringCbCopyNW failed, cbRemaining = %I64u, pDirInformationEntry->FileNameLength = %u, hr = %!HRESULT!, exiting\n",
                       cbRemaining,
                       pDirInformationEntry->FileNameLength,
                       hr);

               dwRet = ERROR_INSUFFICIENT_BUFFER;
               /* Failure */
               __leave;
            }

            /** 
             ** This is either a directory or a file that we need to pass to the caller
             **/
            
            /* 
             * If this is a directory and it falls under the max depth we want to put it on the queue
             * and process it later. This will allow callbacks for the directory to be posted in order 
             * when enumerating with a single thread
             */
            if ( FlagOn(pDirInformationEntry->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) && bWithinDepth )
            {
               pSubDirectory = CreateDirectoryRecord(EnumCtx,
                                                     pEntryInformation->FilePath,
                                                     Record->Depth + 1);

               if ( !pSubDirectory )
               {
                  FpTrace(TRACE_LEVEL_WARNING,
                          FpDirectory,
                          L"CreateDirectoryRecord failed, ERROR_NOT_ENOUGH_MEMORY\n");

                  dwRet = ERROR_NOT_ENOUGH_MEMORY;
                  /* Failure */
                  __leave;
               }

               PushEnumDirectoryRecord(EnumCtx,
                                       pSubDirectory);

               /*
                * Check if we've hit a threshold that requires a new worker thread, if enabled
                */
               if ( IsEnumDirectoryWorkerRequired(EnumCtx) )
               {
                  dwRet = CreateEnumDirectoryWorker(EnumCtx);
                  _ASSERTE(NO_ERROR == dwRet);

                  if ( WINERROR(dwRet) )
                  {
                     FpTrace(TRACE_LEVEL_WARNING,
                             FpDirectory,
                             L"CreateEnumDirectoryWorker failed, dwRet = %!WINERROR!, continuing\n",
                             dwRet);
                  }

                  /* 
                   * If we couldn't create a worker thread it's not fatal. Reset dwRet here though so any 
                   * such failure isn't returned to the caller
                   */
                  dwRet = NO_ERROR;
               }
            
               /* 
                * Defer posting a callback for this directory 
                */
               continue;
            }

            /* 
             * Initialize the remaining members 
             */
            CopyLargeIntegerToFileTime(&(pDirInformationEntry->CreationTime),
                                       &(pEntryInformation->CreationTime));

            CopyLargeIntegerToFileTime(&(pDirInformationEntry->LastAccessTime),
                                       &(pEntryInformation->LastAccessTime));

            CopyLargeIntegerToFileTime(&(pDirInformationEntry->LastWriteTime),
                                       &(pEntryInformation->LastWriteTime));

            pEntryInformation->FileSize       = static_cast<ULONGLONG>(pDirInformationEntry->EndOfFile.QuadPart);
            pEntryInformation->FileAttributes = pDirInformationEntry->FileAttributes;
            pEntryInformation->FilePathLength = static_cast<USHORT>((FPI_DIRECTORYPATH_MAX_CB - cbRemaining + pDirInformationEntry->FileNameLength) / sizeof(WCHAR));
            
            /* 
             * Pass this one on to the caller 
             */
            if ( !ProcessEntryClientCallback(EnumCtx,
                                             pEntryInformation,
                                             Record) )
            {
               FpTrace(TRACE_LEVEL_WARNING,
                       FpDirectory,
                       L"ProcessEntryClientCallback returned FALSE, EnumCtx.Callback = %p, exiting\n",
                       EnumCtx->Callback);

               dwRet = ERROR_CANCELLED;
               /* Failure - Caller aborted */
               __leave;
            }
         }

         /* 
          * Get the next set of records 
          */
         dwRet = QueryDirectoryFileInformation(hDirectory,
                                               FileDirectoryInformation,
                                               pDirInformation,
                                               cbAllocate,
                                               NULL,
                                               FALSE,
                                               NULL,
                                               FALSE);
      }
   }
   __finally
   {
      /*
       * Cleanup the resources we allocated
       */
      if ( INVALID_HANDLE_VALUE != hDirectory )
      {
         CloseHandle(hDirectory);
      }

      free(pDirInformation);
      free(pEntryInformation);

      /*
       * If we're not in the handler because of an exception, and this isn't a threaded enumeration
       * then merge the queues to keep them sorted
       */
      if ( !AbnormalTermination() )
      {
         if ( !FlagOn(EnumCtx->Flags, EDF_THREADEDENUMERATION) )
         {
            MergeEnumDirectoryRecordQueues(EnumCtx);
         }
      }
   }
   
   _ASSERTE(!IsEnumerationError(dwRet));

   /* Success / Failure */
   return ( dwRet );
}

/*++
   Worker threading
 --*/

DWORD 
CreateEnumDirectoryWorker( 
   __in PENUMDIRECTORY_CTX EnumCtx 
)
{
   DWORD dwRet;
   BOOL  bCreateWorker;

   /* Initialize locals */
   bCreateWorker = FALSE;

   /* 
    * This entire set of operations need to be atomic. See ReleaseEnumDirectoryWorker() 
    * for an explanation as to why this is necessary 
    */
   AcquireSpinLock(&(EnumCtx->WorkerLock));
   {
      if ( EnumCtx->WorkerCount < FPI_WORKERTHREADMAX)
      {
         bCreateWorker         = TRUE;
         EnumCtx->WorkerCount += 1;

         FpTrace(TRACE_LEVEL_VERBOSE,
                 FpDirectory,
                 L"Creating worker thread, EnumCtx.WorkerCount = %u\n",
                 EnumCtx->WorkerCount);

         if ( 1 == EnumCtx->WorkerCount )
         {
            /* 
             * The main thread is the ONLY thread that will ever hit this test case. That is because
             * it is always running when there are worker threads, so if any worker thread creates a
             * new worker, WorkerCount will already be > 0 and so will never see this case succeed.
             */

            /* 
             * This is the main thread, creating the first worker. The exit event is in a signaled
             * state so we need to reset it now so that the main thread can wait on it. 
             */
            ResetEvent(EnumCtx->WorkerExit);
         }
      }
   }
   ReleaseSpinLock(&(EnumCtx->WorkerLock));

   if ( !bCreateWorker )
   {
      /* Success */
      return ( NO_ERROR );
   }

   /*
    * We require a worker thread, so start one up
    */
   dwRet = TxCreateThread(NULL,
                          0,
                          &EnumDirectoryThreadRoutine,
                          EnumCtx,
                          TXF_CRTTHREAD,
                          NULL,
                          NULL);

   if ( WINERROR(dwRet) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpDirectory,
              L"TxCreateThread failed, dwRet = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      ReleaseEnumDirectoryWorker(EnumCtx);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD 
WINAPI 
EnumDirectoryThreadRoutine( 
   __in_opt LPVOID Parameter 
)
{
   DWORD              dwRet;
   PENUMDIRECTORY_CTX pEnumCtx;
   
   /* Initialize locals.. */
   dwRet    = NO_ERROR;
   pEnumCtx = reinterpret_cast<PENUMDIRECTORY_CTX>(Parameter);

#ifdef _DEBUG
   SetDebuggerThreadName(GetCurrentThreadId(),
                         __FUNCTION__);
#endif /* _DEBUG */

   /* 
    * The thread that created us is waiting to be signaled that we've completed
    * initializing 
    */
   TxSetThreadInitComplete(NO_ERROR);
 
   __try
   {
      FpTrace(TRACE_LEVEL_VERBOSE,
              FpDirectory,
              L"Worker thread, starting enumeration, pEnumCtx = %p",
              pEnumCtx);

      /*
       * Run the scan for this thread
       */
      dwRet = EnumDirectoryContext(pEnumCtx);

#ifdef _DEBUG
      /* For testing, this will give EnumDirectoryInformation() some time to engage the wait code */
      Sleep(500);
#endif /* _DEBUG */
   }
   __finally
   {
      ReleaseEnumDirectoryWorker(pEnumCtx);
   }

   /* Success / Failure */
   return ( dwRet );
}

void 
ReleaseEnumDirectoryWorker( 
   __in PENUMDIRECTORY_CTX EnumCtx 
)
{
   AcquireSpinLock(&(EnumCtx->WorkerLock));
   {
      /* Drop the worker count and if this is the last worker signal the exit event. This could be the
       * main thread calling this for a failure path. If this is a worker thread calling this, then it
       * could be preempted after decrementing the worker count. When that happens, the main thread could
       * need to spin up a new worker, which would find that the count is 0 and so assume that the event 
       * is signaled already, and reset the exit event. Then this thread could run again, assume that 
       * WorkerCount is still 0, set the exit event and corrupt the wait state. For example..
       *
       * 1) Main creates new Worker-A, resets WorkerExit
       * 2) Main and Worker-A start processing
       * 3) Worker-A finishes processing
       * 4) Worker-A sets WorkerCount = 0
       * 5) Worker-A is preempted by Main
       * 6) Main creates new Worker-B, resets WorkerExit because WorkerCount == 0
       * 7) Main and Worker-B start processing
       * 8) Worker-A is rescheduled, assumes WorkerCount == 0, sets WorkerExit
       * 9) Main finishes processing, waits on WorkerExit 
       * 10) Main exits wait state because WorkerExit was signaled by Worker-A, returns, caller
       *     cleans up, frees callback, etc
       * 11) Worker-B is still running, now abanondoned, tries to callback to caller and blows up
       *
       * To avoid this whole mess, both WorkerCount and WorkerExit must be manipulated inside a
       * lock. This way, the set of operations is atomic and order is maintained.
       */
      EnumCtx->WorkerCount -= 1;

      FpTrace(TRACE_LEVEL_VERBOSE,
              FpDirectory,
              L"Releasing worker thread, EnumCtx.WorkerCount = %u\n",
              EnumCtx->WorkerCount);

      if ( 0 == EnumCtx->WorkerCount )
      {
         /* 
          * No workers running. Signal the exit event so the main thread's wait operation succeeds 
          */
         SetEvent(EnumCtx->WorkerExit);
      }
   }
   ReleaseSpinLock(&(EnumCtx->WorkerLock));
}

/*++
   NTFS ADS Support
 --*/

BOOLEAN
APIENTRY
EnumFileStreamRoutine( 
   __in ULONG StreamCount, 
   __in ULONGLONG StreamSize, 
   __in ULONGLONG StreamSizeOnDisk,
   __in_z LPCWSTR StreamName, 
   __in_opt PVOID CallbackParameter
)
{
   BOOLEAN                       bRet;

   PCENUMSTREAM_CTX              pStreamCtx;
   PENUMDIRECTORY_CTX            pDirectoryCtx;
   PCDIRECTORY_RECORD            pRecord;
   PDIRECTORYENTRY_INFORMATION   pEntryInformation;

   size_t                        cbRemaining;
   ULONGLONG                     cbFileSize;
   USHORT                        cchFilePathLength;

   UNREFERENCED_PARAMETER(StreamCount);
   UNREFERENCED_PARAMETER(StreamSizeOnDisk);

   /* Initialize locals */
   pStreamCtx        = reinterpret_cast<PCENUMSTREAM_CTX>(CallbackParameter);
   pDirectoryCtx     = pStreamCtx->EnumCtx;
   pRecord           = pStreamCtx->Record;
   pEntryInformation = pStreamCtx->EntryInformation;

   /*
    * Replace those members of the entry that are different for streams. The remaining are 
    * inherited from the file's attributes.
    */
   cbFileSize        = pEntryInformation->FileSize;
   cchFilePathLength = pEntryInformation->FilePathLength;
   
   StringCbCopyExW(&(pEntryInformation->FilePath[cchFilePathLength]),
                   FPI_DIRECTORYPATH_MAX_CB - (cchFilePathLength * sizeof(WCHAR)),
                   StreamName,
                   NULL,
                   &cbRemaining,
                   0);

   pEntryInformation->FileSize       = StreamSize;
   pEntryInformation->FilePathLength = static_cast<USHORT>((FPI_DIRECTORYPATH_MAX_CB - cbRemaining) / sizeof(WCHAR));
   
   /*
    * The entry is accurate now, so notify the caller
    */
   bRet = pDirectoryCtx->Callback(pEntryInformation,
                                  pRecord->Depth,
                                  pDirectoryCtx->CallbackParameter);

   if ( !bRet )
   {
      /*
       * The caller has signaled abort, so set the context state to signify as much
       */
      _InterlockedOr8(reinterpret_cast<volatile char*>(&(pDirectoryCtx->State)),
                      FPI_STATE_ABORT);
   }

   /*
    * Return everything to how it was previously
    */
   pEntryInformation->FileSize                    = cbFileSize;
   pEntryInformation->FilePathLength              = cchFilePathLength;
   pEntryInformation->FilePath[cchFilePathLength] = L'\0';

   /* Success / Failure */
   return ( bRet );
}

/*++
   File Information Support
 --*/

DWORD
OpenDirectoryFile(
   __in_z LPCWSTR DirectoryPath,
   __out PHANDLE DirectoryHandle
)
{
   DWORD  dwRet;

   HANDLE hDirectory;
   
   DWORD  dwShareMode;
   DWORD  dwFileType;
   

   /* Initialize outputs.. */
   (*DirectoryHandle) = INVALID_HANDLE_VALUE;

   /* Initialize locals */    
   dwRet       = NO_ERROR;
   dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

   do
   {
      hDirectory = CreateFile(DirectoryPath,
                              FILE_LIST_DIRECTORY|SYNCHRONIZE,
                              dwShareMode,
                              NULL,
                              OPEN_EXISTING,
                              FILE_FLAG_BACKUP_SEMANTICS|SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION|SECURITY_EFFECTIVE_ONLY,
                              NULL);

      if ( INVALID_HANDLE_VALUE != hDirectory )
      {
         /* Success */
         break;
      }

      dwRet = GetLastError();
      
      if ( ERROR_SHARING_VIOLATION != dwRet )
      {         
         /* Failure */
         break;
      }

      dwShareMode >>= 1;
   }
   while ( 0 != dwShareMode );

   if ( INVALID_HANDLE_VALUE == hDirectory )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpDirectory,
              L"CreateFile failed, DirectoryPath = %ws, dwRet = %!WINERROR!\n",
              DirectoryPath,
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   __try
   {
      /*
       * Make sure we actually opened a file on a disk drive
       */
      dwFileType = GetFileType(hDirectory);

      if ( FILE_TYPE_DISK != dwFileType )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"File is not a disk file, FileName = %ws, dwFileType = %u, exiting\n",
                 DirectoryPath,
                 dwFileType);

         dwRet = ERROR_BAD_FILE_TYPE;
         /* Failure */
         __leave;
      }

      /*
       * Everything looks good, so assign the handle to the caller 
       */
      (*DirectoryHandle) = hDirectory;
            
      /*
       * Clear the local alias so cleanup code doesn't close the handle we just
       * assigned to the caller
       */
      hDirectory = INVALID_HANDLE_VALUE;

      /* Success */
      dwRet = NO_ERROR;
   }
   __finally
   {
      if ( INVALID_HANDLE_VALUE != hDirectory )
      {
         CloseHandle(hDirectory);
      }
   }

   /* Success */
   return ( NO_ERROR );
}

DWORD
QueryDirectoryAttributeInformation(
   __in_z LPCWSTR DirectoryPath,
   __out_bcount(InformationLength) PDIRECTORYENTRY_INFORMATION DirectoryInformation,
   __in ULONG InformationLength
)
{
   DWORD                      dwRet;
   WIN32_FILE_ATTRIBUTE_DATA  AttributeData;

   UNREFERENCED_PARAMETER(InformationLength);

   _ASSERTE(InformationLength >= sizeof(DIRECTORYENTRY_INFORMATION));

   if ( !GetFileAttributesEx(DirectoryPath,
                             GetFileExInfoStandard,
                             &AttributeData) )
   {
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_WARNING,
              FpDirectory,
              L"GetFileAttributesEx failed, DirectoryPath = %ws, dwRet = %!WINERROR!, exiting\n",
              DirectoryPath,
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   /* 
    * Populate the basic fields 
    */
   DirectoryInformation->CreationTime   = AttributeData.ftCreationTime;
   DirectoryInformation->LastAccessTime = AttributeData.ftLastAccessTime;
   DirectoryInformation->LastWriteTime  = AttributeData.ftLastWriteTime;
   DirectoryInformation->FileSize       = (static_cast<ULONGLONG>(AttributeData.nFileSizeHigh) << 32) + static_cast<ULONGLONG>(AttributeData.nFileSizeLow);
   DirectoryInformation->FileAttributes = AttributeData.dwFileAttributes;

   /* Success */
   return ( NO_ERROR );
}

DWORD
QueryDirectoryFileInformation(
   __in HANDLE FileHandle,
   __in FILE_INFORMATION_CLASS FileInformationClass,
   __out_bcount(Length) PVOID FileInformation,
   __in ULONG Length,
   __out_opt PULONG ReturnLength,
   __in BOOLEAN ReturnSingleEntry,
   __in_opt LPCWSTR FileName,
   __in BOOLEAN RestartScan
)
{
   DWORD           dwRet;
   NTSTATUS        NtStatus;

   IO_STATUS_BLOCK IoStatus;
   UNICODE_STRING  FileNameString;

   IoStatus.Pointer     = NULL;
   IoStatus.Information = 0;

   if ( FileName )
   {
      RtlInitUnicodeString(&FileNameString,
                           FileName);
   }

   NtStatus = NtQueryDirectoryFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatus,
                                   FileInformation,
                                   Length,
                                   FileInformationClass,
                                   ReturnSingleEntry,
                                   FileName ? &FileNameString : NULL,
                                   RestartScan);

   if ( ReturnLength )
   {
      (*ReturnLength) = static_cast<ULONG>(IoStatus.Information);
   }

   dwRet = NtStatusToWin32Error(NtStatus);

   if ( WINERROR(dwRet) )
   {
      /*
       * ERROR_NO_MORE_FILES can occur and is still returned as failure, however we don't want
       * to trace it
       */
      if ( ERROR_NO_MORE_FILES != dwRet )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpDirectory,
                 L"NtQueryDirectoryFile failed, FileHandle = %p, NtStatus = %!STATUS!, dwRet = %!WINERROR!\n",
                 FileHandle,
                 NtStatus,
                 dwRet);
      }

      /* Failure */
      return ( dwRet );
   }

   /* Success */
   return ( dwRet );
}
