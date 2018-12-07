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

/* DirEnum.cpp
 *    Directory enumeration support
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 */

#include "Stdafx.h"
#include "FragEngp.h"

#include <ThreadUtil.h>
#include <PathUtil.h>
using namespace PathUtil;

/*++

   PUBLIC EXPORTS
   
--*/

DWORD 
FRAGAPI 
FpEnumDirectoryInformation( 
   __in_z LPCWSTR DirectoryPath, 
   __in DWORD Flags, 
   __in PENUMDIRECTORYPROC Callback, 
   __in_opt PVOID CallbackParameter 
) NO_THROW
{
   DWORD             dwRet;
   PDIRECTORY_RECORD pDirectory;
   ENUMDIRECTORY_CTX EnumCtx;
   
   if ( !DirectoryPath || !Callback )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals */
   dwRet      = NO_ERROR;
   pDirectory = NULL;

   ZeroMemory(&EnumCtx,
              sizeof(EnumCtx));

   /* Setup the enum context */
   dwRet = FpiInitializeEnumDirectoryContext(&EnumCtx,
                                             Flags,
                                             Callback,
                                             CallbackParameter);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   /* Build the initial directory record for the list */
   pDirectory = FpiCreateDirectoryRecord(DirectoryPath,
                                         0);

   if ( !pDirectory )
   {
      dwRet = ERROR_NOT_ENOUGH_MEMORY;
      /* Failure */
      return ( dwRet );
   }

   /* Beyond this point we have resources to cleanup */
   __try
   {
      /* Put it up on the list. As this is the first, we do it manually and avoid the merging code */
      InsertHeadList(&(EnumCtx.DirectoryQueue),
                     &(pDirectory->Entry));

      /* Do the scan */
      dwRet = FpiEnumDirectoryContext(&EnumCtx);
   }
   __finally
   {
      FpiCleanupEnumDirectoryContext(&EnumCtx);
   }

   _CrtDumpMemoryLeaks();

   /* Success / Failure */
   return ( dwRet );
}

/*++

   PRIVATE

--*/

DWORD 
FpiInitializeEnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx,
   __in DWORD Flags, 
   __in PENUMDIRECTORYPROC Callback, 
   __in PVOID CallbackParameter 
) NO_THROW
{
   InitializeListHead(&(EnumCtx->DirectoryQueue)); 
   InitializeListHead(&(EnumCtx->SubDirectoryQueue));

   EnumCtx->Flags             = Flags;
   EnumCtx->Callback          = Callback;
   EnumCtx->CallbackParameter = CallbackParameter;   

   /* Success */
   return ( NO_ERROR );
}

void 
FpiCleanupEnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx
) NO_THROW
{
   while ( !IsListEmpty(&(EnumCtx->DirectoryQueue)) )
   {
      FpiFreeDirectoryRecord(CONTAINING_RECORD(RemoveHeadList(&(EnumCtx->DirectoryQueue)),
                                               DIRECTORY_RECORD,
                                               Entry));
   }

   while ( !IsListEmpty(&(EnumCtx->SubDirectoryQueue)) )
   {
      FpiFreeDirectoryRecord(CONTAINING_RECORD(RemoveHeadList(&(EnumCtx->SubDirectoryQueue)),
                                               DIRECTORY_RECORD,
                                               Entry));
   }
}

