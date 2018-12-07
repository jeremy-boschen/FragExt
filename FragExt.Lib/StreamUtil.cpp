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

/* StreamUtil.cpp
 *    NTFS Alternate Data Stream support
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include <NTApi.h>
#include <StreamUtil.h>

/*++
   WPP
 *--*/
#include "StreamUtil.tmh"

/*++

   PRIVATE

 --*/

#ifndef WINERROR
   #define WINERROR( Error ) \
      (NO_ERROR != Error)
#endif /* WINERROR */

#ifndef AlignUp
   #define AlignUp( Size, Align ) \
      (((Size) + ((Align) - 1)) & ~((Align) - 1))      
#endif /* AlignUp */

typedef struct _GETFILESTREAMCOUNT_CTX
{
   BOOL  IsValid;
   ULONG StreamCount;
}GETFILESTREAMCOUNT_CTX, *PGETFILESTREAMCOUNT_CTX;

DWORD
FpiEnumFileStreamInformation(
   __in HANDLE FileHandle,
   __in DWORD Flags, 
   __callback PENUMFILESTREAMINFORMATIONROUTINE Callback, 
   __in_opt PVOID CallbackParameter
);

DWORD
FpiQueryFileStreamInformation(
   __in HANDLE FileHandle,
   __deref_out PFILE_STREAM_INFORMATION* StreamInformation
);

inline
void
FpiFreeFileStreamInformation(
   __in PFILE_STREAM_INFORMATION StreamInformation
)
{
   if ( StreamInformation )
   {
      free(StreamInformation);
   }
}

inline
BOOL
FpiIsDefaultDataStreamInformation(
   PFILE_STREAM_INFORMATION StreamInformation
)
{
   return ( (StreamInformation->StreamNameLength == 14) &&
            (StreamInformation->StreamName[0] == L':')  &&
            (StreamInformation->StreamName[1] == L':')  &&
            (StreamInformation->StreamName[2] == L'$')  &&
            (StreamInformation->StreamName[3] == L'D')  &&
            (StreamInformation->StreamName[4] == L'A')  &&
            (StreamInformation->StreamName[5] == L'T')  &&
            (StreamInformation->StreamName[6] == L'A') );
}

BOOLEAN 
APIENTRY 
FpiGetFileStreamCountCallback( 
   __in ULONG StreamCount, 
   __in ULONGLONG StreamSize,
   __in ULONGLONG StreamSizeOnDisk,
   __in_z LPCWSTR StreamName, 
   __in_opt PVOID CallbackParameter 
);

DWORD
FpiQueryInformationFile(
   __in HANDLE FileHandle,
   __in FILE_INFORMATION_CLASS FileInformationClass,
   __out_bcount(Length) PVOID FileInformation,
   __in ULONG Length,
   __out_opt PULONG ReturnLength
);

/*++

   PUBLIC EXPORTS
   
--*/

