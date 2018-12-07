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

/* FragCtx.cpp
 *    Context related implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 */

#include "Stdafx.h"
#include "FragEngp.h"

/*++
   WPP
 --*/
#include "FragCtx.tmh"


/*++

   PUBLIC EXPORTS
   
--*/

DWORD 
FRAGAPI 
FpCreateContext( 
   __in LPCWSTR FileName,
   __in DWORD Flags, 
   __deref_out PHANDLE CtxHandle
) NO_THROW
{
   DWORD    dwRet;

   HANDLE   hFile;
   DWORD    dwShareMode;
   DWORD    dwFileType;
   
   /* 
    * Validate input parameters 
    */
   if ( !FileName )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, FileName = NULL, exiting\n");

      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   if ( !CtxHandle )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, CtxHandle = NULL, exiting\n");

      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize outputs */
   (*CtxHandle) = NULL;

   /* Initialize locals */
   dwRet = NO_ERROR;
   hFile = INVALID_HANDLE_VALUE; 

   /* 
    * Get a handle to the file passed to us. We will require at least read access, but we'd still
    * like to enable other applications to open the file for write and or delete. Therefore we
    * will try to request a share access mode that will allow this, and fall back to the next 
    * available combination if another application is already blocking that type of access.
    */
   dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

   do
   {
      /*
       * Try to open the file. Note that we use FILE_FLAG_BACKUP_SEMANTICS in case the file is
       * a directory, FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH to disable system caching
       * as we won't be reading any file data, and SECURITY_XXX flags that will only allow us
       * to identify the client using their effective groups/privileges
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

      /* 
       * Lower the sharing mode we're requesting and try again. This will cause us to 
       * request sharing rights in the following order..
       *
       *    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
       *    FILE_SHARE_READ|FILE_SHARE_WRITE
       *    FILE_SHARE_READ
       */
      dwShareMode >>= 1;
   }
   while ( 0 != dwShareMode );

   if ( INVALID_HANDLE_VALUE == hFile )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"CreateFile failed, FileName = %p, dwRet = %!WINERROR!, exiting\n",
              FileName,
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   /* 
    * Beyond this point we have resources to cleanup upon failure.. 
    */

   __try
   {
      /*
       * Make sure we actually opened a file on a disk drive
       */
      dwFileType = GetFileType(hFile);

      if ( FILE_TYPE_DISK != dwFileType )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpFragContext,
                 L"File is not a disk file, FileName = %p, dwFileType = %u, exiting\n",
                 FileName,
                 dwFileType);

         /* Failure */
         __leave;
      }

      /*
       * The file looks good, create the context for it
       */
      dwRet = FpCreateContextEx(hFile,
                                Flags,
                                CtxHandle);

      if ( WINERROR(dwRet) )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpFragContext,
                 L"FpCreateContextEx failed, hFile = %p, Flags = %08lx, CtxHandle = %p, dwRet = %!WINERROR!, exiting\n",
                 hFile,
                 Flags,
                 CtxHandle,
                 dwRet);
      }
   }
   __finally
   {
      CloseHandle(hFile);
   }

   /* Success / Failure */
   return ( dwRet );
}


DWORD 
FRAGAPI 
FpCreateContextEx( 
   __in HANDLE FileHandle,
   __in DWORD Flags, 
   __deref_out PHANDLE CtxHandle
) NO_THROW
{
   DWORD         dwRet;
   
   PFRAG_CONTEXT pFragCtx;

   /* 
    * Validate input parameters
    */
   if ( INVALID_HANDLE_VALUE == FileHandle )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, FileHandle = INVALID_HANDLE_VALUE, exiting\n");

      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   if ( !CtxHandle )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, CtxHandle = NULL, exiting\n");

      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize outputs */
   (*CtxHandle) = NULL;

   /* Initialize locals */
   pFragCtx = NULL;

   /*
    * Build the context 
    */
   dwRet = FpiCreateFragCtx(FileHandle,
                            Flags,
                            &pFragCtx);

   if ( WINERROR(dwRet) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"FpiCreateFragCtx failed, FileHandle = %p, Flags = %08lx, &pFragCtx = %p, dwRet = %!WINERROR!, exiting\n",
              FileHandle,
              Flags,
              &pFragCtx,
              dwRet);

      /* Failure */
      return ( dwRet );
   }

   dbgDumpFragCtx(pFragCtx);

   /* 
    * Everything went cool, so give the context to the caller 
    */
   (*CtxHandle) = FragCtxToHandle(pFragCtx);

   /* Success */
   return ( NO_ERROR );
}

