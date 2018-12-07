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

/* FileDefrag.cpp
 *    FpDefragmentFile implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "FragEngp.h"

#include <PathUtil.h>

DWORD
FRAGAPI
FpDefragmentFile(
   __in_z LPCWSTR FilePath,
   __in DWORD Flags,
   __callback PDEFRAGMENTFILEPROC Callback, 
   __in_opt PVOID CallbackParameter
) NO_THROW

{
   DWORD    dwRet;
   HANDLE   hFile;
   
   hFile = INVALID_HANDLE_VALUE;

   /* Open the file */
   dwRet = FpiOpenFile(FilePath,
                       &hFile);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   __try
   {
      dwRet = FpDefragmentFileEx(hFile,
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
FRAGAPI
FpDefragmentFileEx(
   __in_z HANDLE FileHandle,
   __in DWORD Flags,
   __callback PDEFRAGMENTFILEPROC Callback, 
   __in_opt PVOID CallbackParameter
) NO_THROW

{
   DWORD                      dwRet;
   HANDLE                     hVolume;
   PRETRIEVAL_POINTERS_BUFFER pFragData;
   DWORD                      iIdx;
   ULONGLONG                  iVCN;
   ULONGLONG                  iLCN;
   DWORD                      iSkipCount;
   DWORD                      iBlockCount;
   ULONGLONG                  iClustersTotal;
   ULONGLONG                  iClustersExtra;
   ULONGLONG                  iClustersMoved;
   ULONGLONG                  iClustersRemaining;
   ULONGLONG                  iClustersBlock;
   ULONGLONG                  iClustersExtent;
   ULONGLONG                  iClustersCallback;
   DWORD                      iCallbackIncrement;
   PEXTENT                    pExtent;
   BOOL                       bRet;
   DWORD                      cbRet;
   MOVE_FILE_DATA             MoveData;
   int                        iAttempts;

   /* Initialize locals */
   hVolume            = INVALID_HANDLE_VALUE;
   pFragData          = NULL;
   iSkipCount         = FlagsToClusterSkipCount(Flags);
   iBlockCount        = FlagsToClusterBlockCount(Flags);
   iCallbackIncrement = FlagsToCallbackIncrement(Flags);

   if ( iBlockCount < FPIF_DEFRAG_CLUSTERBLOCKSIZE_MIN )
   {
      iBlockCount = FPIF_DEFRAG_CLUSTERBLOCKSIZE_MIN;
   }

   iBlockCount *= 16;

   if ( iCallbackIncrement < FPIF_DEFRAG_CALLBACKINCREMENT_MIN )
   {
      iCallbackIncrement = FPIF_DEFRAG_CALLBACKINCREMENT_MIN;
   }

   iCallbackIncrement *= iBlockCount;

   /* Open a handle to the file's volume */
   dwRet = FpiOpenFileVolume(FileHandle,
                             FPF_OPENBY_FILEHANDLE,
                             &hVolume);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Load the retrieval pointer data */
   dwRet = FpiGetFileRetrievalPointers(FileHandle,
                                       &pFragData,
                                       NULL);

   if ( NO_ERROR != dwRet )
   {
      /* This code is returned for files that do not have extent data, such as those that fit
       * within their MFT record. Map it to success. We'll also post a callback if the client
       * has passed us something to call */
      if ( ERROR_HANDLE_EOF == dwRet )
      {
         dwRet = NO_ERROR;

         /* Notify the caller of the update */
         if ( Callback )
         {
            dwRet = (*Callback)(0,
                                0,
                                CallbackParameter);
         }
      }

      /* Success / Failure */
      goto __CLEANUP;
   }

   _ASSERTE(NULL != pFragData);
   /* We should never get this far with an extent count of 0. This should be handled by the
    * return of ERROR_HANDLE_EOF above */
   _ASSERTE(pFragData->ExtentCount > 0);
   
   /* Update the extent data so that each extent's NextVcn is the actual number of clusters,
    * not relative to the previous extent. Also sum up the total number of clusters */
   pFragData->Extents[0].NextVcn.QuadPart -= pFragData->StartingVcn.QuadPart;
   iClustersTotal = pFragData->Extents[0].NextVcn.QuadPart;

   for ( iIdx = pFragData->ExtentCount - 1; iIdx >= 1; iIdx-- )
   {
      pFragData->Extents[iIdx].NextVcn.QuadPart -= pFragData->Extents[iIdx - 1].NextVcn.QuadPart;
      iClustersTotal += pFragData->Extents[iIdx].NextVcn.QuadPart;
   }
     
   /* Initialize the starting LCN for searching free space */
   iVCN               = static_cast<ULONGLONG>(pFragData->StartingVcn.QuadPart);
   iLCN               = 0;
   iClustersExtra     = 0;
   iClustersMoved     = 0;
   iClustersRemaining = iClustersTotal;
   pExtent            = reinterpret_cast<PEXTENT>(pFragData->Extents);
   iClustersExtent    = pExtent->ClusterCount;
   iClustersCallback  = 0;
   
   /* Post an initial callback to the client */
   if ( Callback )
   {
      dwRet = (*Callback)(iClustersTotal,
                          0,
                          CallbackParameter);

      if ( NO_ERROR != dwRet )
      {
         /* Failure - Client canceled */
         goto __CLEANUP;
      }
   }

   /* Try to defragment the file no more than 5 times.. */
   for ( iAttempts = 5; iAttempts > 0; iAttempts-- )
   {
      if ( iSkipCount > 0 )
      {
         iClustersExtra = iSkipCount * (iClustersRemaining / iBlockCount);
      }
      
      //
      //TODO: Add support for locating a free range immediately after the first allocated range
      //      of a file. For instance, if a file has 2 extents, each 2MB and there is 2MB immediately
      //      after the first extent, move the 2nd extent into that space.
      //
      iLCN  = 0;
      dwRet = FpiLocateFreeClusterRange(hVolume, 
                                        iClustersRemaining + iClustersExtra, 
                                        &iLCN);

      if ( NO_ERROR != dwRet )
      {
         /* If the cluster map was exhausted we'll remap the error to the disk being full */
         if ( ERROR_HANDLE_EOF == dwRet )
         {
            dwRet = ERROR_DISK_FULL;
         }

         /* Failure */
         goto __CLEANUP;
      }

      while ( iClustersRemaining )
      {
         MoveData.ClusterCount         = static_cast<DWORD>(min(iBlockCount, iClustersRemaining));
         MoveData.FileHandle           = FileHandle;
         MoveData.StartingVcn.QuadPart = static_cast<LONGLONG>(iVCN);
         MoveData.StartingLcn.QuadPart = static_cast<LONGLONG>(iLCN);

         bRet = DeviceIoControl(hVolume,
                                FSCTL_MOVE_FILE,
                                &MoveData,
                                sizeof(MoveData),
                                NULL,
                                0,
                                &cbRet,
                                NULL);

         if ( !bRet )
         {
            dwRet = GetLastError();            
            /* For whatever reason this move failed so break out and search for
             * a new area */
            break;
         }

         /* The virtual cluster number (VCN) will always move up by the number of clusters
          * moved, regardless of whether they were physical or compressed/sparse. The same
          * goes for the total remaining */
         iVCN               += static_cast<ULONGLONG>(MoveData.ClusterCount);
         iClustersRemaining -= static_cast<ULONGLONG>(MoveData.ClusterCount);

         /* The logical cluster number (LCN) only moves up by the number of physical clusters
          * moved. This loop calculates how many physical clusters were part of this move and
          * increments the extent data for the next iteration */
         while ( MoveData.ClusterCount > 0 )
         {
            iClustersBlock = iClustersExtent;
            if ( iClustersBlock > MoveData.ClusterCount )
            {
               iClustersBlock = MoveData.ClusterCount;
            }

            MoveData.ClusterCount -= static_cast<DWORD>(iClustersBlock);
            iClustersExtent       -= iClustersBlock;

            if ( !IsVirtualClusterNumber(pExtent->LCN) )
            {
               iClustersMoved += iClustersBlock;
               iLCN           += iClustersBlock;
            }

            if ( 0 == iClustersExtent )
            {
               pExtent        += 1;
               iClustersExtent = pExtent->ClusterCount;
            }
         }

         /* Notify the caller of the update */
         if ( Callback && ((iClustersCallback + iCallbackIncrement) <= iClustersMoved) )
         {
            dwRet = (*Callback)(iClustersTotal,
                                iClustersMoved,
                                CallbackParameter);

            /* If the callback routine has returned a failure code, then abort */
            if ( NO_ERROR != dwRet )
            {
               /* Callback failure */
               goto __CLEANUP;
            }

            iClustersCallback = iClustersMoved;
         }

         /* If we're using a skip count, adjust the LCN for the next move to account for it */
         if ( iSkipCount > 0 )
         {
            iLCN += iSkipCount;
         }
      }

      if ( 0 == iClustersRemaining )
      {
         dwRet = NO_ERROR;
         /* Success */
         goto __CLEANUP;
      }

      /* This could be our last failed attempt, so set a meaningful error code */
      dwRet = ERROR_GEN_FAILURE;
   }

__CLEANUP:
   FpiFreeRetrievalPointers(pFragData);

   if ( INVALID_HANDLE_VALUE != hVolume )
   {
      CloseHandle(hVolume);
   }

   /* Success / Failure */
   return ( dwRet );
}

#if 0
/*++

   DEFRAG PLAN

--*/

DWORD
FpiBuildFileDefragPlan(
   __in HANDLE FileHandle,
   __in DWORD Flags,
   __inout PDEFRAG_PLAN DefragPlan
) NO_THROW
{
   DWORD                      dwRet;
   HANDLE                     hVolume;
   PRETRIEVAL_POINTERS_BUFFER pRetrievalPointers;
   ULONGLONG                  FileClusterCount;

   /* Initialize locals */
   hVolume            = INVALID_HANDLE_VALUE;
   pRetrievalPointers = NULL;

   /* Get a handle to the volume */
   dwRet = FpiOpenFileVolumeHandle(FileHandle,
                                   &hVolume);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   _ASSERTE(INVALID_HANDLE_VALUE != hVolume);

   __try
   {
      /* We'll be using the file's retrieval pointers for any type of defrag, so
       * load them up. They could change while we're working, but we don't care */
      dwRet = FpiGetFileRetrievalPointers(FileHandle,
                                          &pRetrievalPointers,
                                          NULL);

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         __leave;
      }

      // determine the best place on the disk to place the file. this will depend on the
      // flags..

      dwRet = FpiLocateStartingTargetLcn(FileHandle,
                                         pRetrievalPointers,
                                         &StartingTargetLcn);

      ////
      ////  FPIF_DEFRAGPLAN_COMPACT_FULL|FPIF_DEFRAGPLAN_PLACE_LOCAL 
      ////     - this means defrag the entire file around the largest fragment
      ////
      ////  FPIF_DEFRAGPLAN_COMPACT_FULL|FPIF_DEFRAGPLAN_PLACE_OPTIMAL
      ////     - this means defrag the entire file using heuristics to place the file at the best location
      ////
      ////  FPI_DEFRAGPLAN_COMPACT_BLOCK|FPIF_DEFRAGPLAN_PLACE_LOCAL
      ////  FPI_DEFRAGPLAN_COMPACT_BLOCK|FPIF_DEFRAGPLAN_PLACE_OPTIMAL

      /* Get the total number of clusters we need for this file */
      FileClusterCount = FpiGetClusterCountFromRetrievalPointers(pRetrievalPointers);

      /* Build up the operation list for the clusters */
      dwRet = FpiBuildDefragPlanOperationList(FileHandle,
                                              Flags,
                                              pRetrievalPointers->StartingVcn.QuadPart,
                                              FileClusterCount,
                                              DefragPlan);
   }
   __finally
   {
      if ( pRetrievalPointers )
      {
         FpiFreeRetrievalPointers(pRetrievalPointers);
      }

      CloseHandle(hVolume);
   }

   /* Success / Failure */
   return ( dwRet );
}

void
FpiDestroyFileDefragPlan(
   __in PDEFRAG_PLAN DefragPlan
) NO_THROW
{
   if ( DefragPlan )
   {      
      while ( !IsListEmpty(&(DefragPlan->Operations)) )
      {
         free(CONTAINING_RECORD(RemoveTailList(&(DefragPlan->Operations)),
                                DEFRAG_OPERATION,
                                Entry));       
      }
   }
}

DWORD
FpiBuildDefragPlanOperationList(
   __in HANDLE FileHandle,
   __in DWORD Flags,
   __in LONGLONG StartingVcn,
   __in ULONGLONG ClustersToMove,
   __inout PDEFRAG_PLAN DefragPlan
) NO_THROW
{   
   DWORD             dwRet;
   ULONG             BytesPerCluster;
   DWORD             MinimumFragmentSize;
   ULONGLONG         ClustersPerOperation;
   ULONGLONG         ClustersForOperation;
   PDEFRAG_OPERATION DefragOperation;
   
   if ( FlagOn(Flags, FPIF_DEFRAGPLAN_COMPACT_FULL) )
   {
      ClustersPerOperation = ClustersToMove;
   }
   else
   {   
      /* Determine how many clusters are required to fill the minimum fragment size */
      dwRet = FpiGetFileClusterByteSize(FileHandle,
                                        &BytesPerCluster);

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         return ( dwRet );
      }

      /* Get the minimum size of each fragment we'll try to construct */
      if ( NO_ERROR != GetSettingValue(FRAGENG_SUBKEY,
                                       FRAGENG_MINIMUMFRAGMENTSIZE,
                                       GSVF_HKEY_CURRENT_USER,
                                       &MinimumFragmentSize) )
      {
         MinimumFragmentSize = FPI_DEFRAGPLAN_DEFAULT_MINIMUM_FRAGMENT_SIZE;
      }

      /* MinimumFragmentSize cannot be less than BytesPerCluster */
      if ( MinimumFragmentSize < BytesPerCluster )
      {
         MinimumFragmentSize = BytesPerCluster;
      }

      /* Now calculate how many clusters we have to move per operation to meet the minimum fragment size */
      ClustersPerOperation = static_cast<ULONGLONG>(MinimumFragmentSize / BytesPerCluster);
   }

   /* Build up the list */
   while ( ClustersToMove > 0 )
   {
      DefragOperation = reinterpret_cast<PDEFRAG_OPERATION>(malloc(sizeof(DEFRAG_OPERATION)));
      if ( !DefragOperation )
      {
         dwRet = ERROR_NOT_ENOUGH_MEMORY;
         /* Failure */
         return ( dwRet );
      }

      /* This will also set the MoveType to the default of MOVE_IN */
      ZeroMemory(DefragOperation,
                 sizeof(*DefragOperation));

      /* We either move a chunk, or what remains */
      ClustersForOperation = min(ClustersPerOperation, ClustersToMove);

      DefragOperation->StartingVcn  = static_cast<ULONGLONG>(StartingVcn) & ~0x8000000000000000I64;
      DefragOperation->ClusterCount = ClustersForOperation;

      InsertTailList(&(DefragPlan->Operations),
                     &(DefragOperation->Entry));

      /* Bump up the starting VCN and the remaining cluster count */
      StartingVcn    += ClustersForOperation;
      ClustersToMove -= ClustersForOperation;
   }

   /* Success */
   return ( NO_ERROR );
}

