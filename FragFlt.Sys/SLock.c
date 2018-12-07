/* FragExt - File defragmentation software utility toolkit.
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

/* KeSLock.c
 *    Smart lock implementation
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 12/30/2005 - Created
 */

#include "KeSLock.h"

/**
 * RtlInitStaticUnicodeString
 *    Initializing a compile-time string as a UNICODE_STRING
 */
#ifndef RtlInitStaticUnicodeString
   #define RtlInitStaticUnicodeString( _Destination, _Source ) \
      (_Destination)->Length        = sizeof( (_Source) ) - sizeof(UNICODE_NULL); \
      (_Destination)->MaximumLength = sizeof( (_Source) ); \
      (_Destination)->Buffer        = _Source; 
#endif /* RtlInitStaticUnicodeString */

typedef VOID (FASTCALL* PKEACQUIREQUEUED)( __inout PKSPIN_LOCK SpinLock, __out PKLOCK_QUEUE_HANDLE LockHandle );
typedef VOID (FASTCALL* PKERELEASEQUEUED)( __in PKLOCK_QUEUE_HANDLE LockHandle );
typedef VOID (FASTCALL* PKEACQUIREQUEUEDATDPCLEVEL)( __inout PKSPIN_LOCK SpinLock, __out PKLOCK_QUEUE_HANDLE LockHandle );
typedef VOID (FASTCALL* PKERELEASEQUEUEDFROMDPCLEVEL)( __in PKLOCK_QUEUE_HANDLE LockHandle );

/* !NOT-PAGEABLE! */
BOOLEAN                       SLockQueuedAvailable           = FALSE;
PKEACQUIREQUEUED              SLockAcquireQueued             = NULL;
PKERELEASEQUEUED              SLockReleaseQueued             = NULL;
PKEACQUIREQUEUEDATDPCLEVEL    SLockAcquireQueuedAtDpcLevel   = NULL;
PKERELEASEQUEUEDFROMDPCLEVEL  SLockReleaseQueuedFromDpcLevel = NULL;

VOID
InitializeSmartLockSupport(
)
{
   UNICODE_STRING RoutineName;
   
#pragma warning( push )
#pragma warning( disable : 4055 )
   RtlInitStaticUnicodeString(&RoutineName,
                              L"KeAcquireInStackQueuedSpinLock");
   SLockAcquireQueued = (PKEACQUIREQUEUED)MmGetSystemRoutineAddress(&RoutineName);

   RtlInitStaticUnicodeString(&RoutineName,
                              L"KeReleaseInStackQueuedSpinLock");
   SLockReleaseQueued = (PKERELEASEQUEUED)MmGetSystemRoutineAddress(&RoutineName);
   
   RtlInitStaticUnicodeString(&RoutineName,
                              L"KeAcquireInStackQueuedSpinLockAtDpcLevel");
   SLockAcquireQueuedAtDpcLevel = (PKEACQUIREQUEUEDATDPCLEVEL)MmGetSystemRoutineAddress(&RoutineName);

   RtlInitStaticUnicodeString(&RoutineName,
                              L"KeReleaseInStackQueuedSpinLockFromDpcLevel");
   SLockReleaseQueuedFromDpcLevel = (PKERELEASEQUEUEDFROMDPCLEVEL)MmGetSystemRoutineAddress(&RoutineName);
#pragma warning( pop )

   if ( (NULL != SLockAcquireQueued) &&
        (NULL != SLockReleaseQueued) &&
        (NULL != SLockAcquireQueuedAtDpcLevel) &&
        (NULL != SLockReleaseQueuedFromDpcLevel) )
   {
      SLockQueuedAvailable = TRUE;
   }
}  

VOID
FASTCALL
KeAcquireSmartSpinLock(
   __inout PKSPIN_LOCK SpinLock,
   __out PKLOCK_QUEUE_HANDLE LockHandle
)
{
   if ( SLockQueuedAvailable )
   {
      (SLockAcquireQueued)(SpinLock,
                           LockHandle);
   }
   else
   {
      KeAcquireSpinLock(SpinLock,
                        &LockHandle->OldIrql);
   }
}

VOID
FASTCALL
KeReleaseSmartSpinLock(
   __inout PKSPIN_LOCK SpinLock,
   __in PKLOCK_QUEUE_HANDLE LockHandle
)
{
   ASSERT(DISPATCH_LEVEL == KeGetCurrentIrql());

   if ( SLockQueuedAvailable )
   {
      (SLockReleaseQueued)(LockHandle);
   }
   else
   {
      KeReleaseSpinLock(SpinLock,
                        LockHandle->OldIrql);
   }
}

VOID
KeAcquireSmartSpinLockAtDpcLevel(
   __inout PKSPIN_LOCK SpinLock,
   __out PKLOCK_QUEUE_HANDLE LockHandle
)
{
   ASSERT(DISPATCH_LEVEL == KeGetCurrentIrql());

   if ( SLockQueuedAvailable )
   {
      (SLockAcquireQueuedAtDpcLevel)(SpinLock,
                                     LockHandle);
   }
   else
   {
      KeAcquireSpinLockAtDpcLevel(SpinLock);
   }
}

VOID
KeReleaseSmartSpinLockFromDpcLevel(
   __inout PKSPIN_LOCK SpinLock,
   __in PKLOCK_QUEUE_HANDLE LockHandle
)
{
   ASSERT(DISPATCH_LEVEL == KeGetCurrentIrql());

   if ( SLockQueuedAvailable )
   {
      (SLockReleaseQueuedFromDpcLevel)(LockHandle);
   }
   else
   {
      KeReleaseSpinLockFromDpcLevel(SpinLock);
   }
}
