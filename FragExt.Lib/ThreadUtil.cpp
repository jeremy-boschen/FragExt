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

/* ThreadUtil.cpp
 *    Generic thread creation routines which manage creating threads inside
 *    a DLL
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include <List.h>
#include <SpinLock.h>
#include <ThreadUtil.h>

/*++
   Private Declarations
 --*/

#define TXF_FLAGSMASK \
   (0xf0000000)

typedef struct _TX_THREAD_CTX
{
   /* Context tracking */
   LIST_ENTRY              Entry;
   DWORD                   ThreadID;

   /* DLL locking */
   HMODULE                 TxModule;
   HMODULE                 ClientModule;
   
   /* Caller information */
   LPTHREAD_START_ROUTINE  StartRoutine;
   PVOID                   StartParameter;
   HANDLE                  InitCompleteEvent;
   DWORD                   InitCompleteStatus;
}TX_THREAD_CTX, *PTX_THREAD_CTX;


/*++
   Global Data
 --*/


/*
 * List of TX_THREAD_CTX records for active threads waiting for their
 * start routines to signal initialization completion
 */
SPINLOCK   gTxThreadCtxLock = SPINLOCK_INIT(SPINLOCK_DEF_SPINCOUNT);
LIST_ENTRY gTxThreadCtxList = {&gTxThreadCtxList, &gTxThreadCtxList};


/*++
   Private Support Functions
 --*/


BOOLEAN
IsModuleDLL(
   __in PVOID BaseAddress
)
{
   PIMAGE_DOS_HEADER pDosHeader;
   PIMAGE_NT_HEADERS pNTHeaders;

   pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(BaseAddress);
   if ( IMAGE_DOS_SIGNATURE != pDosHeader->e_magic )
   {
      return ( FALSE );
   }

   pNTHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<PCHAR>(BaseAddress) + pDosHeader->e_lfanew);
   if ( IMAGE_NT_SIGNATURE != pNTHeaders->Signature )
   {
      return ( FALSE );
   }

   if ( IMAGE_FILE_DLL & pNTHeaders->FileHeader.Characteristics )
   {
      return ( TRUE );
   }

   return ( FALSE );
}

BOOLEAN
TxiLockLibrary( 
   __in PVOID Address,
   __out HMODULE* Module
)
{
   HMODULE                  hModule;
   MEMORY_BASIC_INFORMATION MemInfo;
   WCHAR                    chModule[MAX_PATH*2];

   /* Initialize outputs */
   (*Module) = NULL;

   ZeroMemory(chModule,
              sizeof(chModule));

   VirtualQuery(Address, 
                &MemInfo, 
                sizeof(MemInfo));

   if ( !IsModuleDLL(MemInfo.AllocationBase) )
   {
      return ( TRUE );
   }

   if ( 0 != GetModuleFileName(reinterpret_cast<HMODULE>(MemInfo.AllocationBase),
                               chModule,
                               ARRAYSIZE(chModule)) )
   {
      hModule = LoadLibrary(chModule);
      if ( hModule )
      {
         (*Module) = hModule;
         /* Success */
         return ( TRUE );
      }
   }

   /* Failure */
   return ( FALSE );
}

DWORD 
WINAPI 
TxiCreateThreadRoutine( 
   __in PVOID Parameter 
)
{
   DWORD                   dwRet;

   PTX_THREAD_CTX          pThreadCtx;
   
   LPTHREAD_START_ROUTINE  pStartRoutine;
   PVOID                   pStartParameter;
   HMODULE                 hClientModule;
   HMODULE                 hTxModule;
   
   dwRet      = NO_ERROR;
   pThreadCtx = reinterpret_cast<PTX_THREAD_CTX>(Parameter);

   /* 
    * Extract the contents of the thread context that we'll need to use after it's
    * blown away 
    */
   hClientModule   = pThreadCtx->ClientModule;
   hTxModule       = pThreadCtx->TxModule;
   pStartRoutine   = pThreadCtx->StartRoutine;
   pStartParameter = pThreadCtx->StartParameter;
   
   /* 
    * Initialize the remaining tracking info for the context list and put it on the shared list
    */
   pThreadCtx->ThreadID = GetCurrentThreadId();

   AcquireSpinLock(&gTxThreadCtxLock);
   {
      InsertTailList(&gTxThreadCtxList,
                     &(pThreadCtx->Entry));
   }
   ReleaseSpinLock(&gTxThreadCtxLock);
   
   /* 
    * We're done with the context and won't be able to reference it after forwarding 
    * to the client's start routine because they will call TxSetThreadInitComplete, 
    * release the creating thread, and the context will go out of scope
    */
   pThreadCtx = NULL;

   __try
   {
      /*
       * We're ready to forward to the client's start routine, so do so now. We wrap
       * it in an exception handler so we can release any module references we're
       * holding
       */
      dwRet = pStartRoutine(pStartParameter);
   }
   __finally
   {
      if ( hClientModule )
      {
         /*
          * Unlock the caller's library first
          */
         FreeLibrary(hClientModule);
      }

      if ( hTxModule )
      {
         /* 
          * Unlock this library and exit
          */
         FreeLibraryAndExitThread(hTxModule,
                                  dwRet);
      }
   }

   return ( dwRet );
}

unsigned
__stdcall
TxiCrtCreateThreadRoutine( 
   __in void* Parameter 
)
{
   /*
    * The CRT uses a different thread start routine signature, so this one simply
    * exposes TxiCreateThreadRoutine to the CRT
    */
   return ( TxiCreateThreadRoutine(Parameter) );
}


