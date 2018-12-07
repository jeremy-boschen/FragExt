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

#include <PathUtil.h>
#include <ThreadUtil.h>


#pragma region TODO
/*TODO:

NTFS limits the sizes of disks to the following, such that only 1 RTL_BITMAP will ever be used
to represent a volume.

512 bytes:
((200 * (FFFFFFFF + 1)) / 10000000000 = 2TB

1KB
((400 * (FFFFFFFF + 1)) / 10000000000 = 4TB

2KB
((800 * (FFFFFFFF + 1)) / 10000000000 = 8TB

4KB
((1000 * (FFFFFFFF + 1)) / 10000000000 = 16TB

...

64KB
((10000 * (FFFFFFFF + 1)) / 10000000000 = 256TB


This means my code to use multiple buckets is unnecessary and can be removed.


(max-clusters / clusters-per-byte) / (1024 * 1024) = 512MB buffer to hold maxium volume
(0xFFFFFFFF / 0x8) / (0x400 ^ 2) = 512MB

--------------

MS says 64MB is size where there's no performance improvement for having larger
unfragmented blocks. so a file could be defragmented into 64MB blocks if necessary

I also want to be able to defragment only the fragmented portions of a file, which
will be any fragmented section less than 64MB

1) Build an array of file chunks via FpiGetFileFragmentInfo

Partial defrag:
2a) sort array by LBN (LogicalCluster)
2b) defragment in blocks of 64MB at minimum

Full defrag:
2a) sort array by LBN
2b) find the largest fragment
2c) break array down into fragments before the largest, and fragments after the largest

   |-----000xx-111111-x-----------------4444444444444----55555--22---3333--------|

   here we have fragments 0, 1, 4, 3, 2, 5 in LBN order. Fragment 4 is the largest so we
   want to defragment around it. there's enough space to place 3 and 2 before 4, then move
   1 and 0 over. moving 3 will also leave enough room to move 5 after 4. The problem is
   that we have to figure out that we can have space for 5, if we move 3 & 2. To do this, 
   we build a temporary bitmap for the full file, using 4 as the seed (can't think of a better word),
   and a copy of the array

   Stage1: |--------------xxxxxxxxxxxxx----xxxxx--xx---xxxx|

   This is the initial file bitmap, with used clusters marked in x. Since we only have 0 or 1 for each
   bit, we can't mark which is which. Now we go through the fragments we want to move, which are 0, 1,
   2,3 and 5. We iterate over these, and check the bitmap to see if it can be moved. If it can, we 
   clear the bits it used and reduce its cluster size in the array. We repeat this until either either all
   have been moved, or none were. This allows us to keep going, moving things in and out and freeing
   up space. For instance, 3 and 5 are overlapping each others required place. First we'll move fragment 2,
   then some of 3. Now there's no more room for 3, because 5 is in the way. 5 is overlapping its own space,
   so we have a special check to determine if a fragment size is overlapping itself, if we check to see
   if there's space to shift it the required number

   Iter1: |00------------4444444444444----55555--22---3333| all of 0 moved, 0's cluster size is now 0
   Iter2: |00111111------4444444444444----55555--22---3333| all of 1 moved, 1's cluster size is now 0
   Iter3: |0011111122----4444444444444----55555-------3333| all of 2 moved, 2's cluster size is now 0
   Iter4: |001111112233334444444444444----55555-----------| all of 3 moved, 3's cluster size is now 0
   Iter5: |0011111122333344444444444445555----5-----------| some of 5 moved, 5's cluster size is now 1
   Iter6: |00111111223333444444444444455555---------------| all of 5 moved, 5's cluster size is now 0

   This was a simple case, because no fragments overlapped each other's required placement. If one had,
   we'd get this...

   Stage1: |-----000xx-111111-x-----3333--------4444444444444----66666666555555-22----------|

   0 = 3
   1 = 6
   2 = 2
   3 = 4
   5 = 6
   6 = 8

   Start:  |---3333--------4444444444444----66666666555555|
   Iter1.1 |0003333--------4444444444444----66666666555555| 3 of 0 moved, 0 = 0
   Iter1.2 |000333311------4444444444444----66666666555555| 2 of 1 moved, 1 = 4
   Iter1.3 |00033331122----4444444444444----66666666555555| 2 of 2 moved, 2 = 0
   Iter1.4 |000----112233334444444444444----66666666555555| 4 of 3 moved, 3 = 0
   Iter1.5 |000----112233334444444444444555566666666----55| 4 of 5 moved, 5 = 2
   Iter1.6 |000----1122333344444444444445555666666--66--55| 2 of 6 moved, 6 = 6   
      - array iterated, cluster counts went down so try again

   0 = 0
   1 = 4
   2 = 0
   3 = 0
   5 = 2
   6 = 6

   Iter2.1 |00011111122333344444444444445555666666--66--55| 4 of 1 moved, 1 = 0
   Iter2.2 |00011111122333344444444444445555666666--66--55| 0 of 5 moved, 5 = 2
   Iter2.3 |000111111223333444444444444455556666--6666--55| 2 of 6 moved, 6 = 4
         - array iterated, cluster counts went down so try again

   0 = 0
   1 = 0
   2 = 0
   3 = 0
   5 = 2
   6 = 4

   Iter3.1 |000111111223333444444444444455556666--6666--55| 0 of 5 moved, 5 = 2
   Iter3.2 |0001111112233334444444444444555566--666666--55| 2 of 6 moved, 6 = 2
      - array iterated, cluster counts went down so try again

   0 = 0
   1 = 0
   2 = 0
   3 = 0
   5 = 2
   6 = 2

   Iter4.1 |000111111223333444444444444455556666--6666--55| 0 of 5 moved, 5 = 2
   Iter4.2 |00011111122333344444444444445555--66666666--55| 2 of 6 moved, 6 = 0
      - array iterated, cluster counts went down so try again

   0 = 0
   1 = 0
   2 = 0
   3 = 0
   5 = 2
   6 = 0

   Iter5.1 |000111111223333444444444444455555566666666----| 2 of 5 moved, 5 = 0

      - array iterated, cluster count == 0, so everything was moved OK


   Stage1: |xxx111111111111133332222xxx|

   2 = 4
   3 = 4

   Iter1.1 |111111111111133332222| 0 of 2 moved, 2 = 4
   Iter1.2 |111111111111133332222| 0 of 3 moved, 3 = 4

      - array iterated, cluster counts are the same, break

**************************************************************************************************
   This is overkill and doesn't work for files like the last case. Instead...

   Start with the file bitmap representation and clear out clusters that are
   owned by the file, around the largest fragment. after doing that if the
   entire map is empty then the file can be defragmented there...


   Stage1: |xxx111111111111133332222xxx|
   
   Iter1:  |xxx111111111111133332222xxx|
   Iter2:  |xxx11111111111113333----xxx|
   Iter3:  |xxx1111111111111--------xxx|

   RtlAreBitsClear(bm, -----^, bm.size - 1.size);

   another case...

   Stage1: |xxx111111111111100033332222xxx|
   
   Clear bits: |xxx1111111111111-----------xxx|

   this will fail, because there's no space before 1 to place 0

   another case..

   Stage1: |---------333333222111000444|

   Clear bits: |---------333333------------|
   
   if RtlAreBitsClear(bm, 3.LBN - (0.size + 1.size + 2.size), 0.size + 1.size + 2.size) && RtlAreBitsClear(bm, 3.LBN + 3.size, 4.size) then
      can be defragmented around 3
   else
      check next fragment

   if all fragments are checked and the file can't be defragmented 'in-place', then fall back
   to some other algorithm (partial or complete)
 */
