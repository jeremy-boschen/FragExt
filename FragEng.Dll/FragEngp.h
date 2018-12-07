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

/* FragEngp.h
 *    FragEng private implementation interfaces
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include <List.h>
#include <SpinLock.h>

#include <NTApi.h>
#include "FragEng.h"


/*++
 
   Shared macros/functions

--*/

/*
 * Generic Win32 failure macro for functions that return NO_ERROR,
 * ERROR_SUCCESS or SEC_E_OK on success
 */
#define WINERROR( dw ) \
   (0 != (dw))

#ifndef NO_THROW
   #ifdef __cplusplus
      #define NO_THROW throw()
   #else /* __cplusplus */
      #define NO_THROW
   #endif /* __cplusplus */
#endif /* NO_THROW */

#ifndef FlagOn
   #define FlagOn(_F,_SF)        ((_F) & (_SF))
#endif
#ifndef BooleanFlagOn
   #define BooleanFlagOn(F,SF)   ((BOOLEAN)(((F) & (SF)) != 0))
#endif

/* Expected maximum length of an NT path */
#define MAX_DEVICE_NAME       (MAX_PATH + 9)
#define MAX_VOLUMEGUID_NAME   sizeof("\\??\\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\\")

#define MAX_PATH_STREAM       (MAX_PATH + 36)
#define MAX_PATH_NT           (MAX_DEVICE_NAME + MAX_PATH_STREAM)
#define MAX_PATH_GUID         (MAX_VOLUMEGUID_NAME + MAX_PATH_STREAM)

#define MAX_PATH_EXTENDED_CCH (UNICODE_STRING_MAX_CHARS + 1)
#define MAX_PATH_EXTENDED_CB  (UNICODE_STRING_MAX_BYTES + sizeof(WCHAR))

template < typename T > 
inline 
T 
RoundUp( 
   T cbValue, 
   ULONG cbAlign 
) NO_THROW
{
   return ( (cbValue + (cbAlign - 1)) & ~(T(cbAlign) - 1) );
}

/* This structure mirrors the physical layout of the RETRIEVAL_POINTERS_BUFFER::Extents elements 
 * and is used to make code a bit clearer when convering VCN to cluster count */
#pragma pack( push, 8 )
typedef struct _EXTENT
{
   union
   {
      LONGLONG  VCN;
      ULONGLONG ClusterCount;
   };
   LONGLONG     LCN;
}EXTENT, *PEXTENT;
typedef const struct _EXTENT* PCEXTENT;
#pragma pack( pop )

inline
BOOL
IsCompressionUnit(
   LONGLONG LogicalCluster
) NO_THROW
{
   return ( static_cast<LONGLONG>(-1) == LogicalCluster );
}

/* Helper routine for advancing NT style information structures */
template < typename T >
inline
T*
RtlNextEntryFromOffset(
   __in T* Information
)
{
   /* Structure must have a NextEntryOffset field, when it is 0 then there are no more records */
   if ( Information->NextEntryOffset )
   {
      return ( reinterpret_cast<T*>(reinterpret_cast<PUCHAR>(Information) + Information->NextEntryOffset) );
   }
   
   return ( NULL );
}

/* Copying a LARGE_INTEGER to a FILETIME */
inline
FILETIME
LargeIntegerToFileTime(
   __in PLARGE_INTEGER Time
)
{
   FILETIME FileTime = {
      Time->LowPart, 
      Time->HighPart
   };

   return ( FileTime );
}

DWORD 
FpiOpenFile( 
   __in LPCWSTR FileName, 
   __out HANDLE* FileHandle 
);


/*++  ++

RETRIEVAL_POINTERS_BUFFER Support

 -- --*/

DWORD 
FpiGetFileRetrievalPointers( 
   __in HANDLE FileHandle, 
   __deref_out_bcount(*RetrievalPointersLength) PRETRIEVAL_POINTERS_BUFFER* RetrievalPointers,
   __deref_out PULONG RetrievalPointersLength
);

inline
void
FpiFreeRetrievalPointers( 
   PRETRIEVAL_POINTERS_BUFFER RetrievalPointers
)
{
   if ( RetrievalPointers )
   {
      free(RetrievalPointers);
   }
}

