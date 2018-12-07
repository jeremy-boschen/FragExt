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

/* WinLib.cpp
 *    Windowing library
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#include "Stdafx.h"

#include "WinLib.h"

/*++
   WPP
 --*/
#include "WinLib.tmh"

/*
 * Window creation attribute
 */
DWORD __WindowInstanceTlsIndex = 0;

/*++

   WndProc Thunks

 --*/
#ifdef _M_IX86
#pragma pack(push, 1)
struct __X86WndProcThunk
{
public:
   operator WNDPROC( ) {
      return ( (WNDPROC)this );
   }

    /*
      ;
      ; Thunk to transfer control to the WndProc function declared using the __stdcall
      ; calling convention. 
      ;
      ; We can utilize the X86 stdcall calling convention to reconstruct the stack and 
      ; push the extra 'this' parameter.
      ;
      ; What happens is we pop the return address put up by the caller, put this in its
      ; place then repush the callers return address and transfer to the WndProc. The
      ; WndProc is stdcall so it will cleanup the stack correctly before returning to
      ; the caller
      ;
      ; 12 bytes
      ;
      pop   eax
      push  dword ptr this
      push  eax
      jmp   offset WndProc
     */
   UCHAR _PopEax;
   UCHAR _PushImmediate;
   ULONG _ImmediateValue;
   UCHAR _PushEax;
   UCHAR _JmpRelative;
   ULONG _RelativeOffset;

   void
   Initialize(
      __in PVOID ClassInstance,
      __in PVOID WndProc
   )
   {
   /* pop eax              */
      _PopEax           = 0x58;
   /* push DWORD PTR this  */
      _PushImmediate    = 0x68;
      _ImmediateValue   = (ULONG)ClassInstance;
   /* push eax             */
      _PushEax          = 0x50;
   /* jmp offset WndProc   */
      _JmpRelative      = 0xE9;
      _RelativeOffset   = (ULONG)((INT_PTR)WndProc - ((INT_PTR)this + sizeof(__X86WndProcThunk)));

      FlushInstructionCache(GetCurrentProcess(),
                            this,
                            sizeof(__X86WndProcThunk));
   }
};
#pragma pack(pop)
typedef __X86WndProcThunk WNDPROCTHUNK;
#elif _M_AMD64
LRESULT
APIENTRY
X64WndProcProxy(
   __in HWND hWnd,
   __in UINT uMsg,
   __in WPARAM wParam,
   __in LPARAM lParam
);
#pragma pack(push, 2)
struct __X64WndProcThunk
{
public:
   operator WNDPROC( ) {
      return ( (WNDPROC)this );
   }

   /*
      ; Leaf function to transfer to the X64WndProcProxy function
      ;
      ; Because the caller cleans the stack in X64 calling convention we cannot
      ; directly modify the stack. We instead have to call the WndProc ourselves,
      ; but this cannot be done in a leaf function because we'll be modifying the
      ; stack. This thunk then simply uses volatile registers to store our parameters
      ; and transfers to X64WndProcProxy, in X64WndProcProxy.asm. This function will
      ; then properly call the target WndProc and clean the stack correctly before
      ; returning to the caller
      ;
      ; 32 bytes
      mov   r11, this
      mov   r10, offset WndProc
      mov   rax, offset X64WndProcProxy
      jmp   rax
     */
   USHORT   _MovR11;
   ULONG64  _This;
   USHORT   _MovR10;
   ULONG64  _WndProc;
   USHORT   _MovRax;
   ULONG64  _X64WndProcProxy;
   USHORT   _JmpRax;
   
   void
   Initialize(
      __in PVOID ClassInstance,
      __in PVOID WndProc
   )
   {
   /* mov   r11, this            */
      _MovR11           = 0xBB49;
      _This             = (ULONG64)ClassInstance;
   /* mov   r10, WndProc         */
      _MovR10           = 0xBA49;
      _WndProc          = (ULONG64)WndProc;
   /* mov   rax, X64WndProcProxy */
      _MovRax           = 0xB848;
      _X64WndProcProxy  = (ULONG64)X64WndProcProxy;
   /* jmp   rax                  */
      _JmpRax           = 0xE0FF;

      FlushInstructionCache(GetCurrentProcess(),
                            this,
                            sizeof(__X64WndProcThunk));
   }
};
#pragma pack(pop)
typedef __X64WndProcThunk WNDPROCTHUNK;
#else
   #error Unsupported thunking platform
#endif /* _M_IX86 | _M_AMD64 */

