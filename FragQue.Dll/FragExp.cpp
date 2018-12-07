/* FragExt - Shell extension for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2006 Jeremy Boschen. All rights reserved.
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

/* FragExp.h
 *    FragEng export implementations
 *
 * Copyright (C) 2004-2006 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 12/12/2006 - Created
 */

#include "Stdafx.h"
#include "FragExp.h"

/**********************************************************************

   Public

 **********************************************************************/
DWORD FRAGAPI FpOpenFile( LPCWSTR FileName, HANDLE* FileHandle ) throw()
{
   DWORD  dwErr;
   DWORD  dwShareMode;
   DWORD  dwAccessMode;
   DWORD  dwAttributes;
   
   /* Validate parameters */
   if ( !FileName || !FileHandle )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   (*FileHandle) = INVALID_HANDLE_VALUE;

   /* Initialize locals... */
   dwShareMode  = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;
   dwAccessMode = FILE_READ_ATTRIBUTES;

   SetLastError(NO_ERROR);

   dwAttributes = GetFileAttributes(FileName);
   if ( INVALID_FILE_ATTRIBUTES == dwAttributes )
   {
      dwErr = GetLastError();
      /* Failure */
      return ( dwErr );
   }

   if ( (FILE_ATTRIBUTE_ENCRYPTED & dwAttributes) && !_IsWinNT(MAKELONG(5,1)) )
   {
      dwAccessMode |= FILE_READ_DATA;
   }

   while ( 0 != dwShareMode )
   {
      (*FileHandle) = CreateFileW(FileName, 
                                  dwAccessMode, 
                                  dwShareMode, 
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_NO_BUFFERING|FILE_FLAG_BACKUP_SEMANTICS, 
                                  NULL);

      if ( INVALID_HANDLE_VALUE != (*FileHandle) )
      {
         dwErr = NO_ERROR;
         /* Success */
         return ( dwErr );
      }

      /* Failed to open the file with the current share rights, so try the 
       * next least restrictive set */
      dwShareMode >>= 1;
   }

   dwErr = GetLastError();
   /* Failure */
   return ( dwErr );
}

