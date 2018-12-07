/* FragExt - Shell extension for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2008 Jeremy Boschen. All rights reserved.
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

/* KeFragFlt.h
 *    Kernel specific declarations for FragFlt.sys
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#pragma once

#ifndef __KEFRAGFLT_H__
#define __KEFRAGFLT_H__

/**********************************************************************

    DDK/FLTMGR/IFS Support

 **********************************************************************/
#include <fltkernel.h>
#include <ntstrsafe.h>
#include <ntintsafe.h>
#include <ntifs.h>

#ifndef offsetof
   #define offsetof( _Struct, _Member ) ((ULONG_PTR)(&((_Struct *)0)->_Member))
#endif /* _offsetof */

/**
 * RtlInitStaticUnicodeString
 *    Initializing a compile-time string as a UNICODE_STRING
 */
#ifndef RtlInitStaticUnicodeString
   #define RtlInitStaticUnicodeString( _Destination, _Source ) \
      (_Destination)->Length        = sizeof( (_Source) ) - sizeof(UNICODE_NULL); \
      (_Destination)->MaximumLength = sizeof( (_Source) ); \
      (_Destination)->Buffer        = _Source; 
#endif /* RtlInitStaticUnicodeString */

/**
 * RtlUlongAlignUp
 *    Like ROUND_TO_SIZE but using ntintsafe library semantics
 */
__forceinline
NTSTATUS
RtlUlongAlignUp(
   __in ULONG Length,
   __in ULONG Alignment,
   __out ULONG* Result
)
{
   if ( ((Length + (Alignment - 1)) & ~(Alignment - 1)) >= Length )
   {
      (*Result) = ((Length + (Alignment - 1)) & ~(Alignment - 1));
      /* Success */
      return ( STATUS_SUCCESS );
   }

   (*Result) = ULONG_ERROR;
   /* Failure */
   return ( STATUS_INTEGER_OVERFLOW );
}

#ifndef InterlockedSetClearBits
__forceinline
LONG
InterlockedSetAndClearBits(
   __inout LONG volatile* Base,
   __in LONG SetBits,
   __in LONG ClearBits
)
{
   LONG OldBase;
   LONG NewBase;
   
   do
   {
      OldBase = (*Base);
      NewBase = (OldBase & ~ClearBits);
      NewBase = (NewBase | SetBits);

   }
   while ( OldBase != InterlockedCompareExchange((LONG volatile*)Base,
                                                  NewBase,
                                                  OldBase) );

   return ( OldBase );
}
#endif /* InterlockedSetClearBits */

/**
 * _LTEXT
 *    Like _TEXT, but always UNICODE 
 */
#define __LTEXT(x) L ## x
#define _LTEXT(x) __LTEXT(x)

/**
 * _TOSTRING
 *    Evaluates a macro statement and converts it to a string
 */
#define __TOSTRING(x) #x
#define _TOSTRING(x) __TOSTRING(x)

/**
 * ARCH_ALIGN
 *    Architecture dependent pointer alignment 
 */
#if defined(_X86_)
   #define ARCH_ALIGN __declspec(align(4))
#elif defined(_AMD64_)
   #define ARCH_ALIGN __declspec(align(8))
#else
   #error Unsupported target architecture
#endif

/**********************************************************************

    FragFlt - Global Declarations

 **********************************************************************/
#include "FragFlt.h"
#include "KeSLock.h"

