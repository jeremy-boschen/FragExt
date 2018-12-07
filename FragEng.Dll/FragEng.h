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

/* FragEng.h
 *    FragEng API definitions
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#ifndef __FRAGEXP_H__
#define __FRAGEXP_H__

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**********************************************************************

   Public

 **********************************************************************/

#ifdef _WIN64
   #define FRAGAPI
#else /* _WIN64 */
   #define FRAGAPI __stdcall 
#endif /* _WIN64 */

/**
 * Shared
 **/

/* Shared flags */
#define FPF_OPENBY_FILENAME               0x00000001
#define FPF_OPENBY_FILEHANDLE             0x00000002
#define FPF_OPENBY_FRAGCONTEXT            0x00000003

/* Context initilization flags */
#define FPF_CTX_DETACHED                  0x00000000
#define FPF_CTX_RAWEXTENTDATA             0x00000010

#define GetDefragmentRightsRequired( FileAttributes ) \
   ((FILE_ATTRIBUTE_ENCRYPTED & (FileAttributes) ? FILE_READ_DATA|FILE_WRITE_DATA|FILE_APPEND_DATA : 0)|FILE_READ_ATTRIBUTES|SYNCHRONIZE)

/*
   Fragmentation Information
 */
typedef struct _FRAGMENT_INFORMATION
{     
   ULONG     Sequence;
   ULONG     ExtentCount;
   ULONGLONG ClusterCount;
   ULONGLONG LogicalCluster;      
}FRAGMENT_INFORMATION, *PFRAGMENT_INFORMATION;
typedef const struct _FRAGMENT_INFORMATION* PCFRAGMENT_INFORMATION;

typedef struct _FRAGCTX_INFORMATION
{
   ULONGLONG   FileSize;
   ULONGLONG   FileSizeOnDisk;
   ULONGLONG   ClusterCount;
   ULONG       ClusterSize;
   ULONG       ExtentCount;
   ULONG       FragmentCount;
}FRAGCTX_INFORMATION, *PFRAGCTX_INFORMATION;

__inline 
BOOL 
IsVirtualClusterNumber( 
   __in LONGLONG iLCN 
) 
{
   return ( -1 == iLCN );
}

__inline
BOOL
IsCompressionFragment(
   PFRAGMENT_INFORMATION FragmentInformation
)
{
   return ( ((ULONGLONG)-1) == FragmentInformation->LogicalCluster );
}

DWORD 
FRAGAPI 
FpCreateContext( 
   __in_z LPCWSTR FileName,
   __in DWORD Flags, 
   __deref_out HANDLE* FragCtx 
);

DWORD 
FRAGAPI 
FpCreateContextEx( 
   __in HANDLE FileHandle,
   __in DWORD Flags, 
   __deref_out HANDLE* FragCtx 
);

VOID  
FRAGAPI 
FpCloseContext( 
   __in HANDLE FragCtx 
);

DWORD 
FRAGAPI 
FpGetContextInformation( 
   __in HANDLE FragCtx, 
   __out PFRAGCTX_INFORMATION CtxInfo 
);

__declspec(deprecated) 
DWORD 
FRAGAPI 
FpGetContextFilePath( 
   __in HANDLE FragCtx, 
   __out LPWSTR FileName, 
   __in ULONG FileNameLength 
);

DWORD
FRAGAPI
FpGetContextFragmentInformation(
   __in HANDLE FragCtx,
   __in ULONG StartingSequence,
   __out_ecount_part(FragmentInformationCount, *FragmentInformationReturned) PFRAGMENT_INFORMATION FragmentInformation,
   __in ULONG FragmentInformationCount,
   __out_opt PULONG FragmentInformationReturned
);

/**
 * Wrapper functions for retrieving specific members of FRAGCTX_INFORMATION
 **/

__inline 
DWORD 
FpGetContextFileSize( 
   __in HANDLE FragCtx, 
   __out PULONGLONG FileSize 
)
{
   DWORD       dwRet;
   FRAGCTX_INFORMATION CtxInfo;

   dwRet = FpGetContextInformation(FragCtx,
                                   &CtxInfo);

   if ( NO_ERROR == dwRet )
   {
      (*FileSize) = CtxInfo.FileSize;
   }

   /* Success / Failure */
   return ( dwRet );
}

__inline 
DWORD 
FpGetContextFileSizeOnDisk( 
   __in HANDLE FragCtx, 
   __out PULONGLONG FileSizeOnDisk 
)
{
   DWORD       dwRet;
   FRAGCTX_INFORMATION CtxInfo;

   dwRet = FpGetContextInformation(FragCtx,
                                   &CtxInfo);

   if ( NO_ERROR == dwRet )
   {
      (*FileSizeOnDisk) = CtxInfo.FileSizeOnDisk;
   }

   /* Success / Failure */
   return ( dwRet );
}