inline
ULONGLONG
FpiGetClusterCountFromRetrievalPointers(
   __in const RETRIEVAL_POINTERS_BUFFER* RetrievalPointers
)
{
   PCEXTENT  pEntry;
   PCEXTENT  pTail;
   LONGLONG  iVCN;
   ULONGLONG ClusterCount;

   iVCN         = RetrievalPointers->StartingVcn.QuadPart;
   pEntry       = reinterpret_cast<PCEXTENT>(&(RetrievalPointers->Extents[0]));
   pTail        = reinterpret_cast<PCEXTENT>(&(RetrievalPointers->Extents[RetrievalPointers->ExtentCount]));
   ClusterCount = 0;
   
   while ( pEntry < pTail )
   {
      if ( !IsCompressionUnit(pEntry->LCN) )
      {
         ClusterCount += (pEntry->VCN - iVCN);
      }

      iVCN    = pEntry->VCN;
      pEntry += 1;
   }

   return ( ClusterCount );
}

#ifdef _DEBUG
inline
void
dbgDumpRetrievalPointersBuffer(
   PRETRIEVAL_POINTERS_BUFFER pData
)
{
   ULONG    iIdx;
   LONGLONG iVCN;
   LONGLONG iClustersTotal;
   LONGLONG iClustersAllocated;

   DbgPrintf(L"RETRIEVAL_POINTERS_BUFFER(%p) - ExtentCount:%u StartingVcn:%I64i\n", pData, pData->ExtentCount, pData->StartingVcn.QuadPart);

   for ( iIdx = 0, iVCN = pData->StartingVcn.QuadPart, iClustersTotal = 0, iClustersAllocated = 0; iIdx < pData->ExtentCount; iIdx++ )
   {
      DbgPrintf(L"\tExtent:%-8u VCN:%-8I64i LCN:%I64i to %-8I64i Clusters:%I64i\n",
                iIdx,
                pData->Extents[iIdx].NextVcn.QuadPart,
                pData->Extents[iIdx].Lcn.QuadPart,
                pData->Extents[iIdx].Lcn.QuadPart + (-1 == pData->Extents[iIdx].Lcn.QuadPart ? 0 : (pData->Extents[iIdx].NextVcn.QuadPart - iVCN)),
                pData->Extents[iIdx].NextVcn.QuadPart - iVCN);

      iClustersTotal += pData->Extents[iIdx].NextVcn.QuadPart - iVCN;

      if ( -1 != pData->Extents[iIdx].Lcn.QuadPart )
      {
         iClustersAllocated += pData->Extents[iIdx].NextVcn.QuadPart - iVCN;
      }

      iVCN = pData->Extents[iIdx].NextVcn.QuadPart;
   }

   DbgPrintf(L"Total clusters:%I64i Allocated clusters: %I64i\n",
             iClustersTotal,
             iClustersAllocated);
}
#else /* _DEBUG */
#define dbgDumpRetrievalPointersBuffer __noop
#endif /* _DEBUG */

/*++ ++

   FRAG_CONTEXT Support

 -- --*/

typedef struct _FRAG_CONTEXT
{   
   /* 
    * Fragment data 
    */
   ULONGLONG               ClusterCount;
   ULONG                   ExtentCount;
   ULONG                   FragmentCount;
   PFRAGMENT_INFORMATION   FragmentInformation;

   /* 
    * Flags passed when opening the context 
    */
   ULONG                   Flags;
   
   /* 
    * Size, in bytes, of a cluster on the file's volume 
    */
   ULONG                   ClusterSize;   
   /* 
    * Size, in bytes, of the all the fragments, including compressed units 
    */
   ULONGLONG               FileSize;
   /* 
    * Size, in bytes, of all the fragments, not including compressed units 
    */
   ULONGLONG               FileSizeOnDisk;
   
   //TOBEREMOVED
   WCHAR                   FilePath[1];
}FRAG_CONTEXT, *PFRAG_CONTEXT;
typedef const struct _FRAG_CONTEXT* PCFRAG_CONTEXT;

#define FPIF_OPENBY_MASK 0x00000003

#define HandleToFragCtx( FragCtx ) reinterpret_cast<PFRAG_CONTEXT>( (FragCtx) )

#define FragCtxToHandle( FragCtx ) reinterpret_cast<HANDLE>( (FragCtx) )

#define IsValidFragCtxHandle( FragCtx ) ((FragCtx) ? TRUE : FALSE)