class VolumeBitmap
{
public:
   DWORD Open( LPCWSTR VolumeName ) NO_THROW;
   void Close( ) NO_THROW;


private:
   HANDLE _hVolume;
};

class FileBitmap
{
public:
   DWORD Initialize( HANDLE File ) NO_THROW;
   void Uninitialize( ) NO_THROW;

   // _DetermineDefragmentationType()

   // _FileMapLoad() - build file bitmap
   // _FileMapMaskReserved() - mark clusters in the file bitmap that are in use by the volume
};


DWORD FpiBuildFileBitMap( HANDLE hFile, PRTL_BITMAP pBitMap )
{
   hFile;pBitMap;
   // need 1 bit for every cluster, from lowest to highest
   //    get fragdata, FpiGetFragmentInformation will sort by VCN
   //    resort by size (ClusterCount)
   //    allocate bitmap for ClusterCount
   //    for each fragment > LIMIT(64MB default)
   //       get volume bitmap for file with frag placed in middle
   //          set all frag bits in bitmap to 0
   //       if bitmap is clear, can be defragged in place
   //

   // how to use this to defrag a file?
   //    some function to build a list of records that say move x to y

   return ( ERROR_INVALID_FUNCTION );
}

DWORD FpiBuildFileDefragPlan2( LPCWSTR pwszFile, DWORD dwFlags, PDEFRAG_PLAN pDefragPlan )
{
   DWORD dwRet;

   dwRet = NO_ERROR;

   pwszFile;dwFlags;pDefragPlan;

   // this is going to be pretty confusing, no matter what

   // 1) FPIF_DEFRAGPLAN_COMPACT_FULL | FPIF_DEFRAGPLAN_PLACEMENT_OPTIMAL
   
   
   // based on the placement, try to determine the best hint for the cluster range

   // local will call some function to get the frags and find which one it can 
   //    hold the file
   // optimal will determine file type/use, and pick best place on disk to place
   //    the file
   //
   // both of these functions must return a plan. compact will determine the minimum
   //    required size of each file chunk after the plan is executed. full will be
   //    the full file size, partial will be a user configurable size, default is 64MB

   if ( FlagOn(dwFlags, FPIF_DEFRAGPLAN_PLACE_OPTIMAL) )
   {
      //dwRet = FpiBuildFileDefragPlanOptimal(hFile,
      //                                      dwFlags,
      //                                      ppDefragPlan);
   }
   else
   {
      //dwRet = FpiBuildFileDefragPlanLocal(hFile,
      //                                    dwFlags,
      //                                    ppDefragPlan);
   }

   //dwRet = FpiBuildFileDefragPlanLocal(hFile,                                       dwFlags,                                       pDefragPlan);

   /* Success / Failure */
   return ( dwRet );
}

int __cdecl FpiFragmentInfoSortByClusterCountRoutine( const void* pElem1, const void* pElem2 ) NO_THROW
{
   PFRAGMENT_INFORMATION pFrag1;
   PFRAGMENT_INFORMATION pFrag2;

   /* Initialize locals */
   pFrag1 = (PFRAGMENT_INFORMATION)(pElem1);
   pFrag2 = (PFRAGMENT_INFORMATION)(pElem2);

   if ( pFrag1->ClusterCount > pFrag2->ClusterCount )
   {
      return ( -1 );
   }
   else if ( pFrag1->ClusterCount < pFrag2->ClusterCount )
   {
      return ( 1 );
   }
   
   return ( 0 );
}

DWORD FpiBuildFileDefragPlanLocal( LPCWSTR pwszFile, DWORD dwFlags, PDEFRAG_PLAN pDefragPlan )
{
   DWORD         dwRet;   
   HANDLE        hFragCtx;
   //PFRAG_CONTEXT pFragCtx;

   dwRet    = NO_ERROR;
   hFragCtx = NULL;

   pwszFile;dwFlags;pDefragPlan;

   /* Load the fragment info for this file.. */
   dwRet = FpCreateContext(pwszFile,
                           0,
                           &hFragCtx);
   
   //DbgDumpFragmentInfo(pFragInfo, cFragCount);

   // 

   return ( NO_ERROR );
}

