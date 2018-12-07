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
 *    Custom memory allocator which reserves a large block of memory, carves
 *    it up and hands out fixed sized chunks. Blocks are comitted using a lazy
 *    pattern
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#pragma once

#ifndef __CACHELIST_H__
#define __CACHELIST_H__

typedef SLIST_ENTRY CACHELIST_ENTRY;

typedef struct _CACHELIST_STATISTICS
{
   ULONG       SizeOfCache;
   ULONG       SizeOfRegion;
   ULONG       SizeOfRecord;

   ULONG       MaximumRegions;
   ULONG       RegionsCommitted;

   ULONG       MaximumRecords;
   ULONG       RecordsCommitted;
   ULONG       RecordsAvailable;
}CACHELIST_STATISTICS, *PCACHELIST_STATISTICS;

/* 
 * Use non-locking list management 
 *    Caller must serialize access to all cache list functions
 */
#define CACHELIST_NOLOCKLIST 0x00000001UL


DWORD
APIENTRY
CreateCacheList(
   __in ULONG Flags,
   __in ULONG CacheSize,
   __in ULONG RecordSize,
   __out PHANDLE CacheList
);

void
APIENTRY
DestroyCacheList(
   __in HANDLE CacheList
);

PVOID
APIENTRY
PopCacheListEntry(
   __in HANDLE CacheList
);

void
APIENTRY
PushCacheListEntry(
   __in HANDLE CacheList,
   __in PVOID Entry
);

USHORT
APIENTRY
QueryCacheListDepth(
   __in HANDLE CacheList
);

void
APIENTRY
QueryCacheListStatistics(
   __in HANDLE CacheList,
   __out PCACHELIST_STATISTICS Statistics
);

BOOLEAN
APIENTRY
IsCacheListEntry(
   __in HANDLE CacheList,
   __in PVOID Entry
);

#endif /* __CACHELIST_H__ */