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
 
/* SpinLock.cpp
 *    Spin lock implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include "SpinLock.h"


/*++
   Private Declarations
 --*/

DWORD_PTR
GetProcessProcessorCount( 
);

BOOLEAN
TryAcquireSpinLock( 
   __deref_inout volatile SPINLOCK* SLock, 
   __in DWORD dwThreadID 
);

void 
SpinWait( 
   __in USHORT SpinCount
);

/*++
   Private Data
 --*/
const BOOLEAN gIsSingleProcessorProcess = (GetProcessProcessorCount() > 0 ? FALSE : TRUE);

/*++
   Implementation
 --*/
 
void 
APIENTRY
AcquireSpinLock( 
   __deref_inout volatile SPINLOCK* SLock 
)
{
   DWORD    dwThreadID;
   
   USHORT   SpinCount;
   USHORT   CurrentSpin;
   int      BackoffThreshold;

   dwThreadID = GetCurrentThreadId();

   /*
    * Cache the spin count as we don't support it changing
    * while we're spinning 
    */
   SpinCount        = SLock->SpinCount;
   CurrentSpin      = 0;
   BackoffThreshold = 10;

   for ( ;; )
   {
      /*
       * Try to acquire the lock 
       */
      if ( TryAcquireSpinLock(SLock,
                              dwThreadID) )
      {
         /*
          * We acquired the lock, so bail
          */
         return;
      }

      if ( gIsSingleProcessorProcess )
      {
         /* 
          * There is only 1 processor assigned to this process, so there's
          * no point doing a spin wait
          */
         Sleep(1);
      }
      else
      {
         YieldProcessor();

         if ( ++CurrentSpin < SpinCount )
         {
            if ( BackoffThreshold > 0 )
            {
               --BackoffThreshold;
               Sleep(0);
            }
            else
            {
               Sleep(1);
            }

            CurrentSpin = 0;
         }
      }
   }
}

void
APIENTRY
ReleaseSpinLock( 
   __deref_inout volatile SPINLOCK* SLock 
)
{
#ifdef _DEBUG
   if ( SLock->ThreadID != GetCurrentThreadId() )
   {
      /* 
       * Some thread other than the one that originally acquired the spinlock
       * is attempting to release it 
       */
      DebugBreak();
   }
#endif /* DBG */

   SLock->Depth -= 1;

   /*
    * Force the write to complete before anything else occurs and prevent
    * the CPU from reordering anything
    */
   _ForceMemoryWriteCompletion();

   if ( 0 == SLock->Depth )
   {
      _ForceMemoryReadCompletion();

      /* 
       * The owning thread has released the lock as many times as it
       * acquired it, so give it up to another thread now 
       */
      SLock->ThreadID = 0;
   }
}

BOOLEAN
TryAcquireSpinLock( 
   __deref_inout volatile SPINLOCK* SLock, 
   __in DWORD dwThreadID 
)
{
   volatile DWORD dwOwnerID;

   /* 
    * We acquire the lock by assigning the thread ID to the lock. The same thread can
    * acquire the lock multiple times
    */
   dwOwnerID = static_cast<DWORD>(InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&(SLock->ThreadID)),
                                                             static_cast<LONG>(dwThreadID),
                                                             0));

   /*
    * We acquired the lock if it was previously zero, or if the current value is the
    * same thread ID, which indicates recursive acquisition
    */
   if ( (0 != dwOwnerID) && (dwOwnerID != dwThreadID) )
   {
      /* 
       * Another thread beat this one to acquire the lock 
       */
      return ( FALSE );
   }
   
   /*
    * Prevent the CPU from reordering the update to Depth below. The compiler won't because Depth
    * is declared volatile
    */
   _ForceMemoryReadCompletion();

   /* 
    * The thread either already owns the lock or it has acquired it. We don't need to
    * do this with an interlock because only the owning thread ever reaches this point 
    */
   SLock->Depth += 1;

   /* 
    * If this fires then Depth has overflowed because the thread has acquired this lock 
    * recursively, too many times
    */
   _ASSERTE(SLock->Depth > 0);

   /* Success */
   return ( TRUE );
}

DWORD_PTR
GetProcessProcessorCount( 
)
{
   SYSTEM_INFO SysInfo;
   DWORD_PTR   cProcessProcessorCount;
   DWORD_PTR   ProcessAffinity;
   DWORD_PTR   SystemAffinity;

   if ( GetProcessAffinityMask(GetCurrentProcess(),
                               &ProcessAffinity,
                               &SystemAffinity) )
   {
      cProcessProcessorCount = 0;
      
      /*
       * Count the number of bits set in the process affinity
       */
      while ( ProcessAffinity )
      {
         cProcessProcessorCount += (ProcessAffinity & static_cast<DWORD_PTR>(1U));
         ProcessAffinity       >>= 1;
      }
   }
   else
   {
      GetSystemInfo(&SysInfo);
      cProcessProcessorCount = static_cast<DWORD_PTR>(SysInfo.dwNumberOfProcessors);
   }

   return ( cProcessProcessorCount );
}
