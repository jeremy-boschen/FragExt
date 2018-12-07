

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

struct WAITLIST
{
   HANDLE            WaitEvent;
   LIST_ENTRY        ListHead;
   volatile SPINLOCK ListLock;
   volatile BOOL     WaitOwned;
};

struct WAITENTRY
{
   LIST_ENTRY     Entry;
   volatile LONG  IsOwningThread;
};

__inline BOOL CreateWaitListEvent( PHANDLE phWaitEvent )
{
   (*phWaitEvent) = CreateEvent(NULL,
                                TRUE,
                                FALSE,
                                NULL);

   if ( !(*phWaitEvent) )
   {
      /* Failure */
      return ( FALSE );
   }

   /* Success */
   return ( TRUE );
}

static 
BOOL
InitializeWaitList( 
   PWAITLIST pWaitList 
)
{
   InitializeListHead(&pWaitList->ListHead);
   InitializeSpinLock(&pWaitList->ListLock);
   
   pWaitList->WaitOwned = FALSE;
   pWaitList->WaitEvent = CreateEvent(NULL,
                                      TRUE,
                                      FALSE,
                                      NULL);

   
   return ( NULL != pWaitList->WaitEvent ? TRUE : FALSE );
}

static
VOID
EnterWaitList( 
   WAITLIST* pWaitList
)
{
   DWORD     dwWait;
   WAITENTRY WaitEntry;
   int       iPriority;

   /* Setup the wait entry. This will be pushed to the tail of the
    * waiters list if the resource is already owned by another thread. */
   WaitEntry.IsOwningThread = FALSE;

   /* Save the current thread priority so it can be returned once we have
    * ownership. All threads that get to this point will have their priority
    * changed so that each is equal, thereby preventing one from preempting
    * all others */
   iPriority = GetThreadPriority(GetCurrentThread());

   /* Force all threads to share the same priority so there
    * is equal contention for the lock */
   SetThreadPriority(GetCurrentThread(),
                     THREAD_PRIORITY_NORMAL);
   
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
      /* Reset the thread's original priority */
      SetThreadPriority(GetCurrentThread(),
                        iPriority);

      /* Success */
      return;
   }

   /* This is the waiters loop. Any thread that ends up on the waiters list will
    * enter this loop. The thread spins in this loop until its ThreadID is cleared */
   while ( true )
   {
      /* Wait for the owning thread to signal the event. This event is a
       * manual reset event so if there is more than one thread waiting
       * on it, they will all be signaled. */
      dwWait = WaitForSingleObject(pWaitList->WaitEvent,
                                   INFINITE);

      /* If this thread has been cleared to run, reset the event
       * so all other threads re-enter the wait state */
      if ( WaitEntry.IsOwningThread )
      {
         ResetEvent(pWaitList->WaitEvent);
         /* Success */
         break;
      }

      /* This thread wasn't first on the waiters list, so it will eventually
       * be put back into the wait state when the thread that was triggered
       * calls ResetEvent(). Until then, we give up our timeslice as we know
       * it's not needed */
      SwitchToThread();
   }

   /* This thread has now taken ownership of the resource. Return its
    * priority to what it was prior to the wait and return */
   SetThreadPriority(GetCurrentThread(),
                     iPriority);
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

   SetEvent(pWaitList->WaitEvent);
}

