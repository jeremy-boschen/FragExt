

#pragma once

/**********************************************************************
	
   Public

 **********************************************************************/
struct WAITLIST;

BOOL 
InitializeWaitList( 
   __inout WAITLIST* pWaitList 
);

VOID
DestroyWaitList(
   __in WAITLIST* pWaitList
);

VOID 
EnterWaitList( 
   __in WAITLIST* pWaitList
);

BOOL
TryEnterWaitList(
   __in WAITLIST* pWaitList
);

VOID
LeaveWaitList(
   __in WAITLIST* pWaitList
);

/**********************************************************************
	
   Private

 **********************************************************************/
#include "SpinLock.h"

typedef struct _CLIENT_ID 
{
   HANDLE UniqueProcess;
   HANDLE UniqueThread;
}CLIENT_ID, *PCLIENT_ID;

typedef struct _NT_TEB 
{
   NT_TIB      Tib;
   PVOID       EnvironmentPointer;
   CLIENT_ID   Cid;
}NT_TEB, *PNT_TEB;

struct WAITLIST
{   
   LIST_ENTRY        ListHead;
   volatile SPINLOCK ListLock;
   volatile BOOL     WaitOwned;
};

struct WAITENTRY
{
   LIST_ENTRY     Entry;
   DWORD          ThreadId;
   volatile LONG  IsOwningThread;
};

extern "C"
NTSTATUS
NTAPI
NtAlertThread(
   HANDLE ThreadHandle
);

static 
void
InitializeWaitList( 
   PWAITLIST pWaitList 
)
{
   InitializeListHead(&pWaitList->ListHead);
   InitializeSpinLock(&pWaitList->ListLock);
   
   pWaitList->WaitOwned  = FALSE;
}

static
VOID
EnterWaitList( 
   WAITLIST* pWaitList
)
{
   DWORD       dwWait;
   WAITENTRY   WaitEntry;
   HANDLE      hThread;

   /* Setup the wait entry. This will be pushed to the tail of the
    * waiters list if the resource is already owned by another thread. */
   WaitEntry.ThreadId       = GetCurrentThreadId();
   WaitEntry.IsOwningThread = FALSE;
   
   /* Enter the wait lock and try to acquire immediate ownership. If that fails,
    * append the WaitEntry to the waiters list */
   AcquireSpinLock(&pWaitList->ListLock);
   {
      if ( FALSE == pWaitList->WaitOwned )
      {
         WaitEntry.IsOwningThread = TRUE;
         pWaitList->WaitOwned     = TRUE;
      }
      else
      {
         /* Add this thread to the waiting list */
         InsertTailList(&(pWaitList->ListHead),
                        &(WaitEntry.Entry));
      }
   }
   ReleaseSpinLock(&pWaitList->ListLock);

   /* If ownership was granted, the ThreadID will have been cleared and we can
    * just return. 
    *
    * There is a chance here that another thread has cleared the ThreadID after
    * we released the wait lock.
    */
   if ( WaitEntry.IsOwningThread )
   {
      /* Success */
      return;
   }

   /* This is the waiters loop. Any thread that ends up on the waiters list will
    * enter this loop. The thread spins in this loop until its ThreadID is cleared */
   while ( true )
   {
      SleepEx(INFINITE,
              TRUE);

      /* If this thread has been cleared to run, reset the event
       * so all other threads re-enter the wait state */
      if ( WaitEntry.IsOwningThread )
      {
         /* Success */
         break;
      }

      /* This thread wasn't first on the waiters list, so it will eventually
       * be put back into the wait state when the thread that was triggered
       * calls ResetEvent(). Until then, we give up our timeslice as we know
       * it's not needed */
      SwitchToThread();
   }
}

static
BOOL 
TryEnterWaitList( 
   WAITLIST* pWaitList 
)
{
   BOOL bWaitOwned;

   AcquireSpinLock(&pWaitList->ListLock);
   {
      if ( FALSE == pWaitList->WaitOwned )
      {
         pWaitList->WaitOwned = TRUE;
         bWaitOwned           = TRUE;
      }
      else
      {
         bWaitOwned = FALSE;
      }
   }
   ReleaseSpinLock(&pWaitList->ListLock);
   
   /* Success / Failure */
   return ( bWaitOwned );
}

static
VOID
LeaveWaitList( 
   WAITLIST* pWaitList
)
{
   PLIST_ENTRY pEntry;
   WAITENTRY*  pWaiter;

   /* The thread that currently owns the resource is releasing it. First enter
    * the wait lock and if the waiters list.. */
   AcquireSpinLock(&pWaitList->ListLock);
   {
      if ( IsListEmpty(&(pWaitList->ListHead)) )
      {
         /* The waiters list is empty, so clear the ownership flag which allows the
          * next thread that attempts ownership to succeed */
         pWaitList->WaitOwned = FALSE;
         pEntry               = NULL;
      }
      else
      {
         /* There are threads waiting for ownership. The ownership flag cannot be
          * cleared because that would cause a race-condition between the time when
          * the wait lock is exited and the first entry on the waiters list is 
          * triggered to run. Instead, leave it set so that any new thread that tries
          * for ownership will be appened to the waiters list */
         pEntry = RemoveHeadList(&(pWaitList->ListHead));
      }
   }
   ReleaseSpinLock(&pWaitList->ListLock);

   /* If pEntry is NULL, the waiters list was empty so there is nothing left to do */
   if ( !pEntry )
   {
      return;
   }

   /* There was at least one thread on the waiters list. To trigger it, we will set
    * its ownership flag and signal the wait event. This will cause that thread, and 
    * any others to wake up and check their ownership flag. If it is TRUE, that thread 
    * will assume ownership and continue. Any other threads that were awoken will re-
    * enter the wait state when the new owning thread resets the wait event. */
   pWaiter = CONTAINING_RECORD(pEntry,
                               WAITENTRY,
                               Entry);
   
   pWaiter->IsOwningThread = TRUE;

   /* We know the thread we want to wake, so open a handle to it and wait it up */
   hThread = OpenThrea................ this idea sucks... give it up
   //STATUS_INVALID_HANDLE
   NtAlertThread(pWaiter->ThreadHandle);
}

