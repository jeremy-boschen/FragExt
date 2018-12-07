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

/* NtApi.h
 *    NT native API declarations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

/**********************************************************************

   NTAPI

 **********************************************************************/
extern "C"
{

#pragma warning( push )
#pragma warning( disable : 4005 )
#include <ntstatus.h>
#pragma warning( pop )

typedef __success(return >= 0) LONG NTSTATUS;

#define NT_SUCCESS(Status) \
   (((NTSTATUS)(Status)) >= 0)

typedef struct _IO_STATUS_BLOCK 
{
   union 
   {
      NTSTATUS Status;
      PVOID    Pointer;
   };

   ULONG_PTR   Information;
}IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef
VOID
(NTAPI *PIO_APC_ROUTINE)(
   __in PVOID ApcContext,
   __out PIO_STATUS_BLOCK IoStatusBlock,
   __in ULONG Reserved
);

typedef struct _UNICODE_STRING 
{
   USHORT  Length;
   USHORT  MaximumLength;
   PWCH    Buffer;
}UNICODE_STRING, *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

#define OBJ_INHERIT             0x00000002L
#define OBJ_PERMANENT           0x00000010L
#define OBJ_EXCLUSIVE           0x00000020L
#define OBJ_CASE_INSENSITIVE    0x00000040L
#define OBJ_OPENIF              0x00000080L
#define OBJ_OPENLINK            0x00000100L
#define OBJ_KERNEL_HANDLE       0x00000200L
#define OBJ_FORCE_ACCESS_CHECK  0x00000400L
#define OBJ_VALID_ATTRIBUTES    0x000007F2L

typedef struct _OBJECT_ATTRIBUTES 
{
   ULONG             Length;
   HANDLE            RootDirectory;
   PUNICODE_STRING   ObjectName;
   ULONG             Attributes;
   PVOID             SecurityDescriptor;
   PVOID             SecurityQualityOfService;
}OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
typedef const struct _OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

__forceinline
VOID
InitializeObjectAttributes(
   __out POBJECT_ATTRIBUTES InitializedAttributes,
   __in PUNICODE_STRING ObjectName,
   __in_opt ULONG Attributes,
   __in_opt HANDLE RootDirectory,
   __in_opt PSECURITY_DESCRIPTOR SecurityDescriptor
)
{
   InitializedAttributes->Length = sizeof(OBJECT_ATTRIBUTES);
   InitializedAttributes->RootDirectory = RootDirectory;
   InitializedAttributes->Attributes = Attributes; 
   InitializedAttributes->ObjectName = ObjectName; 
   InitializedAttributes->SecurityDescriptor = SecurityDescriptor; 
   InitializedAttributes->SecurityQualityOfService = NULL;
}

typedef struct _FILE_BASIC_INFORMATION 
{
   LARGE_INTEGER  CreationTime;
   LARGE_INTEGER  LastAccessTime;
   LARGE_INTEGER  LastWriteTime;
   LARGE_INTEGER  ChangeTime;
   ULONG          FileAttributes;
}FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef struct _FILE_STANDARD_INFORMATION 
{
   LARGE_INTEGER  AllocationSize;
   LARGE_INTEGER  EndOfFile;
   ULONG          NumberOfLinks;
   BOOLEAN        DeletePending;
   BOOLEAN        Directory;
}FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_NETWORK_OPEN_INFORMATION 
{
   LARGE_INTEGER  CreationTime;
   LARGE_INTEGER  LastAccessTime;
   LARGE_INTEGER  LastWriteTime;
   LARGE_INTEGER  ChangeTime;
   LARGE_INTEGER  AllocationSize;
   LARGE_INTEGER  EndOfFile;
   ULONG          FileAttributes;
}FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

typedef struct _FILE_POSITION_INFORMATION 
{
   LARGE_INTEGER CurrentByteOffset;
} FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;

typedef struct _FILE_INTERNAL_INFORMATION 
{
   LARGE_INTEGER  IndexNumber;
}FILE_INTERNAL_INFORMATION, *PFILE_INTERNAL_INFORMATION;

typedef struct _FILE_EA_INFORMATION 
{
   ULONG EaSize;
}FILE_EA_INFORMATION, *PFILE_EA_INFORMATION;

typedef struct _FILE_ACCESS_INFORMATION 
{
   ACCESS_MASK AccessFlags;
}FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

typedef struct _FILE_MODE_INFORMATION 
{
   ULONG Mode;
}FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;

typedef struct _FILE_ALIGNMENT_INFORMATION 
{
   ULONG AlignmentRequirement;
}FILE_ALIGNMENT_INFORMATION, *PFILE_ALIGNMENT_INFORMATION;

typedef struct _FILE_NAME_INFORMATION 
{
   ULONG FileNameLength;
   WCHAR FileName[1];
}FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_ALL_INFORMATION 
{
   FILE_BASIC_INFORMATION     BasicInformation;
   FILE_STANDARD_INFORMATION  StandardInformation;
   FILE_INTERNAL_INFORMATION  InternalInformation;
   FILE_EA_INFORMATION        EaInformation;
   FILE_ACCESS_INFORMATION    AccessInformation;
   FILE_POSITION_INFORMATION  PositionInformation;
   FILE_MODE_INFORMATION      ModeInformation;
   FILE_ALIGNMENT_INFORMATION AlignmentInformation;
   FILE_NAME_INFORMATION      NameInformation;
}FILE_ALL_INFORMATION, *PFILE_ALL_INFORMATION;

typedef struct _FILE_COMPRESSION_INFORMATION 
{
   LARGE_INTEGER  CompressedFileSize;
   USHORT         CompressionFormat;
   UCHAR          CompressionUnitShift;
   UCHAR          ChunkShift;
   UCHAR          ClusterShift;
   UCHAR          Reserved[3];
}FILE_COMPRESSION_INFORMATION, *PFILE_COMPRESSION_INFORMATION;

typedef struct _FILE_STREAM_INFORMATION
{
   ULONG          NextEntryOffset;
   ULONG          StreamNameLength;
   LARGE_INTEGER  StreamSize;
   LARGE_INTEGER  StreamAllocationSize;
   WCHAR          StreamName[1];
}FILE_STREAM_INFORMATION, *PFILE_STREAM_INFORMATION;

typedef struct _FILE_DIRECTORY_INFORMATION 
{
   ULONG          NextEntryOffset;
   ULONG          FileIndex;
   LARGE_INTEGER  CreationTime;
   LARGE_INTEGER  LastAccessTime;
   LARGE_INTEGER  LastWriteTime;
   LARGE_INTEGER  ChangeTime;
   LARGE_INTEGER  EndOfFile;
   LARGE_INTEGER  AllocationSize;
   ULONG          FileAttributes;
   ULONG          FileNameLength;
   WCHAR          FileName[1];
}FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_ID_FULL_DIR_INFORMATION 
{
   ULONG          NextEntryOffset;
   ULONG          FileIndex;
   LARGE_INTEGER  CreationTime;
   LARGE_INTEGER  LastAccessTime;
   LARGE_INTEGER  LastWriteTime;
   LARGE_INTEGER  ChangeTime;
   LARGE_INTEGER  EndOfFile;
   LARGE_INTEGER  AllocationSize;
   ULONG          FileAttributes;
   ULONG          FileNameLength;
   ULONG          EaSize;
   LARGE_INTEGER  FileId;
   WCHAR          FileName[1];
}FILE_ID_FULL_DIR_INFORMATION, *PFILE_ID_FULL_DIR_INFORMATION;

typedef struct _FILE_NAMES_INFORMATION 
{
   ULONG  NextEntryOffset;
   ULONG  FileIndex;
   ULONG  FileNameLength;
   WCHAR  FileName[1];
}FILE_NAMES_INFORMATION, *PFILE_NAMES_INFORMATION;

typedef enum _FILE_INFORMATION_CLASS 
{
   FileDirectoryInformation         = 1,
   FileBasicInformation             = 4,
   FileStandardInformation          = 5,
   FileInternalInformation          = 6,
   FileEaInformation                = 7,
   FileAccessInformation            = 8,
   FileNameInformation              = 9,
   FileNamesInformation             = 12,
   FilePositionInformation          = 14,
   FileModeInformation              = 16,
   FileAlignmentInformation         = 17,
   FileAllInformation               = 18,
   FileStreamInformation            = 22,
   FileCompressionInformation       = 28,
   FileIdFullDirectoryInformation   = 38
}FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

NTSTATUS
NTAPI
NtQueryInformationFile(
    __in HANDLE FileHandle,
    __out PIO_STATUS_BLOCK IoStatusBlock,
    __out_bcount(Length) PVOID FileInformation,
    __in ULONG Length,
    __in FILE_INFORMATION_CLASS FileInformationClass
);

NTSTATUS
NTAPI
NtQueryAttributesFile(
   __in POBJECT_ATTRIBUTES ObjectAttributes,
   __out PFILE_BASIC_INFORMATION FileInformation
);

NTSTATUS
NTAPI
NtQueryFullAttributesFile(
   __in POBJECT_ATTRIBUTES ObjectAttributes,
   __out PFILE_NETWORK_OPEN_INFORMATION FileInformation
);

typedef enum _OBJECT_INFORMATION_CLASS 
{
   ObjectNameInformation = 1
}OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef struct _OBJECT_NAME_INFORMATION
{
   UNICODE_STRING Name;
}OBJECT_NAME_INFORMATION, *POBJECT_NAME_INFORMATION;

NTSTATUS
NTAPI
NtQueryObject(
   __in HANDLE Handle,
   __in OBJECT_INFORMATION_CLASS ObjectInformationClass,
   __out_bcount_opt(ObjectInformationLength) PVOID ObjectInformation,
   __in ULONG ObjectInformationLength,
   __out_opt PULONG ReturnLength
);

typedef struct __declspec(align(8)) _FILE_FS_VOLUME_INFORMATION 
{
   LARGE_INTEGER  VolumeCreationTime;
   ULONG          VolumeSerialNumber;
   ULONG          VolumeLabelLength;
   BOOLEAN        SupportsObjects;
   WCHAR          VolumeLabel[1];
}FILE_FS_VOLUME_INFORMATION, *PFILE_FS_VOLUME_INFORMATION;

typedef struct __declspec(align(8)) _FILE_FS_SIZE_INFORMATION 
{
   LARGE_INTEGER  TotalAllocationUnits;
   LARGE_INTEGER  AvailableAllocationUnits;
   ULONG          SectorsPerAllocationUnit;
   ULONG          BytesPerSector;
}FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

typedef struct __declspec(align(8)) _FILE_FS_FULL_SIZE_INFORMATION 
{
   LARGE_INTEGER  TotalAllocationUnits;
   LARGE_INTEGER  CallerAvailableAllocationUnits;
   LARGE_INTEGER  ActualAvailableAllocationUnits;
   ULONG          SectorsPerAllocationUnit;
   ULONG          BytesPerSector;
}FILE_FS_FULL_SIZE_INFORMATION, *PFILE_FS_FULL_SIZE_INFORMATION;

typedef enum _FS_INFORMATION_CLASS 
{
   FileFsVolumeInformation   = 1,
   FileFsSizeInformation     = 3,
   FileFsFullSizeInformation = 7
}FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

NTSTATUS
NTAPI
NtQueryVolumeInformationFile(
   __in HANDLE FileHandle,
   __out PIO_STATUS_BLOCK IoStatusBlock,
   __out_bcount(Length) PVOID FsInformation,
   __in ULONG Length,
   __in FS_INFORMATION_CLASS FsInformationClass
);

NTSTATUS
NTAPI
NtQueryDirectoryFile(
   __in HANDLE FileHandle,
   __in_opt HANDLE Event,
   __in_opt PIO_APC_ROUTINE ApcRoutine,
   __in_opt PVOID ApcContext,
   __out PIO_STATUS_BLOCK IoStatusBlock,
   __out_bcount(Length) PVOID FileInformation,
   __in ULONG Length,
   __in FILE_INFORMATION_CLASS FileInformationClass,
   __in BOOLEAN ReturnSingleEntry,
   __in_opt PUNICODE_STRING FileName,
   __in BOOLEAN RestartScan
);

typedef struct _RTL_BITMAP 
{
   ULONG  SizeOfBitMap;
   PULONG Buffer;
}RTL_BITMAP, *PRTL_BITMAP;

VOID
NTAPI
RtlInitializeBitMap(
   PRTL_BITMAP BitMapHeader,
   PULONG BitMapBuffer,
   ULONG SizeOfBitMap
);

BOOLEAN
NTAPI
RtlAreBitsClear(
   PRTL_BITMAP BitMapHeader,
   ULONG StartingIndex,
   ULONG Length
);

ULONG
NTAPI
RtlFindClearBits(
   PRTL_BITMAP BitMapHeader,
   ULONG NumberToFind,
   ULONG HintIndex
);

ULONG
NTAPI
RtlFindLastBackwardRunClear(
   IN PRTL_BITMAP BitMapHeader,
   IN ULONG FromIndex,
   IN PULONG StartingRunIndex
);

ULONG
NTAPI
RtlNumberOfClearBits(
   IN PRTL_BITMAP BitMapHeader
);

ULONG
NTAPI
RtlNumberOfSetBits(
   IN PRTL_BITMAP BitMapHeader
);

ULONG
NTAPI
RtlFindLongestRunClear(
   IN PRTL_BITMAP BitMapHeader,
   OUT PULONG     StartingIndex
); 

ULONG
NTAPI
RtlNtStatusToDosError(
   __in NTSTATUS Status
);

__inline 
DWORD
NtStatusToWin32Error(
   __in NTSTATUS Status
)
{
   DWORD dwRet;
   
   dwRet = RtlNtStatusToDosError(Status);
   if ( ERROR_MR_MID_NOT_FOUND == dwRet )
   {
      dwRet = (DWORD)Status;
   }

   return ( dwRet );
}

VOID
NTAPI
RtlInitUnicodeString(
   __out PUNICODE_STRING DestinationString,
   __in_opt PCWSTR SourceString
);

__inline
void
RtlInitFixedUnicodeString(
   __out PUNICODE_STRING DestinationString,
   __in_opt PCWSTR SourceString,
   __in USHORT SourceStringLength
)
{
   DestinationString->Buffer = (PWCH)SourceString;
   DestinationString->Length = DestinationString->MaximumLength = SourceStringLength;
}

BOOLEAN
NTAPI
RtlEqualUnicodeString(
   __in PCUNICODE_STRING String1,
   __in PCUNICODE_STRING String2,
   __in BOOLEAN CaseInSensitive
);

BOOLEAN
NTAPI
RtlPrefixUnicodeString(
   __in PCUNICODE_STRING String1,
   __in PCUNICODE_STRING String2,
   __in BOOLEAN CaseInSensitive
);

/*++

   Mount Manager

--*/

#define MOUNTMGR_DOS_DEVICE_NAME       L"\\\\.\\MountPointManager"

#define MOUNTMGRCONTROLTYPE            0x0000006D
#define IOCTL_MOUNTMGR_QUERY_POINTS    CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MOUNTMGR_MOUNT_POINT 
{
   ULONG    SymbolicLinkNameOffset;
   USHORT   SymbolicLinkNameLength;
   ULONG    UniqueIdOffset;
   USHORT   UniqueIdLength;
   ULONG    DeviceNameOffset;
   USHORT   DeviceNameLength;
}MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

typedef struct _MOUNTMGR_MOUNT_POINTS
{
   ULONG                Size;
   ULONG                NumberOfMountPoints;
   MOUNTMGR_MOUNT_POINT MountPoints[1];
}MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

__inline
void
InitializeMountPointsNameStrings(
   __in PMOUNTMGR_MOUNT_POINTS MountPoints,
   __in ULONG MountPointIndex,
   __inout_opt PUNICODE_STRING SymbolicLinkName,
   __inout_opt PUNICODE_STRING UniqueId,
   __inout_opt PUNICODE_STRING DeviceName
)
{
   PUCHAR                Base;
   PMOUNTMGR_MOUNT_POINT MountPoint;

   Base       = (PUCHAR)MountPoints;
   MountPoint = &(MountPoints->MountPoints[MountPointIndex]);

   if ( SymbolicLinkName )
   {
      SymbolicLinkName->Length = SymbolicLinkName->MaximumLength = MountPoint->SymbolicLinkNameLength;
      SymbolicLinkName->Buffer = (PWCH)(Base + MountPoint->SymbolicLinkNameOffset);
   }

   if ( UniqueId )
   {
      UniqueId->Length = UniqueId->MaximumLength = MountPoint->UniqueIdLength;
      UniqueId->Buffer = (PWCH)(Base + MountPoint->UniqueIdOffset);
   }

   if ( DeviceName )
   {
      DeviceName->Length = DeviceName->MaximumLength = MountPoint->DeviceNameLength;
      DeviceName->Buffer = (PWCH)(Base + MountPoint->DeviceNameOffset);
   }
}

__inline
BOOLEAN
IsMountMgrVolumeName(
   __in PUNICODE_STRING Name
)
{
   USHORT Length;
   PCWCH  Buffer;

   Length = Name->Length;
   Buffer = Name->Buffer;

   return ( ((96 == Length) || ((98 == Length) && (L'\\' == Buffer[48]))) &&
            Buffer[0]  == L'\\' &&
           (Buffer[1]  == L'?'  || Buffer[1] == L'\\') &&
            Buffer[2]  == L'?'  &&
            Buffer[3]  == L'\\' &&
            Buffer[4]  == L'V'  &&
            Buffer[5]  == L'o'  &&
            Buffer[6]  == L'l'  &&
            Buffer[7]  == L'u'  &&
            Buffer[8]  == L'm'  &&
            Buffer[9]  == L'e'  &&
            Buffer[10] == L'{'  &&
            Buffer[19] == L'-'  &&
            Buffer[24] == L'-'  &&
            Buffer[29] == L'-'  &&
            Buffer[34] == L'-'  &&
            Buffer[47] == L'}' );
}

NTSTATUS
NTAPI
NtDeviceIoControlFile(
   __in HANDLE FileHandle,
   __in_opt HANDLE Event,
   __in_opt PIO_APC_ROUTINE ApcRoutine,
   __in_opt PVOID ApcContext,
   __out PIO_STATUS_BLOCK IoStatusBlock,
   __in ULONG IoControlCode,
   __in_bcount_opt(InputBufferLength) PVOID InputBuffer,
   __in ULONG InputBufferLength,
   __out_bcount_opt(OutputBufferLength) PVOID OutputBuffer,
   __in ULONG OutputBufferLength
);


/*++

   NtCreateFile

--*/

#define FILE_SUPERSEDE                          0x00000000
#define FILE_OPEN                               0x00000001
#define FILE_CREATE                             0x00000002
#define FILE_OPEN_IF                            0x00000003
#define FILE_OVERWRITE                          0x00000004
#define FILE_OVERWRITE_IF                       0x00000005
#define FILE_MAXIMUM_DISPOSITION                0x00000005

#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_REMOTE_INSTANCE               0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000

#define FILE_COPY_STRUCTURED_STORAGE            0x00000041
#define FILE_STRUCTURED_STORAGE                 0x00000441

#define FILE_VALID_OPTION_FLAGS                 0x00ffffff
#define FILE_VALID_PIPE_OPTION_FLAGS            0x00000032
#define FILE_VALID_MAILSLOT_OPTION_FLAGS        0x00000032
#define FILE_VALID_SET_FLAGS                    0x00000036

#define FILE_SUPERSEDED                         0x00000000
#define FILE_OPENED                             0x00000001
#define FILE_CREATED                            0x00000002
#define FILE_OVERWRITTEN                        0x00000003
#define FILE_EXISTS                             0x00000004
#define FILE_DOES_NOT_EXIST                     0x00000005

NTSTATUS
NTAPI
NtCreateFile(
   __out PHANDLE FileHandle,
   __in ACCESS_MASK DesiredAccess,
   __in POBJECT_ATTRIBUTES ObjectAttributes,
   __out PIO_STATUS_BLOCK IoStatusBlock,
   __in_opt PLARGE_INTEGER AllocationSize,
   __in ULONG FileAttributes,
   __in ULONG ShareAccess,
   __in ULONG CreateDisposition,
   __in ULONG CreateOptions,
   __in_bcount_opt(EaLength) PVOID EaBuffer,
   __in ULONG EaLength
);

NTSTATUS
NTAPI
NtOpenFile(
   __out PHANDLE FileHandle,
   __in ACCESS_MASK DesiredAccess,
   __in POBJECT_ATTRIBUTES ObjectAttributes,
   __out PIO_STATUS_BLOCK IoStatusBlock,
   __in ULONG ShareAccess,
   __in ULONG OpenOptions
);

NTSTATUS
NTAPI
NtClose(
   __in HANDLE Handle
);

/*++

   Waitable Timers

--*/
NTSTATUS
NTAPI
NtCancelTimer(
   __in HANDLE TimerHandle,
   __out_opt PBOOLEAN PreviousState
);

}; /* extern "C" */