DWORD
FpiOpenFileVolumeHandle(
   __in HANDLE FileHandle,
   __out PHANDLE VolumeHandle
) NO_THROW
{
   DWORD  dwRet;
   WCHAR  chVolumeName[MAX_VOLUMEGUID_NAME];

   /* Get the volume's GUID name */
   dwRet = FpiQueryFileVolumeGuidName(FileHandle,
                                      chVolumeName,
                                      _countof(chVolumeName));

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   /* We need to specify an access that will force a mount of the volume */
   (*VolumeHandle) = CreateFileW(chVolumeName,
                                 FILE_GENERIC_READ,
                                 FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);

   if ( INVALID_HANDLE_VALUE == (*VolumeHandle) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   /* Success */
   return ( NO_ERROR );
}

/*++


 --*/
DWORD
FpiGetFileClusterByteSize(
   __in HANDLE FileHandle,
   __out PULONG BytesPerCluster
) NO_THROW
{
   DWORD                      dwRet;
   NTSTATUS                   NtStatus;
   FILE_FS_SIZE_INFORMATION   FsSizeInformation;

   /* Initialize outputs */
   (*BytesPerCluster) = 0;

   NtStatus = FpiQueryVolumeInformationFile(FileHandle,
                                            FileFsSizeInformation,
                                            &FsSizeInformation,
                                            sizeof(FsSizeInformation),
                                            NULL);

   if ( !NT_SUCCESS(NtStatus) )
   {
      dwRet = NtStatusToWin32Error(NtStatus);
      /* Failure */
      return ( dwRet );
   }

   (*BytesPerCluster) = (FsSizeInformation.SectorsPerAllocationUnit * FsSizeInformation.BytesPerSector);
   /* Success */
   return ( NO_ERROR );
}

ULONGLONG
FpiGetFileClusterBlockCount(
   __in HANDLE FileHandle,
   __in DWORD Flags
) NO_THROW
{

   ULONG                         BytesPerCluster;
   NTSTATUS                      NtStatus;
   FILE_COMPRESSION_INFORMATION  CompressionInfo;
   DWORD                         MinimumFragmentSize;
   ULONGLONG                     ClusterBlockCount;
   
   /* Determine how many clusters are required to fill the minimum fragment size */
   dwRet = FpiGetFileClusterByteSize(FileHandle,
                                     &BytesPerCluster);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   if ( FlagOn(Flags, FPIF_DEFRAGPLAN_COMPACT_FULL) )
   {
      /* The user wants to defragment the file into a single fragment, so we'll use the
       * file's size as the cluster block count */
      NtStatus = FpiQueryInformationFile(FileHandle,
                                         FileCompressionInformation,
                                         &CompressionInfo,
                                         sizeof(CompressionInfo),
                                         NULL);

      if ( NT_SUCCESS(NtStatus) )
      {
         ClusterBlockCount = static_cast<ULONGLONG>(CompressionInfo.CompressedFileSize.QuadPart) / static_cast<ULONGLONG>(BytesPerCluster);

         /* Success */
         return ( ClusterBlockCount );
      }

      //TODO can this ever fail? if so, revert to using the allocationsize
      _ASSERTE(NT_SUCCESS(NtStatus));
   }   
         
   /* Get the minimum size of each fragment we'll try to construct */
   if ( NO_ERROR != GetSettingValue(FRAGENG_SUBKEY,
                                    FRAGENG_MINIMUMFRAGMENTSIZE,
                                    GSVF_HKEY_CURRENT_USER,
                                    &MinimumFragmentSize) )
   {
      MinimumFragmentSize = FPI_DEFRAGPLAN_DEFAULT_MINIMUM_FRAGMENT_SIZE;
   }

   /* MinimumFragmentSize cannot be less than BytesPerCluster */
   if ( MinimumFragmentSize < BytesPerCluster )
   {
      MinimumFragmentSize = BytesPerCluster;
   }

   /* Now calculate how many clusters we have to move per operation to meet the minimum fragment size */
   ClusterBlockCount = static_cast<ULONGLONG>(MinimumFragmentSize / BytesPerCluster);

   /* Success */
   return ( ClusterBlockCount );
}

/*++

   OPTIMAL PLACMENT SUPPORT ROUTINES

--*/

DWORD
FpiLocateStartingTargetLcn(
   __in HANDLE FileHandle,
   __in DWORD Flags,
   __in PRETRIEVAL_POINTERS_BUFFER RetrievalPointers,
   __out LONGLONG* StartingTargetLcn
)
{
   // get the hint for this file

   // if it does not currently fall into that section, we have to move the 
   // file to that section


}

VOLUME_LOCATION
FpiGetFileOptimalVolumeLocation(
   HANDLE FileHandle
) NO_THROW
{

   //TODO: Add feature to map a file type (extension) to a specific location
#if 0
   // recently modified to start, kinda recently to middle, never modified to end

   FILETIME  LastAccessedTime;
   FILETIME  LastModifiedTime;
   DWORD     dwTimeDelta;
      
   if ( !GetFileTime(FileHandle,
                     NULL,
                     &LastAccessedTime,
                     &LastModifiedTime) )
   {
      /* Failure - Default to start of disk */
      return ( VolumeLocationStart );
   }

   /* Optimal placement weight
    *    0) Explicit based on filename $Mft, $Log
    *    1) Explicit based on file type - NOT IMPLEMENTED
    *    2) Last accessed time
    *    3) Last modified time
    */
   
   /* Check the last accessed time */
   if ( (LastAccessedTime.dwLowDateTime > 0) || (LastAccessedTime.dwHighDateTime > 0) )
   {
      if ( ERROR_SUCCESS != GetSettingValue(FRAGENG_SUBKEY,
                                            FRAGENG_FILEACCESSEDDELTA_FREQUENT,
                                            GSVF_HKEY_CURRENT_USER,
                                            &dwTimeDelta) )
      {
         dwTimeDelta = FPI_DEFRAGPLAN_DEFAULT_FILEACCESSED_DELTA_FREQUENT;
      }

      if ( IsExpiredFileTime(&LastAccessedTime,
                             DaysToFileTimeUnits(static_cast<ULONGLONG>(dwTimeDelta)) )
      {
         return ( VolumeLocationStart );
      }
   }

   if ( ERROR_SUCCESS != GetSettingValue(FRAGENG_SUBKEY,
                                         FRAGENG_FILEACCESSEDDELTA,
                                         GSVF_HKEY_CURRENT_USER,
                                         &dwTimeDelta) )
   {
      dwTimeDelta = FPI_DEFRAGPLAN_DEFAULT_FILEACCESSED_DELTA;
   }

   if ( IsExpiredFileTime(&LastAccessedTime,
                          TimeDelta) )
   {
      return ( VolumeLocationStart );
   }
#endif //0
   FileHandle;
   return ( VolumeLocationStart );
}

LONGLONG
FpiGetStartingLcnHintForLocation(
   HANDLE VolumeHandle,
   VOLUME_LOCATION VolumeLocation
) NO_THROW
{
   LONGLONG                      StartLcn;
   NTSTATUS                      NtStatus;
   FILE_FS_FULL_SIZE_INFORMATION SizeInformation;

   NtStatus = FpiQueryVolumeInformationFile(VolumeHandle,
                                            FileFsFullSizeInformation,
                                            &SizeInformation,
                                            sizeof(SizeInformation),
                                            NULL);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Who knows, just return 0 */
      return ( 0 );
   }

   switch ( VolumeLocation )
   {
      case VolumeLocationStart:
         StartLcn = 0;
         break;

      case VolumeLocationMiddle:
         StartLcn = SizeInformation.TotalAllocationUnits.QuadPart / 3;
         break;

      case VolumeLocationEnd:
         StartLcn = (SizeInformation.TotalAllocationUnits.QuadPart / 3) * 2;
         break;

      DEFAULT_UNREACHABLE;
   }

   return ( StartLcn );
}
#endif // 0