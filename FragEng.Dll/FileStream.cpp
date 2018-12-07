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

/* Stream.cpp
 *    FragEng NTFS Alternate Data Stream support
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "FragEngp.h"

/*++

   PUBLIC EXPORTS
   
--*/

DWORD 
FRAGAPI 
FpEnumFileStreamInformation( 
   __in LPCWSTR FileName, 
   __in DWORD Flags, 
   __callback PENUMFILESTREAMINFORMATIONPROC Callback, 
   __in_opt PVOID CallbackParameter
) NO_THROW
{
   DWORD  dwRet;
   HANDLE hFile;

   if ( !FileName || !Callback )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals */
   hFile = INVALID_HANDLE_VALUE;

   /* Try to open the file... */
   dwRet = FpiOpenFile(FileName,
                       &hFile);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   __try
   {
      dwRet = FpiEnumFileStreamInformation(hFile,
                                           Flags,
                                           Callback,
                                           CallbackParameter);
   }
   __finally
   {
      CloseHandle(hFile);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD FRAGAPI FpGetFileStreamCount( 
   __in_z LPCWSTR FileName,
   __in DWORD Flags, 
   __out PULONG StreamCount
) NO_THROW
{
   DWORD                   dwRet;
   GETFILESTREAMCOUNT_CTX  CallbackCtx;

   /* Validate paramters */
   if ( !FileName || !StreamCount )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize outputs */
   (*StreamCount) = 0;

   CallbackCtx.IsValid     = FALSE;
   CallbackCtx.StreamCount = 0;

   dwRet = FpEnumFileStreamInformation(FileName,
                                       Flags,
                                       &FpiGetFileStreamCountCallback,
                                       &CallbackCtx);

   /* If our context is valid, dwRet can be ignored */
   if ( CallbackCtx.IsValid )
   {
      (*StreamCount) = CallbackCtx.StreamCount;
      /* Success */
      return ( NO_ERROR );
   }

   /* Success / Failure */
   return ( dwRet );
}


/*++

   PRIVATE IMPLEMENTATION
   
--*/


DWORD
FpiEnumFileStreamInformation(
   __in HANDLE FileHandle,
   __in DWORD Flags, 
   __callback PENUMFILESTREAMINFORMATIONPROC Callback, 
   __in_opt PVOID CallbackParameter
) NO_THROW
{
   DWORD                    dwRet;
   ULONG                    iStreamCount;
   PFILE_STREAM_INFORMATION pStreamInformation;
   PFILE_STREAM_INFORMATION pStreamIterator;
   WCHAR                    chTermChar;
   ULONG                    cchTermChar;

   _ASSERTE(INVALID_HANDLE_VALUE != FileHandle);
   _ASSERTE(NULL != Callback);

   /* Initialize locals */
   dwRet              = NO_ERROR;
   pStreamInformation = NULL;

   __try
   {
      /* Retrieve all the streams for the file */
      dwRet = FpiQueryFileStreamInformation(FileHandle,
                                            &pStreamInformation);

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      /* Get a count of the streams */
      for ( iStreamCount = 0, pStreamIterator = pStreamInformation; pStreamIterator; pStreamIterator = RtlNextEntryFromOffset(pStreamIterator) )
      {
         iStreamCount += 1;
      }

      /* Now reiterate over the streams and pass them to the caller */
      pStreamIterator = pStreamInformation;

      /* warning C4127: conditional expression is constant	*/
      for ( pStreamIterator = pStreamInformation; pStreamIterator; pStreamIterator = RtlNextEntryFromOffset(pStreamIterator) )
      {
         /* If the caller doesn't want the default data stream, and we haven't found it already, test for it */
         if ( !FlagOn(Flags, FPF_STREAM_NODEFAULTDATASTREAM) || !IsDefaultDataStreamInformation(pStreamIterator) )
         {
            /* Ensure the stream name is null terminated. FpiQueryFileStreamInformation() guarantees that
             * we can touch this memory by allocating padding */
            cchTermChar = pStreamIterator->StreamNameLength / sizeof(WCHAR);
            chTermChar  = pStreamIterator->StreamName[cchTermChar];
            
            dwRet = (*Callback)(iStreamCount,
                                static_cast<ULONGLONG>(pStreamIterator->StreamSize.QuadPart),
                                static_cast<ULONGLONG>(pStreamIterator->StreamAllocationSize.QuadPart),
                                pStreamIterator->StreamName,
                                CallbackParameter);

            if ( NO_ERROR != dwRet )
            {
               /* Failure */
               __leave;
            }

            /* Reset whatever character was swapped out for the null term */
            pStreamIterator->StreamName[cchTermChar] = chTermChar;
         }
      }

      /* Success */
      dwRet = NO_ERROR;
   }
   __finally
   {
      FpiFreeFileStreamInformation(pStreamInformation);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
FpiQueryFileStreamInformation(
   __in HANDLE FileHandle,
   __deref_out PFILE_STREAM_INFORMATION* StreamInformation
) NO_THROW
{
   DWORD                      dwRet;
   NTSTATUS                   NtStatus;
   size_t                     cbAllocate;
   ULONG                      cbRequired;
   PFILE_STREAM_INFORMATION   pStreamInformation;

   _ASSERTE(INVALID_HANDLE_VALUE != FileHandle);
   _ASSERTE(NULL != StreamInformation);

   /* Initialize outputs */
   (*StreamInformation) = NULL;

   /* Initialize locals */
   dwRet              = NO_ERROR;
   pStreamInformation = NULL;
   cbAllocate         = sizeof(FILE_STREAM_INFORMATION);
   cbRequired         = 0;
   
   __try
   {
      #pragma warning( suppress : 4127 )
      while ( 1 )
      {
         FpiFreeFileStreamInformation(pStreamInformation);
         pStreamInformation = NULL;

         /* Grow the buffer until we get a size that works */
         cbAllocate += (MAX_PATH_STREAM * sizeof(WCHAR));
         if ( cbAllocate < cbRequired )
         {
            cbAllocate += (2 * (cbRequired - cbAllocate));
         }

         /* Ensure that we have enough space for our extra trailing bytes */
         cbAllocate += RoundUp(cbAllocate + sizeof(WCHAR), 
                               sizeof(WCHAR));
         
         /* Allocate the name buffer and initialize it */
         pStreamInformation = reinterpret_cast<PFILE_STREAM_INFORMATION>(malloc(cbAllocate));
         if ( !pStreamInformation )
         {
            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            /* Failure */
            __leave;
         }

         ZeroMemory(pStreamInformation,
                    cbAllocate);

         /* Get the stream names. Note that we guarantee at least two trailing bytes for
          * the caller to manipulate */
         NtStatus = FpiQueryInformationFile(FileHandle,
                                            FileStreamInformation,
                                            pStreamInformation,
                                            static_cast<ULONG>(cbAllocate) - sizeof(WCHAR),
                                            &cbRequired);

         if ( NT_SUCCESS(NtStatus) )
         {
            (*StreamInformation) = pStreamInformation;
            pStreamInformation = NULL;

            dwRet = NO_ERROR;
            /* Success */
            __leave;
         }

         /* The first two of these will set cbRequired, the last will expect us to grow the buffer
          * ourselves. The code above handles that */
         if ( (STATUS_BUFFER_OVERFLOW != NtStatus) && (STATUS_BUFFER_TOO_SMALL != NtStatus) && (STATUS_INFO_LENGTH_MISMATCH != NtStatus) )
         {
            dwRet = NtStatusToWin32Error(NtStatus);
            /* Failure */
            __leave;
         }
      }
   }
   __finally
   {
      FpiFreeFileStreamInformation(pStreamInformation);
   }

   /* Failure */
   return ( dwRet );
}

DWORD 
FRAGAPI 
FpiGetFileStreamCountCallback( 
   __in ULONG StreamCount, 
   __in ULONGLONG /*StreamSize*/, 
   __in ULONGLONG /*StreamSizeOnDisk*/,
   __in_z LPCWSTR /*StreamName*/, 
   __in_opt PVOID CallbackParameter
) NO_THROW
{
   PGETFILESTREAMCOUNT_CTX CallbackCtx;

   CallbackCtx = reinterpret_cast<PGETFILESTREAMCOUNT_CTX>(CallbackParameter);

   CallbackCtx->StreamCount = StreamCount;
   CallbackCtx->IsValid     = TRUE;

   /* Abort the enumeration as we have what we want now */
   return ( ERROR_CANCELLED );
}