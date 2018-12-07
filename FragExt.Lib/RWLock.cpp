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
 
/* RWLock.cpp
 *    General purpose reader/writer locks
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 05/24/2009 - Created
 */

#include "Stdafx.h"

#include <RWLock.h>
#include <NTVersion.h>

extern "C"
{

short 
_InterlockedIncrement16(
   short volatile* Addend
);
#pragma intrinsic(_InterlockedIncrement16)

short 
_InterlockedDecrement16(
   short volatile* Addend
);
#pragma intrinsic(_InterlockedDecrement16)

};

/*++

Private Declarations
   
 --*/

#define WINERROR( dw ) \
   (0 != (dw))

typedef 
VOID
(WINAPI*
PINITIALIZESRWLOCK)(
   __out PSRWLOCK SRWLock
);

typedef
VOID
(WINAPI*
PRELEASESRWLOCKEXCLUSIVE)(
   __inout PSRWLOCK SRWLock
);

typedef
VOID
(WINAPI*
PRELEASESRWLOCKSHARED)(
   __inout PSRWLOCK SRWLock
);

typedef
VOID
(WINAPI*
PACQUIRESRWLOCKEXCLUSIVE)(
   __inout PSRWLOCK SRWLock
);

typedef
VOID
(WINAPI*
PACQUIRESRWLOCKSHARED)(
   __inout PSRWLOCK SRWLock
);

typedef struct _SRWLOCKTABLE
{
   PINITIALIZESRWLOCK         pInitializeSRWLock;
   PACQUIRESRWLOCKEXCLUSIVE   pAcquireSRWLockExclusive;
   PRELEASESRWLOCKEXCLUSIVE   pReleaseSRWLockExclusive;
   PACQUIRESRWLOCKSHARED      pAcquireSRWLockShared;
   PRELEASESRWLOCKSHARED      pReleaseSRWLockShared;
}SRWLOCKTABLE, *PSRWLOCKTABLE;

#define IsSRWLockSupported( ) \
   (NULL != gSRWLockTable.pInitializeSRWLock)

void
APIENTRY
RwxInitializeLibrary(
);

/*
 * Global function pointer table for SRW API
 */
SRWLOCKTABLE gSRWLockTable = {0};

/*++
   __Module
      Initializes/uninitializes global data in this module
 --*/
struct __Module
{
   __Module(
   ) throw()
   {
      RwxInitializeLibrary();
   }
};

static __Module ModuleInitializer();

/*++

   RW Lock API

 --*/

DWORD
APIENTRY
InitializeRWLock(
   __out PRWLOCK Lock
)
{
   DWORD  dwRet;
   HANDLE hWait;

   Lock->ExclusiveWaitEvent      = NULL;
   Lock->ExclusiveOwnerId   = 0;
   Lock->ExclusiveLockCount = 0;
   Lock->SharedLockCount = 0;

   dwRet = NO_ERROR;

   __try
   {
      if ( !InitializeCriticalSectionAndSpinCount(&(Lock->ExclusiveLock),
                                                  0x80000000|4000) )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
   }      
   }
   __except( STATUS_NO_MEMORY == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
   {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
   }

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   hWait = CreateEvent(NULL, 
                       FALSE, 
                       FALSE, 
                       NULL);

   if ( !hWait )
   {
      /*
       * Save the last error value before calling DeleteCriticalSection, in case it resets it
       */
      dwRet = GetLastError();
      
      /*
       * We won't call UninitializeRWLock so we need to delete
       * the CRITICAL_SECTION here
       */
      DeleteCriticalSection(&(Lock->ExclusiveLock));

      /* Failure */
      return ( dwRet );
   }

   Lock->ExclusiveWaitEvent = hWait;
   
   dwRet = NO_ERROR;
   /* Success */   
   return ( dwRet );
}

void
APIENTRY
UninitializeRWLock(
   __inout PRWLOCK Lock
)
{
   DeleteCriticalSection(&(Lock->ExclusiveLock));
   
   CloseHandle(Lock->ExclusiveWaitEvent);

   ZeroMemory(Lock,
              sizeof(*Lock));
}

void
APIENTRY
AcquireRWLockExclusive(
   __inout PRWLOCK Lock
)
{
   /*
    * Acquire the exclusive lock
    */
   EnterCriticalSection(&(Lock->ExclusiveLock));

   /*
    * We're the only thread that can execute this, so update the exclusive lock count. This
    * will allow us to recursively acquire the lock in exclusive mode
    */
   Lock->ExclusiveLockCount += 1;

    /*
     * If there are other threads that are holding the lock in shared mode..
     */
   while ( Lock->SharedLockCount > 0 ) 
   {
      /*
       * Wait for the shared clients to exit and signal us
       */
      WaitForSingleObject(Lock->ExclusiveWaitEvent, 
                          INFINITE);
   }

   /*
    * We now own the lock exlusively and we're the only thread that can execute here
    * so set the thread id
    */
   Lock->ExclusiveOwnerId = GetCurrentThreadId();
}