/* Helpers for extracting settings from Flags parameters */
inline
DWORD
FlagsToFileOpenByType(
   __in DWORD dwFlags
) 
{
   return ( FPIF_OPENBY_MASK & dwFlags );
}

inline
BOOL
IsValidFileOpenByType(
   __in DWORD dwFlags
)
{
   DWORD dwOpenType;
   dwOpenType = FlagsToFileOpenByType(dwFlags);

   return ( (FPF_OPENBY_FILENAME    == dwOpenType) ||
            (FPF_OPENBY_FILEHANDLE  == dwOpenType) ||
            (FPF_OPENBY_FRAGCONTEXT == dwOpenType) );
}

inline 
DWORD 
FlagsToClusterSkipCount( 
   __in DWORD dwFlags 
) NO_THROW
{
   return ( (dwFlags & FPF_DEFRAG_SKIPBLOCKCOUNTMASK) >> FPF_DEFRAG_SKIPBLOCKCOUNTSHIFT );
}

inline 
DWORD 
FlagsToClusterBlockCount( 
   __in DWORD dwFlags 
) NO_THROW
{
   return ( (dwFlags & FPF_DEFRAG_CLUSTERBLOCKCOUNTMASK) >> FPF_DEFRAG_CLUSTERBLOCKCOUNTSHIFT );
}

inline 
DWORD 
FlagsToCallbackIncrement( 
   __in DWORD dwFlags 
) NO_THROW
{
   return ( (dwFlags & FPF_DEFRAG_CALLBACKINCREMENTMASK) >> FPF_DEFRAG_CALLBACKINCREMENTSHIFT );
}

/* Context creation/cleanup */
DWORD 
FpiCreateFragCtx( 
   __in HANDLE FileHandle, 
   __in DWORD Flags,
   __deref_out PFRAG_CONTEXT* FragCtx 
);

void 
FpiDestroyFragCtx( 
   __in PFRAG_CONTEXT pFragCtx
);

DWORD 
FpiGetFragmentInformation( 
   __in HANDLE FileHandle, 
   __deref_out_ecount(*FragmentInformationCount) PFRAGMENT_INFORMATION* FragmentInformation, 
   __deref_out ULONG* FragmentInformationCount
);

inline
void 
FpiFreeFragmentInformation( 
   __in PFRAGMENT_INFORMATION FragmentInformation 
)
{
   if ( FragmentInformation )
   {
      free(FragmentInformation);
   }
}

DWORD 
FpiCompactFragmentInformation( 
   __deref_inout_ecount(*pFragmentCount) PFRAGMENT_INFORMATION* ppFragmentInfo, 
   __deref_inout ULONG* pFragmentCount
);

typedef enum FRAGMENTINFORMATION_SORTTYPE
{
   FragmentSortTypeSequence,
   FragmentSortTypeExtentCount,
   FragmentSortTypeClusterCount,
   FragmentSortTypeLogicalCluster,
   FragmentSortTypeInvalid
}FRAGMENTINFORMATION_SORTTYPE;

void
FpiSortFragmentInformation(
   __in FRAGMENTINFORMATION_SORTTYPE SortType,
   __inout_ecount(InformationCount) PFRAGMENT_INFORMATION* FragmentInformation,
   __in ULONG InformationCount
);

int
__cdecl
FpiFragmentInformationCompareSequence(
   const void* Element1,
   const void* Element2
);

int
__cdecl
FpiFragmentInformationCompareExtentCount(
   const void* Element1,
   const void* Element2
);

int
__cdecl
FpiFragmentInformationCompareClusterCount(
   const void* Element1,
   const void* Element2
);

int
__cdecl
FpiFragmentInformationCompareLogicalCluster(
   const void* Element1,
   const void* Element2
);

typedef int (__cdecl* PCOMPAREROUTINE)(const void*, const void*);

/* Debugging helpers */
#ifdef _DEBUG
inline 
void 
dbgDumpFragmentInformation( 
   __in PFRAGMENT_INFORMATION pFragInfo, 
   __in ULONG iFragCount 
) NO_THROW
{
   DbgPrintf(L"FRAGMENT_INFORMATION - Count:%u\n", iFragCount);

   while ( iFragCount-- )
   {
      DbgPrintf(L"\t#%-8u Extents:%-8u Clusters:%-16I64u LCN:%-16I64u\n",
                pFragInfo->Sequence,
                pFragInfo->ExtentCount,
                pFragInfo->ClusterCount,
                pFragInfo->LogicalCluster);

      pFragInfo++;
   }
}