#pragma endregion TODO


/**********************************************************************

   FRAG_CONTEXT Management

 **********************************************************************/


/**********************************************************************

   Public - Exported functions

 **********************************************************************/
__declspec(deprecated)
DWORD FRAGAPI FpGetContextFilePath( HANDLE hFragCtx, LPWSTR FileName, ULONG FileNameLength ) NO_THROW
{
   PFRAG_CONTEXT pFragCtx;

   /* Validate parameters */
   if ( !IsValidFragCtxHandle(hFragCtx) )
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
   pFragCtx = HandleToFragCtx(hFragCtx);

   if ( FAILED(StringCchCopy(FileName,
                             static_cast<size_t>(FileNameLength),
                             pFragCtx->FilePath)) )
   {
      /* Failure */
      return ( ERROR_BUFFER_OVERFLOW );
   }

   /* Success */
   return ( NO_ERROR );
}


/**********************************************************************

   Private

 **********************************************************************/

DWORD 
FpiGetFullFilePath( 
   __in HANDLE hFile, 
   __out_ecount_z(cchFileName) LPWSTR pwszFilePath, 
   __in size_t cchFilePath
) NO_THROW
{
   /*****!!!!!!!!! this assumes SeChangeNotifyPrivilege  is granted !!!!!!!!!!!!!!!!******/

   DWORD                      dwRet;   
   PFILE_NAME_INFORMATION     pFileName;
   POBJECT_NAME_INFORMATION   pObjectName;
   ULONG                      cbFileDeviceName;
   PMOUNTMGR_MOUNT_POINT      pMountPoint;
   PMOUNTMGR_MOUNT_POINTS     pMountPoints;
   ULONG                      iIdx;
   UNICODE_STRING             VolumeName;
   UNICODE_STRING             DeviceName;
   
   /* Initialize locals */
   pFileName    = NULL;
   pObjectName  = NULL;
   pMountPoint  = NULL;
   pMountPoints = NULL;
                        
   /* Get the file's path. This won't include the volume name */
   dwRet = FpiQueryFileNameInformation(hFile,
                                       &pFileName);

   if ( NO_ERROR != dwRet )
   {  
      /* Failure */
      goto __CLEANUP;
   }

   /* Get the object path. This will include the NT device name and the full file path */
   dwRet = FpiQueryObjectNameInformation(hFile,
                                         &pObjectName);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      goto __CLEANUP;
   }

   if ( pObjectName->Name.Length < pFileName->FileNameLength )
   {
      dwRet = ERROR_BAD_PATHNAME;
      /* Failure */
      goto __CLEANUP;
   }
   
   cbFileDeviceName = (pObjectName->Name.Length - pFileName->FileNameLength);

   /* Build a mount point to limit the mount points returned */
   pMountPoint = FpiBuildMountManagerMountPoint(NULL,
                                                0,
                                                NULL,
                                                0,
                                                pObjectName->Name.Buffer,
                                                static_cast<USHORT>(cbFileDeviceName));

   if ( !pMountPoint )
   {
      dwRet = ERROR_OUTOFMEMORY;
      /* Failure */
      goto __CLEANUP;
   }
   
   /* Get the mount points for this device name */
   dwRet = FpiQueryMountManagerMountPoints(pMountPoint,
                                           &pMountPoints);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      goto __CLEANUP;
   }

   dbgDumpMountMgrMountPoints(pMountPoints);

   /* Find the volume guid for the device name we have */
   for ( iIdx = 0; iIdx < pMountPoints->NumberOfMountPoints; iIdx++ )
   {
      InitializeMountPointsNameStrings(pMountPoints,
                                       iIdx,
                                       &VolumeName,
                                       NULL,
                                       &DeviceName);
         
      if ( IsMountMgrVolumeName(&VolumeName) )
      {
         if ( RtlPrefixUnicodeString(&DeviceName,
                                     &(pObjectName->Name),
                                     FALSE) )
         {
            /* FILE_NAME_INFORMATION always returns a path starting with a single \. The volume name may or may not,
             * so we remove it if it does */
            if ( L'\\' == VolumeName.Buffer[(VolumeName.Length / sizeof(WCHAR)) - 1] )
            {
               VolumeName.Length -= sizeof(WCHAR);
            }

            /* Verify that the caller passed enough space for the full path */
            if ( (cchFilePath * sizeof(WCHAR)) < (VolumeName.Length + pFileName->FileNameLength + sizeof(WCHAR)) )
            {
               dwRet = ERROR_INSUFFICIENT_BUFFER;
               /* Failure */
               goto __CLEANUP;
            }
            
            /* Copy the volume name */
            CopyMemory(pwszFilePath,
                       VolumeName.Buffer,
                       VolumeName.Length);

            /* Append the file path */
            CopyMemory(reinterpret_cast<PUCHAR>(pwszFilePath) + VolumeName.Length,
                       pFileName->FileName,
                       pFileName->FileNameLength);

            dwRet = NO_ERROR;
            /* Success */
            goto __CLEANUP;
         }
      }
   }   

   /* Failure */
   dwRet = ERROR_PATH_NOT_FOUND;

__CLEANUP:
   FpiFreeMountManagerMountPoints(pMountPoints);
   FpiFreeMountManagerMountPoint(pMountPoint);
   FpiFreeObjectNameInformation(pObjectName);
   FpiFreeFileNameInformation(pFileName);
   
   /* Success / Failure */
   return ( dwRet );
}