VOID 
FRAGAPI 
FpCloseContext( 
   __in HANDLE CtxHandle 
) NO_THROW
{
   PFRAG_CONTEXT pFragCtx;

   /* 
    * Validate parameters 
    */
   if ( !IsValidFragCtxHandle(CtxHandle) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, CtxHandle = %p, exiting\n",
              CtxHandle);

      SetLastError(ERROR_INVALID_PARAMETER);
      /* Failure */
      return;
   }

   /* Initialize locals... */
   pFragCtx = HandleToFragCtx(CtxHandle);
   
   FpiDestroyFragCtx(pFragCtx);

   /* Success */
   SetLastError(NO_ERROR);
}

DWORD 
FRAGAPI 
FpGetContextInformation( 
   __in HANDLE CtxHandle, 
   __out PFRAGCTX_INFORMATION CtxInfo 
) NO_THROW
{
   PFRAG_CONTEXT pFragCtx;

   /* 
    * Validate parameters 
    */
   if ( !IsValidFragCtxHandle(CtxHandle) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, CtxHandle = %p, exiting\n",
              CtxHandle);

      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   if ( !CtxInfo )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, CtxInfo = NULL, exiting\n");
      
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals */
   pFragCtx = HandleToFragCtx(CtxHandle);

   /* 
    * Copy fixed size members... 
    */
   CtxInfo->FileSize       = pFragCtx->FileSize;
   CtxInfo->FileSizeOnDisk = pFragCtx->FileSizeOnDisk;
   CtxInfo->ClusterCount   = pFragCtx->ClusterCount;
   CtxInfo->FragmentCount  = pFragCtx->FragmentCount;
   CtxInfo->ExtentCount    = pFragCtx->ExtentCount;
   CtxInfo->ClusterSize    = pFragCtx->ClusterSize;
   
   /* Success */
   return ( NO_ERROR );
}

DWORD
FRAGAPI
FpGetContextFragmentInformation(
   __in HANDLE CtxHandle,
   __in ULONG StartingSequenceId,
   __out_ecount_part(FragmentInformationCount, *FragmentsReturned) PFRAGMENT_INFORMATION FragmentInformation,
   __in ULONG FragmentInformationCount,
   __out_opt PULONG FragmentsReturned
) NO_THROW
{
   PFRAG_CONTEXT pFragCtx;
   ULONG         ceRemaining;
   ULONG         ceCopied;

   /* Validate parameters */
   if ( !IsValidFragCtxHandle(CtxHandle) )
   {
       FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, CtxHandle = %p, exiting\n",
              CtxHandle);

      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   if ( !FragmentInformation )
   {
       FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, FragmentInformation = NULL, exiting\n");

       /* Failure */
       return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize outputs */
   if ( FragmentsReturned )
   {
      (*FragmentsReturned) = 0;
   }

   /* Initialize locals */
   pFragCtx = HandleToFragCtx(CtxHandle);

   /* 
    * Ensure the fragment exists. Sequence IDs are 1 based, so they are always less than or equal to
    * the fragment count 
    */
   if ( (StartingSequenceId == 0) || (StartingSequenceId > pFragCtx->FragmentCount) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"StartingSequenceId is out of range, StartingSequenceId = %u, pFragCtx->FragmentCount = %u, exiting\n",
              StartingSequenceId,
              pFragCtx->FragmentCount);

      /* Failure */
      return ( ERROR_INVALID_INDEX );
   }

   _ASSERTE(NULL != pFragCtx->FragmentInformation);

   /*
    * Determine how many FRAGMENT_INFORMATION elements we can actually copy... 
    */
   ceRemaining = (pFragCtx->FragmentCount - StartingSequenceId + 1);
   ceCopied    = min(ceRemaining, FragmentInformationCount);

   /* 
    * Copy what we can 
    */
   CopyMemory(&(FragmentInformation[0]),
              &(pFragCtx->FragmentInformation[StartingSequenceId - 1]),
              ceCopied * sizeof(FRAGMENT_INFORMATION));
   
   if ( FragmentsReturned )
   {
      (*FragmentsReturned) = ceCopied;
   }

   /* 
    * Zero out the first element we're not populating. We do this because FragmentsReturned is optional and
    * a caller can identify the valid set by enumerating the returned elements until an empty one is found
    */
   if ( ceCopied < FragmentInformationCount )
   {
      ZeroMemory(&(FragmentInformation[ceCopied]),
                 sizeof(FRAGMENT_INFORMATION));
   }

   /* Success */
   return ( ceCopied < ceRemaining ? ERROR_MORE_DATA : NO_ERROR );
}

