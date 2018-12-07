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
 
/* SpinLock.h
 *    Simple spinlock implementation that allows for reentrancy by the owning
 *    thread.
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#pragma once

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#pragma pack(push, 2)
typedef struct _SPINLOCK
{
/*0*/ volatile DWORD    ThreadID;
/*4*/ volatile USHORT   Depth;
/*6*/ USHORT            SpinCount;
/*8*/
}SPINLOCK;
#pragma pack(pop)

#define SPINLOCK_DEF_SPINCOUNT \
   4000

inline
void
InitializeSpinLock( 
   __deref_out volatile SPINLOCK* SLock,
   __in USHORT SpinCount
)
{
   SLock->ThreadID  = 0;
   SLock->Depth     = 0;
   SLock->SpinCount = SpinCount;
}

inline
void
InitializeSpinLock(
   __deref_inout volatile SPINLOCK* SLock
)
{
   InitializeSpinLock(SLock,
                      4000);
}

#define SPINLOCK_INIT( SpinCount ) \
   {static_cast<DWORD>(0U), static_cast<USHORT>(0U), static_cast<USHORT>(SpinCount)}

void 
APIENTRY
AcquireSpinLock( 
   __deref_inout volatile SPINLOCK* SLock 
);

void 
APIENTRY
ReleaseSpinLock( 
   __deref_inout volatile SPINLOCK* SLock 
);

#endif /* __SPINLOCK_H__ */