__declspec(restrict)
PDIRECTORY_RECORD 
FpiCreateDirectoryRecord( 
   __in_z_opt LPCWSTR DirectoryPath,
   __in USHORT Depth
) NO_THROW
{
   HRESULT           hr;
   size_t            cchLength;
   size_t            cbAllocate;
   PDIRECTORY_RECORD pRecord;

   /* Determine the length of the path */
   hr = StringCchLengthW(DirectoryPath,
                         UNICODE_STRING_MAX_CHARS + 1,
                         &cchLength);

   if ( FAILED(hr) )
   {
      /* Failure */
      return ( NULL );
   }
   
   /* The DIRECTORY_RECORD includes the terminating null char */
   cbAllocate = (sizeof(WCHAR) * cchLength) + sizeof(DIRECTORY_RECORD);

   /* If the path doesn't already end in a \, add room for one */
   if ( L'\\' != DirectoryPath[cchLength - 1] )
   {
      cbAllocate += sizeof(WCHAR);
   }

   cbAllocate = RoundUp(cbAllocate,
                        MEMORY_ALLOCATION_ALIGNMENT);
   _ASSERTE(cbAllocate > ((sizeof(WCHAR) * cchLength) + sizeof(DIRECTORY_RECORD)));
   
   pRecord = reinterpret_cast<PDIRECTORY_RECORD>(malloc(cbAllocate));
   if ( !pRecord )
   {
      /* Failure */
      return ( NULL );
   }

   ZeroMemory(pRecord,
              cbAllocate);

   pRecord->Depth = Depth;

   /* Copy the directory path, append the trailing \ and null terminator */   
   CopyMemory(&(pRecord->DirectoryPath[0]),
              DirectoryPath,
              cchLength * sizeof(WCHAR));

   if ( L'\\' != pRecord->DirectoryPath[cchLength - 1] )
   {
      pRecord->DirectoryPath[cchLength++] = L'\\';
   }

   pRecord->DirectoryPath[cchLength] = L'\0';

   /* Success / Failure */
   return ( pRecord );
}