inline 
void 
dbgDumpFragCtx( 
   __in PFRAG_CONTEXT pFragCtx 
) NO_THROW
{
   DbgPrintf(L"FRAG_CONTEXT(%p)\n"
             L"\t   ExtentCount: %u\n"
             L"\t  ClusterCount: %I64u\n"
             L"\t         Flags: %x\n"
             L"\t   ClusterSize: %u\n"
             L"\t      FileSize: %I64u\n"
             L"\tFileSizeOnDisk: %I64u\n"
             L"\t      FileName: %s\n",
             pFragCtx,
             pFragCtx->ExtentCount,
             pFragCtx->ClusterCount,
             pFragCtx->Flags,
             pFragCtx->ClusterSize,
             pFragCtx->FileSize,
             pFragCtx->FileSizeOnDisk,
             pFragCtx->FilePath);

   dbgDumpFragmentInformation(pFragCtx->FragmentInformation,
                              pFragCtx->FragmentCount);

}

#else /* _DEBUG */
#define dbgDumpFragmentInformation __noop
#define dbgDumpFragCtx __noop
#endif /* _DEBUG */

/*++
 
   Defrag Plan

   A defrag plan is an ordered series of file move operations executed by the
   defragmenter. Some operations move sections of a file out to make room for
   other sections, while other operations move sections into their properly
   defragmented place. The defragmenter doesn't really care once it starts
   working, it just executes the steps.

   Plans are built to satisfy certain requirements, like fastest defrag possible,
   partial defrag, full defrag or placing files in optimal locations on a volume.

--*/

#define FPIF_DEFRAG_OPERATION_MOVE_IN  0
#define FPIF_DEFRAG_OPERATION_MOVE_OUT 1

typedef struct _DEFRAG_OPERATION
{
   LIST_ENTRY     Entry;
   ULONGLONG      MoveType    : 1;
   ULONGLONG      StartingVcn : 63;
   ULONGLONG      ClusterCount;
}DEFRAG_OPERATION, *PDEFRAG_OPERATION;

typedef struct _DEFRAG_PLAN
{
   DWORD      Flags;
   LIST_ENTRY Operations;
}DEFRAG_PLAN, *PDEFRAG_PLAN;

typedef struct _FILE_VOLUME_BITMAP
{
   RTL_BITMAP BitMap;
}FILE_VOLUME_BITMAP, *PFILE_VOLUME_BITMAP;

/* Attempt to defragment the file into a single fragment */
#define FPIF_DEFRAGPLAN_COMPACT_FULL      0x00000001
/* Attempt to defragment the file into multiple fragments, each at least the size specified by the
 * registry setting FRAGENG_MINIMUMFRAGMENTSIZE */
#define FPI_DEFRAGPLAN_COMPACT_BLOCK      0x00000002


///////// there is really only 1 method, block. when full is requested, the file size is queried and
///////// the block size becomes the file size.


/* Attempt to defragment the file around the largest fragment possible */
#define FPIF_DEFRAGPLAN_PLACE_LOCAL          0x00000010
/* Attempt to defragment the file in the best possible location for its usage */
#define FPIF_DEFRAGPLAN_PLACE_OPTIMAL        0x00000020

typedef enum _VOLUME_LOCATION
{
   /* Volume locations for optimal placement */
   VolumeLocationStart,
   VolumeLocationMiddle,
   VolumeLocationEnd
}VOLUME_LOCATION;

enum
{
   /* Default minimum fragment size, in bytes */
   FPI_DEFRAGPLAN_DEFAULT_MINIMUM_FRAGMENT_SIZE         = 64 * 1024 * 1024,

   /* Default last accessed time delta, in days */
   FPI_DEFRAGPLAN_DEFAULT_FILEACCESSED_DELTA_FREQUENT   = 30,
   FPI_DEFRAGPLAN_DEFAULT_FILEACCESSED_DELTA_INFREQUENT = 60,

   /* Default last modified time delta, in days */
   FPI_DEFRAGPLAN_DEFAULT_FILEMODIFIED_DELTA_FREQUENT   = 30,
   FPI_DEFRAGPLAN_DEFAULT_FILEMODIFIED_DELTA_INFREQUENT = 60
};

inline
ULONGLONG
DaysToFileTimeUnits(
   __in ULONGLONG Days
)   
{
   /* Convert days to 100ns units */
   return ( Days * 24 * 60 * 60 * 10000000 );
}

