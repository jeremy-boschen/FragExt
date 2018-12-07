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

/* CacheList.h
 *    
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include <List.h>
#include "CacheList.h"

/*++
   WPP
 *--*/
#include "CacheList.tmh"

extern "C"
{

long 
_InterlockedAnd(
   long volatile* value,
   long mask
);
#pragma intrinsic(_InterlockedAnd)

#ifdef _M_AMD64
__int64 
_InterlockedAnd64(
   __int64 volatile* value,
   __int64 mask
);
#pragma intrinsic(_InterlockedAnd64)
#endif /* _M_AMD64 */

long 
_InterlockedOr(
   long volatile* value,
   long mask
);
#pragma intrinsic(_InterlockedOr)

#ifdef _M_AMD64
__int64 
_InterlockedOr64(
   __int64 volatile* value,
   __int64 mask
);
#pragma intrinsic(_InterlockedOr64)
#endif /* _M_AMD64 */

}; /* extern "C" */

#ifndef WINERROR
   #define WINERROR( Error ) \
      (NO_ERROR != Error)
#endif /* WINERROR */

#ifndef AlignUp
   #define AlignUp( Size, Align ) \
      (((Size) + ((Align) - 1)) & ~((Align) - 1))
#endif /* AlignUp */

#define CLFI_LOCKBIT_ACTIVE   1
#define CLFI_LOCKBIT_COMMIT   2
#define CLFI_LOCKBIT_MASK     3

/*
 * CACHELIST
 *    Control header for the cache entries, aligned to typical
 *    size of an L2 cache line
 */
typedef struct __declspec(align(128)) _CACHELIST
{
   /*
    * List of entries which are free to use
    */
   union {
      SLIST_HEADER         FreeSListHead;
      
      struct {
         SINGLE_LIST_ENTRY FreeListHead;
         USHORT            FreeListDepth;
      };
   };

   /*
    * Lock to limit commitment to a single thread
    */
   HANDLE            CommitLock;

   /*
    * Size of the cache memory block, in bytes
    */
   ULONG             CacheBlockSize;
   /*
    * Size of a single record
    */
   ULONG             CacheRecordSize;
   /*
    * Maximum number of regions that can be stored. A region
    * is an range of memory within the allocated range that 
    * is committed when the list of free entries is exhausted
    */
   ULONG             CacheRegionMax;
   /*
    * Number of regions that have been committed
    */
   volatile ULONG    CacheRegionCommit;
}CACHELIST, *PCACHELIST;
typedef const struct _CACHELIST* PCCACHELIST;

#define CacheListToHandle( CacheList ) \
   reinterpret_cast<HANDLE>(CacheList)

#define HandleToCacheList( Handle ) \
   reinterpret_cast<PCACHELIST>(Handle)

#define IsLockingCacheList( CacheList ) \
   (static_cast<ULONG_PTR>(CLFI_LOCKBIT_ACTIVE) == (static_cast<ULONG_PTR>(CLFI_LOCKBIT_ACTIVE) & reinterpret_cast<ULONG_PTR>(CacheList->CommitLock)))

#define HandleFromTaggedHandle( Handle ) \
   reinterpret_cast<HANDLE>(reinterpret_cast<ULONG_PTR>(Handle) & ~static_cast<ULONG_PTR>(0x3U))

DWORD
CommitNextCacheBlockRegion(
   __inout PCACHELIST CacheList,
   __out PVOID* Entry
);

UCHAR
SetCommitLockTagBit(
   __inout PCACHELIST CacheList,
   __in UCHAR Bit
);

UCHAR
ClearCommitLockTagBit(
   __inout PCACHELIST CacheList,
   __in UCHAR Bit
);

/*++

   PUBLIC

 --*/