DWORD 
APIENTRY 
FpEnumFileStreamInformation( 
   __in LPCWSTR FileName, 
   __in DWORD Flags, 
   __callback PENUMFILESTREAMINFORMATIONROUTINE Callback, 
   __in_opt PVOID CallbackParameter
)
{
   DWORD  dwRet;

   DWORD  dwShareMode;
   HANDLE hFile;
   DWORD  dwFileType;

   if ( !FileName || !Callback )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   dwRet       = NO_ERROR;
   dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

   do
   {
      /* 
       * Try to open the file... 
       */
      hFile = CreateFile(FileName,
                         FILE_READ_ATTRIBUTES|SYNCHRONIZE,
                         dwShareMode,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH|SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION|SECURITY_EFFECTIVE_ONLY,
                         NULL);

      if ( INVALID_HANDLE_VALUE != hFile )
      {
         /* Success */
         break;
      }

      dwRet = GetLastError();

      if ( ERROR_SHARING_VIOLATION != dwRet )
      {         
         /* Failure */
         break;
      }

      dwShareMode >>= 1;
   }
   while ( 0 != dwShareMode );

   if ( INVALID_HANDLE_VALUE == hFile )
   {
      FpTrace(TRACE_LEVEL_ERROR,
              FpDataStream,
              L"CreateFile failed, FileName = %p, dwRet = %!WINERROR!\n",
              FileName,
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   __try
   {
      /*
       * Make sure we actually opened a file on a disk drive
       */
      dwFileType = GetFileType(hFile);

      if ( FILE_TYPE_DISK != dwFileType )
      {
         FpTrace(TRACE_LEVEL_ERROR,
                 FpDataStream,
                 L"File is not a disk file, FileName = %p, dwFileType = %u, exiting\n",
                 FileName,
                 dwFileType);

         /* Failure */
         __leave;
      }

      /*
       * Everything looks good, so try to enumerate any data streams
       */
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

DWORD 
APIENTRY 
FpGetFileStreamCount( 
   __in_z LPCWSTR FileName,
   __in DWORD Flags, 
   __out PULONG StreamCount
)
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
   __callback PENUMFILESTREAMINFORMATIONROUTINE Callback, 
   __in_opt PVOID CallbackParameter
)
{
   DWORD                    dwRet;
   ULONG                    iStreamCount;
   PFILE_STREAM_INFORMATION pStreamInformation;
   PFILE_STREAM_INFORMATION pStreamEntry;
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

      if ( WINERROR(dwRet) )
      {
         FpTrace(TRACE_LEVEL_ERROR,
                 FpDataStream,
                 L"FpiQueryFileStreamInformation failed, dwRet = %!WINERROR!, exiting\n",
                 dwRet);

         /* Failure */
         __leave;
      }

      /* 
       * Get a count of the streams 
       */
      iStreamCount = 0;

      for ( pStreamEntry = pStreamInformation; pStreamEntry; pStreamEntry = RtlNextEntryFromOffset(pStreamEntry) )
      {
         /*
          * Only count streams the caller is interested in as these will be the only ones passed to them
          */
         if ( !FlagOn(Flags, FSX_STREAM_NODEFAULTDATASTREAM) || !FpiIsDefaultDataStreamInformation(pStreamEntry) )
         {
            if ( pStreamEntry->StreamNameLength > 0 )
            {
               iStreamCount += 1;
            }
         }
      }

      FpTrace(TRACE_LEVEL_VERBOSE,
              FpDataStream,
              L"FileHandle = %p, Flags = %08lx, iStreamCount = %u, continuing\n",
              FileHandle,
              Flags,
              iStreamCount);

      /*
       * If there are no streams, there is nothing more to do
       */
      if ( 0 == iStreamCount )
      {
         dwRet = NO_ERROR;
         /* Success */
         __leave;
      }

      /* 
       * Now reiterate over the streams and pass them to the caller 
       */
      for ( pStreamEntry = pStreamInformation; pStreamEntry; pStreamEntry = RtlNextEntryFromOffset(pStreamEntry) )
      {
         /* 
          * We'll only notify the client if they either want the default data stream, or they don't and this isn't it
          */
         if ( !FlagOn(Flags, FSX_STREAM_NODEFAULTDATASTREAM) || !FpiIsDefaultDataStreamInformation(pStreamEntry) )
         {
            /*
             * Streams must have names, directories can have streams with no names and these we skip
             */
            if ( pStreamEntry->StreamNameLength > 0 )
            {
               /* 
                * Ensure the stream name is null terminated. QueryFileStreamInformation() guarantees that
                * we can modify this memory by allocating padding 
                */
               cchTermChar = pStreamEntry->StreamNameLength / sizeof(WCHAR);
               chTermChar  = pStreamEntry->StreamName[cchTermChar];
               
               if ( !(*Callback)(iStreamCount,
                                 static_cast<ULONGLONG>(pStreamEntry->StreamSize.QuadPart),
                                 static_cast<ULONGLONG>(pStreamEntry->StreamAllocationSize.QuadPart),
                                 pStreamEntry->StreamName,
                                 CallbackParameter) )
               {
                  FpTrace(TRACE_LEVEL_WARNING,
                          FpDataStream,
                          L"Callback returned FALSE, Callback = %p, exiting\n",
                          Callback);

                  dwRet = ERROR_CANCELLED;
                  /* Failure - Caller aborted */
                  __leave;
               }

               /* Reset whatever character was swapped out for the null term */
               pStreamEntry->StreamName[cchTermChar] = chTermChar;
            }
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
)
{
   DWORD                      dwRet;

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
      for ( ;; )
      {
         FpiFreeFileStreamInformation(pStreamInformation);
         pStreamInformation = NULL;

         /* 
          * Grow the buffer until we get a size that works 
          */
         cbAllocate += (MAX_PATH_STREAM * sizeof(WCHAR));
         if ( cbAllocate < cbRequired )
         {
            cbAllocate += (2 * (cbRequired - cbAllocate));
         }

         /* 
          * Ensure there is enough space for an extra wchar that FpiEnumFileStreamInformation
          * can modify
          */
         cbAllocate += AlignUp(cbAllocate + sizeof(WCHAR), 
                               MEMORY_ALLOCATION_ALIGNMENT);
         
         /* 
          * Allocate the name buffer and initialize it 
          */
         pStreamInformation = reinterpret_cast<PFILE_STREAM_INFORMATION>(malloc(cbAllocate));
         if ( !pStreamInformation )
         {
            FpTrace(TRACE_LEVEL_ERROR,
                    FpDataStream,
                    L"malloc failed, cbAllocate = %I64u, exiting\n",
                    cbAllocate);

            dwRet = ERROR_NOT_ENOUGH_MEMORY;
            /* Failure */
            __leave;
         }

         ZeroMemory(pStreamInformation,
                    cbAllocate);

         /* 
          * Get the stream names
          */
         dwRet = FpiQueryInformationFile(FileHandle,
                                         FileStreamInformation,
                                         pStreamInformation,
                                         static_cast<ULONG>(cbAllocate),
                                         &cbRequired);

         if ( !WINERROR(dwRet) )
         {
            /*
             * Assign the stream information to the caller
             */
            (*StreamInformation) = pStreamInformation;

            /*
             * Clear the local alias so cleanup code doesn't free it, now that the caller owns it
             */
            pStreamInformation = NULL;

            /* Success */
            __leave;
         }

         /* 
          * The first two of these will set cbRequired, the last will expect us to grow the buffer
          * ourselves. The code above handles all cases
          */
         if ( (ERROR_MORE_DATA != dwRet) && (ERROR_INSUFFICIENT_BUFFER != dwRet) && (ERROR_BAD_LENGTH != dwRet) )
         {
            FpTrace(TRACE_LEVEL_ERROR,
                    FpDataStream,
                    L"FpiQueryInformationFile failed, dwRet = %!WINERROR!, exiting\n",
                    dwRet);

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

BOOLEAN 
APIENTRY 
FpiGetFileStreamCountCallback(
   __in ULONG StreamCount, 
   __in ULONGLONG StreamSize, 
   __in ULONGLONG StreamSizeOnDisk,
   __in_z LPCWSTR StreamName, 
   __in_opt PVOID CallbackParameter
)
{
   PGETFILESTREAMCOUNT_CTX CallbackCtx;

   UNREFERENCED_PARAMETER(StreamSize);
   UNREFERENCED_PARAMETER(StreamSizeOnDisk);
   UNREFERENCED_PARAMETER(StreamName);

   CallbackCtx = reinterpret_cast<PGETFILESTREAMCOUNT_CTX>(CallbackParameter);

   CallbackCtx->StreamCount = StreamCount;
   CallbackCtx->IsValid     = TRUE;

   /* Abort the enumeration as we have what we want now */
   return ( FALSE );
}

DWORD
FpiQueryInformationFile(
   __in HANDLE FileHandle,
   __in FILE_INFORMATION_CLASS FileInformationClass,
   __out_bcount(Length) PVOID FileInformation,
   __in ULONG Length,
   __out_opt PULONG ReturnLength
)
{
   DWORD             dwRet;

   NTSTATUS          NtStatus;
   IO_STATUS_BLOCK   IoStatus;

   IoStatus.Pointer     = NULL;
   IoStatus.Information = 0;

   NtStatus = NtQueryInformationFile(FileHandle,
                                     &IoStatus,
                                     FileInformation,
                                     Length,
                                     FileInformationClass);

   if ( ReturnLength )
   {
      (*ReturnLength) = static_cast<ULONG>(IoStatus.Information);
   }

   dwRet = NtStatusToWin32Error(NtStatus);

   if ( !NT_SUCCESS(NtStatus) )
   {
      FpTrace(TRACE_LEVEL_ERROR,
              FpDataStream,
              L"NtQueryInformationFile failed, FileHandle = %p, NtStatus = %!STATUS!, dwRet = %!WINERROR!\n",
              FileHandle,
              NtStatus,
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   /* Success */
   return ( dwRet );
}