DWORD
FpiBuildFileDefragPlan(
   __in HANDLE File,
   __in DWORD Flags,
   __inout PDEFRAG_PLAN DefragPlan
);

void
FpiDestroyFileDefragPlan(
   __in PDEFRAG_PLAN DefragPlan
);

DWORD
FpiBuildDefragPlanOperationList(
   __in HANDLE FileHandle,
   __in DWORD Flags,
   __in LONGLONG StartingVcn,
   __in ULONGLONG ClustersToMove,
   __inout PDEFRAG_PLAN DefragPlan
);

DWORD
FpiOpenFileVolume(
   __in HANDLE FileHandle,
   __out PHANDLE VolumeHandle
);

DWORD
FpiOpenFileVolumeHandle(
   __in HANDLE FileHandle,
   __out PHANDLE VolumeHandle
);

DWORD
FpiBuildFileVolumeBitMap(
   HANDLE FileHandle,
   __out PFILE_VOLUME_BITMAP* FileBitMap
);

inline
void
FpiDestroyFileVolumeBitMap(
   __in PFILE_VOLUME_BITMAP FileBitMap
)
{
   free(FileBitMap);
}

/* Plan helpers */
DWORD
FpiGetFileClusterByteSize(
   __in HANDLE FileHandle,
   __out PULONG BytesPerCluster
);

ULONGLONG
FpiGetFileClusterBlockCount(
   __in HANDLE FileHandle,
   __in DWORD Flags
);

/* Optimal placement support */

DWORD
FpiLocateStartingTargetLcn(
   __in HANDLE FileHandle,
   __in DWORD Flags,
   __in PRETRIEVAL_POINTERS_BUFFER RetrievalPointers,
   __out LONGLONG* StartingTargetLcn
);

VOLUME_LOCATION
FpiGetFileOptimalVolumeLocation(
   HANDLE FileHandle
);

LONGLONG
FpiGetStartingLcnHintForLocation(
   HANDLE VolumeHandle,
   VOLUME_LOCATION VolumeLocation
);

inline
int
IsExpiredFileTime( 
   __in const FILETIME* FileTime, 
   __in ULONGLONG TimeDelta
)
{
   FILETIME       SystemFileTime;
   ULARGE_INTEGER SystemTimeDelta;

   GetSystemTimeAsFileTime(&SystemFileTime);
   
   /* Adjust the system time */
   SystemTimeDelta.LowPart   = SystemFileTime.dwLowDateTime;
   SystemTimeDelta.HighPart  = SystemFileTime.dwHighDateTime;
   SystemTimeDelta.QuadPart -= TimeDelta;

   /* Copy the system time back into the file time */
   SystemFileTime.dwLowDateTime  = SystemTimeDelta.LowPart;
   SystemFileTime.dwHighDateTime = SystemTimeDelta.HighPart;

   /* The filetime is expired if it's equal or later than the adjusted system time */
   return ( CompareFileTime(FileTime,
                            &SystemFileTime) >= 0 );
}

/*++

   File/Volume support routines

--*/
DWORD 
FpiGetFullFilePath( 
   __in HANDLE FileHandle, 
   __out_ecount_z(cchFileName) LPWSTR pwszFilePath, 
   __in size_t cchFilePath
);

DWORD
FpiQueryFileNameInformation(
   __in HANDLE hFile,
   __deref_out PFILE_NAME_INFORMATION* ppFileNameInfo
);

void
FpiFreeFileNameInformation(
   __deref_out PFILE_NAME_INFORMATION pFileNameInfo
);

DWORD
FpiQueryFileVolumeGuidName(
   __in HANDLE hFile,
   __out_ecount_z(cchVolumeName) LPWSTR pwszVolumeName,
   __in size_t cchVolumeName
);

DWORD
FpiQueryObjectNameInformation(
   __in HANDLE hFile,
   __deref_out POBJECT_NAME_INFORMATION* ppObjectNameInfo
);

void
FpiFreeObjectNameInformation(
   __in POBJECT_NAME_INFORMATION pObjectNameInfo
);

PMOUNTMGR_MOUNT_POINT
FpiBuildMountManagerMountPoint(
   __in_opt PVOID SymbolicLinkName,
   __in_opt USHORT SymbolicLinkNameLength,
   __in_opt PVOID UniqueId,
   __in_opt USHORT UniqueIdLength,
   __in_opt PVOID DeviceName,
   __in_opt USHORT DeviceNameLength
);