#define DECLARE_STACKSIZEUSED( _Function, _SizeOf ) \
   enum { __STACKSIZE_USED_##_Function = _SizeOf }
   
#if DBG
   #define FKxDumpProcedureIrql( _Name ) \
      DbgPrint(_Name ## "!IRQL=%d\n", (ULONG)KeGetCurrentIrql())
#else /* DBG */
   #define FKxDumpProcedureIrql( _Name ) __noop
#endif /* DBG */

#define FKX_FORCEINLINE __forceinline

#define FKX_DELAY_ONE_MICROSECOND   (-10)
#define FKX_DELAY_ONE_MILLISECOND   (FKX_DELAY_ONE_MICROSECOND * 1000)
#define FKX_DELAY_ONE_SECOND        (FKX_DELAY_ONE_MILLISECOND * 1000)

/*
 *  DBGTRACE levels for FKxDbgTrace
 */
typedef enum _FKX_DBGTRACE_LEVEL
{
   FKX_DBGTRACE_MESSAGE  = 0,
   FKX_DBGTRACE_ERROR    = 1,
   FKX_DBGTRACE_WARNING  = 2,
   FKX_DBGTRACE_INFO     = 4,
   FKX_DBGTRACE_TRACE    = 8,
   FKX_DBGTRACE_ALL      = FKX_DBGTRACE_ERROR|FKX_DBGTRACE_WARNING|FKX_DBGTRACE_INFO|FKX_DBGTRACE_TRACE,
   FKX_DBGTRACE_MASK     = FKX_DBGTRACE_ALL
}FKX_DBGTRACE_LEVEL;

#if DBG
   #define FKxDbgTrace( _Level, _String ) \
      (((FKX_DBGTRACE_MESSAGE == (_Level)) || FlagOn(FKxData.DebugFlags, (_Level))) ? DbgPrint _String : (__noop))
#else /* DBG */
   #define FKxDbgTrace __noop
#endif /* DBG */

ULONG
FKxExceptionFilter(
   __in struct _EXCEPTION_POINTERS* ExceptionPointers,
   __in PCSTR Function,
   __in INT Line
);

/*
 * Exception logging macro for debug builds
 */
#if DBG
   #define FKX_LOG_EXCEPTION FKxExceptionFilter(GetExceptionInformation(), __FUNCTION__, __LINE__)
#else
   #define FKX_LOG_EXCEPTION __noop
#endif

/**
 * Memory tags 
 **/
#define FKX_TAG_INITIAL_VOLUMES        'VxFK'
#define FKX_TAG_KLOG_RECORD            'KxFK'
#define FKX_TAG_KLOG_WORK_QUEUE        'WxFK'
#define FKX_TAG_STREAM_CONTEXT         'SxFK'
#define FKX_TAG_STARTUP_REGISTRY_DATA  'RxFK'

/**
 * Maximum number of processes which may be registered
 **/
#define FKX_RPMAX  512

/**
 * FKX_DATA
 *    Global driver data object
 *
 * Remarks
 *    Keeping all global variables in a structure makes it 
 *    easier to watch them all in a debugger.
 *
 *    The ARCH_ALIGN is redundant on some members, but it's
 *    easier to use it on all.
 */
typedef struct _FKX_DATA
{   
   ARCH_ALIGN PDRIVER_OBJECT           DriverObject;              /* This driver */   
   ARCH_ALIGN PFLT_FILTER              FilterObject;              /* Filter returned from FltRegisterFilter */   
   ARCH_ALIGN PFLT_PORT                ServerPort;                /* Filter server port created via FltCreateCommunicationPort */
   ARCH_ALIGN PFLT_PORT                ClientPort;                /* One and only client connection port */   
   ARCH_ALIGN LARGE_INTEGER            SystemStartTime;
   ARCH_ALIGN LIST_ENTRY               KLogRecordList;            /* Buffer list of data sent up to the user mode service */    
   ARCH_ALIGN KSPIN_LOCK               KLogRecordListLock;        /* Spinlock for KLogRecordList */
   ARCH_ALIGN NPAGED_LOOKASIDE_LIST    KLogRecordFreeList;        /* Lookaside list used for allocating buffers */
   ARCH_ALIGN ULONG                    MaxRecordsToAllocate;      /* Variables used to throttle maximum record buffers */
   ARCH_ALIGN ULONG                    RecordsAllocated;          /* Variable and lock for maintaining sequence numbers */
   ARCH_ALIGN PKEVENT                  LoggerTriggerEvent;        /* Event to be signaled when data is available */
   ARCH_ALIGN ULONG                    LoggerTriggerCount;        /* Number of records required to signal LoggerTriggerEvent */
   ARCH_ALIGN ULONG                    RuntimeControls;           /* Flags to control runtime behavior */
   ARCH_ALIGN ULONG                    ProcessIdList[FKX_RPMAX];  /* Processes not logged by the filter */
   ARCH_ALIGN ULONG                    DebugFlags;                /* Global debug flags */
}FKX_DATA;

/*
 * Debug flags
 */
typedef enum _FKX_DEBUG_FLAGS
{
   FKX_DBGF_TRACEMASK            = FKX_DBGTRACE_MASK,
   FKX_DBGF_BREAK_ON_DRIVERENTRY = 0x01000000,
   FKX_DBGF_BREAK_ON_FLTUNLOAD   = 0x02000000
}FKX_DEBUG_FLAGS;

/*
 * Runtime flags
 */
typedef enum _FKX_RUNTIME_FLAGS
{
   FKX_RTF_FEATUREMASK  = 0x0000ffff
}FKX_RUNTIME_FLAGS;

#define IsFeatureControlOn( _FC, _Data ) ((_FC) & (FKX_RTF_FEATUREMASK & (_Data)->RuntimeControls))

/**
 * FKxData
 *    Global data structure, declared in FltGlobal.c
 */
extern FKX_DATA FKxData;

/**
 * FKxLogErrorEntry
 *    Writes an error to the system event log
 */
VOID
FKxLogErrorEntry(
   __in ULONG UniqueErrorValue,
   __in NTSTATUS ErrorCode,
   __in NTSTATUS FinalStatus,
   __in_opt PVOID DumpData,
   __in USHORT DumpDataSize
);

/**********************************************************************

    FragFlt - Driver initialization support

 **********************************************************************/
NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
);

VOID
FKxProcessStartupSettings(
   __in PUNICODE_STRING RegistryPath,
   __in BOOLEAN DebugFlagsOnly
);

VOID
FKxFilterStartupVolume(
   __in PUNICODE_STRING VolumeName
);

/**********************************************************************

    FragFlt - Mini-filter support

 **********************************************************************/
FKX_FORCEINLINE
VOID
FKxNotifyLogger(
);

VOID
FLTAPI
FKxNotifyLoggerWorkRoutine(
   IN PFLT_GENERIC_WORKITEM FltWorkItem,
   IN PFLT_FILTER Filter,
   IN PVOID Context
);

enum
{
   SizeOfVolumeGuidName    = sizeof(FKX_UNKNOWN_VOLUMEGUIDNAME),
   LengthOfVolumeGuidName  = SizeOfVolumeGuidName - sizeof(L'\0'),
   CharsOfVolumeGuidName   = SizeOfVolumeGuidName / sizeof(WCHAR)
};

/**
 * FKX_KLOG_RECORD
 *    Creation information stored by the filter. These are used to 
 *    build the usermode equivalent FKX_LOG_RECORD structures when
 *    they are requested by the logger process.
 *
 * Remarks
 *    These aren't allocated directly, but as part of the
 *    FKX_KLOG_RECORD_LIST structure below
 */
typedef struct _FKX_KLOG_RECORD
{
   ULONG                      OriginatingProcess;                    /* ID of user process which created file */
   ULONG                      AccessGranted;                         /* Access mode granted to the creator */
   LARGE_INTEGER              TimeOfCapture;                         /* System time the record was logged */
   PFLT_FILE_NAME_INFORMATION FileNameInformation;                   /* FltMgr's name cache for the file */
   WCHAR                      VolumeGuidName[CharsOfVolumeGuidName]; /* GUID portion of volume's guid name */
}FKX_KLOG_RECORD, *PFKX_KLOG_RECORD;

/**
 * FKX_STREAM_CONTEXT
 *    Stream instance context used for tracking file and logging information
 *    during the lifetime of a file stream
 */
typedef struct _FKX_STREAM_CONTEXT
{
   PFLT_VOLUME                Volume;
   PFLT_FILE_NAME_INFORMATION FileNameInformation;
   ULONG                      OriginatingProcess;
   ULONG                      AccessGranted;
   LONG                       LoggedCount;
   LARGE_INTEGER              LastWriteTime;
}FKX_STREAM_CONTEXT, *PFKX_STREAM_CONTEXT;

#define FKX_STREAM_CONTEXT_SIZEOF sizeof(FKX_STREAM_CONTEXT)

/**
 * FKxCalculateLogRecordSize
 *    Determines the size required for a user-mode FKX_LOG_RECORD
 *
 * NOTE! This is called while a spinlock is held so it CANNOT
 * be __inline/__forceinline/etc
 */
ULONG
FKxCalculateLogRecordSize(
   __in PFKX_KLOG_RECORD KLogRecord
);

__forceinline
ULONG
FKxCalculateVolumeGuidNameLength(
   __in ULONG RecordLength,
   __in PFLT_FILE_NAME_INFORMATION NameInformation
)
{
   return ( RecordLength - (RTL_SIZEOF_THROUGH_FIELD(FKX_LOG_RECORD, AccessGranted) + (NameInformation->Name.Length - NameInformation->Volume.Length) + sizeof(L'\0')) );
}

/**
 * FKX_KLOG_RECORD_LIST
 *    Non-Paged log record tracking data
 */
typedef struct _FKX_KLOG_RECORD_LIST
{
   LIST_ENTRY       List;   
   FKX_KLOG_RECORD  KLogRecord;
}FKX_KLOG_RECORD_LIST, *PFKX_KLOG_RECORD_LIST;

/* Default and maximum number of records to allocate on FKxData.KLogRecordList. The size
 * is an attempt to keep the total use of non-paged pool below 512KB */
#define FKX_RECORD_DEF  500
#define FKX_RECORD_MAX  ((1024 * 512) / sizeof(FKX_KLOG_RECORD_LIST))

#ifdef _FKX_TRACE_STREAMCONTEXT_REFERENCES
   #define FKxFltDumpStreamContextReferenceCount( StreamContext ) \
      { \
         if ( NULL != StreamContext ) \
            DbgPrint("StreamContext(%p)!ReferenceCount=%u!" __FUNCTION__ "\n", StreamContext, ((ULONG*)StreamContext)[-2]); \
      }
#else
   #define FKxFltDumpStreamContextReferenceCount __noop
#endif

/**********************************************************************

    Function Prototypes, Mini-Filter Model

 **********************************************************************/
NTSTATUS
FKxFltUnload(
   FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS 
FKxFltPortConnect(
   __in PFLT_PORT ClientPort,
   __in_opt PVOID ServerPortCookie,
   __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
   __in ULONG SizeOfContext,
   __deref_out_opt PVOID* ConnectionPortCookie
);

VOID
FKxFltPortDisconnect(
   __in_opt PVOID ConnectionCookie
);

NTSTATUS
FKxFltInstanceSetup(
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in FLT_INSTANCE_SETUP_FLAGS Flags,
   __in DEVICE_TYPE VolumeDeviceType,
   __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

NTSTATUS
FKxFltInstanceQueryTeardown(
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

NTSTATUS
FKxFltMessageNotify(
   __in PVOID ConnectionCookie,
   __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
   __in ULONG InputBufferSize,
   __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
   __in ULONG OutputBufferSize,
   __out PULONG ReturnOutputBufferLength
);

VOID
FKxFltContextCleanup(
   __in PFLT_CONTEXT Context,
   __in FLT_CONTEXT_TYPE ContextType
);

FLT_PREOP_CALLBACK_STATUS
FKxFltPreCreate(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
FKxFltPostCreate(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in_opt PVOID CompletionContext,
   __in FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FKxFltPreWrite(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
FKxFltPostWrite(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in_opt PVOID CompletionContext,
   __in FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FKxFltPreSetInformation(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID *CompletionContext
);

FLT_POSTOP_CALLBACK_STATUS
FKxFltPostSetInformation(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in_opt PVOID CompletionContext,
   __in FLT_POST_OPERATION_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FKxFltPreCleanup(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID *CompletionContext
);

/**
 * FKxFilterRegistration
 *    Mini-filter registration info, declared in FltRegister.c
 */
extern const FLT_REGISTRATION FKxFltRegistration;
/**********************************************************************

    Support routines

 **********************************************************************/
__inline
BOOLEAN
FKxFltIsAligned(
   __in_opt PFLT_CALLBACK_DATA CallbackData,
   __in PVOID Address,
   __in ULONG Alignment
);

NTSTATUS
FKxFltInitializeStreamContext(
   __in ULONG ProcessId,
   __in ULONG AccessMode,
   __in PFLT_VOLUME Volume,
   __in PFLT_FILE_NAME_INFORMATION FileNameInformation,
   __in LARGE_INTEGER LastWriteTime,
   __inout PFKX_STREAM_CONTEXT StreamContext
);

VOID
FKxFltMarkStreamContextStale(
   __in PFKX_STREAM_CONTEXT StreamContext
);

NTSTATUS
FKxFltRecordAllocate(
   __out PFKX_KLOG_RECORD_LIST* RecordList
);

VOID
FKxFltRecordFree(
   __in PFKX_KLOG_RECORD_LIST RecordList
);

VOID
FKxFltRecordLogEmpty(
);

NTSTATUS
FKxFltRecordLogRead(
   __out_bcount_part(OutputBufferLength, *ReturnOutputBufferLength) PUCHAR OutputBuffer,
   __in ULONG OutputBufferLength,
   __out PULONG ReturnOutputBufferLength
);

NTSTATUS
FKxFltRecordLogWrite(
   __in ULONG ProcessId,
   __in ULONG AccessMode,
   __in PFLT_VOLUME Volume,
   __in PFLT_FILE_NAME_INFORMATION FileNameInformation
);

PFKX_LOG_RECORD
FKxLookupLogRecordByFileName(
   __in_bcount(OutputBufferLength) PUCHAR OutputBuffer,
   __in ULONG OutputBufferLength,
   __in PUNICODE_STRING FileName
);

VOID
KFxFltLogStreamGenericWorkRoutine(
   __in PFLT_GENERIC_WORKITEM FltWorkItem,
   __in PFLT_FILTER Filter,
   __in PVOID Context
);

NTSTATUS
KFxFltLogStreamContext(
   __inout PFKX_STREAM_CONTEXT StreamContext,
   __in BOOLEAN ReleaseContext
);

NTSTATUS
FKxFltRegisterProcessId(
   __in ULONG ProcessId
);

VOID
FKxFltUnregisterProcessId(
   __in ULONG ProcessId
);

BOOLEAN
FKxFltIsRegisteredProcessId(
   __in ULONG ProcessId
);

#endif /* __KEFRAGFLT_H__ */