DWORD 
FpiEnumDirectoryContext( 
   __in PENUMDIRECTORY_CTX EnumCtx 
) NO_THROW
{
   DWORD             dwRet;
   PDIRECTORY_RECORD pRecord;

   dwRet   = NO_ERROR;
   pRecord = NULL;

   while ( NULL != (pRecord = FpiPopEnumDirectoryRecord(EnumCtx)) )
   {
      __try
      {
         dwRet = FpiFindFilesDirectoryRecord(EnumCtx,
                                             pRecord);
      }
      __finally
      {
         FpiFreeDirectoryRecord(pRecord);
         pRecord = NULL;
      }

      if ( (NO_ERROR != dwRet) && (ERROR_NO_MORE_FILES != dwRet) && (ERROR_ACCESS_DENIED != dwRet) )
      {
         /* Failure */
         break; 
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD 
FpiFindFilesDirectoryRecord( 
   __in PENUMDIRECTORY_CTX EnumCtx, 
   __in PDIRECTORY_RECORD Record 
) NO_THROW
{
   DWORD                         dwRet;
   NTSTATUS                      NtStatus;
   HRESULT                       hr;

   BOOL                          fUnderMaxDepth;
   HANDLE                        hDirectory;
   PENUMDIRECTORY_INFORMATION    pEntryInformation;
   PFILE_DIRECTORY_INFORMATION   pDirInformation;
   PFILE_DIRECTORY_INFORMATION   pDirInformationEntry;
   PDIRECTORY_RECORD             pSubDirectory;
   LPWSTR                        pwszFilePart;
   size_t                        cbRemaining;

   _ASSERTE(NULL != EnumCtx);
   _ASSERTE(NULL != Record);

   /* Initialize locals */
   dwRet             = NO_ERROR;
   fUnderMaxDepth    = Record->Depth < DecodeMaxTraversalDepth(EnumCtx->Flags);
   hDirectory        = INVALID_HANDLE_VALUE;
   pEntryInformation = NULL;
   pDirInformation   = NULL;
   
   /* Allocate an ENUMDIRECTORY_INFORMATION structure which will hold the longest possible file path */
   pEntryInformation = reinterpret_cast<PENUMDIRECTORY_INFORMATION>(malloc(sizeof(ENUMDIRECTORY_INFORMATION) + UNICODE_STRING_MAX_BYTES));
   if ( !pEntryInformation )
   {
      /* Failure */
      return ( ERROR_NOT_ENOUGH_MEMORY );      
   }

   /* Beyond this point we have resources to cleanup... */
   __try
   {
      ZeroMemory(pEntryInformation,
                 sizeof(ENUMDIRECTORY_INFORMATION) + UNICODE_STRING_MAX_BYTES);

      /* Retrieve the base information for this directory */
      dwRet = FpiQueryDirectoryInformation(Record->DirectoryPath,
                                           pEntryInformation,
                                           sizeof(ENUMDIRECTORY_INFORMATION) + UNICODE_STRING_MAX_BYTES);

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      /* Setup the base file path. This cannot fail becase we don't allow a DIRECTORY_RECORD to hold
       * a path any longer than UNICODE_STRING_MAX_BYTES */
      StringCbCopyExW(&(pEntryInformation->FilePath[0]),
                      UNICODE_STRING_MAX_BYTES + sizeof(WCHAR),
                      Record->DirectoryPath,
                      &pwszFilePart,
                      &cbRemaining,
                      0);

      
      /* Post a callback for this directory */
      if ( !(*EnumCtx->Callback)(pEntryInformation,
                                 Record->Depth,
                                 EnumCtx->CallbackParameter) )
      {
         dwRet = ERROR_CANCELLED;
         /* Failure - Caller cancelled */
         __leave;
      }

      /* Open the directory we'll be querying.. */
      dwRet = FpiOpenDirectoryFile(Record->DirectoryPath,
                                   &hDirectory);

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      /* Allocate a buffer for our FILE_NAMES_INFORMATION records */
      pDirInformation = reinterpret_cast<PFILE_DIRECTORY_INFORMATION>(malloc(FPIF_ENUMDIRECTORY_NAMESBUFFER_CB));
      if ( !pDirInformation )
      {
         dwRet = ERROR_NOT_ENOUGH_MEMORY;
         /* Failure */
         __leave;
      }

      ZeroMemory(pDirInformation,
                 FPIF_ENUMDIRECTORY_NAMESBUFFER_CB);

      /* Retrieve the first set of files in the directory */
      NtStatus = FpiQueryDirectoryFile(hDirectory,
                                       NULL,
                                       FileDirectoryInformation,
                                       pDirInformation,
                                       FPIF_ENUMDIRECTORY_NAMESBUFFER_CB,
                                       NULL,
                                       FALSE,
                                       NULL,
                                       TRUE);

      while ( NT_SUCCESS(NtStatus) )
      {
         for ( pDirInformationEntry = pDirInformation; pDirInformationEntry; pDirInformationEntry = RtlNextEntryFromOffset(pDirInformationEntry) )
         {
            if ( FlagOn(pDirInformationEntry->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) )
            {
               /* Skip over . & .. entries */
               if ( ((pDirInformationEntry->FileNameLength == 2) && (L'.' == pDirInformationEntry->FileName[0])) ||
                    ((pDirInformationEntry->FileNameLength == 4) && (L'.' == pDirInformationEntry->FileName[0]) && (L'.' == pDirInformationEntry->FileName[1])) )
               {
                  continue;
               }
            }
            else 
            {
               /* If the caller only wants directories, and this isn't one, skip it */
               if ( FlagOn(EnumCtx->Flags, FPF_ENUMDIR_DIRECTORIESONLY) )
               {
                  continue;
               }
            }

            /* Append the file name to the directory that's already in the structure. This will copy to
             * the character after the directory's trailing \, and no further than what was allocated */
            hr = StringCbCopyNW(pwszFilePart,
                                cbRemaining,
                                pDirInformationEntry->FileName,
                                pDirInformationEntry->FileNameLength);

            if ( FAILED(hr) )
            {
               dwRet = ERROR_INSUFFICIENT_BUFFER;
               /* Failure */
               __leave;
            }
         
            /** 
             ** This is either a directory or a file that we need to pass to the caller
             **/
            
            /* If this is a directory and it falls under the max depth we want to put it on the queue
             * and process it later. This will allow callbacks for the directory to be posted in order */
            if ( FlagOn(pDirInformationEntry->FileAttributes, FILE_ATTRIBUTE_DIRECTORY) && fUnderMaxDepth )
            {
               pSubDirectory = FpiCreateDirectoryRecord(pEntryInformation->FilePath,
                                                        Record->Depth + 1);

               if ( !pSubDirectory )
               {
                  dwRet = ERROR_NOT_ENOUGH_MEMORY;
                  /* Failure */
                  __leave;
               }

               FpiPushEnumDirectoryRecord(EnumCtx,
                                          pSubDirectory);
            
               /* Defer posting a callback for this directory */
               continue;
            }

            /* Initialize the remaining members */
            pEntryInformation->CreationTime   = LargeIntegerToFileTime(&pDirInformationEntry->CreationTime);
            pEntryInformation->LastAccessTime = LargeIntegerToFileTime(&pDirInformationEntry->LastAccessTime);
            pEntryInformation->LastWriteTime  = LargeIntegerToFileTime(&pDirInformationEntry->LastWriteTime);
            pEntryInformation->FileSize       = static_cast<ULONGLONG>(pDirInformationEntry->EndOfFile.QuadPart);
            pEntryInformation->FileAttributes = pDirInformationEntry->FileAttributes;
            
            /* Pass this one on to the caller */
            if ( !(*EnumCtx->Callback)(pEntryInformation,
                                       Record->Depth,
                                       EnumCtx->CallbackParameter) )
            {
               dwRet = ERROR_CANCELLED;
               /* Success - Caller aborted */
               __leave;
            }
         }

         /* Get the next set of records */
         NtStatus = FpiQueryDirectoryFile(hDirectory,
                                          NULL,
                                          FileDirectoryInformation,
                                          pDirInformation,
                                          FPIF_ENUMDIRECTORY_NAMESBUFFER_CB,
                                          NULL,
                                          FALSE,
                                          NULL,
                                          FALSE);
      }

      if ( !NT_SUCCESS(NtStatus) )
      {
         dwRet = NtStatusToWin32Error(NtStatus);
      }
   }
   __finally
   {
      if ( INVALID_HANDLE_VALUE != hDirectory )
      {
         CloseHandle(hDirectory);
      }

      free(pDirInformation);
      free(pEntryInformation);

      if ( !AbnormalTermination() )
      {
         FpiMergeEnumDirectoryRecordQueues(EnumCtx);
      }
   }
   
   _ASSERTE((NO_ERROR == dwRet) || (ERROR_NO_MORE_FILES == dwRet) || (ERROR_FILE_NOT_FOUND == dwRet) || (ERROR_ACCESS_DENIED == dwRet));

   /* Success / Failure */
   return ( dwRet );
}

DWORD
FpiOpenDirectoryFile(
   __in_z LPCWSTR DirectoryPath,
   __out PHANDLE DirectoryHandle
) NO_THROW
{
   DWORD  dwRet;
   HANDLE hDirectory;

   /* Initialize outputs.. */
   (*DirectoryHandle) = INVALID_HANDLE_VALUE;

   hDirectory = CreateFile(DirectoryPath,
                           FILE_LIST_DIRECTORY|SYNCHRONIZE,
                           FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_BACKUP_SEMANTICS,
                           NULL);

   if ( INVALID_HANDLE_VALUE != hDirectory )
   {
      (*DirectoryHandle) = hDirectory;
      /* Success */
      return ( NO_ERROR );
   }

   dwRet = GetLastError();
   /* Failure */
   return ( dwRet );
}

DWORD
FpiQueryDirectoryInformation(
   __in_z LPCWSTR DirectoryPath,
   __out_bcount(InformationLength) PENUMDIRECTORY_INFORMATION DirectoryInformation,
   __in ULONG InformationLength
) NO_THROW
{
   DWORD                      dwRet;
   WIN32_FILE_ATTRIBUTE_DATA  AttributeData;

   UNREFERENCED_PARAMETER(InformationLength);

   _ASSERTE(InformationLength >= sizeof(ENUMDIRECTORY_INFORMATION));

   if ( !GetFileAttributesEx(DirectoryPath,
                             GetFileExInfoStandard,
                             &AttributeData) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   /* Populate the basic fields */
   DirectoryInformation->CreationTime   = AttributeData.ftCreationTime;
   DirectoryInformation->LastAccessTime = AttributeData.ftLastAccessTime;
   DirectoryInformation->LastWriteTime  = AttributeData.ftLastWriteTime;
   DirectoryInformation->FileSize       = (static_cast<ULONGLONG>(AttributeData.nFileSizeHigh) << 32) + AttributeData.nFileSizeLow;
   DirectoryInformation->FileAttributes = AttributeData.dwFileAttributes;

   /* Success / Failure */
   return ( NO_ERROR );
}