__inline 
DWORD 
FpGetContextClusterSize( 
   __in HANDLE FragCtx, 
   __out PULONG ClusterSize 
)
{
   DWORD       dwRet;
   FRAGCTX_INFORMATION CtxInfo;

   dwRet = FpGetContextInformation(FragCtx,
                                   &CtxInfo);

   if ( NO_ERROR == dwRet )
   {
      (*ClusterSize) = CtxInfo.ClusterSize;
   }

   /* Success / Failure */
   return ( dwRet );
}

__inline 
DWORD 
FpGetContextClusterCount( 
   __in HANDLE FragCtx, 
   __out PULONGLONG ClusterCount 
)
{
   DWORD       dwRet;
   FRAGCTX_INFORMATION CtxInfo;

   dwRet = FpGetContextInformation(FragCtx,
                                   &CtxInfo);

   if ( NO_ERROR == dwRet )
   {
      (*ClusterCount) = CtxInfo.ClusterCount;
   }

   /* Success / Failure */
   return ( dwRet );
}

__inline 
DWORD 
FpGetContextExtentCount( 
   __in HANDLE FragCtx,
   __out PULONG ExtentCount 
)
{
   DWORD       dwRet;
   FRAGCTX_INFORMATION CtxInfo;

   dwRet = FpGetContextInformation(FragCtx,
                                   &CtxInfo);

   if ( NO_ERROR == dwRet )
   {
      (*ExtentCount) = CtxInfo.ExtentCount;
   }

   /* Success / Failure */
   return ( dwRet );
}

__inline 
DWORD 
FpGetContextFragmentCount( 
   __in HANDLE FragCtx, 
   __out PULONG FragmentCount 
)
{
   DWORD       dwRet;
   FRAGCTX_INFORMATION CtxInfo;

   dwRet = FpGetContextInformation(FragCtx,
                                   &CtxInfo);

   if ( NO_ERROR == dwRet )
   {
      (*FragmentCount) = CtxInfo.FragmentCount;
   }

   /* Success / Failure */
   return ( dwRet );
}

typedef 
BOOLEAN 
(FRAGAPI* 
PENUMFILEFRAGMENT_INFORMATIONPROC)( 
   __in HANDLE FragCtx, 
   __in const PFRAGMENT_INFORMATION FragmentInformation, 
   __in_opt PVOID Parameter 
);

DWORD 
FRAGAPI 
FpEnumContextFragmentInformation( 
   __in HANDLE FragCtx, 
   __callback PENUMFILEFRAGMENT_INFORMATIONPROC Callback, 
   __in_opt PVOID CallbackParameter 
);

/* Defrag flags */

/* Reserved */
#define FPF_DEFRAG_RESERVED               0x000000ffU

/* Number of cluster blocks that must be moved between each callback */
#define FPF_DEFRAG_CALLBACKINCREMENTMASK  0x0f000000U
#define FPF_DEFRAG_CALLBACKINCREMENTSHIFT 24U

/* Number of 16-block clusters to move per operation. Valid values are (0-15) + 1 */
#define FPF_DEFRAG_CLUSTERBLOCKCOUNTMASK  0x00f00000U
#define FPF_DEFRAG_CLUSTERBLOCKCOUNTSHIFT 20U

/* FUTURE: Number of clusters to pad each block with. This will effectively
 * fragment a file */
#define FPF_DEFRAG_SKIPBLOCKCOUNTMASK     0x000f0000U
#define FPF_DEFRAG_SKIPBLOCKCOUNTSHIFT    16U

__inline 
DWORD 
EncodeClusterSkipCount( 
   DWORD Count 
)
{
   return ( (Count << FPF_DEFRAG_SKIPBLOCKCOUNTSHIFT) & FPF_DEFRAG_SKIPBLOCKCOUNTMASK );
}

__inline 
DWORD 
EncodeClusterBlockCount(
   DWORD Count 
) 
{
   return ( (Count << FPF_DEFRAG_CLUSTERBLOCKCOUNTSHIFT) & FPF_DEFRAG_CLUSTERBLOCKCOUNTMASK );
}

__inline 
DWORD 
EncodeCallbackIncrement( 
   DWORD Count 
)
{
   return ( (Count << FPF_DEFRAG_CALLBACKINCREMENTSHIFT) & FPF_DEFRAG_CALLBACKINCREMENTMASK );
}

/**
 * Defrag
 **/

typedef 
DWORD 
(FRAGAPI* 
PDEFRAGMENTFILEPROC)( 
   __in ULONGLONG ClustersTotal, 
   __in ULONGLONG ClustersMoved, 
   __in_opt PVOID CallbackParameter
);

DWORD 
FRAGAPI 
FpDefragmentFile( 
   __in_z LPCWSTR FilePath, 
   __in DWORD Flags, 
   __callback PDEFRAGMENTFILEPROC Callback, 
   __in_opt PVOID CallbackParameter
);

DWORD
FRAGAPI
FpDefragmentFileEx(
   __in HANDLE FileHandle,
   __in DWORD Flags,
   __callback PDEFRAGMENTFILEPROC Callback, 
   __in_opt PVOID CallbackParameter
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */

#endif /* __FRAGEXP_H__ */