DWORD
FpiQueryFileNameInformation(
   __in HANDLE hFile,
   __deref_out PFILE_NAME_INFORMATION* ppFileNameInfo
) NO_THROW
{
   NTSTATUS                NtStatus;
   size_t                  cbAllocate;
   ULONG                   cbRequired;
   PFILE_NAME_INFORMATION  pFileNameInfo;

   /* Initialize outputs */
   (*ppFileNameInfo) = NULL;

   /* Initialize locals */
   pFileNameInfo = NULL;

   /* Get the full path for the file. Note that this will not include the volume */
   cbAllocate = sizeof(FILE_NAME_INFORMATION);
   cbRequired = 0;
   
   #pragma warning( suppress : 4127 )
   while ( 1 )
   {
      if ( pFileNameInfo )
      {
         FpiFreeFileNameInformation(pFileNameInfo);
         pFileNameInfo = NULL;
      }

      /* Grow the buffer until we get a size that works */
      cbAllocate += (MAX_PATH_STREAM * sizeof(WCHAR));
      if ( cbAllocate < cbRequired )
      {
         cbAllocate += (2 * (cbRequired - cbAllocate));
      }
      
      /* Allocate the name buffer and initialize it */
      pFileNameInfo = reinterpret_cast<PFILE_NAME_INFORMATION>(malloc(cbAllocate));
      if ( !pFileNameInfo )
      {
         NtStatus = STATUS_NO_MEMORY;
         /* Failure */
         break;
      }

      ZeroMemory(pFileNameInfo,
                 cbAllocate);

      NtStatus = FpiQueryInformationFile(hFile,
                                         FileNameInformation,
                                         pFileNameInfo,
                                         static_cast<ULONG>(cbAllocate),
                                         &cbRequired);

      if ( NT_SUCCESS(NtStatus) )
      {
         (*ppFileNameInfo) = pFileNameInfo;
         /* Success */
         return ( NO_ERROR );
      }

      /* The first two of these will set cbRequired, the last will expect us to grow the buffer
       * ourselves. The code above handles that */
      if ( (STATUS_BUFFER_OVERFLOW != NtStatus) && (STATUS_BUFFER_TOO_SMALL != NtStatus) && (STATUS_INFO_LENGTH_MISMATCH != NtStatus) )
      {
         /* Failure */
         break;
      }
   }

   FpiFreeFileNameInformation(pFileNameInfo);

   /* Failure */
   return ( NtStatusToWin32Error(NtStatus) );
}

void
FpiFreeFileNameInformation(
   __deref_out PFILE_NAME_INFORMATION pFileNameInfo
) NO_THROW
{
   free(pFileNameInfo);
}

DWORD
FpiQueryObjectNameInformation(
   __in HANDLE hFile,
   __deref_out POBJECT_NAME_INFORMATION* ppObjectNameInfo
) NO_THROW
{
   NTSTATUS                   NtStatus;
   size_t                     cbAllocate;
   ULONG                      cbRequired;
   POBJECT_NAME_INFORMATION   pObjectNameInfo;

   /* Initialize outputs */
   (*ppObjectNameInfo) = NULL;

   /* Initialize locals */
   pObjectNameInfo = NULL;

   /* Get the full path for the file. Note that this will not include the volume */
   cbAllocate = sizeof(OBJECT_NAME_INFORMATION);
   cbRequired = 0;

   #pragma warning( suppress : 4127 )
   while ( 1 )
   {
      if ( pObjectNameInfo )
      {
         FpiFreeObjectNameInformation(pObjectNameInfo);
         pObjectNameInfo = NULL;
      }

      /* Grow the buffer until we get a size that works */
      cbAllocate += (MAX_PATH_NT * sizeof(WCHAR));
      if ( cbAllocate < cbRequired )
      {
         cbAllocate += (2 * (cbRequired - cbAllocate));
      }
      
      /* Allocate the name buffer and initialize it */
      pObjectNameInfo = reinterpret_cast<POBJECT_NAME_INFORMATION>(malloc(cbAllocate));
      if ( !pObjectNameInfo )
      {
         NtStatus = STATUS_NO_MEMORY;
         /* Failure */
         break;
      }

      ZeroMemory(pObjectNameInfo,
                 cbAllocate);

      NtStatus = FpiQueryObject(hFile,
                                ObjectNameInformation,
                                pObjectNameInfo,
                                static_cast<ULONG>(cbAllocate),
                                &cbRequired);

      if ( NT_SUCCESS(NtStatus) )
      {
         (*ppObjectNameInfo) = pObjectNameInfo;
         /* Success */
         return ( NO_ERROR );
      }

      /* The first two of these will set cbRequired, the last will expect us to grow the buffer
       * ourselves. The code above handles that */
      if ( (STATUS_BUFFER_OVERFLOW != NtStatus) && (STATUS_BUFFER_TOO_SMALL != NtStatus) && (STATUS_INFO_LENGTH_MISMATCH != NtStatus) )
      {
         /* Failure */
         break;
      }
   }

   FpiFreeObjectNameInformation(pObjectNameInfo);
   /* Failure */
   return ( NtStatusToWin32Error(NtStatus) );
}

void
FpiFreeObjectNameInformation(
   __in POBJECT_NAME_INFORMATION pObjectNameInfo
) NO_THROW
{
   free(pObjectNameInfo);
}

