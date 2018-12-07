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

/* KeSLock.h
 *    Spin lock wrapper to enable queued-locks when available
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 12/30/2005 - Created
 *
 * Remarks
 *    The _smart_ moniker is in place so the code will be encouraged 
 *    to do amazing things on its own. So far it hasn't worked. Maybe
 *    it needs to be watered and given more sunlight?
 *
 *    Seriously, I don't know that using queued locks will offer any
 *    kind of performance gain but what the hell.
 */

#pragma once

#ifndef __KESLOCK_H__
#define __KESLOCK_H__

#include <ntddk.h>

/**********************************************************************

   KeSLock - Smart Spin Lock Wrapper

   These routines will enable the use of the most efficient spin lock 
   mechanism available on the target system.

 **********************************************************************/

/**
 * InitializeSmartLockSupport
 *    Initializes runtime data for smart spin lock support
 *
 * Parameters
 *    None
 *
 * IRQL Required
 *    PASSIVE_LEVEL
 *
 * Paging
 *    Pagable. Runs from the INIT segment.
 *    
 * Remarks
 *    This function must be called _once_, prior to any calls 
 *    being made to the XxxSmartSpinLock functions. Not doing 
 *    so will result in traditional spin lock mechanisms being
 *    used.
 */
VOID
InitializeSmartLockSupport(
);

/**
 * KeAcquireSmartSpinLock
 *    Calls either KeAcquireSpinLock or KeAcquireInStackQueuedSpinLock 
 *    depending on which is available on the target system.
 */
VOID
FASTCALL
KeAcquireSmartSpinLock(
   __inout PKSPIN_LOCK SpinLock,
   __out PKLOCK_QUEUE_HANDLE LockHandle
);

/**
 * KeAcquireSmartSpinLock
 *    Calls one of KeReleaseSpinLock or KeReleaseInStackQueuedSpinLock 
 *    depending on which is most appropriate for the target system.
 */
VOID
FASTCALL
KeReleaseSmartSpinLock(
   __inout PKSPIN_LOCK SpinLock,
   __in PKLOCK_QUEUE_HANDLE LockHandle
);

/**
 * KeAcquireSmartSpinLockAtDpcLevel
 *    Calls either KeAcquireSpinLockAtDpcLevel or KeAcquireInStackQueuedSpinLockAtDpcLevel
 *    depending on which is most appropriate for the target system.
 */
VOID
KeAcquireSmartSpinLockAtDpcLevel(
   __inout PKSPIN_LOCK SpinLock,
   __out PKLOCK_QUEUE_HANDLE LockHandle
);

/**
 * KeReleaseSmartSpinLockFromDpcLevel
 *    Calls either KeReleaseSpinLockAtDpcLevel or KeReleaseInStackQueuedSpinLockAtDpcLevel
 *    depending on which is most appropriate for the target system.
 */
VOID
KeReleaseSmartSpinLockFromDpcLevel(
   __inout PKSPIN_LOCK SpinLock,
   __in PKLOCK_QUEUE_HANDLE LockHandle
);

#endif /* __KESLOCK_H__ */