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

/* FragEng.cpp
 *    FragEng export implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 12/12/2009 - Created
 */

#include "Stdafx.h"
#include "FragEngp.h"

/*++

   SHARED SUPPORT ROUTINES

--*/

DWORD 
FRAGAPI 
FpiOpenFile( 
   __in_z LPCWSTR FileName, 
   __deref_out HANDLE* FileHandle 
) NO_THROW
{
   DWORD dwRet;
   DWORD dwAttributes;
   DWORD dwShareMode;
   DWORD dwFlags;
    
   /* Validate parameters */
   if ( !FileName || !FileHandle )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   (*FileHandle) = INVALID_HANDLE_VALUE;

   /* Initialize locals... */
   dwShareMode  = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
   dwFlags      = FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH;

   dwAttributes = GetFileAttributes(FileName);
   if ( INVALID_FILE_ATTRIBUTES == dwAttributes )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   if ( FlagOn(dwAttributes, FILE_ATTRIBUTE_DIRECTORY) )
   {
      dwFlags |= FILE_FLAG_BACKUP_SEMANTICS;
   }

   while ( 0 != dwShareMode )
   {
      (*FileHandle) = CreateFileW(FileName, 
                                  FILE_READ_ATTRIBUTES|FILE_READ_DATA|SYNCHRONIZE, 
                                  dwShareMode, 
                                  NULL,
                                  OPEN_EXISTING,
                                  dwFlags, 
                                  NULL);

      if ( INVALID_HANDLE_VALUE != (*FileHandle) )
      {
         /* Success */
         return ( NO_ERROR );
      }

      dwRet = GetLastError();
      if ( ERROR_SHARING_VIOLATION != dwRet )
      {
         /* Failure */
         return ( dwRet );
      }

      /* Failed to open the file with the current share rights, so try the next least
       * restrictive set. This will cause us to attempt opening the file in this order:
       *    1) FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
       *    2) FILE_SHARE_READ|FILE_SHARE_WRITE
       *    3) FILE_SHARE_READ
       */
      dwShareMode >>= 1;
   }

   dwRet = GetLastError();
   /* Failure */
   return ( dwRet );
}


/*++

   
   FSCTL_GET_RETRIEVAL_POINTERS HELPERS


--*/


DWORD 
FpiGetFileRetrievalPointers( 
   __in HANDLE FileHandle, 
   __deref_out_bcount(*RetrievalPointersLength) PRETRIEVAL_POINTERS_BUFFER* RetrievalPointers,
   __deref_out_opt PULONG RetrievalPointersLength
) NO_THROW
{
   DWORD                      dwRet;
   BOOL                       bRet;
   DWORD                      cbRequired;
   DWORD                      cbAllocate;
   PRETRIEVAL_POINTERS_BUFFER pRetrievalPointers;
   STARTING_VCN_INPUT_BUFFER  iVCN;

   /* Initialize outputs */
   (*RetrievalPointers) = NULL;
   if ( RetrievalPointersLength )
   {
      (*RetrievalPointersLength) = 0;
   }

   /* Initialize locals */
   dwRet      = NO_ERROR;
   cbAllocate = FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents);
   cbRequired = 0;
   pRetrievalPointers = NULL;

   __try
   {
      /* Retrieve all of the file's extent info */
      /* warning C4127: conditional expression is constant	*/
      #pragma warning ( suppress : 4127 )
      while ( 1 )
      {
         if ( pRetrievalPointers )
         {
            free(pRetrievalPointers);
            pRetrievalPointers = NULL;
         }

         /* Add room for 128 extents.. */
         cbAllocate += (sizeof(EXTENT) * 128);
         if ( cbAllocate < cbRequired )
         {
            /* We must have sent a call and didn't provide enough room, so we'll give at least
             * what we were informed to give, and some extra for safe measure */
            cbAllocate += (2 * (cbRequired - cbAllocate));
         }

         pRetrievalPointers = reinterpret_cast<PRETRIEVAL_POINTERS_BUFFER>(malloc(cbAllocate));
         if ( !pRetrievalPointers )
         {
            dwRet = ERROR_OUTOFMEMORY;
            /* Failure */
            __leave;
         }

         ZeroMemory(pRetrievalPointers,
                    cbAllocate);

         /* Request all the retrieval pointers for the file */
         iVCN.StartingVcn.QuadPart = 0;
         bRet = DeviceIoControl(FileHandle,
                                FSCTL_GET_RETRIEVAL_POINTERS,
                                &iVCN,
                                sizeof(iVCN),
                                pRetrievalPointers,
                                cbAllocate,
                                &cbRequired,
                                NULL);

         if ( bRet )
         {
            dbgDumpRetrievalPointersBuffer(pRetrievalPointers);

            /* Do not allow returning 0 extents, which occurs for files when they fit in their MFT 
             * record. When this occurs, we return a special error to signify as much.
             */
            if ( 0 == pRetrievalPointers->ExtentCount )
            {
               dwRet = ERROR_HANDLE_EOF;
               /* Failure */ 
               __leave;
            }

            /* Everything went OK, so assign the data to the caller */
            (*RetrievalPointers) = pRetrievalPointers;
            if ( RetrievalPointersLength )
            {
               (*RetrievalPointersLength) = cbRequired;
            }

            /* The caller owns this now. Reset it so it's not freed in the termination handler */
            pRetrievalPointers = NULL;

            dwRet = NO_ERROR;
            /* Success */
            __leave;
         }

         dwRet = GetLastError();
         if ( (ERROR_MORE_DATA != dwRet) && (ERROR_INSUFFICIENT_BUFFER != dwRet) )
         {
            /* Failure */
            __leave;
         }

         /* If we get here, we didn't pass a buffer large enough for all or any of
          * the data. We'll loop back around, increase the buffer size and try again */
      }
   }
   __finally
   {
      /* For failure paths, pRetrievalPointers will be valid and we must free it. For
       * success paths, this is a no-op */
      FpiFreeRetrievalPointers(pRetrievalPointers);
   }

   /* Success / Failure */
   return ( dwRet );
}