PMOUNTMGR_MOUNT_POINT
FpiBuildMountManagerMountPoint(
   __in_opt PVOID SymbolicLinkName,
   __in_opt USHORT SymbolicLinkNameLength,
   __in_opt PVOID UniqueId,
   __in_opt USHORT UniqueIdLength,
   __in_opt PVOID DeviceName,
   __in_opt USHORT DeviceNameLength
) NO_THROW
{

   size_t                cbAllocate;
   PMOUNTMGR_MOUNT_POINT pMountPoint;

   cbAllocate  = sizeof(MOUNTMGR_MOUNT_POINT);
   cbAllocate += SymbolicLinkNameLength;
   cbAllocate += UniqueIdLength;
   cbAllocate += DeviceNameLength;

   pMountPoint = reinterpret_cast<PMOUNTMGR_MOUNT_POINT>(malloc(cbAllocate));
   if ( pMountPoint )
   {
      ZeroMemory(pMountPoint,
                 cbAllocate);

      if ( SymbolicLinkName )
      {
         pMountPoint->SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
         pMountPoint->SymbolicLinkNameLength = SymbolicLinkNameLength;
         CopyMemory((PUCHAR)(pMountPoint) + pMountPoint->SymbolicLinkNameOffset,
                    SymbolicLinkName,
                    SymbolicLinkNameLength);
      }
      
      if ( UniqueId )
      {
         pMountPoint->UniqueIdOffset = sizeof(MOUNTMGR_MOUNT_POINT) + SymbolicLinkNameLength;
         pMountPoint->UniqueIdLength = UniqueIdLength;
         CopyMemory((PUCHAR)(pMountPoint) + pMountPoint->UniqueIdOffset,
                    UniqueId,
                    UniqueIdLength);
      }

      if ( DeviceName )
      {
         pMountPoint->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT) + SymbolicLinkNameLength + UniqueIdLength;
         pMountPoint->DeviceNameLength = DeviceNameLength;
         CopyMemory((PUCHAR)(pMountPoint) + pMountPoint->DeviceNameOffset,
                    DeviceName,
                    DeviceNameLength);
      }
   }

   /* Success / Failure */
   return ( pMountPoint );
}

void
FpiFreeMountManagerMountPoint(
   __in PMOUNTMGR_MOUNT_POINT pMountPoint
) NO_THROW
{
   free(pMountPoint);
}

ULONG
FpiGetMountManagerMountPointSize(
   __in PMOUNTMGR_MOUNT_POINT pMountPoint
) NO_THROW
{
   ULONG cbSize;

   cbSize = sizeof(MOUNTMGR_MOUNT_POINT) + pMountPoint->SymbolicLinkNameLength + pMountPoint->UniqueIdLength + pMountPoint->DeviceNameLength;

   /* Success */
   return ( cbSize );
}