void
FpiFreeMountManagerMountPoint(
   __in PMOUNTMGR_MOUNT_POINT pMountPoint
);

ULONG
FpiGetMountManagerMountPointSize(
   __in PMOUNTMGR_MOUNT_POINT pMountPoint
);

DWORD
FpiQueryMountManagerMountPoints(
   __in PMOUNTMGR_MOUNT_POINT pMountPointFilter,
   __deref_out PMOUNTMGR_MOUNT_POINTS* ppMountPoints
);

void
FpiFreeMountManagerMountPoints(
   __in PMOUNTMGR_MOUNT_POINTS pMountPoints
);

#ifdef _DEBUG
inline
void
dbgDumpMountMgrMountPoint(
   PUCHAR pOffsetBase,
   PMOUNTMGR_MOUNT_POINT pMountPoint
)
{
   PCHAR  pUniqueId;
   ULONG  cbUniqueId;

   PULONG BasicId;
   GUID*  DmioGuid;
   WCHAR  chUniqueId[MAX_PATH_NT];
   
   enum
   {
      CB_DMIOID_PREFIX = sizeof("DMIO:ID:") - sizeof('\0')
   };

   pUniqueId  = (PCHAR)(pOffsetBase + pMountPoint->UniqueIdOffset);
   cbUniqueId = pMountPoint->UniqueIdLength;

   if ( (cbUniqueId >= sizeof(WCHAR)) && (L'\\' == ((WCHAR*)pUniqueId)[0]) )
   {
      StringCchPrintfW(chUniqueId,
                       _countof(chUniqueId),
                       L"%.*s",
                       cbUniqueId / sizeof(WCHAR), pUniqueId);
   }
   else if ( (cbUniqueId >= CB_DMIOID_PREFIX) && (0 == strncmp(pUniqueId, "DMIO:ID:", CB_DMIOID_PREFIX)) )
   {
      DmioGuid = (GUID*)(pUniqueId + CB_DMIOID_PREFIX);
      StringCchPrintfW(chUniqueId,
                       _countof(chUniqueId),
                       L"DMIO:ID{%08x-%04x-%04x-%2x%2x-%02x%02x%02x%02x%02x%02x}",
                       DmioGuid->Data1,    DmioGuid->Data2,    DmioGuid->Data3,
                       DmioGuid->Data4[0], DmioGuid->Data4[1], DmioGuid->Data4[2], DmioGuid->Data4[3],
                       DmioGuid->Data4[4], DmioGuid->Data4[5], DmioGuid->Data4[6], DmioGuid->Data4[7]);
   }
   else if ( cbUniqueId )
   {
      BasicId = (PULONG)pUniqueId;
      StringCchPrintfW(chUniqueId, _countof(chUniqueId),
                       L"BASIC:Signature %x - Offset %x - Length %x",
                       BasicId[0], BasicId[1], BasicId[2]);
   }
   else
   {
      (*chUniqueId) = L'\0';
   }
   
   DbgPrintf(L"MOUNTMGR_MOUNT_POINT(%p)\n"
             L"\tSymbolicLinkName[%u]:%.*s\n"
             L"\t        UniqueId[%u]:%s\n"
             L"\t      DeviceName[%u]:%.*s\n", 
             pOffsetBase, 
             pMountPoint->SymbolicLinkNameLength,
             pMountPoint->SymbolicLinkNameLength / sizeof(WCHAR), pOffsetBase + pMountPoint->SymbolicLinkNameOffset,
             pMountPoint->UniqueIdLength,
             chUniqueId,
             pMountPoint->DeviceNameLength,
             pMountPoint->DeviceNameLength / sizeof(WCHAR),       pOffsetBase + pMountPoint->DeviceNameOffset);
}

inline
void
dbgDumpMountMgrMountPoints(
   PMOUNTMGR_MOUNT_POINTS pMountPoints
)
{  
   ULONG iIdx;

   DbgPrintf(L"MOUNTMGR_MOUNT_POINTS(%p) Size:%u, NumberOfMountPoints:%u\n", pMountPoints, pMountPoints->Size, pMountPoints->NumberOfMountPoints);

   for ( iIdx = 0; iIdx < pMountPoints->NumberOfMountPoints; iIdx++ )
   {
      dbgDumpMountMgrMountPoint((PUCHAR)pMountPoints,
                                &(pMountPoints->MountPoints[iIdx]));
   }
}
#else /* _DEBUG */
#define dbgDumpMountMgrMountPoint  __noop
#define dbgDumpMountMgrMountPoints __noop
#endif /* _DEBUG */