DWORD FRAGAPI FpOpenContext( LPCWSTR FileName, DWORD Flags, HANDLE* hFragCtx ) throw()
{
   DWORD          dwErr;
   DWORD          dwAttr;
   size_t         cchFile;
   size_t         cbData;
   LARGE_INTEGER  cbFile;
   ULARGE_INTEGER cbFileOnDisk;
   PFRAG_CONTEXT  pFragCtx;

   UNREFERENCED_PARAMETER(Flags);

   /* Initialize locals... */
   pFragCtx = NULL;

   /* Validate input parameters and initialize output parameters */
   if ( !FileName || !hFragCtx )
   {
      dwErr = ERROR_INVALID_PARAMETER;
      /* Failure */
      goto __CLEANUP;
   }

   (*hFragCtx) = NULL;

   dwAttr = GetFileAttributes(FileName);
   if ( INVALID_FILE_ATTRIBUTES == dwAttr )
   {
      dwErr = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   /* Allocate and initialize the context record */
   cchFile = wcslen(FileName);
   _ASSERTE(cchFile > 0);

   cbData   = (cchFile * sizeof(WCHAR)) + sizeof(FRAG_CONTEXT);
   pFragCtx = reinterpret_cast<PFRAG_CONTEXT>(new BYTE[cbData]);
   if ( !pFragCtx )
   {
      dwErr = ERROR_OUTOFMEMORY;
      /* Failure */
      return ( dwErr );
   }

   ZeroMemory(pFragCtx,
              cbData);

   /* Attempt to open the file. This sets File to what is expected by the cleanup code upon failure. */
   dwErr = FpOpenFile(FileName,
                      &pFragCtx->File);
   
   if ( NO_ERROR != dwErr )
   {
      /* Failure */
      goto __CLEANUP;
   }

   if ( !GetFileSizeEx(pFragCtx->File,
                       &cbFile) )
   {
      dwErr = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   pFragCtx->FileSize       = static_cast<ULONGLONG>(cbFile.QuadPart);
   pFragCtx->FileSizeOnDisk = 0;
   pFragCtx->ClusterSize    = _GetFileBytesPerCluster(FileName);
   if ( 0 == pFragCtx->ClusterSize )
   {
      dwErr = ERROR_CAN_NOT_COMPLETE;
      /* Failure */
      goto __CLEANUP;
   }
   
   if ( FAILED(StringCchCopyW(pFragCtx->FileName,
                              cchFile + 1,
                              FileName)) )
   {
      dwErr = ERROR_BAD_ARGUMENTS;
      /* Failure */
      goto __CLEANUP;
   }

   dwErr = FpiInitializeFragInfo(pFragCtx,
                                 Flags);

   if ( NO_ERROR != dwErr )
   {
      /* Failure */
      goto __CLEANUP;
   }

   if ( (FILE_ATTRIBUTE_COMPRESSED|FILE_ATTRIBUTE_SPARSE_FILE) & dwAttr )
   {
      cbFileOnDisk.LowPart = GetCompressedFileSize(FileName,
                                                   &cbFileOnDisk.HighPart);

      if ( INVALID_FILE_SIZE == cbFileOnDisk.LowPart )
      {
         dwErr = GetLastError();
         if ( NO_ERROR != dwErr )
         {
            /* Failure */
            goto __CLEANUP;
         }
      }
   }
   else
   {
      cbFileOnDisk.QuadPart = (static_cast<ULONGLONG>(pFragCtx->ClusterSize) * pFragCtx->ClusterCount);
   }

   pFragCtx->FileSizeOnDisk = cbFileOnDisk.QuadPart;

   (*hFragCtx) = FragContextToHandle(pFragCtx);

   /* Success */
   pFragCtx = NULL;
   dwErr    = NO_ERROR;

__CLEANUP:
   if ( NULL != pFragCtx )
   {
      if ( INVALID_HANDLE_VALUE != pFragCtx->File )
      {
         CloseHandle(pFragCtx->File);
      }

      delete [] reinterpret_cast<BYTE*>(pFragCtx);
   }

   /* Success / Failure */
   return ( dwErr );
}

VOID FRAGAPI FpCloseContext( HANDLE hFragCtx ) throw()
{
   PFRAG_CONTEXT pFragCtx;
   HANDLE        hFile;

   /* Validate parameters */
   if ( !IsValidFragContextHandle(hFragCtx) )
   {
      SetLastError(ERROR_INVALID_HANDLE);
      /* Failure */
      return;
   }

   /* Initialize locals... */
   pFragCtx = HandleToFragContext(hFragCtx);
   
   /* To be able to preserve an error caused by the call to CloseHandle(), the handle is cached and the
    * record is deleted prior to the handle being closed */
   hFile = pFragCtx->File;

   delete [] reinterpret_cast<BYTE*>(pFragCtx);
   SetLastError(NO_ERROR);

   if ( INVALID_HANDLE_VALUE != hFile )
   {
      CloseHandle(hFile);
   }
}

DWORD FRAGAPI FpGetFileInfo( HANDLE FragCtx, PFRAGCTXINFO CtxInfo )
{
   PFRAG_CONTEXT pFragCtx;

   /* Validate parameters */
   if ( !IsValidFragContextHandle(FragCtx) )
   {
      /* Failure */
      return ( ERROR_INVALID_HANDLE );
   }

   if ( !CtxInfo )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals */
   pFragCtx = HandleToFragContext(FragCtx);

   /* Copy fixed size members... */
   CtxInfo->FileSize       = pFragCtx->FileSize;
   CtxInfo->FileSizeOnDisk = pFragCtx->FileSizeOnDisk;
   CtxInfo->ClusterCount   = pFragCtx->ClusterCount;
   CtxInfo->FragmentCount  = pFragCtx->FragmentCount;
   CtxInfo->ExtentCount    = pFragCtx->ExtentCount;
   CtxInfo->ClusterSize    = pFragCtx->ClusterSize;
   
   /* Success */
   return ( NO_ERROR );
}

DWORD FRAGAPI FpGetFileName( HANDLE hFragCtx, LPWSTR FileName, ULONG FileNameLength ) throw()
{
   PFRAG_CONTEXT pFragCtx;

   /* Validate parameters */
   if ( !IsValidFragContextHandle(hFragCtx) )
   {
      /* Failure */
      return ( ERROR_INVALID_HANDLE );
   }

   if ( !FileName )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals */
   pFragCtx = HandleToFragContext(hFragCtx);

   if ( FAILED(StringCchCopy(FileName,
                             static_cast<size_t>(FileNameLength),
                             pFragCtx->FileName)) )
   {
      /* Failure */
      return ( ERROR_BUFFER_OVERFLOW );
   }

   /* Success */
   return ( NO_ERROR );
}

DWORD FRAGAPI FpGetFileFragmentInfo( HANDLE FragCtx, ULONG SequenceId, PFRAGMENTINFO FragmentInfo ) throw()
{
   PFRAG_CONTEXT pFragCtx;

   /* Validate parameters */
   if ( !IsValidFragContextHandle(FragCtx) )
   {
      /* Failure */
      return ( ERROR_INVALID_HANDLE );
   }

   /* Initialize locals */
   pFragCtx = HandleToFragContext(FragCtx);

   /* Ensure the fragment exists. Sequence IDs are 1 based, so they are always less than or equal to
    * the fragment count */
   if ( (SequenceId == 0) || (SequenceId > pFragCtx->FragmentCount) )
   {
      /* Failure */
      return ( ERROR_INVALID_INDEX );
   }

   _ASSERT(NULL != pFragCtx->FragmentInfo);
   /* The sequence id is a 1 based index */
   (*FragmentInfo) = pFragCtx->FragmentInfo[SequenceId - 1];

   /* Success */
   return ( NO_ERROR );
}

DWORD FRAGAPI FpEnumFileFragmentInfo( HANDLE hFragCtx, PENUMFILEFRAGMENTINFOPROC Callback, PVOID Parameter ) throw()
{
   DWORD         dwErr;
   PFRAG_CONTEXT pFragCtx;
   PFRAGMENTINFO pFragInfo;
   ULONG         iFragCount;
   ULONG         iIdx;

   /* Validate in/out parameters */
   if ( !IsValidFragContextHandle(hFragCtx) )
   {
      /* Failure */
      return ( ERROR_INVALID_HANDLE );
   }

   if ( !Callback )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals */
   dwErr      = NO_ERROR;
   pFragCtx   = HandleToFragContext(hFragCtx);
   pFragInfo  = pFragCtx->FragmentInfo;
   iFragCount = pFragCtx->FragmentCount;

   for ( iIdx = 0; iIdx < iFragCount; iIdx++ )
   {
      /* Pass this one on to the caller */
      dwErr = (*Callback)(Parameter,
                          hFragCtx,
                          &pFragInfo[iIdx]);

      if ( NO_ERROR != dwErr )
      {
         /* Caller failed prematurely */
         break;
      }
   }

   /* Success / Failure */
   return ( dwErr );
}

DWORD FRAGAPI FpDefragmentFile( LPCWSTR FileName, DWORD Flags, PDEFRAGMENTFILEPROC Callback, PVOID Parameter ) throw()
{
   DWORD                      dwErr;
   HANDLE                     hFile;
   HANDLE                     hVolume;
   ULONG                      cbFragData;
   PRETRIEVAL_POINTERS_BUFFER pFragData;
   DWORD                      iIdx;
   ULONGLONG                  iVCN;
   ULONGLONG                  iLCN;
   DWORD                      iBlockSize;
   ULONGLONG                  iClustersTotal;
   ULONGLONG                  iClustersMoved;
   ULONGLONG                  iClustersRemaining;
   ULONGLONG                  iClustersBlock;
   ULONGLONG                  iClustersExtent;
   PEXTENT                    pExtent;
   BOOL                       bRet;
   DWORD                      cbRet;
   MOVE_FILE_DATA             MoveData;
   
   /* Validate parameters */
   

   /* Initialize locals */
   hFile      = INVALID_HANDLE_VALUE;
   hVolume    = INVALID_HANDLE_VALUE;
   pFragData  = NULL;
   iBlockSize = GetClusterBlockSize(Flags);
   iBlockSize = max(iBlockSize, FPIF_DEFRAG_CLUSTERBLOCKSIZE_MIN);

   /* Open the file */
   dwErr = FpOpenFile(FileName,
                      &hFile);

   if ( NO_ERROR != dwErr )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Open a handle to the file's volume */
   hVolume = FpiOpenFileVolume(FileName);

   if ( INVALID_HANDLE_VALUE == hVolume )
   {
      dwErr = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   /* Load the retrieval pointer data */
   dwErr = FpiGetRetrievalPointers(hFile,
                                   FPF_RAWEXTENTDATA,
                                   reinterpret_cast<PVOID*>(&pFragData),
                                   &cbFragData);

   if ( NO_ERROR != dwErr )
   {
      if ( ERROR_HANDLE_EOF == dwErr )
      {
         /* This code is returned for files that do not have extent data, such as those that fit
          * within their MFT record. Map it to success and return */
         dwErr = NO_ERROR;
      }

      /* Success / Failure */
      goto __CLEANUP;
   }

   _ASSERTE(NULL != pFragData);

   /* Update the extent data so that each extent's Lcn is the actual number of clusters,
    * not relative to the previous extent. Also sum up the total number of clusters */
   iClustersTotal = pFragData->Extents[0].NextVcn.QuadPart;

   for ( iIdx = pFragData->ExtentCount - 1; iIdx >= 1; iIdx-- )
   {
      pFragData->Extents[iIdx].NextVcn.QuadPart -= pFragData->Extents[iIdx - 1].NextVcn.QuadPart;
      iClustersTotal += pFragData->Extents[iIdx].NextVcn.QuadPart;
   }

   /* Initialize the starting LCN for searching free space */
   iVCN               = pFragData->StartingVcn.QuadPart;
   iLCN               = 0;
   iClustersMoved     = 0;
   iClustersRemaining = iClustersTotal;
   pExtent            = reinterpret_cast<PEXTENT>(pFragData->Extents);
   iClustersExtent    = pExtent->ClusterCount;

   while ( true )
   {
      dwErr = FpiLocateFreeClusterRange(hVolume, 
                                        iClustersRemaining, 
                                        &iLCN);

      if ( NO_ERROR != dwErr )
      {
         /* Failure */
         goto __CLEANUP;
      }

      while ( iClustersRemaining )
      {
         MoveData.ClusterCount         = static_cast<DWORD>(min(iBlockSize, iClustersRemaining));
         MoveData.FileHandle           = hFile;
         MoveData.StartingVcn.QuadPart = iVCN;
         MoveData.StartingLcn.QuadPart = iLCN;

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
            dwErr = GetLastError();            
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

            if ( !IsVirtualCluster(pExtent->LCN) )
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
         if ( NULL != Callback )
         {
            dwErr = (*Callback)(Parameter,
                                FileName,
                                iClustersTotal,
                                iClustersMoved);

            /* If the callback routine has returned a failure code, then abort */
            if ( NO_ERROR != dwErr )
            {
               /* Callback failure */
               goto __CLEANUP;
            }
         }
      }

      if ( 0 == iClustersRemaining )
      {
         dwErr = NO_ERROR;
         /* Success */
         goto __CLEANUP;
      }
   }

__CLEANUP:
   FpiFreeRetrievalPointers(pFragData);

   if ( INVALID_HANDLE_VALUE != hVolume )
   {
      CloseHandle(hVolume);
   }

   if ( NULL != hFile )
   {
      CloseHandle(hFile);
   }

   /* Success / Failure */
   return ( dwErr );
}

DWORD FRAGAPI FpEnumFileStreamInfo( PVOID File, DWORD Flags, PENUMFILESTREAMINFOPROC Callback, PVOID Parameter ) throw()
{
   DWORD                    dwErr;
   SYSTEM_INFO              SysInfo;
   HANDLE                   hFile;
   ULONG                    cbBuff;
   PVOID                    pBuff;
   NTSTATUS                 NtStatus;
   IO_STATUS_BLOCK          IoStatus;
   ULONG                    iStreams;
   PFILE_STREAM_INFORMATION pStream;
   WCHAR                    chTerm;
   BOOL                     bDefault;

   /* Validate input params */
   if ( !File || !Callback || !((FPF_OPENBYFILENAME|FPF_OPENBYFILEHANDLE) & Flags) )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize locals */
   GetSystemInfo(&SysInfo);

   hFile  = INVALID_HANDLE_VALUE;
   cbBuff = 0;
   pBuff  = NULL;
   
   if ( FPF_OPENBYFILENAME & Flags )
   {
      /* Attempt to open the file */
      dwErr = FpOpenFile(reinterpret_cast<LPCWSTR>(File),
                         &hFile);
   }
   else
   {
      hFile = reinterpret_cast<HANDLE>(File);
      if ( INVALID_HANDLE_VALUE != hFile )
      {
         dwErr = NO_ERROR;
      }
      else
      {
         dwErr = ERROR_INVALID_HANDLE;
      }
   }

   if ( NO_ERROR != dwErr )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Get the stream information */
   while ( true )
   {
      cbBuff += SysInfo.dwAllocationGranularity;
      pBuff   = VirtualAlloc(NULL,
                             cbBuff,
                             MEM_COMMIT,
                             PAGE_READWRITE);

      if ( !pBuff )
      {
         dwErr = ERROR_OUTOFMEMORY;
         /* Failure */
         goto __CLEANUP;
      }

      NtStatus = NtQueryInformationFile(hFile,
                                        &IoStatus,
                                        pBuff,
                                        cbBuff,
                                        FileStreamInformation);

      if ( NT_SUCCESS(NtStatus) )
      {
         break;
      }

      if ( STATUS_BUFFER_OVERFLOW != NtStatus )
      {
         dwErr = RtlNtStatusToDosError(NtStatus);
         if ( NO_ERROR == dwErr )
         {
            dwErr = ERROR_GEN_FAILURE;
         }

         /* Failure */
         goto __CLEANUP;
      }

      VirtualFree(pBuff,
                  0,
                  MEM_RELEASE);

      pBuff = NULL;
   }

   /* First get a count of the streams */
   pStream  = reinterpret_cast<PFILE_STREAM_INFORMATION>(pBuff);
   iStreams = 0;

   while ( true )
   {
      iStreams += 1;

      if ( 0 == pStream->NextEntryOffset )
      {
         break;
      }

      /* Bump up to the next stream record */
      pStream   = reinterpret_cast<PFILE_STREAM_INFORMATION>(reinterpret_cast<ULONG_PTR>(pStream) + pStream->NextEntryOffset);
   }

   /* Enumerate over the stream data */
   bDefault = false;
   pStream = reinterpret_cast<PFILE_STREAM_INFORMATION>(pBuff);

   while ( true )
   {
      if ( FPF_STREAM_NODEFAULTDATASTREAM & Flags )
      {
         bDefault = (0 == memcmp(L"::$DATA", 
                                 pStream->StreamName, 
                                 min(pStream->StreamNameLength, sizeof(L"::$DATA"))) ? TRUE : FALSE);
      }

      if ( !bDefault )
      {
         /* Make sure the stream name is null terminated */
         chTerm = pStream->StreamName[pStream->StreamNameLength / sizeof(WCHAR)];
         pStream->StreamName[pStream->StreamNameLength / sizeof(WCHAR)] = L'\0';

         dwErr = (*Callback)(Parameter,
                             iStreams,
                             static_cast<ULONGLONG>(pStream->EndOfStream.QuadPart),
                             pStream->StreamName);

         if ( NO_ERROR != dwErr )
         {
            /* Failure */
            goto __CLEANUP;
         }

         /* Reset whatever character was swapped out for the null term */
         pStream->StreamName[pStream->StreamNameLength / sizeof(WCHAR)] = chTerm;
      }

      if ( 0 == pStream->NextEntryOffset )
      {
         break;
      }

      /* Bump up to the next stream record */
      pStream = reinterpret_cast<PFILE_STREAM_INFORMATION>(reinterpret_cast<ULONG_PTR>(pStream) + pStream->NextEntryOffset);
   }

   /* Success */
   dwErr = NO_ERROR;

__CLEANUP:
   if ( NULL != pBuff )
   {
      VirtualFree(pBuff,
                  0,
                  MEM_RELEASE);
   }

   /* Only close the file handle if this routine created it */
   if ( (Flags & FPF_OPENBYFILENAME) && (INVALID_HANDLE_VALUE != hFile) )
   {
      CloseHandle(hFile);
   }

   /* Success / Failure */
   return ( dwErr );
}

/**********************************************************************

   Private

 **********************************************************************/
LPVOID FpiAlloc( DWORD dwFlags, SIZE_T cbAlloc ) throw()
{
   return ( HeapAlloc(GetProcessHeap(),
                      dwFlags,
                      cbAlloc) );
}

VOID FpiFree( DWORD dwFlags, LPVOID pMem ) throw()
{
   HeapFree(GetProcessHeap(),
            dwFlags,
            pMem);
}

DWORD FpiGetFileHandle( PVOID pFile, DWORD Flags, HANDLE* phFile ) throw()
{
   DWORD  dwErr;
   BOOL   bRet;

   /* Initialize locals */
   dwErr = ERROR_INVALID_PARAMETER;

   if ( FPF_OPENBYFILENAME & Flags )
   {
      dwErr = FpOpenFile(reinterpret_cast<LPCWSTR>(pFile),
                         phFile);
   }
   else if ( (FPF_OPENBYFILEHANDLE|FPF_OPENBYCONTEXT) & Flags )
   {
      bRet = DuplicateHandle(GetCurrentProcess(),
                             Flags & FPF_OPENBYFILEHANDLE ? reinterpret_cast<HANDLE>(pFile) : reinterpret_cast<PFRAG_CONTEXT>(pFile)->File,
                             GetCurrentProcess(),
                             phFile,
                             0,
                             FALSE,
                             DUPLICATE_SAME_ACCESS);

      if ( !bRet )
      {
         dwErr = GetLastError();
      }
      else
      {
         dwErr = NO_ERROR;
      }
   }

   /* Success / Failure */
   return ( dwErr );
}

HANDLE FpiOpenFileVolume( LPCWSTR pwszFile ) throw()
{
   HANDLE hVolume;
   size_t cchVolumeGuid;
   WCHAR  chVolumeGuid[50];
   WCHAR  chVolumeName[MAX_PATH];

   /* Initialize locals */
   hVolume = INVALID_HANDLE_VALUE;

   /* Determine the actual volume name for the file */
   if ( !GetVolumePathNameW(pwszFile,
                            chVolumeName,
                            _countof(chVolumeName)) )
   {
      /* Failure */
      return ( INVALID_HANDLE_VALUE );
   }

   /* Get the guid name for the volume */
   if ( !GetVolumeNameForVolumeMountPointW(chVolumeName,
                                           chVolumeGuid,
                                           _countof(chVolumeGuid)) )
   {
      /* Failure */
      return ( INVALID_HANDLE_VALUE );
   }

   /* CreateFile() requires that there is no trailing \ on a volume guid
    * name, so remove it if it is there */
   cchVolumeGuid = wcslen(chVolumeGuid);
   if ( cchVolumeGuid > 0 )
   {
      if ( L'\\' == chVolumeGuid[--cchVolumeGuid] )
      {
         chVolumeGuid[cchVolumeGuid] = L'\0';
      }
   }


   /* Open the volume with an access right that will force it to be mounted
    * if it presently isn't */
   hVolume = CreateFileW(chVolumeGuid,
                         FILE_GENERIC_READ,
                         FILE_SHARE_READ|FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL);
   
   /* Success / Failure */
   return ( hVolume );
}

DWORD FpiGetRetrievalPointers( HANDLE hFile, DWORD dwFlags, PVOID* ppBuff, ULONG* pcbBuff ) throw()
{
   DWORD                      dwErr;
   BOOL                       bRet;
   SYSTEM_INFO                SysInfo;
   DWORD                      cbRet;
   DWORD                      cbBuff;
   PRETRIEVAL_POINTERS_BUFFER pBuff;
   LONGLONG                   iVCN;

   UNREFERENCED_PARAMETER(dwFlags);

   /* Initialize outputs */
   (*ppBuff)  = NULL;   
   (*pcbBuff) = 0;

   /* Initialize locals */
   GetSystemInfo(&SysInfo);

   dwErr  = NO_ERROR;
   cbBuff = 0;
   pBuff  = NULL;

   /* Retrieve all of the file's extent info */
   while ( true )
   {
      cbBuff += SysInfo.dwPageSize;
      pBuff   = reinterpret_cast<PRETRIEVAL_POINTERS_BUFFER>(FpiAlloc(HEAP_ZERO_MEMORY,
                                                                      cbBuff));

      if ( !pBuff )
      {
         dwErr = ERROR_OUTOFMEMORY;
         /* Failure */
         goto __CLEANUP;
      }

      iVCN  = 0;
      cbRet = 0;
      bRet  = DeviceIoControl(hFile,
                              FSCTL_GET_RETRIEVAL_POINTERS,
                              &iVCN,
                              sizeof(iVCN),
                              pBuff,
                              cbBuff,
                              &cbRet,
                              NULL);

      if ( bRet )
      {
         /* Do not allow returning 0 extents. This occurs for directories when they fit in a file record */
         if ( 0 == pBuff->ExtentCount )
         {
            dwErr = ERROR_HANDLE_EOF;
            /* Failure */ 
            goto __CLEANUP;
         }


         /* Success */
         break;
      }

      dwErr = GetLastError();
      if ( ERROR_MORE_DATA != dwErr )
      {
         /* ERROR_HANDLE_EOF is returned for files with no extent data */

         /* Failure */
         goto __CLEANUP;
      }

      /* Need a larger buffer, so free this one and start over */
      FpiFree(0,
              pBuff);

      pBuff = NULL;
   }

   (*ppBuff)  = pBuff;   
   (*pcbBuff) = cbRet;
   dwErr      = NO_ERROR;
   pBuff      = NULL;

__CLEANUP:
   if ( NULL != pBuff )
   {
      FpiFree(0,
              pBuff);
   }

   /* Success / Failure */
   return ( dwErr );
}

VOID FpiFreeRetrievalPointers( PVOID pBuff ) throw()
{
   if ( NULL != pBuff )
   {
      FpiFree(0,
              pBuff);
   }
}

DWORD FpiGetFragmentInfo( HANDLE hFile, DWORD dwFlags, PFRAGMENTINFO* FragmentInfo, ULONG* FragmentCount ) throw()
{
   DWORD                      dwErr;
   ULONG                      cbBuff;
   PRETRIEVAL_POINTERS_BUFFER pBuff;
   ULONG                      iFragCount;
   PFRAGMENTINFO              pFragInfo;
   PFRAGMENTINFO              pRecord;
   LONGLONG                   iLCN;
   LONGLONG                   iVCN;
   ULONG                      iIdx;
   ULONG                      iJdx;
   ULONG                      iSequence;

   /* Validate parameters */
   if ( (INVALID_HANDLE_VALUE == hFile) || !FragmentInfo )
   {
      /* Failure */
      return ( ERROR_INVALID_PARAMETER );
   }

   /* Initialize outputs */
   (*FragmentInfo)  = NULL;
   (*FragmentCount) = 0;

   /* Initialize locals */
   pBuff      = NULL;
   iFragCount = 0;
   pFragInfo  = NULL;

   /* Get the full set of extent data for the file */
   dwErr = FpiGetRetrievalPointers(hFile,
                                   dwFlags,
                                   reinterpret_cast<PVOID*>(&pBuff),
                                   &cbBuff);

   if ( NO_ERROR != dwErr )
   {
      /* Failure */
      goto __CLEANUP;
   } 

   /* At this point, all of the extent info for the file has been retrieved */
   if ( FPF_RAWEXTENTDATA & dwFlags )
   {
      /* The caller wants to see the raw data, so just send it as is */
      iFragCount = pBuff->ExtentCount;
      pFragInfo  = reinterpret_cast<PFRAGMENTINFO>(FpiAlloc(HEAP_ZERO_MEMORY,
                                                            sizeof(FRAGMENTINFO) * iFragCount));

      if ( !pFragInfo )
      {
         dwErr = ERROR_OUTOFMEMORY;
         /* Failure */
         goto __CLEANUP;
      }

      iVCN = pBuff->StartingVcn.QuadPart;

      for ( iIdx = 0; iIdx < iFragCount; iIdx++ )
      {
         pFragInfo[iIdx].Sequence       = iIdx + 1;
         pFragInfo[iIdx].ExtentCount    = 1;
         pFragInfo[iIdx].ClusterCount   = static_cast<ULONGLONG>(pBuff->Extents[iIdx].NextVcn.QuadPart) - iVCN;
         pFragInfo[iIdx].LogicalCluster = static_cast<ULONGLONG>(pBuff->Extents[iIdx].Lcn.QuadPart);

         iVCN = pBuff->Extents[iIdx].NextVcn.QuadPart;
      }

      /* Success */
      dwErr = NO_ERROR;
   }
   else
   {
      /* The loop below expects the first VCN to be 0 */
      _ASSERTE(0 == pBuff->StartingVcn.QuadPart);
      /* There must be extent data */
      _ASSERTE(pBuff->ExtentCount > 0);

      /* As it is now, the extent array cannot be sorted because each element depends on
       * the value of the previous to determine the number of clusters it uses. Therefore
       * the array is first fixed up to remove this limitation by converting each Lcn 
       * from a relative value to a fixed value. 
       *
       * Enumeration is started at the last element, so the previous element is available. This
       * skips element 0, but element 0 is always based from 0 so it doesn't need to be fixed up.
       */
      for ( iIdx = pBuff->ExtentCount - 1; iIdx > 0; iIdx-- )
      {
         pBuff->Extents[iIdx].NextVcn.QuadPart -= pBuff->Extents[iIdx - 1].NextVcn.QuadPart;
      }

      /* Sort the extent data by LCN */
      if ( pBuff->ExtentCount > 1 )
      {
         qsort_s(&pBuff->Extents[0],
                 pBuff->ExtentCount,
                 sizeof(pBuff->Extents[0]),
                 FpiEnumFragmentInfoCompareProc,
                 NULL);
      }

      /* Determine how many non-compressed/sparse fragments there are */
      for ( iIdx = 0; iIdx < pBuff->ExtentCount; iIdx++ )
      {
         if ( !IsVirtualCluster(pBuff->Extents[iIdx].Lcn.QuadPart) )
         {
            iFragCount++;
         }
      }

      pFragInfo = reinterpret_cast<PFRAGMENTINFO>(FpiAlloc(HEAP_ZERO_MEMORY,
                                                           sizeof(FRAGMENTINFO) * iFragCount));

      if ( !pFragInfo )
      {
         dwErr = ERROR_OUTOFMEMORY;
         /* Failure */
         goto __CLEANUP;
      }

      /* Compact the remaining extent data into fragments and populate the outgoing recordset */
      for ( iIdx = 0, iSequence = 1, pRecord = pFragInfo; iIdx < pBuff->ExtentCount; iIdx++ )
      {
         /* Ignore compressed/sparse extents */
         if ( IsVirtualCluster(pBuff->Extents[iIdx].Lcn.QuadPart) )
         {
            continue;
         }

         pRecord->Sequence       = iSequence++;
         pRecord->ExtentCount    = 1;
         pRecord->ClusterCount   = pBuff->Extents[iIdx].NextVcn.QuadPart;
         pRecord->LogicalCluster = pBuff->Extents[iIdx].Lcn.QuadPart;

         /* Calculate where this extent is expected to end */
         iLCN = pBuff->Extents[iIdx].Lcn.QuadPart + pBuff->Extents[iIdx].NextVcn.QuadPart;
         
         /* Scan the remaining extent array for an extent that starts at the LCN
          * adjacent to the current one, which would mean it is sequential on the 
          * disk. This scan ends at the first non-matching LCN. */
         for ( iJdx = iIdx + 1; iJdx < pBuff->ExtentCount; iJdx++ )
         {
            if ( iLCN != pBuff->Extents[iJdx].Lcn.QuadPart )
            {
               /* The array is sorted, so the sequence is broken */
               break;
            }

            /* Add this extent's size into the current one */
            pRecord->ExtentCount  += 1;
            pRecord->ClusterCount += pBuff->Extents[iJdx].NextVcn.QuadPart;
            iLCN                  += pBuff->Extents[iJdx].NextVcn.QuadPart;
            
            /* Decrement the total fragment count as the current one has been included
             * into another */
            _ASSERTE(iFragCount > 0);
            iFragCount -= 1;

            /* Also update iIdx to reflect that this element has been accounted for */
            iIdx++;            
         }
      
         /* Move on to the next record */
         _ASSERTE(pRecord < &pFragInfo[iFragCount]);
         pRecord++;
      }

      /* Success */
      dwErr = NO_ERROR;
   }

   ATLASSERT(NO_ERROR == dwErr);

   (*FragmentInfo)  = pFragInfo;
   (*FragmentCount) = iFragCount;
   pFragInfo        = NULL;

__CLEANUP:
   if ( NULL != pFragInfo )
   {
      FpiFree(0,
              pFragInfo);
   }

   if ( NULL != pBuff )
   {
      FpiFreeRetrievalPointers(pBuff);
   }

   /* Success / Failure */
   return ( dwErr );
}

VOID FpiFreeFragmentInfo( PFRAGMENTINFO FragmentInfo ) throw()
{
   if ( NULL != FragmentInfo )
   {
      FpiFree(0,
              FragmentInfo);
   }
}

DWORD FpiInitializeFragInfo( PFRAG_CONTEXT pFragCtx, DWORD dwFlags ) throw()
{
   _ASSERTE(NULL != pFragCtx);

   DWORD          dwErr;
   PFRAGMENTINFO  pFragInfo;
   PFRAGMENTINFO  pFragInfoTail;
   ULONGLONG      iClusterCount;
   ULONG          iExtentCount;
   
   /* Load the fragment data */
   dwErr = FpiGetFragmentInfo(pFragCtx->File,
                              dwFlags,
                              &pFragCtx->FragmentInfo,
                              &pFragCtx->FragmentCount);

   if ( NO_ERROR != dwErr )
   {
      if ( ERROR_HANDLE_EOF != dwErr )
      {
         /* Failure */
         return ( dwErr );
      }

      /* Map ERROR_HANDLE_EOF to success. This is the case for files that fit in their MFT records
       * and do not have extent data. */

      /* Success */
      return ( NO_ERROR );
   }

   /* Enum the data to get a total count of clusters and extents */
   pFragInfo     = pFragCtx->FragmentInfo;
   pFragInfoTail = &pFragCtx->FragmentInfo[pFragCtx->FragmentCount];
   iClusterCount = 0;
   iExtentCount  = 0;

   while ( pFragInfo < pFragInfoTail )
   {
      iClusterCount += pFragInfo->ClusterCount;
      iExtentCount  += pFragInfo->ExtentCount;

      pFragInfo++;
   }

   pFragCtx->ClusterCount = iClusterCount;
   pFragCtx->ExtentCount  = iExtentCount;
   
   /* Success / Failure */
   return ( dwErr );
}

int __cdecl FpiEnumFragmentInfoCompareProc( void* pContext, const void* pElem1, const void* pElem2 ) throw()
{
   PEXTENT pExtent1;
   PEXTENT pExtent2; 

   UNREFERENCED_PARAMETER(pContext);

   /* Initialize locals */
   pExtent1 = (PEXTENT)pElem1;
   pExtent2 = (PEXTENT)pElem2;

   if ( pExtent1->LCN < pExtent2->LCN )
   {
      return ( -1 );
   }
   else if ( pExtent1->LCN > pExtent2->LCN )
   {
      return ( 1 );
   }
   else
   {
      return ( 0 );
   }
}

DWORD FpiLocateFreeClusterRange( HANDLE hVolume, ULONGLONG iClustersRequired, ULONGLONG* piLCN ) throw()
{
   DWORD                 dwErr;
   PVOID                 pBuff;
   BOOL                  bRet;
   DWORD                 cbRet;
   ULONG                 iOffset;
   LONGLONG              iLCN;
   RTL_BITMAP            Bitmap;
   BOOL                  bBlocks;
   ULONGLONG             iBlocks;
   ULONG                 iRequired;
   PVOLUME_BITMAP_BUFFER pMapData;
   ULONG                 cBitMapSize;
   ULONG                 cBitMapAdjust;

   enum
   {
      /* Number of bytes allocated for VOLUME_BITMAP_BUFFER::Buffer */
      SizeOfBitMap  = 1024 * 512,
      /* Total number of bits that can fill VOLUME_BITMAP_BUFFER::Buffer */
      CountOfBitMap = SizeOfBitMap * CHAR_BIT,
      /* Number of bytes allocated for the entire VOLUME_BITMAP_BUFFER. Note that
       * padding bytes are added which allows the system to round the StartingLcn
       * down while still allowing CountOfBitMap bits in the buffer. It is assumed
       * that the system will round down by a maximum of 64 clusters. */
      SizeOfBuffer  = SizeOfBitMap + (sizeof(VOLUME_BITMAP_BUFFER) - sizeof(BYTE)) + (64 / CHAR_BIT)
   };

   /* Initialize locals */
   pBuff = NULL;

   /* Allocate a buffer for the bitmap. The size must allow for non-overflow into a ULONG when
    * multiplied by CHAR_BIT. In this case 512KB allows for 4,194,304 clusters. */
   pBuff = FpiAlloc(0,
                    SizeOfBuffer);

   if ( !pBuff )
   {
      dwErr = ERROR_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }

   pMapData = reinterpret_cast<PVOLUME_BITMAP_BUFFER>(pBuff);

   /* 
    * If the number of clusters required is larger than what will fit in the buffer, then the search needs to be
    * broken down into sections. When this happens, each section excluding the last will require an entirely clear
    * buffer. So the first thing to do is to find as many fully clear sections as is required. 
    *
    * After all fully clear sections have been located, the final section is located. If there were no full sections
    * required, then this final section can lie anywhere within the block. Otherwise the final section must begin at
    * the zero offset or the process is repeated.
    *
    * If the volume bitmap is exhausted during this process, then the disk cannot satisfy the required number of
    * contiguous clusters and so the operation fails.
    */
   while ( true )
   {
      iLCN    = 0;
      iBlocks = iClustersRequired / CountOfBitMap;
      bBlocks = (iBlocks > 0 ? TRUE : FALSE);
   
      while ( iBlocks )
      {
         /* Force all the bits to be set */
         FillMemory(pBuff,
                    SizeOfBuffer,
                    0xFF);

         /* Get the next bitmap chunk */
         bRet = DeviceIoControl(hVolume,
                                FSCTL_GET_VOLUME_BITMAP,
                                &iLCN,
                                sizeof(iLCN),
                                pBuff,
                                SizeOfBuffer,
                                &cbRet,
                                NULL);

         if ( !bRet )
         {
            dwErr = GetLastError();

            if ( ERROR_MORE_DATA != dwErr )
            {
               /* Failure - Unable to retrieve bitmap */
               goto __CLEANUP;
            }
         }

         /* The StartingLcn may be rounded down and needs to be adjusted to what was passed. The bitmap
          * buffer is allocated with 1 extra byte to give the system 8 clusters to round down */
         cBitMapAdjust = static_cast<ULONG>(iLCN - pMapData->StartingLcn.QuadPart);

         /* Determine the number of clusters in the returned data */
         cBitMapSize = FpiCalculateBitMapClusterCount(pMapData,
                                                      cbRet);
         
         /* Make sure that enough data was returned to fill the entire bitmap chunk */
         if ( cBitMapSize < CountOfBitMap )
         {
            dwErr = ERROR_HANDLE_EOF;
            /* Failure */
            goto __CLEANUP;
         }

         /* Initialize the bitmap for this scan */
         RtlInitializeBitMap(&Bitmap,
                             reinterpret_cast<PULONG>(pMapData->Buffer),
                             cBitMapSize);

         /* Scan the bitmap for the first set bit, which indicates that this chunk is not
          * empty and so cannot be used */
         if ( !RtlAreBitsClear(&Bitmap,
                               cBitMapAdjust,
                               cBitMapSize) )
         {
            /* This block is not empty, so the search must start over at the start of the
             * last empty run in the bitmap */
            iBlocks = iClustersRequired / CountOfBitMap;

            if ( RtlFindLastBackwardRunClear(&Bitmap,
                                             cBitMapSize - 1,
                                             &iOffset) && (iOffset > 0) )
            {
               iLCN += iOffset;
            }
            else
            {
               iLCN += cBitMapSize;
            }
         }
         else
         {
            /* The block is empty, so move on to the next one */
            iBlocks -= 1;
            iLCN    += cBitMapSize;
         }
      }/* end-of: while ( iBlocks ) */

      /* If we got to this point, then either all fully clear sections have been found or
       * none were required. */
      iRequired = static_cast<ULONG>(iClustersRequired % CountOfBitMap);

      while ( true )
      {
         /* Force all the bits to be set */
         FillMemory(pBuff,
                    SizeOfBuffer,
                    0xFF);

         /* Get the next bitmap chunk */
         bRet = DeviceIoControl(hVolume,
                                FSCTL_GET_VOLUME_BITMAP,
                                &iLCN,
                                sizeof(iLCN),
                                pBuff,
                                SizeOfBuffer,
                                &cbRet,
                                NULL);

         if ( !bRet )
         {
            dwErr = GetLastError();

            if ( ERROR_MORE_DATA != dwErr )
            {
               /* Failure - Unable to retrieve bitmap */
               goto __CLEANUP;
            }
         }

         /* The StartingLcn may be rounded down and needs to be adjusted to what was passed. The bitmap
          * buffer is allocated with 1 extra byte to give the system 8 clusters to round down */
         cBitMapAdjust = static_cast<ULONG>(iLCN - pMapData->StartingLcn.QuadPart);

         /* Determine the number of clusters in the returned data */
         cBitMapSize = FpiCalculateBitMapClusterCount(pMapData,
                                                      cbRet);

         /* Make sure enough data was returned to satisfy the request */
         if ( cBitMapSize < iRequired )
         {
            dwErr = ERROR_HANDLE_EOF;
            /* Failure */
            goto __CLEANUP;
         }

         /* Initialize the bitmap for this scan */
         RtlInitializeBitMap(&Bitmap,
                             reinterpret_cast<PULONG>(pMapData->Buffer),
                             cBitMapSize);

         /* Locate a clear run in the block */
         if ( bBlocks )
         {
            if ( RtlAreBitsClear(&Bitmap,
                                 cBitMapAdjust,
                                 iRequired) )
            {
               iOffset = 0;
            }
            else
            {
               iOffset = 0xFFFFFFFF;
            }
         }
         else
         {
            iOffset = RtlFindClearBits(&Bitmap,
                                       iRequired,
                                       0);
         }

         if ( 0xFFFFFFFF == iOffset )
         {
            /* This block is not empty, so the search must start over at the start of the
             * last empty run in the bitmap */
            if ( RtlFindLastBackwardRunClear(&Bitmap,
                                             cBitMapSize - 1,
                                             &iOffset) && (iOffset > 0) )
            {
               iLCN += iOffset;
            }
            else
            {
               iLCN += cBitMapSize;
            }
         }
         else
         {         
            /* The entire required cluster count has been located, so calculate the start
             * and return to the caller */
            iLCN    += iOffset;
            iLCN    += iRequired;
            iLCN    -= iClustersRequired;
            (*piLCN) = iLCN;
            dwErr    = NO_ERROR;
            /* Success */
            goto __CLEANUP;
         }
      }/* while ( true ) - Scan for single bitmap block */
   }/* while ( true ) - Scan for multiple bitmap blocks */

__CLEANUP:
   if ( NULL != pBuff )
   {
      FpiFree(0,
              pBuff);
   }

   /* Success / Failure */
   return ( dwErr );
}

ULONG FpiCalculateBitMapClusterCount( PVOLUME_BITMAP_BUFFER pBitMap, ULONG cbData ) throw()
{
   ULONG cBitMap;

   /* Calculate the maximum number of clusters given the byte size of the entire VOLUME_BITMAP_BUFFER */
   cBitMap = (cbData - (sizeof(VOLUME_BITMAP_BUFFER) - sizeof(BYTE))) * CHAR_BIT;

   /* The VOLUME_BITMAP_BUFFER returns the number of clusters remaining on the disk, from the cluster
    * returned in the StartingLcn member. This can be less than the maximum specified by cBitMap so
    * take the lower of the two values */
   if ( static_cast<ULONGLONG>(cBitMap) > static_cast<ULONGLONG>(pBitMap->BitmapSize.QuadPart - pBitMap->StartingLcn.QuadPart) )
   {
      cBitMap = static_cast<ULONG>(pBitMap->BitmapSize.QuadPart - pBitMap->StartingLcn.QuadPart);
   }

   return ( cBitMap );
}