/*++
   Export Implementations
 --*/

DWORD 
APIENTRY
TxCreateThread( 
   __in_opt LPSECURITY_ATTRIBUTES ThreadAttributes,
   __in SIZE_T StackSize,
   __in LPTHREAD_START_ROUTINE StartRoutine, 
   __in_opt PVOID StartParameter, 
   __in DWORD Flags,
   __out_opt PHANDLE ThreadHandle, 
   __out_opt PDWORD ThreadId 
)
{
   DWORD          dwRet;
   HANDLE         hThread;
   HANDLE         hEvent;
   HMODULE        hClientModule;
   HMODULE        hTxModule;
   TX_THREAD_CTX  ThreadCtx;
   

   /* Initialize locals.. */
   dwRet          = NO_ERROR;
   hThread        = NULL;
   hEvent         = NULL;
   hClientModule  = NULL;
   hTxModule      = NULL;
   
   ZeroMemory(&ThreadCtx,
              sizeof(ThreadCtx));
         
   __try
   {
      /* 
       * Lock the caller's module and this one if necessary. If the StartRoutine
       * does not reside in a DLL then the module will not be locked. The same
       * goes for this library and TxiCreateThreadRoutine
       */
      if ( !TxiLockLibrary(StartRoutine,
                           &hClientModule) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      if ( !TxiLockLibrary(&TxiCreateThreadRoutine,
                           &hTxModule) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      /* 
       * Create an event to wait on until the new thread calls TxSetThreadInitComplete() 
       */
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

      /* 
       * Initialize a thread context that will be passed to TxiCreateThreadRoutine. The
       * remaining fields will be initialized by TxiCreateThreadRoutine
       */
      ThreadCtx.ClientModule       = hClientModule;
      ThreadCtx.TxModule           = hTxModule;
      ThreadCtx.StartRoutine       = StartRoutine;
      ThreadCtx.StartParameter     = StartParameter;
      ThreadCtx.InitCompleteEvent  = hEvent;
      ThreadCtx.InitCompleteStatus = NO_ERROR;
         
      /* 
       * Kick off the thread.. 
       */
      if ( FlagOn(Flags, TXF_CRTTHREAD) )
      {
         hThread = reinterpret_cast<HANDLE>(_beginthreadex(ThreadAttributes,
                                                           static_cast<unsigned int>(StackSize),
                                                           &TxiCrtCreateThreadRoutine,
                                                           &ThreadCtx,
                                                           static_cast<unsigned int>(Flags & ~TXF_FLAGSMASK),
                                                           reinterpret_cast<unsigned int*>(ThreadId)));
      }
      else
      {
         hThread = CreateThread(ThreadAttributes,
                                StackSize,
                                &TxiCreateThreadRoutine,
                                &ThreadCtx,
                                Flags & ~TXF_FLAGSMASK,
                                ThreadId);
      }

      if ( !hThread )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      /* 
       * The thread owns the DLL locks now and we won't be freeing it 
       */
      hClientModule  = NULL;
      hTxModule = NULL;

      /* 
       * Wait for the thread to signal the 'startup complete' event 
       */
      WaitForSingleObject(hEvent,
                          INFINITE);

      /* 
       * Extract the status set by the thread so we can return the result of the thread's 
       * initilization now instead of any failure this routine has had
       */
      dwRet = ThreadCtx.InitCompleteStatus;
         
      /* 
       * If the caller requested it, return the thread handle. This means it won't be freed 
       * here so clear the local copy 
       */
      if ( ThreadHandle )
      {
         (*ThreadHandle) = hThread;
         hThread = NULL;
      }
   }
   __finally
   {
      if ( hThread )
      {
         CloseHandle(hThread);
      }

      if ( hEvent )
      {
         CloseHandle(hEvent);
      }

      if ( hClientModule )
      {
         FreeLibrary(hClientModule);
      }

      if ( hTxModule )
      {
         FreeLibrary(hTxModule);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

void 
APIENTRY
TxSetThreadInitComplete( 
   __in DWORD dwCompleteStatus 
)
{
   DWORD          dwTid;
   PLIST_ENTRY    pEntry;
   PTX_THREAD_CTX pThreadCtx;

   dwTid      = GetCurrentThreadId();
   pThreadCtx = NULL;
   
   /* 
    * Walk the context list and find the one for this thread 
    */
   AcquireSpinLock(&gTxThreadCtxLock);
   {
      for ( pEntry = gTxThreadCtxList.Flink; pEntry != &gTxThreadCtxList; pEntry = pEntry->Flink )
      {
         pThreadCtx = CONTAINING_RECORD(pEntry,
                                        TX_THREAD_CTX,
                                        Entry);

         if ( dwTid == pThreadCtx->ThreadID )
         {
            /* 
             * We don't want to reference the context after this routine because it will
             * be blown away once TxCreateThread() returns, so take it out 
             */
            RemoveEntryList(pEntry);
            break;
         }

         /* 
          * Code below expects this to be NULL when no context is found 
          */
         pThreadCtx = NULL;
      }
   }
   ReleaseSpinLock(&gTxThreadCtxLock);

   if ( pThreadCtx )
   {
      /*
       * We found the context info for this thread, so assign the completion status
       * and signal the creating thread that we're done and it can continue
       */
      pThreadCtx->InitCompleteStatus = dwCompleteStatus;

      SetEvent(pThreadCtx->InitCompleteEvent);
   }
}