DWORD
APIENTRY
CreateCacheList(
   __in ULONG Flags,
   __in ULONG  CacheSize,
   __in ULONG  RecordSize,
   __out PHANDLE CacheList
)
{
   DWORD       dwRet;

   PCACHELIST  pCacheList;
   PVOID       pMemoryBlock;
   HANDLE      hCommitLock;

   SYSTEM_INFO SystemInfo;

   _ASSERTE(CacheSize > RecordSize);
   _ASSERTE(NULL != CacheList);
   
   /* Initialize outputs */
   (*CacheList) = NULL;

   /* Initialize locals */
   dwRet       = NO_ERROR;
   hCommitLock = NULL;
   
   /*
    * We'll use the system allocation granularity and page granularity to
    * do allocations/commits
    */
   GetSystemInfo(&SystemInfo);

   /*
    * We want to pad the CACHELIST to fill an L2 cache line, and also to force
    * the first SLIST_ENTRY to meet its MEMORY_ALLOCATION_ALIGNMENT requirement
    */
   C_ASSERT(__alignof(struct _CACHELIST) >= MEMORY_ALLOCATION_ALIGNMENT);
   
   /*
    * The start of the cache contains the CACHELIST record, so make sure
    * there is room for it
    */
   CacheSize = max(CacheSize, sizeof(CACHELIST));

   /*
    * Align the cache size to the allocation granularity of VirtualAlloc
    */
   CacheSize = AlignUp(CacheSize,
                       SystemInfo.dwAllocationGranularity);

   /*
    * Attempt to reserve a block of memory for the entire cache. We will commit
    * the first page later so we can setup the CACHELIST header, then commit
    * regions of size allocation granularity when the list is empty and records
    * are requested
    */
   pMemoryBlock = VirtualAlloc(NULL,
                               CacheSize,
                               MEM_RESERVE,
                               PAGE_NOACCESS);

   if ( !pMemoryBlock ) {
      dwRet = GetLastError();

      FpTrace(TRACE_LEVEL_WARNING,
              FpCacheList,
              L"VirtualAlloc failed, MEM_RESERVE, CacheSize = %u, dwRet = %!WINERROR!, exiting\n",
              CacheSize,
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   __try {
      if ( !FlagOn(Flags, CACHELIST_NOLOCKLIST) ) {
         /*
          * Create the commit lock. This is a manual reset event, so that we can
          * free all waiting threads at once
          */
         hCommitLock = CreateEvent(NULL,
                                   TRUE,
                                   TRUE,
                                   NULL);

         if ( !hCommitLock ) {
            dwRet = GetLastError();

            FpTrace(TRACE_LEVEL_WARNING,
                    FpCacheList,
                    L"CreateEvent failed, dwRet = %!WINERROR!, exiting\n",
                    dwRet);

            /* Failure */
            __leave;
         }
      }

      /*
       * Align the record size to the memory allocation granularity of the platform, 
       * which is always large enough to hold an SLIST_ENTRY
       */
      C_ASSERT(MEMORY_ALLOCATION_ALIGNMENT >= sizeof(SLIST_ENTRY));

      RecordSize = AlignUp(RecordSize,
                           MEMORY_ALLOCATION_ALIGNMENT);

      /*
       * Everything is ready, so commit enough pages of the cache block so we can
       * store the CACHELIST there
       */
      pCacheList = reinterpret_cast<PCACHELIST>(VirtualAlloc(pMemoryBlock,
                                                             AlignUp(sizeof(CACHELIST), SystemInfo.dwPageSize),
                                                             MEM_COMMIT,
                                                             PAGE_READWRITE));

      if ( !pCacheList ) {
         dwRet = GetLastError();

         FpTrace(TRACE_LEVEL_WARNING,
                 FpCacheList,
                 L"VirtualAlloc failed, MEM_COMMIT, pMemoryBlock = %p, SystemInfo.dwPageSize = %u, Commit Size = %u, dwRet = %!WINERROR!, exiting\n",
                 pMemoryBlock,
                 SystemInfo.dwPageSize,
                 AlignUp(sizeof(CACHELIST), SystemInfo.dwPageSize),
                 dwRet);

         /* Failure */
         __leave;
      }

      if ( !FlagOn(Flags, CACHELIST_NOLOCKLIST) ) {
         InitializeSListHead(&(pCacheList->FreeSListHead));

         /*
          * Set the tag bit the rest of the code will use to determine
          * that the caller is using the locking version
          */
         hCommitLock = reinterpret_cast<HANDLE>(reinterpret_cast<ULONG_PTR>(hCommitLock)|CLFI_LOCKBIT_ACTIVE);
      }
      else {
         InitializeListHead(&(pCacheList->FreeListHead));         
         pCacheList->FreeListDepth = 0;
      }
      
      pCacheList->CommitLock        = hCommitLock;
      pCacheList->CacheBlockSize    = CacheSize;
      pCacheList->CacheRecordSize   = RecordSize;
      pCacheList->CacheRegionMax    = static_cast<ULONG>(CacheSize / SystemInfo.dwAllocationGranularity);
      pCacheList->CacheRegionCommit = 0;

      /*
       * Everything is ready, assign the CACHELIST to the caller
       */
      (*CacheList) = CacheListToHandle(pCacheList);

      /*
       * Clear the local aliases so cleanup code doesn't free them
       */
      hCommitLock  = NULL;
      pMemoryBlock = NULL;

      /* Success */
      dwRet = NO_ERROR;
   }
   __finally {
      if ( hCommitLock ) {
         _ASSERTE(NULL != HandleFromTaggedHandle(hCommitLock));
         CloseHandle(hCommitLock);
      }

      if ( pMemoryBlock ) {
         VirtualFree(pMemoryBlock,
                     0,
                     MEM_RELEASE);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

void
APIENTRY
DestroyCacheList(
   __in HANDLE CacheList
)
{
   PCACHELIST pCacheList;

   _ASSERTE(NULL != CacheList);

   pCacheList = HandleToCacheList(CacheList);

   /*
    * Remove the cache from the process' working set
    */
   VirtualUnlock(pCacheList,
                 pCacheList->CacheBlockSize);

   /*
    * Free the entire cache
    */
   VirtualFree(pCacheList,
               0,
               MEM_RELEASE);
}

PVOID
APIENTRY
PopCacheListEntry(
   __in HANDLE CacheList
)
{
   DWORD       dwRet;

   PCACHELIST  pCacheList;
   PVOID       pEntry;

   _ASSERTE(NULL != CacheList);

   /* Initialize locals */
   dwRet      = NO_ERROR;
   pCacheList = HandleToCacheList(CacheList);

   /*
    * Try to get an entry off the list
    */
   if ( IsLockingCacheList(pCacheList) ) {
      pEntry = InterlockedPopEntrySList(&(pCacheList->FreeSListHead));
      if ( pEntry ) {
         /* Success */
         return ( pEntry );
      }
   }
   else {
      pEntry = PopEntryList(&(pCacheList->FreeListHead));      
      if ( pEntry ) {
         pCacheList->FreeListDepth -= 1;
         /* Success */
         return ( pEntry );
      }
   }

   /*
    * The list was empty, so try to acquire one by repopulating the list
    */
   dwRet = CommitNextCacheBlockRegion(pCacheList,
                                      &pEntry);

   /*
    * Don't bother trying again and just bail
    */
   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_WARNING,
               FpCacheList,
               L"CommitNextCacheBlockRegion failed, pCacheList = %p, dwRet = %!WINERROR!, exiting\n",
               pCacheList,
               dwRet);

      /* Failure */
      return ( NULL );
   }

   /*
    * We have a record now. For non-locking, pCacheList->FreeListDepth will have been set
    * correctly and we don't need to update it
    */

   /* Success */
   return ( pEntry );
}

void
APIENTRY
PushCacheListEntry(
   __in HANDLE CacheList,
   __inout PVOID Entry
)
{
   _ASSERTE(NULL != CacheList);
   _ASSERTE(NULL != Entry);

   PCACHELIST pCacheList;

   /* Initialize locals */
   pCacheList = HandleToCacheList(CacheList);

   if ( IsLockingCacheList(pCacheList) ) {
      InterlockedPushEntrySList(&(pCacheList->FreeSListHead),
                                reinterpret_cast<PSLIST_ENTRY>(Entry));
   }
   else {
      PushEntryList(&(pCacheList->FreeListHead),
                    reinterpret_cast<PSINGLE_LIST_ENTRY>(Entry));
      pCacheList->FreeListDepth += 1;
   }
}

USHORT
APIENTRY
QueryCacheListDepth(
   __in HANDLE CacheList
)
{
   _ASSERTE(NULL != CacheList);

   PCACHELIST pCacheList;

   /* Initialize locals */
   pCacheList = HandleToCacheList(CacheList);

   if ( IsLockingCacheList(pCacheList) ) {
      return ( QueryDepthSList(&(pCacheList->FreeSListHead)) );
   }

   return ( pCacheList->FreeListDepth );
}

void
APIENTRY
QueryCacheListStatistics(
   __in HANDLE CacheList,
   __out PCACHELIST_STATISTICS Statistics
)
{
   _ASSERTE(NULL != CacheList);
   _ASSERTE(NULL != Statistics);

   PCACHELIST     pCacheList;
   ULONG          CacheRegionCommit;
   USHORT         RecordsAvailable;

   ULONG          cbCacheActual;
   
   /*
    * Initialize outputs
    */
   ZeroMemory(Statistics,
              sizeof(*Statistics));

   /* Initialize locals */
   pCacheList        = HandleToCacheList(CacheList);
   CacheRegionCommit = 0;
   RecordsAvailable  = 0;
   cbCacheActual     = (pCacheList->CacheBlockSize - sizeof(CACHELIST));

   /*
    * Volatile fields are not guaranteed to be accurate. This is mostly used for debugging/tuning, so 
    * there's not much need to ensure they are
    */
   if ( IsLockingCacheList(pCacheList) ) {
      RecordsAvailable = QueryDepthSList(&(pCacheList->FreeSListHead));
   }
   else {
      RecordsAvailable = pCacheList->FreeListDepth;
   }

   CacheRegionCommit = pCacheList->CacheRegionCommit;

   Statistics->SizeOfCache      = pCacheList->CacheBlockSize;
   Statistics->SizeOfRegion     = (pCacheList->CacheBlockSize / pCacheList->CacheRegionMax);
   Statistics->SizeOfRecord     = pCacheList->CacheRecordSize;
   Statistics->RegionsCommitted = CacheRegionCommit;
   Statistics->RecordsCommitted = (((CacheRegionCommit * (cbCacheActual / pCacheList->CacheRegionMax)) - (CacheRegionCommit * (cbCacheActual % pCacheList->CacheRegionMax))) / pCacheList->CacheRecordSize);
   Statistics->MaximumRegions   = pCacheList->CacheRegionMax;
   Statistics->MaximumRecords   = (pCacheList->CacheBlockSize / pCacheList->CacheRecordSize);
   Statistics->RecordsAvailable = RecordsAvailable;
}

BOOLEAN
APIENTRY
IsCacheListEntry(
   __in HANDLE CacheList,
   __in PVOID Entry
)
{
   _ASSERTE(NULL != CacheList);
   _ASSERTE(NULL != Entry);

   PCACHELIST pCacheList;
   PUCHAR     pEntryRegion;

   /* Initialize locals */
   pCacheList = HandleToCacheList(CacheList);

   /*
    * Calculate the address of the start of the region used for list entries.
    */
   pEntryRegion = reinterpret_cast<PUCHAR>(pCacheList) + sizeof(CACHELIST);
      
   return ( (Entry >= pEntryRegion) && (Entry < (pEntryRegion + pCacheList->CacheBlockSize - sizeof(CACHELIST))) );
}

DWORD
CommitNextCacheBlockRegion(
   __inout PCACHELIST CacheList,
   __out PVOID* Entry
)
{
   DWORD    dwRet;

   ULONG    iRegion;

   PVOID    pEntry;

   PUCHAR   pRegion;
   ULONG    cbRegion;
   ULONG    cbActual;

   PUCHAR   pRecord;
   PUCHAR   pRecordTail;

   UCHAR    CommitLockTag;   

   USHORT   iRecordCount;

   /* Initialize outputs */
   (*Entry) = NULL;

   /*
    * Do a quick check to see if we've already committed all regions, which 
    * if we have there's no reason to continue
    */
   if ( !(CacheList->CacheRegionCommit < CacheList->CacheRegionMax) ) {
      FpTrace(TRACE_LEVEL_INFORMATION,
              FpCacheList,
              L"CACHELIST commit limit reached, CacheList->CacheRegionMax = %u\n",
              CacheList->CacheRegionMax);

      /* Failure */
      return ( ERROR_OUTOFMEMORY );
   }

   /* Initialize locals */
   dwRet = NO_ERROR;
   
   /*
    * Serialize access beyond this point so that only one thread at a time
    * can extend the committed region.
    *
    * For non-locking list management, we assume that only a single thread will end up
    * calling this function, so we skip this piece and move right into the commit
    */
   while ( IsLockingCacheList(CacheList) ) {
      /*
       * Try to set the ownership bit first
       */
      CommitLockTag = SetCommitLockTagBit(CacheList,
                                          CLFI_LOCKBIT_COMMIT);

      if ( 0 == (CommitLockTag & CLFI_LOCKBIT_COMMIT) ) {
         /*
          * This thread owns the lock, so break out and commit the region now
          */
         break;
      }

      /**
       * Another thread is already committing a region, so wait for it
       * to complete
       **/
       
      /*
       * Now we wait for the commit lock. When this is signaled, all waiting threads
       * will be released. We may be getting to this point before another thread that
       * is also trying to commit resets the lock, so this wait may return immediately
       */
      dwRet = WaitForSingleObject(CacheList->CommitLock,
                                  INFINITE);

      if ( WAIT_OBJECT_0 != dwRet ) {
         FpTrace(TRACE_LEVEL_ERROR,
                 FpCacheList,
                 L"WaitForSingleObject failed, CacheList = %p, CacheList->CommitLock = %p, dwRet = %!WINERROR!\n",
                 CacheList,
                 CacheList->CommitLock,
                 dwRet);

         /* Failure */
         return ( dwRet );
      }

      /*
       * Check if the queue has been refilled or an entry has been returned while we tried to wait
       */
      pEntry = InterlockedPopEntrySList(&(CacheList->FreeSListHead));
      if ( NULL != pEntry ) {
         (*Entry) = pEntry;
         /* Success */
         return ( NO_ERROR );
      }

      /*
       * Either the other thread hasn't finished refilling the cache, or while
       * waiting it was refilled and exhausted. If it has been exhausted and
       * all regions are full then there is nothing we can do and we'll bail
       */
      if ( !(CacheList->CacheRegionCommit < CacheList->CacheRegionMax) ) {
         FpTrace(TRACE_LEVEL_INFORMATION,
                 FpCacheList,
                 L"CACHELIST commit limit reached, CacheList->CacheRegionMax = %u\n",
                 CacheList->CacheRegionMax);

         /* Failure */
         return ( ERROR_OUTOFMEMORY );
      }

      /*
       * The cache was refilled and exhausted by other threads, while this thread was waiting,
       * so loop back around and try to refill it
       */
   }

   __try {
      if ( IsLockingCacheList(CacheList) ) {
         /*
          * We own the cache now, so reset the wait event to force other threads
          * which are trying to do the same to wait for us to finish
          */
         ResetEvent(CacheList->CommitLock);
      }

      /*
       * This thread owns the cache now, and can try to populate the next region. It is 
       * possible that this thread got here after all regions were committed though, so
       * we have to recheck the count. This can happen when a thread first checks the
       * count, sees that it isn't maxed out, is preemted by a thread that commits a
       * new region, is scheduled again, grabs the tag bit and ends up here.
       *
       * If the commit count hasn't been maxed out then at this point no other thread
       * can update it. This thread won't update the count yet so that other threads
       * won't think the cache is exhausted if they end up checking the queue before
       * this thread starts refilling it
       */
      if ( !(CacheList->CacheRegionCommit < CacheList->CacheRegionMax) ) {
         FpTrace(TRACE_LEVEL_INFORMATION,
                 FpCacheList,
                 L"CACHELIST commit limit reached, CacheList->CacheRegionMax = %u\n",
                 CacheList->CacheRegionMax);

         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      /*
       * Calculate the size of a region, and the amount of the region that will actually be
       * committed at once. These are both guaranteed to be algined
       */
      cbRegion = (CacheList->CacheBlockSize / CacheList->CacheRegionMax);
      cbActual = (cbRegion - sizeof(CACHELIST)) - ((cbRegion - sizeof(CACHELIST)) % CacheList->CacheRecordSize);
      
      /*
       * Cache the region which we'll be updating
       */
      iRegion = CacheList->CacheRegionCommit;

      /*
       * Calculate the offset of this region. The first region has to account for the
       * CACHE_LIST record at the start of the allocated memory
       */
      pRegion = (reinterpret_cast<PUCHAR>(CacheList) + (iRegion * cbRegion));
      
      /*
       * Commit the entire region. We may not actually populate it, based on how much
       * extra space is available in each region, but the next populattion will have it
       * available to initialize. The first region we commit will have the first page
       * already committed because that was done to store the CACHE_LIST record.
       */
      if ( !VirtualAlloc(pRegion,
                         cbRegion,
                         MEM_COMMIT,
                         PAGE_READWRITE) ) {
         dwRet = GetLastError();

         FpTrace(TRACE_LEVEL_INFORMATION,
                 FpCacheList,
                 L"VirtualAlloc failed, pRegion = %p, cbRegion = %u, MEM_COMMIT, dwRet = %!WINERROR!\n",
                 pRegion,
                 cbRegion,
                 dwRet);

         /* Failure */
         __leave;
      }      

      /*
       * Adjust the start of the region for record allocation to account for the CACHELIST 
       * record stored at the start of the cache.
       */
      pRegion += sizeof(CACHELIST);

      /*
       * Now calculate the record start. Because the region size may not be a multiple 
       * of the record size we can end up with unused space at the end of a region. 
       * When this happens we move back to the start of the wasted space in the previous 
       * region and use that as our starting record.
       */
      pRecord = (pRegion - (iRegion * (cbRegion - cbActual)));
      
      /*
       * If we're not on the last region, we need to leave some wasted space for the
       * next population to use. Otherwise, if we are on the last region there won't
       * be another population so we'll use that space now
       */
      if ( iRegion < (CacheList->CacheRegionMax - 1) ) {
         pRecordTail  = (pRecord + cbActual);
      }
      else {
         pRecordTail = reinterpret_cast<PUCHAR>(CacheList) + CacheList->CacheBlockSize - ((CacheList->CacheBlockSize - sizeof(CACHELIST)) % CacheList->CacheRecordSize);
      }

#ifdef _DEBUG
      FillMemory(pRecord,
                 pRecordTail - pRecord,
                 0xFE);
#endif /* _DEBUG */

      /*
       * Adjust the tail so that we give out the final record to the caller
       */
      pRecordTail -= CacheList->CacheRecordSize;

      if ( IsLockingCacheList(CacheList) ) {
         /*
          * Update the region so other threads are aware of the update. We defer this until
          * this point so that there is little delay between when it could reach maximum and
          * when we refill the cache
          */
         InterlockedIncrement(reinterpret_cast<volatile long*>(&(CacheList->CacheRegionCommit)));

         /*
          * Now add entries to the free list. These can be pulled off right as they're going on if
          * there are other threads trying to pop them off.
          */
         do {
            InterlockedPushEntrySList(&(CacheList->FreeSListHead),
                                      reinterpret_cast<PSLIST_ENTRY>(pRecord));

            pRecord += CacheList->CacheRecordSize;
         }         
         while ( pRecord < pRecordTail );
      }
      else {
         /*
          * Same steps as locking code above, without the locks
          */
         CacheList->CacheRegionCommit += 1;

         /*
          * Get a count of how many we're adding so we can update the list depth after
          */
         iRecordCount = (USHORT)((ULONG)(pRecordTail - pRecord) / CacheList->CacheRecordSize);
         
         do {
            PushEntryList(&(CacheList->FreeListHead),
                          reinterpret_cast<PSINGLE_LIST_ENTRY>(pRecord));

            pRecord += CacheList->CacheRecordSize;
         }
         while ( pRecord < pRecordTail );

         /*
          * Update the free list count
          */
         CacheList->FreeListDepth += iRecordCount;
      }

      /*
       * Assign the final record to the caller. We kept this record off the list by adjusting
       * pRecordTail back one record
       */
      (*Entry) = pRecord;

      /* Success */
      dwRet = NO_ERROR;
   }
   __finally {
      if ( IsLockingCacheList(CacheList) ) {
         /*
          * Set the lock so waiting threads can continue. If they try to set the cache lock bit
          * before we can clear it, they will fail and go into a wait again. Because it is a 
          * manual reset event, the wait will be immediately satisfied and they will start the
          * entire sequence over again
          */
         SetEvent(CacheList->CommitLock);

         /*
          * There is a chance here that another thread that was awoken by SetEvent above, will
          * have a higher priority than this thread, and it will start trying to acquire the 
          * lock bit, fail, wait on CommitLock, succeed, and retry the lock bit again. It could
          * continue doing this and preempt this thread because of its priority, but it should
          * be very rare and would cause more problems than just this anyway, so we ignore it
          */

         /*
          * Clear the ownership bit now that we're done. This will allow other threads that are
          * waiting on it to acquire ownership
          */
         ClearCommitLockTagBit(CacheList,
                               CLFI_LOCKBIT_COMMIT);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

UCHAR
SetCommitLockTagBit(
   __inout PCACHELIST CacheList,
   __in UCHAR Bit
)
{
#ifdef _M_AMD64
   #define InterlockedOrPointer  _InterlockedOr64
#else /* _WIN64 */
   #define InterlockedOrPointer  _InterlockedOr
#endif /* _WIN64 */

   LONG_PTR iRet;
      
   iRet = InterlockedOrPointer(reinterpret_cast<volatile LONG_PTR*>(&(CacheList->CommitLock)),
                               static_cast<LONG_PTR>(Bit & CLFI_LOCKBIT_MASK));

   return ( static_cast<UCHAR>(iRet & CLFI_LOCKBIT_MASK) );
}

UCHAR
ClearCommitLockTagBit(
   __inout PCACHELIST CacheList,
   __in UCHAR Bit
)
{
#ifdef _M_AMD64
   #define InterlockedAndPointer  _InterlockedAnd64
#else /* _WIN64 */
   #define InterlockedAndPointer  _InterlockedAnd
#endif /* _WIN64 */

   LONG_PTR iRet;

   iRet = InterlockedAndPointer(reinterpret_cast<volatile LONG_PTR*>(&(CacheList->CommitLock)),
                                ~static_cast<LONG_PTR>(Bit & CLFI_LOCKBIT_MASK));

   return ( static_cast<UCHAR>(iRet & CLFI_LOCKBIT_MASK) );
}