DWORD 
FRAGAPI 
FpEnumContextFragmentInformation( 
   __in HANDLE CtxHandle, 
   __callback PENUMFILEFRAGMENT_INFORMATIONPROC Callback, 
   __in_opt PVOID CallbackParameter 
) NO_THROW
{
   DWORD          dwRet;
   PFRAG_CONTEXT  pFragCtx;
   ULONG          iIdx;
   ULONG          iFragmentCount;

   /* Validate in/out parameters */
   if ( !IsValidFragCtxHandle(CtxHandle) )
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, CtxHandle = %p, exiting\n",
              CtxHandle);

      /* Failure */
      return ( ERROR_INVALID_HANDLE );
   }

   if ( !Callback ) 
   {
      FpTrace(TRACE_LEVEL_WARNING,
              FpFragContext,
              L"InvalidParameter, Callback = NULL, exiting\n");

      /* Failure */
      return ( ERROR_INVALID_HANDLE );
   }

   /* Initialize locals */
   dwRet    = NO_ERROR;
   pFragCtx = HandleToFragCtx(CtxHandle);
   
   for ( iIdx = 0, iFragmentCount = pFragCtx->FragmentCount; iIdx < iFragmentCount; iIdx++ )
   {
      /* Pass this one on to the caller */
      if ( !(*Callback)(CtxHandle,
                        &(pFragCtx->FragmentInformation[iIdx]),
                        CallbackParameter ) )
      {
         FpTrace(TRACE_LEVEL_WARNING,
                 FpFragContext,
                 L"Callback returned FALSE, Callback = %p, CtxHandle = %p, exiting\n",
                 Callback,
                 CtxHandle);

         dwRet = ERROR_CANCELLED;
         /* Failure - Caller aborted */
         break;
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

/*++

   PRIVATE IMPLEMENTATION
   
--*/


DWORD 
FpiCreateFragCtx( 
   __in HANDLE hFile, 
   __in DWORD dwFlags,
   __deref_out PFRAG_CONTEXT* ppFragCtx 
)
{
   DWORD                         dwRet;
   FILE_FS_FULL_SIZE_INFORMATION VolumeInfo;
   WCHAR                         chFilePath[MAX_PATH_GUID];
   size_t                        cbFilePath;
   size_t                        cbAllocate;
   PFRAG_CONTEXT                 pFragCtx;
   ULONG                         iIdx;
   
   _ASSERTE(INVALID_HANDLE_VALUE != hFile);
   _ASSERTE(NULL != ppFragCtx);

   /* Initialize outputs */
   (*ppFragCtx) = NULL;

   /* Initialize locals... */
   pFragCtx = NULL;

   ZeroMemory(&VolumeInfo,
              sizeof(VolumeInfo));

   ZeroMemory(chFilePath,
              sizeof(chFilePath));

   /* Retrieve the metrics for the volume's cluster size */
   dwRet = FpiQueryVolumeInformationFile(hFile,
                                         FileFsFullSizeInformation,
                                         &VolumeInfo,
                                         sizeof(VolumeInfo),
                                         NULL);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }
   
#if 0
   /* Get the full path for this file so it can be opened later if necessary */
   dwRet = FpiGetFullFilePath(hFile,
                              chFilePath,
                              _countof(chFilePath));

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }
#endif

   /* Allocate and initialize the context record */
   cbFilePath = wcslen(chFilePath) * sizeof(WCHAR);
   cbAllocate = sizeof(FRAG_CONTEXT) + cbFilePath + sizeof(WCHAR);

   pFragCtx = reinterpret_cast<PFRAG_CONTEXT>(malloc(cbAllocate));
   if ( !pFragCtx )
   {
      dwRet = ERROR_OUTOFMEMORY;
      /* Failure */
      return ( dwRet );
   }

   /* Beyond this point we have resources to cleanup.. */
   __try
   {
      ZeroMemory(pFragCtx,
                 cbAllocate);
      
      pFragCtx->Flags       = dwFlags;
      pFragCtx->ClusterSize = VolumeInfo.BytesPerSector * VolumeInfo.SectorsPerAllocationUnit;

      /* Load the fragment data, in raw format */
      dwRet = FpiGetFragmentInformation(hFile,
                                        &(pFragCtx->FragmentInformation),
                                        &(pFragCtx->FragmentCount));

      /* FpiGetFragmentInformation() will return ERROR_HANDLE_EOF when there are no fragments for the file */
      if ( (NO_ERROR != dwRet) && (ERROR_HANDLE_EOF != dwRet) )
      {
         /* Failure */
         __leave;
      }

      /* We keep a total of the extents, clusters and size of the file, so calculate those now */
      for ( iIdx = 0; iIdx < pFragCtx->FragmentCount; iIdx++ )
      {
         pFragCtx->ExtentCount  += pFragCtx->FragmentInformation[iIdx].ExtentCount;
         pFragCtx->ClusterCount += pFragCtx->FragmentInformation[iIdx].ClusterCount;
         
         /* The FileSizeOnDisk field is a representation of actual disk storage, based on the clusters.
          * so we only include these clusters if they are not part of a compression unit */
         if ( !IsCompressionFragment(&(pFragCtx->FragmentInformation[iIdx])) )
         {
            pFragCtx->FileSizeOnDisk += (static_cast<ULONGLONG>(pFragCtx->ClusterSize) * pFragCtx->FragmentInformation[iIdx].ClusterCount);
         }
      }

      /* The FileSize field is the uncompressed file size, based on the total number of clusters in the file */
      pFragCtx->FileSize = static_cast<ULONGLONG>(pFragCtx->ClusterSize) * pFragCtx->ClusterCount;

      /* If the caller doesn't want raw extent data, compact the fragment array */
      if ( !FlagOn(dwFlags, FPF_CTX_RAWEXTENTDATA) && pFragCtx->FragmentCount )
      {
         dwRet = FpiCompactFragmentInformation(&(pFragCtx->FragmentInformation),
                                               &(pFragCtx->FragmentCount));

         if ( NO_ERROR != dwRet )
         {
            /* Failure */
            __leave;
         }
      }

      /* Save the file's path so we can reopen it if necessary later on */
      CopyMemory(&(pFragCtx->FilePath[0]),
                 chFilePath,
                 cbFilePath);
      pFragCtx->FilePath[cbFilePath / sizeof(WCHAR)] = L'\0';

      /* Give the context to the caller */
      (*ppFragCtx) = pFragCtx;
      pFragCtx = NULL;
      
      /* Success */
      dwRet = NO_ERROR;
   }
   __finally
   {
      FpiDestroyFragCtx(pFragCtx);
   }

   /* Success / Failure */
   return ( dwRet );
}