void
APIENTRY
ReleaseRWLockExclusive(
   __inout PRWLOCK Lock
)
{
   /*
    * We're the only thread that can execute this at the same time because we own the
    * lock exclusively and no other thread will get past AcquireRWLockExclusive 
    * until we unlock, so revert the count this thread put on the exclusive lock count. 
    *
    * This is like a recursion count so shared waiters will know when this thread has
    * done the proper number of ReleaseRWLockExclusive calls in relation to 
    * AcquireRWLockExclusive calls 
    */
   Lock->ExclusiveLockCount -= 1;

   /*
    * If we released the last reference on the recursion count, clear the thread ID too
    */
   if ( 0 == Lock->ExclusiveLockCount )
   {
      Lock->ExclusiveOwnerId = 0;
   }

   /*
    * Give up the exclusive lock.. this may or may not release the critical section, 
    * because this thread could have additional calls to AcquireRWLockExclusive,
    * and calls to EnterCriticalSection for the same thread, stack up
    */
   LeaveCriticalSection(&(Lock->ExclusiveLock));
}

void
APIENTRY
AcquireRWLockShared( 
   __inout PRWLOCK Lock
)
{
   /*
    * Add this thread to the number of those wanting shared access
    */
   _InterlockedIncrement16(reinterpret_cast<volatile SHORT*>(&(Lock->SharedLockCount)));

   /*
    * If there are no exclusive locks at this point then AcquireRWLockExclusive
    * will block because SharedLockCount is not 0
    */
   if ( Lock->ExclusiveLockCount > 0 )
   {
      /*
       * There is an exclusive lock in progress, or one being requested in parallel
       * with our shared request. We can't take shared access during an exclusive
       * lock and we also want to give exclusive requestors priority so we're going
       * to revert the reference we put on the shared lock count and force this
       * thread to wait for the exclusive lock to finish.
       */
      if ( 0 == _InterlockedDecrement16(reinterpret_cast<volatile SHORT*>(&(Lock->SharedLockCount))) )
      {
         /*
          * This thread was the last to take its reference off the shared lock count,
          * and there may be one or more threads waiting on the event so we need to
          * signal them that all shared waiters are now out of the lock
          */
         SetEvent(Lock->ExclusiveWaitEvent);
      }

      /*
       * So now there is either an exclusive lock in progress, or one or more waiting 
       * to be granted. This thread wants shared access, and can't get it until the
       * thread that has exclusive access gives it up so we need to wait for that to
       * happen. Note that other threads could come in and request exclusive access
       * while we're waiting on the exclusive lock and there is no guarantee that they
       * or this thread will acquire it first.
       */
      EnterCriticalSection(&(Lock->ExclusiveLock));
         /*
          * This thread requesting shared access now owns the exclusive lock, so no other
          * threads requesting exclusive access or that got to this point requesting shared
          * access own the lock. They may be doing increments/decrements on SharedLockCount 
          * above or in ReleaseRWLockShared though, and suceeding in acquiring the lock 
          * for shared access if ExclusiveLockCount is 0
          *
          * To signal that the lock is owned by a shared waiter we add a reference back
          * to SharedLockCount so that when we release the exclusive lock, any exclusive
          * waiters will block on the event
          */
         _InterlockedIncrement16(reinterpret_cast<volatile SHORT*>(&(Lock->SharedLockCount)));
      LeaveCriticalSection(&(Lock->ExclusiveLock));
   }

   /*
    * The lock is in shared mode now, and any thread requesting an exclusive lock will 
    * now have to wait
    */
   Lock->ExclusiveOwnerId = 0;
}

void
APIENTRY
ReleaseRWLockShared( 
   __inout PRWLOCK Lock
)
{
   /*
    * Release this thread's reference on SharedLockCount
    */
   if ( 0 == _InterlockedDecrement16(reinterpret_cast<volatile SHORT*>(&(Lock->SharedLockCount))) )
   {
      /*
       * This thread had the last reference on SharedLockCount so we want to check if
       * there are any threads waiting for an exclusive lock, which they could not have
       * been granted while there were other threads holding a shared lock
       */
      if ( Lock->ExclusiveLockCount > 0 )
      {
         /*
          * There are one or more threads waiting for an exclusive lock and as far as 
          * this thread knows, there are no other threads that have requested a shared
          * lock, so we signal any thread waiting for the exclusive lock to try and
          * reaquire it now.
          */
         SetEvent(Lock->ExclusiveWaitEvent);
      }
   }
}