typedef __declspec(align(MEMORY_ALLOCATION_ALIGNMENT)) union _WNDPROCTHUNK_RECORD
{
   SLIST_ENTRY    Entry;
   WNDPROCTHUNK   WndProcThunk;
}WNDPROCTHUNK_RECORD;
typedef union _WNDPROCTHUNK_RECORD* PWNDPROCTHUNK_RECORD;

#ifndef PAGE_SIZE
   #define PAGE_SIZE 4096
#endif /* PAGE_SIZE */

PWNDPROCTHUNK_RECORD
AllocateWndProcThunkRecord(
);

void
FreeWndProcThunkRecord(
   PVOID WndProc
);

SLIST_HEADER gWndProcThunkList;

PVOID
APIENTRY
__CreateWndProcThunk(
   __in PVOID ClassInstance,
   __in PVOID WndProc
)
{
   PWNDPROCTHUNK_RECORD Record;

   Record = AllocateWndProcThunkRecord();
   if ( !Record ) {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      /* Failure */
      return ( NULL );
   }

   Record->WndProcThunk.Initialize(ClassInstance,
                                   WndProc);

   return ( (WNDPROC)*Record->WndProcThunk );
}

void
APIENTRY
__DestroyWndProcThunk(
   PVOID WndProc
)
{
   FreeWndProcThunkRecord(WndProc);
}

PWNDPROCTHUNK_RECORD
AllocateWndProcThunkRecord(
)
{
   PWNDPROCTHUNK_RECORD pRecord;
   PWNDPROCTHUNK_RECORD pRecordTail;
   
   PVOID                pRecordBlock;

   /*
    * Try to pop a record of the list..
    */
   pRecord = reinterpret_cast<PWNDPROCTHUNK_RECORD>(InterlockedPopEntrySList(&gWndProcThunkList));
   if ( pRecord ) {
      /* Success */
      return ( pRecord );
   }

   /*
    * The list is empty, either because it's exhaused or hasn't been populated yet. Either way we
    * need some more memory for it, so let's do that now
    */
   pRecordBlock = VirtualAlloc(NULL,
                               PAGE_SIZE,
                               MEM_COMMIT,
                               PAGE_EXECUTE_READWRITE);

   if ( !pRecordBlock ) {
      /* Failure */
      return ( NULL );
   }

   /*
    * Check if another thread beat us to it and has already put entries up on the list
    *
    * Make sure the page is present in memory before we try to do it
    */
   FillMemory(pRecordBlock,
              PAGE_SIZE,
              0xCC);
   
   pRecord = reinterpret_cast<PWNDPROCTHUNK_RECORD>(InterlockedPopEntrySList(&gWndProcThunkList));
   if ( pRecord ) {
      /*
       * We don't need this now...
       */
      VirtualFree(pRecordBlock,
                  0,
                  MEM_FREE);

      /* Success */
      return ( pRecord );
   }

   /*
    * Partition up the page of memory we allocated and add each record to the list. Save
    * the last record as the one returned. Using the last gives better locallity to the
    * most recent entries added to the list
    */
   pRecord     = reinterpret_cast<PWNDPROCTHUNK_RECORD>(pRecordBlock);
   /*!POINTER MATH!*/
   pRecordTail = pRecord + (PAGE_SIZE / sizeof(WNDPROCTHUNK_RECORD)) - 1;

   do {
      InterlockedPushEntrySList(&gWndProcThunkList,
                                &(pRecord->Entry));

      pRecord += 1;
   }
   while ( pRecord < pRecordTail );
   
   /* Success */
   return ( pRecord );
}

void
FreeWndProcThunkRecord(
   PVOID WndProc
)
{
   PWNDPROCTHUNK_RECORD Record;

   Record = CONTAINING_RECORD(WndProc,
                              WNDPROCTHUNK_RECORD,
                              WndProcThunk);
   
   FillMemory(Record,
              sizeof(WNDPROCTHUNK_RECORD),
              0xCC);

   InterlockedPushEntrySList(&gWndProcThunkList,
                             &(Record->Entry));
}

/*++
   __Module
      Initializes/uninitializes global data in this module
 --*/
struct __Module
{
   __Module(
   ) throw()
   {
      InitializeSListHead(&gWndProcThunkList);

      __WindowInstanceTlsIndex = TlsAlloc();
   }

   ~__Module(
   ) throw()
   {
      TlsFree(__WindowInstanceTlsIndex);
   }
};

static __Module ModuleInitializer();