void 
FpiDestroyFragCtx( 
   __in PFRAG_CONTEXT pFragCtx
) NO_THROW
{  
   if ( pFragCtx )
   {
      /* Free the fragment info, if there is any */
      FpiFreeFragmentInformation(pFragCtx->FragmentInformation);

      free(pFragCtx);
   }
}

DWORD 
FpiGetFragmentInformation( 
   __in HANDLE FileHandle, 
   __deref_out_ecount(*FragmentInformationCount) PFRAGMENT_INFORMATION* FragmentInformation, 
   __deref_out ULONG* FragmentInformationCount
) NO_THROW
{
   DWORD                      dwRet;
   size_t                     cbAllocate;
   PRETRIEVAL_POINTERS_BUFFER pRetrievalPointers;
   ULONG                      iFragCount;
   PFRAGMENT_INFORMATION      pFragInfo;
   LONGLONG                   iVCN;
   ULONG                      iIdx;

   _ASSERTE(INVALID_HANDLE_VALUE != FileHandle);
   _ASSERTE(NULL != FragmentInformation);
   _ASSERTE(NULL != FragmentInformationCount);

   /* Initialize outputs */
   (*FragmentInformation)      = NULL;
   (*FragmentInformationCount) = 0;

   /* Initialize locals */
   pRetrievalPointers = NULL;
   iFragCount         = 0;
   pFragInfo          = NULL;

   /* Get the full set of extent data for the file */
   dwRet = FpiGetFileRetrievalPointers(FileHandle,
                                       &pRetrievalPointers,
                                       NULL);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   } 

   /* Beyond this point we have resources to cleanup... */
   __try
   {
      /* At this point, all of the extent info for the file has been retrieved */
      iFragCount = pRetrievalPointers->ExtentCount;
      cbAllocate = sizeof(FRAGMENT_INFORMATION) * iFragCount;

      pFragInfo = reinterpret_cast<PFRAGMENT_INFORMATION>(malloc(cbAllocate));
      if ( !pFragInfo )
      {
         dwRet = ERROR_OUTOFMEMORY;
         /* Failure */
         __leave;
      }

      ZeroMemory(pFragInfo,
                 cbAllocate);

      for ( iIdx = 0, iVCN = pRetrievalPointers->StartingVcn.QuadPart; iIdx < iFragCount; iIdx++ )
      {
         pFragInfo[iIdx].Sequence       = iIdx + 1;
         pFragInfo[iIdx].ExtentCount    = 1;
         pFragInfo[iIdx].ClusterCount   = static_cast<ULONGLONG>(pRetrievalPointers->Extents[iIdx].NextVcn.QuadPart - iVCN);
         pFragInfo[iIdx].LogicalCluster = static_cast<ULONGLONG>(pRetrievalPointers->Extents[iIdx].Lcn.QuadPart);

         iVCN = pRetrievalPointers->Extents[iIdx].NextVcn.QuadPart;
      }

      (*FragmentInformation) = pFragInfo;
      (*FragmentInformationCount) = iFragCount;
      pFragInfo = NULL;
      
      /* Success */
      dwRet = NO_ERROR;
   }
   __finally
   {
      FpiFreeFragmentInformation(pFragInfo);
      FpiFreeRetrievalPointers(pRetrievalPointers);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD 
FpiCompactFragmentInformation( 
   __deref_inout_ecount(*FragmentInformation) PFRAGMENT_INFORMATION* FragmentInformation, 
   __deref_inout ULONG* FragmentInformationCount
) NO_THROW
{
   DWORD                   dwRet;
   PFRAGMENT_INFORMATION   pFragInfo;
   ULONG                   iIdx;
   ULONG                   iJdx;
   ULONG                   iFragCount;
   ULONG                   iSequence;
   ULONGLONG               iLCN;

   PFRAGMENT_INFORMATION   pCallerInfo;
   ULONG                   iCallerCount;

   /* Initialize locals.. */
   dwRet        = NO_ERROR;
   pFragInfo    = NULL;

   /* Swap out the caller's data because we always reset it */
   pCallerInfo  = (*FragmentInformation);
   iCallerCount = (*FragmentInformationCount);

   (*FragmentInformation)      = NULL;
   (*FragmentInformationCount) = 0;

   /* Beyond this point, we have resources that we must cleanup */
   __try
   {
      dbgDumpFragmentInformation(pCallerInfo, 
                          iCallerCount);

      /* Walk the fragment data and join any fragments that belong together. This happens with compressed, sparse files */
      for ( iFragCount = 0, iIdx = 0; iIdx < iCallerCount; iIdx++ )
      {
         /* Ignore compressed/sparse extents */
         if ( IsCompressionFragment(&(pCallerInfo[iIdx])) )
         {
            continue;
         }

         /* This is a keeper... */
         iFragCount += 1;

         /* Calculate the LCN for where an adjacent fragment would start */
         iLCN = pCallerInfo[iIdx].LogicalCluster + pCallerInfo[iIdx].ClusterCount;
         
         /* Scan the rest of the array for adjacent fragments. The only cases where this should work
          * are when the file is compressed/sparse and extents get added to reflect compression units
          *
          * Note that we start at the next element, and for each iteration we update iJdx and the
          * outer loops counter, iIdx. This is because once we process an element here, the outer
          * loop shouldn't see it
          */
         for ( iJdx = iIdx + 1; iJdx < iCallerCount; iJdx++, iIdx++ )
         {
            /* Ignore compressed/sparse extents */
            if ( IsCompressionFragment(&(pCallerInfo[iJdx])) )
            {
               continue;
            }

            if ( iLCN != pCallerInfo[iJdx].LogicalCluster )
            {
               /* This is a valid fragment and the next virtual cluster in the file, but it's
                * not adjacent to the last one so we have to finish here */
               break;
            }

            /* Don't want to see something we already processed */
            _ASSERTE(0 != pCallerInfo[iJdx].Sequence);

            /* This fragment is adjacent to the previous one, so merge the two */
            pCallerInfo[iIdx].ExtentCount  += pCallerInfo[iJdx].ExtentCount;
            pCallerInfo[iIdx].ClusterCount += pCallerInfo[iJdx].ClusterCount;

            /* Recalculate the LCN for the next adjacent fragment should start */
            iLCN += pCallerInfo[iJdx].ClusterCount;
            
            /* Mark this fragment as processed so we know to ignore it when we compact the array later */
            pCallerInfo[iJdx].Sequence = 0;
         }
      }

      if ( iFragCount == iCallerCount )
      {
         /* We didn't compact anything, so there's no need to create a new array. Give the data
          * back to the caller */
         (*FragmentInformation)      = pCallerInfo;
         (*FragmentInformationCount) = iCallerCount;

         /* The caller owns this now. Reset it so it's not freed in the termination handler */
         pCallerInfo = NULL;
         
         dwRet = NO_ERROR;
         /* Success */
         __leave;
      }

      /* We were able to compact the array, but we'll have holes with compressed and/or unused
       * fragments so we need to create an array without them */

      /* Allocate a new array which we'll build using the old one */
      pFragInfo = reinterpret_cast<PFRAGMENT_INFORMATION>(malloc(sizeof(FRAGMENT_INFORMATION) * iFragCount));
      if ( !pFragInfo )
      {
         dwRet = ERROR_NOT_ENOUGH_MEMORY;
         /* Failure */
         __leave;
      }

      ZeroMemory(pFragInfo,
                 sizeof(FRAGMENT_INFORMATION) * iFragCount);

      /* Copy the valid fragments to the new array 
       *
       * iIdx index the old data 
       * iJdx index the new data
       *
       * We are regenerating the sequence ids for thes fragments,
       * and that starts at 1
       */
      for ( iIdx = 0, iJdx = 0, iSequence = 1; (iIdx < iCallerCount) && (iJdx < iFragCount); iIdx++ )
      {
         /* Skip compression fragments and ones we marked to be skipped */
         if ( !IsCompressionFragment(&(pCallerInfo[iIdx])) && (0 != pCallerInfo[iIdx].Sequence) )
         {
            pFragInfo[iJdx].Sequence       = iSequence;
            pFragInfo[iJdx].ExtentCount    = pCallerInfo[iIdx].ExtentCount;
            pFragInfo[iJdx].ClusterCount   = pCallerInfo[iIdx].ClusterCount;
            pFragInfo[iJdx].LogicalCluster = pCallerInfo[iIdx].LogicalCluster;

            /* Update our new sequence and new index for the next new data element */
            iSequence += 1;
            iJdx      += 1;
         }
      }

      dbgDumpFragmentInformation(pFragInfo,
                          iFragCount);

      /* Everything went OK, so give the caller the new data */
      (*FragmentInformation)      = pFragInfo;
      (*FragmentInformationCount) = iFragCount;
      
      /* The caller owns the new data now. Reset this so it's not freed in the termination handler*/
      pFragInfo = NULL;
   }
   __finally
   {
      /* We always attempt to free the original data. For certain code paths this ends
       * up being a no-op. Same goes for new data */
      FpiFreeFragmentInformation(pCallerInfo);
      FpiFreeFragmentInformation(pFragInfo);
   }
   
   /* Success / Failure */
   return ( dwRet );
}

void
FpiSortFragmentInformation(
   __in FRAGMENTINFORMATION_SORTTYPE SortType,
   __inout_ecount(InformationCount) PFRAGMENT_INFORMATION FragmentInformation,
   __in ULONG InformationCount
) NO_THROW
{
   PCOMPAREROUTINE CompareRoutine[] = {
      &FpiFragmentInformationCompareSequence,
      &FpiFragmentInformationCompareExtentCount,
      &FpiFragmentInformationCompareClusterCount,
      &FpiFragmentInformationCompareLogicalCluster
   };

   if ( SortType < FragmentSortTypeInvalid )
   {
      qsort(FragmentInformation,
            InformationCount,
            sizeof(*FragmentInformation),
            CompareRoutine[SortType]);
   }
}

int
__cdecl
FpiFragmentInformationCompareSequence(
   const void* Element1,
   const void* Element2
) NO_THROW
{
   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->Sequence < reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->Sequence )
   {
      return ( -1 );
   }

   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->Sequence > reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->Sequence )
   {
      return ( 1 );
   }

   return ( 0 );
}

