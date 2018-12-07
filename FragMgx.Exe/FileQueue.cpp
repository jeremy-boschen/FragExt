/* FragExt - Shell extension for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2010 Jeremy Boschen. All rights reserved.
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
 
/* FileQueue.h
 *    IMgxFileQueue and CMgxFileQueue definitions
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#include "Stdafx.h"
#include "FileQueue.h"

/*++
   WPP
 *--*/
#include "FileQueue.tmh"

#ifndef AlignUp
   #define AlignUp( Size, Align ) \
      (((Size) + ((Align) - 1)) & ~((Align) - 1))      
#endif /* AlignUp */

HRESULT
CMgxFileQueue::InternalConstruct( 
) throw()
{
   DWORD dwRet;

   dwRet = NO_ERROR;

   __try {
      InitializeCriticalSection(&(_cxDataLock));
   }
   __except( STATUS_NO_MEMORY == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
   }

   if ( WINERROR(dwRet) ) {
      FpTrace(TRACE_LEVEL_ERROR,
              FpFileQueue,
              L"InitializeCriticalSectionAndSpinCount failed, error = %!WINERROR!, exiting\n",
              dwRet);

      /* Failure */
      return ( __HRESULT_FROM_WIN32(dwRet) );
   }

   /* Success */
   return ( S_OK );
}

void
CMgxFileQueue::InternalDestruct( 
)
{
   DeleteCriticalSection(&_cxDataLock);
}


/*++

	CMgxFileQueue : CMgxFileQueue

 --*/

CMgxFileQueue::CMgxFileQueue( 
)
{
   _QueueUpdateRouteine = NULL;

   ZeroMemory(&_cxDataLock,
              sizeof(_cxDataLock));
}

STDMETHODIMP
CMgxFileQueue::Initialize(
)
{
   HRESULT  hr;
}

STDMETHODIMP
CMgxFileQueue::Uninitialize(
)
{
   HRESULT  hr;
}

STDMETHODIMP
CMgxFileQueue::SetQueueUpdateRoutine( 
   __in_opt PFQ_UPDATE_ROUTINE pCallback, 
   __in_opt PVOID pParameter 
)
{
   HRESULT  hr;
}

STDMETHODIMP
CMgxFileQueue::InsertFile( 
   __in_z LPCWSTR FileName 
)
{
   HRESULT     hr;
   PQUEUEFILE  pQueueFile;

   /*
    * Create a record for this file, then append it to the tail of the list
    */
   pQueueFile = CreateQueueFileRecord(FileName,
                                      0);

   if ( !pQueueFile ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFileQueue,
              L"CreateQueueFileRecord failed, exiting\n");

      hr = E_OUTOFMEMORY;
      /* Failure */
      return ( hr );
   }

   EnterCriticalSection(&_cxDataLock); {
      InsertTailList(&_FileQueueList,
                     &(pQueueFile->Entry));
   }
   LeaveCriticalSection(&_cxDataLock);

   /*
    * Notify any registered clients that the queue has been updated
    */
   
   _PostQueueUpdate(QC_FILEINSERTED,
                    pQueFile->FileName);
}

STDMETHODIMP
CMgxFileQueue::DeleteFile( 
   __in_z LPCWSTR FileName 
)
{
   HRESULT  hr;
}

STDMETHODIMP
CMgxFileQueue::Flush( 
)
{
   HRESULT  hr;
}

CMgxFileQueue::PQUEUEFILE 
CMgxFileQueue::CreateQueueFileRecord(
   __in_z LPCWSTR FileName, 
   __in USHORT Flags 
)
{
   HRESULT     hr;
   size_t      cbLength;
   size_t      cbAllocate;
   PQUEUEFILE  pQueueFile;
   
   hr = StringCbLengthW(FileName,
                        FQ_FILENAME_MAX_CB,
                        &cbLength);

   if ( FAILED(hr) ) {
      /* Failure */
      return ( NULL );
   }

   /* 
    * This cannot overflow because we restrict the length of the string to FQ_FILENAME_MAX_CB. That 
    * plus the size of a QUEUEFILE, aligned to 8|16 is well within the limits of a size_t
    */
   cbAllocate = AlignUp((cbLength + sizeof(QUEUEFILE)),
                        MEMORY_ALLOCATION_ALIGNMENT);

   if ( cbAllocate < FQ_QUEUEFILE_MIN_CB ) {
      /*
       * To reduce memory fragmentation, we'll always allocate at least this much for a QUEUEFILE
       */
      cbAllocate = FQ_QUEUEFILE_MIN_CB;
   }

   pQueueFile = reinterpret_cast<PQUEUEFILE>(malloc(cbAllocate));
   if ( !pQueueFile ) {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFileQueue,
              L"malloc failed, cbAllocate = %I64u, exiting\n",
              cbAllocate);

      /* Failure */
      return ( NULL );
   }

   FillMemory(pQueueFile,
              cbAllocate,
              MEMORY_FILL_CHAR);
   
   /* 
    * 1 extra character in QUEUEFILE::FileName exists to account for the terminating null character 
    * so this cannot fail
    */
   hr = StringCbCopyW(pQueueFile->FileName,
                      cbLength + sizeof(WCHAR),
                      FileName);
   _ASSERTE(SUCCEEDED(hr));
   
   pQueueFile->Flags = Flags;

   /* Success */
   return ( pQueueFile );
}

void 
CMgxFileQueue::DestroyQueueFileRecord( 
   __in PQUEUEFILE QueueFile 
) throw()
{
   free(QueueFile);
}
