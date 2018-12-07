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
 
/* RWLock.h
 *    Shared Reader/Exclusive writer lock from NTDLL
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 05/24/2009 - Created
 */

#pragma once

typedef struct _RWLOCK
{
   /*
    * Used to acquire exclusive access to the resource
    */
   CRITICAL_SECTION  ExclusiveLock;
   /*
    * Used by exclusive waiters to hang until all outstanding shared
    * clients have released the resource
    */
   HANDLE            ExclusiveWaitEvent;
   /*
    * Thread ID of the thread which owns the resource exclusively
    */
   volatile DWORD    ExclusiveOwnerId;
   /*
    * Number of times the thread identified by ExOwnerId has acquired
    * the resource 
    */
   volatile USHORT   ExclusiveLockCount;
   /*
    * Number of clients with shared access to the resource
    */
   volatile USHORT   SharedLockCount;
}RWLOCK, *PRWLOCK;

DWORD
APIENTRY
InitializeRWLock(
   __out PRWLOCK Lock
);

void
APIENTRY
UninitializeRWLock(
   __inout PRWLOCK Lock
);

void
APIENTRY
AcquireRWLockExclusive(
   __inout PRWLOCK Lock
);

void
APIENTRY
ReleaseRWLockExclusive(
   __inout PRWLOCK Lock
);

void
APIENTRY
AcquireRWLockShared( 
   __inout PRWLOCK Lock
);

void
APIENTRY
ReleaseRWLockShared( 
   __inout PRWLOCK Lock
);

/*++
   
   RWXLOCK - Wrapper around slim reader/writer locks and the above reader/writer lock

 --*/
typedef union _RWXLOCK
{
   SRWLOCK  SRWLock;
   PRWLOCK  RWLock;
}RWXLOCK, *PRWXLOCK;

DWORD
APIENTRY
InitializeRWXLock(
   __out PRWXLOCK Lock
);

void
APIENTRY
UninitializeRWXLock(
   __inout PRWXLOCK Lock
);

void
APIENTRY
AcquireRWXLockExclusive(
   __inout PRWXLOCK Lock
);

void
APIENTRY
ReleaseRWXLockExclusive(
   __inout PRWXLOCK Lock
);

void
APIENTRY
AcquireRWXLockShared( 
   __inout PRWXLOCK Lock
);

void
APIENTRY
ReleaseRWXLockShared( 
   __inout PRWXLOCK Lock
);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _RTL_RESOURCE
{
    RTL_CRITICAL_SECTION Lock;
    HANDLE               SharedSemaphore;
    ULONG                SharedWaiters;
    HANDLE               ExclusiveSemaphore;
    ULONG                ExclusiveWaiters;
    LONG                 NumberActive;
    HANDLE               OwningThread;
    ULONG                TimeoutBoost;
    PVOID                DebugInfo;
}RTL_RESOURCE, *PRTL_RESOURCE;

VOID
NTAPI
RtlInitializeResource(
    __in PRTL_RESOURCE Resource
);

VOID
NTAPI
RtlDeleteResource(
    __in PRTL_RESOURCE Resource
);
  
VOID
NTAPI
RtlDumpResource(
    __in PRTL_RESOURCE Resource
);

BOOLEAN
NTAPI
RtlAcquireResourceShared(
    __in PRTL_RESOURCE Resource,
    __in BOOLEAN Wait
);

BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    __in PRTL_RESOURCE Resource,
    __in BOOLEAN Wait
);

VOID
NTAPI
RtlConvertSharedToExclusive(
    __in PRTL_RESOURCE Resource
);

VOID
NTAPI
RtlConvertExclusiveToShared(
    __in PRTL_RESOURCE Resource
);

VOID
NTAPI
RtlReleaseResource(
    __in PRTL_RESOURCE Resource
);

#ifdef __cplusplus
}; /* extern "C" { */
#endif /* __cplusplus */

inline
BOOLEAN 
NTAPI
RtlInitializeResourceEx(
   __in PRTL_RESOURCE Resource
)
{
   BOOLEAN bInitialized;

   bInitialized = TRUE;

   __try
   {
      /* RtlInitializeResource() raises an exception if it fails during
       * its initialization of the resource, so we catch it and map the
       * exception to a failure return code */
      RtlInitializeResource(Resource);
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
      bInitialized = FALSE;
   }

   if ( !bInitialized )
   {
      ZeroMemory(Resource, 
                 sizeof(Resource));
   }

   /* Success / Failure */
   return ( bInitialized );
}