///
// ctx stuff
///

DWORD FpiDecodeFile( const VOID* pFile, DWORD Flags, HANDLE* phFile ) NO_THROW;
DWORD FpiOpenFileVolume( PVOID pFile, DWORD dwFlags, HANDLE* phVolume ) NO_THROW;
DWORD FpiGetFileVolumePath( HANDLE hFile, LPWSTR pwszBuf, size_t cchBuf ) NO_THROW;



ULONG FpiGetFileClusterSize( LPCWSTR pwszFile ) NO_THROW;

DWORD FpiLocateFreeClusterRange( HANDLE hVolume, ULONGLONG iClusterCount, ULONGLONG* piLCN ) NO_THROW;
ULONG FpiCalculateBitMapClusterCount( PVOLUME_BITMAP_BUFFER pBitmap, ULONG cbData ) NO_THROW;
ULONG FpiFindLastClusterInUse( PRTL_BITMAP pBitMap ) NO_THROW;

#ifdef _FRAGENG_DUMPBITMAP
VOID DbgPrintBitMap( PRTL_BITMAP BitMapHeader );
#else
#define DbgPrintBitMap __noop
#endif /* _FRAGENG_DUMPBITMAP */

/**
  * File defragmenting
  **/
 enum
{
   FPIF_DEFRAG_CLUSTERBLOCKSIZE_MIN  = 1,
   FPIF_DEFRAG_CALLBACKINCREMENT_MIN = 1
};

 /************** BITMAPS ******************/

typedef struct _FILE_BITMAP
{
   HANDLE         File;
   HANDLE         Volume;

   PFRAGMENT_INFORMATION  FragInfo;
   ULONG          FragCount;

   RTL_BITMAP     FileMap;
}FILE_BITMAP, *PFILE_BITMAP;

inline void FileBitmapReset( PFILE_BITMAP FileBitmap )
{
   FileBitmap->File   = INVALID_HANDLE_VALUE;
   FileBitmap->Volume = INVALID_HANDLE_VALUE;

   RtlInitializeBitMap(&(FileBitmap->FileMap),
                       NULL,
                       0);

   FileBitmap->FragInfo  = NULL;
   FileBitmap->FragCount = 0;
}

DWORD FileBitmapInitialize( PFILE_BITMAP FileBitmap, DWORD Flags, PVOID File );
void FileBitmapUninitialize( PFILE_BITMAP FileBitmap );

DWORD FileBitmapLoadFileMap( HANDLE File, PRTL_BITMAP pBitMap );
DWORD FileBitmapFreeFileMap( PRTL_BITMAP pBitMap );

/*++

   NT NATIVE API HELPERS

--*/