DWORD
FpiQueryFileVolumeGuidName(
   __in HANDLE hFile,
   __out_ecount_z(cchVolumeName) LPWSTR pwszVolumeName,
   __in size_t cchVolumeName
)
{
   DWORD                      dwRet;
   BY_HANDLE_FILE_INFORMATION HandleInfo;
   HANDLE                     hFind;
   size_t                     cchVolumeNameLength;
   WCHAR                      chVolumeName[MAX_PATH+1];
   WCHAR                      chFileSystem[MAX_PATH+1];
   DWORD                      dwSerialNumber;
   DWORD                      dwComponentLength;
   DWORD                      dwFileSystemFlags;

   /* Initialize locals.. */
   hFind = INVALID_HANDLE_VALUE;

   ZeroMemory(&HandleInfo,
              sizeof(BY_HANDLE_FILE_INFORMATION));

   if ( !GetFileInformationByHandle(hFile,
                                    &HandleInfo) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   ZeroMemory(chVolumeName,
              sizeof(chVolumeName));

   hFind = FindFirstVolume(chVolumeName,
                           _countof(chVolumeName));

   if ( INVALID_HANDLE_VALUE == hFind )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   do
   {
      if ( !GetVolumeInformation(chVolumeName,
                                 NULL,
                                 0,
                                 &dwSerialNumber,
                                 &dwComponentLength,
                                 &dwFileSystemFlags,
                                 chFileSystem,
                                 _countof(chFileSystem)) )
      {
         dwRet = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }

      if ( dwSerialNumber == HandleInfo.dwVolumeSerialNumber )
      {
         /* Strip off any trailing \ */
         cchVolumeNameLength = wcslen(pwszVolumeName);
         if ( cchVolumeNameLength && (L'\\' == pwszVolumeName[cchVolumeNameLength - 1]) )
         {
            pwszVolumeName[--cchVolumeNameLength] = L'\0';
         }

         if ( FAILED(StringCchCopy(pwszVolumeName,
                                   cchVolumeName,
                                   chVolumeName)) )
         {
            dwRet = ERROR_INSUFFICIENT_BUFFER;
            /* Failure */
            goto __CLEANUP;
         }

         dwRet = NO_ERROR;
         /* Success */
         goto __CLEANUP;
      }

      ZeroMemory(chVolumeName,
                 sizeof(chVolumeName));
   }
   while ( FindNextVolume(hFind,
                          chVolumeName,
                          _countof(chVolumeName)) );

   /* Failure */
   dwRet = ERROR_PATH_NOT_FOUND;

__CLEANUP:
   if ( INVALID_HANDLE_VALUE != hFind )
   {
      FindVolumeClose(hFind);
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
FpiQueryMountManagerMountPoints(
   __in_opt PMOUNTMGR_MOUNT_POINT pMountPointFilter,
   __deref_out PMOUNTMGR_MOUNT_POINTS* ppMountPoints
) NO_THROW
{
   NTSTATUS                NtStatus;
   HANDLE                  hMountManager;
   size_t                  cbAllocate;
   ULONG                   cbRequired;
   MOUNTMGR_MOUNT_POINT    MountPoint;
   PMOUNTMGR_MOUNT_POINTS  pMountPoints;
   ULONG                   cbMountPointFilter;
   
   /* Initialize outputs */
   (*ppMountPoints) = NULL;

   /* Initialize locals */
   NtStatus      = STATUS_SUCCESS;
   pMountPoints  = NULL;
   hMountManager = INVALID_HANDLE_VALUE;

   /* If the caller didn't provide a MOUNTMGR_MOUNT_POINT to limit the query,
    * we have to construct an empty one */
   if ( pMountPointFilter )
   {
      cbMountPointFilter = FpiGetMountManagerMountPointSize(pMountPointFilter);
   }
   else
   {
      ZeroMemory(&MountPoint,
                 sizeof(MountPoint));
      
      pMountPointFilter  = &MountPoint;
      cbMountPointFilter = sizeof(MountPoint);
   }

   hMountManager = CreateFileW(MOUNTMGR_DOS_DEVICE_NAME,
                               0,
                               FILE_SHARE_READ|FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);

   if ( INVALID_HANDLE_VALUE == hMountManager )
   {
      /* Failure */
      return ( GetLastError() );
   }

   __try
   {      
      /* Get the full path for the file. Note that this will not include the volume */
      cbAllocate = sizeof(MOUNTMGR_MOUNT_POINTS);
      cbRequired = 0;
      
      #pragma warning( suppress : 4127 )      
      while ( 1 )
      {
         if ( pMountPoints )
         {
            FpiFreeMountManagerMountPoints(pMountPoints);
            pMountPoints = NULL;
         }

         /* Add space for a 2 MOUNTMGR_MOUNT_POINT, each with 3 paths */
         cbAllocate += (2 * (sizeof(MOUNTMGR_MOUNT_POINT) + (3 * (MAX_DEVICE_NAME * sizeof(WCHAR)))));
         if ( cbAllocate < cbRequired )
         {
            cbAllocate += (2 * (cbRequired - cbAllocate));
         }

         pMountPoints = reinterpret_cast<PMOUNTMGR_MOUNT_POINTS>(malloc(cbAllocate));
         if ( !pMountPoints )
         {
            NtStatus = STATUS_NO_MEMORY;
            /* Failure */
            break;
         }

         ZeroMemory(pMountPoints,
                    cbAllocate);

         /* I don't think this is required, but I've seen samples using it so we'll include it */
         pMountPoints->Size = static_cast<ULONG>(cbAllocate);

         /* Send the request and get the data */
         NtStatus = FpiDeviceIoControlFile(hMountManager,
                                           NULL,
                                           IOCTL_MOUNTMGR_QUERY_POINTS,
                                           pMountPointFilter,
                                           cbMountPointFilter,
                                           pMountPoints,
                                           static_cast<ULONG>(cbAllocate),
                                           &cbRequired);

         
         if ( NT_SUCCESS(NtStatus) )
         {
            /* Success */
            break;
         }

         /* The first two of these will set cbRequired, the last will expect us to grow the buffer
          * ourselves. The code above handles that */
         if ( (STATUS_BUFFER_OVERFLOW != NtStatus) && (STATUS_BUFFER_TOO_SMALL != NtStatus) && (STATUS_INFO_LENGTH_MISMATCH != NtStatus) )
         {
            /* Failure */
            break;
         }
      }
   }
   __finally
   {
      CloseHandle(hMountManager);
   }

   if ( NT_SUCCESS(NtStatus) )
   {
      (*ppMountPoints) = pMountPoints;
      /* Success */
      return ( NO_ERROR );
   }

   FpiFreeMountManagerMountPoints(pMountPoints);
   /* Failure */
   return ( NtStatusToWin32Error(NtStatus) );
}

void
FpiFreeMountManagerMountPoints(
   __in PMOUNTMGR_MOUNT_POINTS pMountPoints
) NO_THROW
{
   free(pMountPoints);
}

DWORD 
FpiDecodeFile( 
   __in const VOID* pFile, 
   __in DWORD dwOpenByType, 
   __deref_out HANDLE* phFile 
) NO_THROW
{
   DWORD dwRet;

   /* Initialize outputs */
   dwRet     = NO_ERROR;
   (*phFile) = INVALID_HANDLE_VALUE;
   
   /* First determine the correct file type, path or handle */
   switch ( dwOpenByType )
   {
      case FPF_OPENBY_FRAGCONTEXT:
      {
         /* Reset the file to the path the path */
         dwRet = FpiOpenFile(reinterpret_cast<PCFRAG_CONTEXT>(pFile)->FilePath,
                            phFile);
         break;
      }
      case FPF_OPENBY_FILENAME:
      {
         dwRet = FpiOpenFile(reinterpret_cast<LPCWSTR>(pFile),
                            phFile);
         break;
      }
      case FPF_OPENBY_FILEHANDLE:
      {
         /* Create a copy of the handle with explicit rights that we require */
         if ( !DuplicateHandle(GetCurrentProcess(),
                               reinterpret_cast<HANDLE>(const_cast<VOID*>(pFile)),
                               GetCurrentProcess(),
                               phFile,
                               FILE_READ_ATTRIBUTES|FILE_READ_DATA|SYNCHRONIZE,
                               FALSE,
                               0) )
         {
            /* Failure */
            dwRet = GetLastError();         
         }

         break;
      }
#ifdef _DEBUG
      default:
         _ASSERTE(IsValidFileOpenByType(dwOpenByType));
         break;
#endif /* _DEBUG */
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD FpiOpenFileVolume( PVOID pFile, DWORD dwFlags, HANDLE* phVolume ) NO_THROW
{
   DWORD  dwRet;
   HANDLE hVolume;
   size_t cchVolumeGuid;
   WCHAR  chVolumeGuid[50];
   WCHAR  chVolumeName[MAX_PATH+1];

   _ASSERTE(FlagOn(dwFlags, FPF_OPENBY_FILEHANDLE|FPF_OPENBY_FILENAME|FPF_OPENBY_FRAGCONTEXT));

   /* Initialize outputs */
   (*phVolume) = INVALID_HANDLE_VALUE;

   /* Initialize locals */
   hVolume = INVALID_HANDLE_VALUE;

   /* If we were passed a file handle, get the volume name */
   if ( FlagOn(dwFlags, FPF_OPENBY_FILEHANDLE) )
   {
      dwRet = FpiGetFileVolumePath(reinterpret_cast<HANDLE>(pFile),
                                   chVolumeName,
                                   _countof(chVolumeName));

      if ( NO_ERROR != dwRet )
      {
         /* Failure */
         return ( dwRet );
      }
   }
   else
   {      
      if ( !GetVolumePathNameW((FPF_OPENBY_FRAGCONTEXT & dwFlags ? reinterpret_cast<PFRAG_CONTEXT>(pFile)->FilePath : reinterpret_cast<LPCWSTR>(pFile)),
                            chVolumeName,
                            _countof(chVolumeName)) )
      {
         dwRet = GetLastError();
         /* Failure */
         return ( dwRet );
      }
   }
   


   /* Get the guid name for the volume */
   if ( !GetVolumeNameForVolumeMountPointW(chVolumeName,
                                           chVolumeGuid,
                                           _countof(chVolumeGuid)) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
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

   if ( INVALID_HANDLE_VALUE != hVolume )
   {
      (*phVolume) = hVolume;
      /* Success */
      return ( NO_ERROR );
   }

   dwRet = GetLastError();   
   /* Failure */
   return ( dwRet );
}

DWORD FpiGetFileVolumePath( HANDLE hFile, LPWSTR pwszBuf, size_t cchBuf ) NO_THROW
{
   DWORD                      dwRet;
   BY_HANDLE_FILE_INFORMATION HandleInfo;
   HANDLE                     hFind;
   WCHAR                      chVolumeName[MAX_PATH+1];
   WCHAR                      chFileSystem[MAX_PATH+1];
   DWORD                      dwSerialNumber;
   DWORD                      dwComponentLength;
   DWORD                      dwFileSystemFlags;

   /* Initialize locals.. */
   hFind = INVALID_HANDLE_VALUE;

   ZeroMemory(&HandleInfo,
              sizeof(BY_HANDLE_FILE_INFORMATION));

   if ( !GetFileInformationByHandle(hFile,
                                    &HandleInfo) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   ZeroMemory(chVolumeName,
              sizeof(chVolumeName));

   hFind = FindFirstVolume(chVolumeName,
                           _countof(chVolumeName));

   if ( INVALID_HANDLE_VALUE == hFind )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   do
   {
      if ( !GetVolumeInformation(chVolumeName,
                                 NULL,
                                 0,
                                 &dwSerialNumber,
                                 &dwComponentLength,
                                 &dwFileSystemFlags,
                                 chFileSystem,
                                 _countof(chFileSystem)) )
      {
         dwRet = GetLastError();
         /* Failure */
         goto __CLEANUP;
      }

      if ( dwSerialNumber == HandleInfo.dwVolumeSerialNumber )
      {
         if ( FAILED(StringCchCopy(pwszBuf,
                                   cchBuf,
                                   chVolumeName)) )
         {
            dwRet = ERROR_INSUFFICIENT_BUFFER;
            /* Failure */
            goto __CLEANUP;
         }

         dwRet = NO_ERROR;
         /* Success */
         goto __CLEANUP;
      }

      ZeroMemory(chVolumeName,
                 sizeof(chVolumeName));
   }
   while ( FindNextVolume(hFind,
                          chVolumeName,
                          _countof(chVolumeName)) );

   /* Failure */
   dwRet = ERROR_PATH_NOT_FOUND;

__CLEANUP:
   if ( INVALID_HANDLE_VALUE != hFind )
   {
      FindVolumeClose(hFind);
   }

   /* Success / Failure */
   return ( dwRet );
}

ULONG FpiGetFileClusterSize( LPCWSTR pwszFile ) NO_THROW
{
   BOOL  bRet;
   WCHAR chBuf[MAX_PATH];
   DWORD iSectorsPerCluster;
   DWORD iBytesPerSector;
   DWORD iFreeClusters;
   DWORD iTotalClusters;

   bRet = GetVolumePathName(pwszFile,
                            chBuf,
                            _countof(chBuf));

   if ( !bRet )
   {
      /* Failure */
      return ( 0 );
   }

   bRet = GetDiskFreeSpace(chBuf,
                           &iSectorsPerCluster,
                           &iBytesPerSector,
                           &iFreeClusters,
                           &iTotalClusters);

   if ( !bRet )
   {
      /* Failure */
      return ( 0 );
   }

   /* Success */
   return ( iSectorsPerCluster * iBytesPerSector );
}

DWORD FpiLocateFreeClusterRange( HANDLE hVolume, ULONGLONG iClustersRequired, ULONGLONG* piLCN ) NO_THROW
{
   DWORD                 dwRet;
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
      /* Number of bytes allocated for VOLUME_BITMAP_BUFFER::Buffer. This also has to be an integral
       * multiple of the number of bits in a ULONG (32) */
      SizeOfBitMap  = 1024 * 1024,
      /* Total number of bits that can fill VOLUME_BITMAP_BUFFER::Buffer */
      CountOfBitMap = SizeOfBitMap * CHAR_BIT,
      /* Number of bytes allocated for the entire VOLUME_BITMAP_BUFFER */
      SizeOfBuffer  = SizeOfBitMap + RTL_SIZEOF_THROUGH_FIELD(VOLUME_BITMAP_BUFFER, BitmapSize)
   };

   C_ASSERT(0 == (CountOfBitMap % (sizeof(ULONG) * CHAR_BIT)));

   _dTrace(1, L"Clusters required=%I64u\n", iClustersRequired);

   /* Initialize locals */
   pBuff = NULL;

   /* Allocate a buffer for the bitmap */
   pBuff = malloc(SizeOfBuffer);

   if ( !pBuff )
   {
      dwRet = ERROR_OUTOFMEMORY;
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

   /* warning C4127: conditional expression is constant	*/
   #pragma warning ( suppress : 4127 )
   while ( 1 )
   {
      iLCN    = (*piLCN);
      iBlocks = iClustersRequired / CountOfBitMap;
      bBlocks = (iBlocks > 0 ? TRUE : FALSE);
   
      while ( iBlocks )
      {
         /* Force all the bits to be set */
         FillMemory(pBuff,
                    SizeOfBuffer,
                    0xff);

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
            dwRet = GetLastError();

            if ( (ERROR_MORE_DATA != dwRet) && (ERROR_INSUFFICIENT_BUFFER != dwRet) )
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
            dwRet = ERROR_HANDLE_EOF;
            /* Failure */
            goto __CLEANUP;
         }

         /* Initialize the bitmap for this scan */
         RtlInitializeBitMap(&Bitmap,
                             reinterpret_cast<PULONG>(pMapData->Buffer),
                             cBitMapSize);

         DbgPrintBitMap(&Bitmap);

         /* Scan the bitmap for the first set bit, which indicates that this chunk is not
          * empty and so cannot be used */
         if ( !RtlAreBitsClear(&Bitmap,
                               cBitMapAdjust,
                               cBitMapSize) )
         {
            /* This block is not empty, so the search must start over at the start of the
             * last empty run in the bitmap */
            iBlocks = iClustersRequired / CountOfBitMap;

            iLCN += FpiFindLastClusterInUse(&Bitmap) + 1;
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

      /* warning C4127: conditional expression is constant	*/
      #pragma warning ( suppress : 4127 )
      while ( 1 )
      {
         /* Force all the bits to be set */
         FillMemory(pBuff,
                    SizeOfBuffer,
                    0xff);

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
            dwRet = GetLastError();

            if ( (ERROR_MORE_DATA != dwRet) && (ERROR_INSUFFICIENT_BUFFER != dwRet) )
            {
               /* Failure - Unable to retrieve bitmap */
               goto __CLEANUP;
            }
         }

         /* The StartingLcn may be rounded down and needs to be adjusted to what was passed */
         cBitMapAdjust = static_cast<ULONG>(iLCN - pMapData->StartingLcn.QuadPart);

         /* Determine the number of clusters in the returned data */
         cBitMapSize = FpiCalculateBitMapClusterCount(pMapData,
                                                      cbRet);

         /* Make sure enough data was returned to satisfy the request */
         if ( cBitMapSize < iRequired )
         {
            dwRet = ERROR_HANDLE_EOF;
            /* Failure */
            goto __CLEANUP;
         }

         /* Initialize the bitmap for this scan */
         RtlInitializeBitMap(&Bitmap,
                             reinterpret_cast<PULONG>(pMapData->Buffer),
                             cBitMapSize);

         DbgPrintBitMap(&Bitmap);

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
               iOffset = 0xffffffff;
            }
         }
         else
         {
            iOffset = RtlFindClearBits(&Bitmap,
                                       iRequired,
                                       0);
         }

         if ( 0xffffffff == iOffset )
         {
            /* This block is not empty, so the search must start over at the tail of the
             * current run, but only if it's empty. So we scan from the tail to the first
             * set bit and use that as our next starting LCN */
            iLCN += FpiFindLastClusterInUse(&Bitmap) + 1;
         }
         else
         {         
            /* The entire required cluster count has been located, so calculate the start
             * and return to the caller */
            iLCN    += iOffset;
            iLCN    += iRequired;
            iLCN    -= iClustersRequired;
            (*piLCN) = iLCN;
            dwRet    = NO_ERROR;
            /* Success */
            goto __CLEANUP;
         }
      }/* while ( 1 ) - Scan for single bitmap block */
   }/* while ( 1 ) - Scan for multiple bitmap blocks */

__CLEANUP:
   free(pBuff);

   /* Success / Failure */
   return ( dwRet );
}

ULONG FpiCalculateBitMapClusterCount( PVOLUME_BITMAP_BUFFER pBitMap, ULONG cbData ) NO_THROW
{
   ULONG cBitMap;

   /* Calculate the maximum number of clusters given the byte size of the entire VOLUME_BITMAP_BUFFER */
   cBitMap = (cbData - RTL_SIZEOF_THROUGH_FIELD(VOLUME_BITMAP_BUFFER, BitmapSize)) * CHAR_BIT;

   /* The VOLUME_BITMAP_BUFFER returns the number of clusters remaining on the disk, from the cluster
    * returned in the StartingLcn member. This can be less than the maximum specified by cBitMap so
    * take the lower of the two values */
   if ( static_cast<ULONGLONG>(cBitMap) > static_cast<ULONGLONG>(pBitMap->BitmapSize.QuadPart - pBitMap->StartingLcn.QuadPart) )
   {
      cBitMap = static_cast<ULONG>(pBitMap->BitmapSize.QuadPart - pBitMap->StartingLcn.QuadPart);
   }

   return ( cBitMap );
}

ULONG FpiFindLastClusterInUse( PRTL_BITMAP pBitMap ) NO_THROW
{
   ULONG idx;
   ULONG iBit;
   
   for ( idx = pBitMap->SizeOfBitMap / (sizeof(ULONG) * CHAR_BIT); idx > 0; idx-- )
   {
      if ( 0 != (0xffffffff & pBitMap->Buffer[idx - 1]) )
      {
         /* Find the highest bit set in this element so it can be added
          * to the total.. eg..
          * 11111111111111111111111111111100 - Bit 31
          * 00001111101111111111111111110000 - Bit 27
          * 00000000000000000000000000001000 - Bit 4
          * 00000000000000000000000000000000 - Bit 0
          */
         _BitScanReverse(&iBit,
                         pBitMap->Buffer[idx - 1]);

         return ( (((idx - 1) * (sizeof(ULONG) * CHAR_BIT))) + iBit );
      }
   }

   return ( 0 );
}


#ifdef _FRAGENG_DUMPBITMAP
VOID DbgPrintBitMap( PRTL_BITMAP BitMapHeader ) NO_THROW
{
    ULONG iRun;

    _dTrace(1, L"BitMap=0x%08lx, Elements=%u, Total=%u, Clear=%u, Set=%u, Longest=%u, Buffer=@0x%08lx\n", BitMapHeader, BitMapHeader->SizeOfBitMap / 32, BitMapHeader->SizeOfBitMap, RtlNumberOfClearBits(BitMapHeader), RtlNumberOfSetBits(BitMapHeader), RtlFindLongestRunClear(BitMapHeader, &iRun), BitMapHeader->Buffer);
}
#endif /* _FRAGENG_DUMPBITMAP */

/**********************************************************************
 *
 * Directory enumeration
 *
 **********************************************************************/