/*++
 

 --*/

void
APIENTRY
RwxInitializeLibrary(
)
{
   HMODULE hModule;

   hModule = GetModuleHandle(L"NTDLL.DLL");
   if ( !hModule )
   {
      /* Failure */
      return;
   }

   gSRWLockTable.pInitializeSRWLock       = reinterpret_cast<PINITIALIZESRWLOCK>(GetProcAddress(hModule, "RtlInitializeSRWLock"));;
   gSRWLockTable.pAcquireSRWLockExclusive = reinterpret_cast<PACQUIRESRWLOCKEXCLUSIVE>(GetProcAddress(hModule, "RtlAcquireSRWLockExclusive"));
   gSRWLockTable.pReleaseSRWLockExclusive = reinterpret_cast<PRELEASESRWLOCKEXCLUSIVE>(GetProcAddress(hModule, "RtlReleaseSRWLockExclusive"));
   gSRWLockTable.pAcquireSRWLockShared    = reinterpret_cast<PACQUIRESRWLOCKSHARED>(GetProcAddress(hModule, "RtlAcquireSRWLockShared"));
   gSRWLockTable.pReleaseSRWLockShared    = reinterpret_cast<PRELEASESRWLOCKSHARED>(GetProcAddress(hModule, "RtlReleaseSRWLockShared"));

   if ( gSRWLockTable.pInitializeSRWLock       &&
        gSRWLockTable.pAcquireSRWLockExclusive &&
        gSRWLockTable.pReleaseSRWLockExclusive &&
        gSRWLockTable.pAcquireSRWLockShared    &&
        gSRWLockTable.pReleaseSRWLockShared )
   {
      /* Success */
      return;
   }

   ZeroMemory(&(gSRWLockTable),
              sizeof(gSRWLockTable));

   /* Failure */
   return;
}

DWORD
APIENTRY
InitializeRWXLock(
   __out PRWXLOCK Lock
)
{
   DWORD dwRet;

   /* Initialize locals.. */
   dwRet = NO_ERROR;

   if ( IsSRWLockSupported() )
   {
      gSRWLockTable.pInitializeSRWLock(&(Lock->SRWLock));
   }
   else
   {
      Lock->RWLock = reinterpret_cast<PRWLOCK>(malloc(sizeof(RWLOCK)));
      if ( !Lock->RWLock )
      {
         /* Failure */
         dwRet = ERROR_NOT_ENOUGH_MEMORY;
      }
      else
      {
         ZeroMemory(Lock->RWLock,
                    sizeof(RWLOCK));

         dwRet = InitializeRWLock(Lock->RWLock);
         if ( WINERROR(dwRet) )
         {
            /* Failure */
            free(Lock->RWLock);

            Lock->RWLock = NULL;
         }
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

void
APIENTRY
UninitializeRWXLock(
   __inout PRWXLOCK Lock
)
{
   if ( !IsSRWLockSupported() )
   {
      if ( Lock->RWLock )
      {
         UninitializeRWLock(Lock->RWLock);
         free(Lock->RWLock);
      }
   }
}

void
APIENTRY
AcquireRWXLockExclusive(
   __inout PRWXLOCK Lock
)
{
   if ( IsSRWLockSupported() )
   {
      gSRWLockTable.pAcquireSRWLockExclusive(&(Lock->SRWLock));
   }
   else
   {
      AcquireRWLockExclusive(Lock->RWLock);
   }
}

void
APIENTRY
ReleaseRWXLockExclusive(
   __inout PRWXLOCK Lock
)
{
   if ( IsSRWLockSupported() )
   {
      gSRWLockTable.pReleaseSRWLockExclusive(&(Lock->SRWLock));
   }
   else
   {
      ReleaseRWLockExclusive(Lock->RWLock);
   }
}

void
APIENTRY
AcquireRWXLockShared( 
   __inout PRWXLOCK Lock
)
{
   if ( IsSRWLockSupported() )
   {
      gSRWLockTable.pAcquireSRWLockShared(&(Lock->SRWLock));
   }
   else
   {
      AcquireRWLockShared(Lock->RWLock);
   }
}

void
APIENTRY
ReleaseRWXLockShared( 
   __inout PRWXLOCK Lock
)
{
   if ( IsSRWLockSupported() )
   {
      gSRWLockTable.pReleaseSRWLockShared(&(Lock->SRWLock));
   }
   else
   {
      ReleaseRWLockShared(Lock->RWLock);
   }
}