inline
NTSTATUS
FpiQueryAttributesFile(
   __in_z LPCWSTR NtFilePath,
   __out PFILE_BASIC_INFORMATION FileInformation
)
{
   NTSTATUS          NtStatus;
   UNICODE_STRING    ObjectNameString;
   OBJECT_ATTRIBUTES ObjectAttributes;

   RtlInitUnicodeString(&ObjectNameString,
                        NtFilePath);

   InitializeObjectAttributes(&ObjectAttributes,
                              &ObjectNameString,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   NtStatus = NtQueryAttributesFile(&ObjectAttributes,
                                    FileInformation);

   /* Success / Failure */
   return ( NtStatus );
}

inline
NTSTATUS
FpiQueryFullAttributesFile(
   __in_z LPCWSTR NtFilePath,
   __out PFILE_NETWORK_OPEN_INFORMATION FileInformation
)
{
   NTSTATUS          NtStatus;
   UNICODE_STRING    ObjectNameString;
   OBJECT_ATTRIBUTES ObjectAttributes;

   RtlInitUnicodeString(&ObjectNameString,
                        NtFilePath);

   InitializeObjectAttributes(&ObjectAttributes,
                              &ObjectNameString,
                              OBJ_CASE_INSENSITIVE,
                              NULL,
                              NULL);

   NtStatus = NtQueryFullAttributesFile(&ObjectAttributes,
                                        FileInformation);

   /* Success / Failure */
   return ( NtStatus );
}

inline
NTSTATUS 
FpiQueryInformationFile(
   __in HANDLE FileHandle,
   __in FILE_INFORMATION_CLASS FileInformationClass,
   __out_bcount(Length) PVOID FileInformation,
   __in ULONG Length,
   __out_opt PULONG ReturnLength
)
{
   NTSTATUS        NtStatus;
   IO_STATUS_BLOCK IoStatus;

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

   /* Success / Failure */
   return ( NtStatus );
}

inline
NTSTATUS
FpiQueryDirectoryFile(
   __in HANDLE FileHandle,
   __in_opt HANDLE Event,
   __in FILE_INFORMATION_CLASS FileInformationClass,
   __out_bcount(Length) PVOID FileInformation,
   __in ULONG Length,
   __out_opt PULONG ReturnLength,
   __in BOOLEAN ReturnSingleEntry,
   __in_opt LPCWSTR FileName,
   __in BOOLEAN RestartScan
)
{
   NTSTATUS        NtStatus;
   IO_STATUS_BLOCK IoStatus;
   UNICODE_STRING  FileNameString;

   IoStatus.Pointer     = NULL;
   IoStatus.Information = 0;

   if ( FileName )
   {
      RtlInitUnicodeString(&FileNameString,
                           FileName);
   }

   NtStatus = NtQueryDirectoryFile(FileHandle,
                                   Event,
                                   NULL,
                                   NULL,
                                   &IoStatus,
                                   FileInformation,
                                   Length,
                                   FileInformationClass,
                                   ReturnSingleEntry,
                                   FileName ? &FileNameString : NULL,
                                   RestartScan);

   if ( ReturnLength )
   {
      (*ReturnLength) = static_cast<ULONG>(IoStatus.Information);
   }

   /* Success / Failure */
   return ( NtStatus );
}

inline
NTSTATUS
FpiQueryVolumeInformationFile(
   __in HANDLE FileHandle,
   __in FS_INFORMATION_CLASS FsInformationClass,
   __out_bcount(Length) PVOID FsInformation,
   __in ULONG Length,
   __out_opt PULONG ReturnLength
)
{
   NTSTATUS        NtStatus;
   IO_STATUS_BLOCK IoStatus;

   IoStatus.Pointer     = NULL;
   IoStatus.Information = 0;

   NtStatus = NtQueryVolumeInformationFile(FileHandle,
                                           &IoStatus,
                                           FsInformation,
                                           Length,
                                           FsInformationClass);

   if ( ReturnLength )
   {
      (*ReturnLength) = static_cast<ULONG>(IoStatus.Information);
   }

   /* Success / Failure */
   return ( NtStatus );
}

inline
NTSTATUS
FpiDeviceIoControlFile(
   __in HANDLE FileHandle,
   __in_opt HANDLE Event,
   __in ULONG IoControlCode,
   __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
   __in ULONG InputBufferLength,
   __out_bcount_opt(OutputBufferLength) PVOID OutputBuffer,
   __in ULONG OutputBufferLength,
   __out_opt PULONG ReturnLength
)
{
   NTSTATUS        NtStatus;
   IO_STATUS_BLOCK IoStatus;

   IoStatus.Pointer     = NULL;
   IoStatus.Information = 0;

   NtStatus = NtDeviceIoControlFile(FileHandle,
                                    Event,
                                    NULL,
                                    NULL,
                                    &IoStatus,
                                    IoControlCode,
                                    InputBuffer,
                                    InputBufferLength,
                                    OutputBuffer,
                                    OutputBufferLength);

   if ( ReturnLength )
   {
      (*ReturnLength) = static_cast<ULONG>(IoStatus.Information);
   }

   /* Success / Failure */
   return ( NtStatus );
}

inline
NTSTATUS
FpiQueryObject(
   __in HANDLE Handle,
   __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
   __out_bcount_opt(ObjectInformationLength) PVOID ObjectInformation,
   __in ULONG ObjectInformationLength,
   __out_opt PULONG ReturnLength
)
{   
   NTSTATUS NtStatus;

   NtStatus = NtQueryObject(Handle,
                            ObjectInformationClass,
                            ObjectInformation,
                            ObjectInformationLength,
                            ReturnLength);
   
   /* Success / Failure */
   return ( NtStatus );
}
