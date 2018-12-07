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

/* SqList.h
 *   Interlocked single linked list implementation for Win2K+
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 *
 * Version History
 *   0.0.001 - 09/20/2005 - Created
 *
 * Remarks
 *    These routines rely on the CMPXCHG8B instruction.
 */

#pragma once

#pragma warning( push )
/* nonstandard extension used : nameless struct/union */
#pragma warning( disable : 4201 )

#ifdef _WIN64

#pragma warning( push )
/* structure padded due to align() */
#pragma warning( disable:4324 )
typedef struct __declspec(align(16)) _SQLIST_ENTRY* PSQLIST_ENTRY;
typedef struct __declspec(align(16)) _SQLIST_ENTRY
{
   PSQLIST_ENTRY Next;
}SQLIST_ENTRY;
#pragma warning( pop )

typedef union __declspec(align(16)) _SQLIST_HEADER
{
   struct {
      union
      {
         ULONGLONG      Region;
         struct 
         {
            ULONGLONG   Depth    : 16;
            ULONGLONG   Sequence : 9;
            ULONGLONG   Next     : 39;

         };
      };
      ULONGLONG         Alignment;
   };
}SQLIST_HEADER, *PSQLIST_HEADER;

#else /* _WIN64 */

#define SQLIST_ENTRY  SINGLE_LIST_ENTRY
#define _SQLIST_ENTRY _SINGLE_LIST_ENTRY
#define PSQLIST_ENTRY PSINGLE_LIST_ENTRY

typedef union _SQLIST_HEADER 
{
   ULONGLONG      Region;
   struct {
      ULONG_PTR   Next;
      WORD        Depth;
      WORD        Sequence;
   };
}SQLIST_HEADER, *PSQLIST_HEADER;

#endif /* _WIN64 */
#pragma warning( pop )

#define InterlockedCompareExchange64 _InterlockedCompareExchange64

#ifndef IS_ALIGNED
#define IS_ALIGNED(_pointer, _alignment) \
    (0 == (((ULONG_PTR)(_pointer)) & ((_alignment) - 1)))
#endif /* IS_ALIGNED */

static
VOID
WINAPI
InitializeSqListHead(
   __inout PSQLIST_HEADER ListHead
)
{
   ListHead->Region = 0;
}

__inline
PSQLIST_ENTRY
FirstEntrySqList(
   __in PSQLIST_HEADER ListHead
)
{
   return ( (PSQLIST_ENTRY)ListHead->Next );
}

static
PSQLIST_ENTRY
WINAPI
PopEntrySqList(
    __inout SQLIST_HEADER volatile* ListHead
)
{
   SQLIST_HEADER ListHeadNew;
   SQLIST_HEADER ListHeadOld;
   PSQLIST_ENTRY ListEntry;

   do
   {
      PrefetchForWrite(ListHead);

      /* Load the address, sequence & depth */
      ListHeadOld.Region = ListHead->Region;

      /* Extract the address, and if NULL then the list is empty */
      ListEntry = (PSQLIST_ENTRY)ListHeadOld.Next;
      if ( !ListEntry )
      {
         break;
      }

      __try
      {
         /* There's a chance that another thread could have pop'd this entry and free'd
          * it between us reading in the address and dereferencing it, so if this causes
          * an exception it is caught, the compare fails and the process repeats */
         ListHeadNew.Next = (ULONG_PTR)*(&ListEntry->Next);

         /* We'll adjust the depth but leave the sequence alone */
         ListHeadNew.Depth = ListHeadOld.Depth - 1;
      }
      __except( EXCEPTION_EXECUTE_HANDLER )
      {
      }
   }
   while ( InterlockedCompareExchange64((LONGLONG*)&ListHead->Region, 
                                        (LONGLONG)ListHeadNew.Region, 
                                        (LONGLONG)ListHeadOld.Region) != (LONGLONG)ListHeadOld.Region );

  return ( ListEntry );
}

static
PSQLIST_ENTRY
WINAPI
PushEntrySqList (
    __inout SQLIST_HEADER volatile* ListHead,
    __inout SQLIST_ENTRY* ListEntry
)
{
   SQLIST_HEADER ListHeadNew;
   SQLIST_HEADER ListHeadOld;

   PrefetchForWrite(ListHead);

#ifdef _WIN64
   _ASSERTE(IS_ALIGNED(ListEntry, 16));
#endif /* _WIN64 */

   /* Set the new entry pointer here, as it will only be redundant to do
    * so in the loop */
   ListHeadNew.Next = (ULONG_PTR)ListEntry;

   do
   {
      /* Load the address, sequence & depth */
      ListHeadOld.Region = ListHead->Region;

      /* Setup the new entry to point to the old first entry */
      ListEntry->Next = (PSQLIST_ENTRY)ListHeadOld.Next;

      /* Update the sequence and depth */
      ListHeadNew.Depth    = ListHeadOld.Depth    + 1;
      ListHeadNew.Sequence = ListHeadOld.Sequence + 1;

      /* Assign the new list head if it hasn't changed. If another thread has changed it between
       * when we read it and now, the compare will fail and we'll start over */
   }
   while ( InterlockedCompareExchange64((LONGLONG*)&ListHead->Region,
                                        (LONGLONG)ListHeadNew.Region,
                                        (LONGLONG)ListHeadOld.Region) != (LONGLONG)ListHeadOld.Region );

   /* Return the first entry */
   return ( (PSQLIST_ENTRY)ListHeadOld.Next );
}

static
PSQLIST_ENTRY
WINAPI
FlushSqList(
    __inout SQLIST_HEADER* ListHead
)
{
   SQLIST_HEADER ListHeadOld;
   SQLIST_HEADER ListHeadNew;

   /* Initialize the list pointer to be NULL. We can do it outside of the loop
    * as its never touched inside it */
   ListHeadNew.Next = 0;
   
   do
   {
      /* Read in the entire 64bit union value, which includes the address, sequence & depth */
      ListHeadOld.Region = ListHead->Region;

      /* If the address is NULL, the list is empty */
      if ( !ListHeadOld.Next ) 
      {
         break;
      }

      /* Copy the sequence number and reset the depth */
      ListHeadNew.Sequence = ListHeadOld.Sequence;
      ListHeadNew.Depth    = 0;

      /* Assign the new list header if it hasn't changed since we started reading it. If it has
       * changed, this will faill and we'll just start over */
   }
   while ( InterlockedCompareExchange64((LONGLONG*)&ListHead->Region,
                                        (LONGLONG)ListHeadNew.Region,
                                        (LONGLONG)ListHeadOld.Region) != (LONGLONG)ListHeadOld.Region );
   
   /* Return our view of the first entry in the list */
   return ( (PSQLIST_ENTRY)ListHeadOld.Next );
}

static
USHORT
WINAPI
QueryDepthSqList(
    __in SQLIST_HEADER* ListHead
)
{
   return ( ListHead->Depth );
}