int
__cdecl
FpiFragmentInformationCompareExtentCount(
   const void* Element1,
   const void* Element2
) NO_THROW
{
   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->ExtentCount < reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->ExtentCount )
   {
      return ( -1 );
   }

   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->ExtentCount > reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->ExtentCount )
   {
      return ( 1 );
   }

   /* Fallback to sorting by sequence, as these two elements are equal */
   return ( FpiFragmentInformationCompareSequence(Element1,
                                                  Element2) );
}

int
__cdecl
FpiFragmentInformationCompareClusterCount(
   const void* Element1,
   const void* Element2
) NO_THROW
{
   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->ClusterCount < reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->ClusterCount )
   {
      return ( -1 );
   }

   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->ClusterCount > reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->ClusterCount )
   {
      return ( 1 );
   }

   /* Fallback to sorting by sequence, as these two elements are equal */
   return ( FpiFragmentInformationCompareSequence(Element1,
                                                  Element2) );
}

int
__cdecl
FpiFragmentInformationCompareLogicalCluster(
   const void* Element1,
   const void* Element2
) NO_THROW
{
   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->LogicalCluster < reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->LogicalCluster )
   {
      return ( -1 );
   }

   if ( reinterpret_cast<PCFRAGMENT_INFORMATION>(Element1)->LogicalCluster > reinterpret_cast<PCFRAGMENT_INFORMATION>(Element2)->LogicalCluster )
   {
      return ( 1 );
   }

   /* Fallback to sorting by sequence, as these two elements are equal */
   return ( FpiFragmentInformationCompareSequence(Element1,
                                                  Element2) );
}