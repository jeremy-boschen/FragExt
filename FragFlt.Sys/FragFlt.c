/* FragExt - File defragmentation software utility toolkit.
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

/* KeFragFlt.c
 *    Implementation of the FragFlt mini-filter
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#include "KeFragFlt.h"

/**********************************************************************

   Global Data

 **********************************************************************/
FKX_DATA FKxData = {0};

/**********************************************************************

   Section Assignment

 **********************************************************************/
#ifdef ALLOC_PRAGMA
   #pragma alloc_text( "INIT", DriverEntry )
   #pragma alloc_text( "INIT", FKxProcessStartupSettings )
   #pragma alloc_text( "INIT", FKxFilterStartupVolume )

   #pragma alloc_text( "PAGE", FKxFltUnload )
   #pragma alloc_text( "PAGE", FKxFltPortConnect )
   #pragma alloc_text( "PAGE", FKxFltPortDisconnect )
   #pragma alloc_text( "PAGE", FKxFltInstanceSetup )
   #pragma alloc_text( "PAGE", FKxFltInstanceQueryTeardown )
   #pragma alloc_text( "PAGE", FKxFltMessageNotify )
   #pragma alloc_text( "PAGE", FKxFltRecordLogEmpty )
#endif /* ALLOC_PRAGMA */

ULONG
FKxExceptionFilter(
   __in struct _EXCEPTION_POINTERS* ExceptionPointers,
   __in PCSTR Function,
   __in INT Line
)
{
   /*
      IRQL:           ANY
      THREAD CONTEXT: ARBITRARY
    */
#if defined(_WIN64)
   PEXCEPTION_RECORD64 ExceptionRecord = (PEXCEPTION_RECORD64)ExceptionPointers->ExceptionRecord;
#else
   PEXCEPTION_RECORD32 ExceptionRecord = (PEXCEPTION_RECORD32)ExceptionPointers->ExceptionRecord;
#endif

   DECLARE_STACKSIZEUSED(FKxExceptionFilter, 
                         sizeof(ExceptionRecord));

   DbgPrint("FragFlt!Exception in function %s(%d)!Code=%0.8lx, Flags=%0.8lx, Address=%0.8lx\n", 
            Function, 
            Line, 
            ExceptionRecord->ExceptionCode, 
            ExceptionRecord->ExceptionFlags, 
            ExceptionRecord->ExceptionAddress);

   return ( EXCEPTION_EXECUTE_HANDLER );
}

VOID
FKxLogErrorEntry(
   __in ULONG UniqueErrorValue,
   __in NTSTATUS ErrorCode,
   __in NTSTATUS FinalStatus,
   __in_opt PVOID DumpData,
   __in USHORT DumpDataSize
)
{
   /*
      IRQL:           ANY
      THREAD CONTEXT: ARBITRARY
    */
   NTSTATUS             NtStatus;
   ULONG                PacketSize;
   PIO_ERROR_LOG_PACKET IoErrorPacket;

   DECLARE_STACKSIZEUSED(FKxLogErrorEntry,
                         sizeof(NtStatus) + 
                         sizeof(PacketSize) + 
                         sizeof(IoErrorPacket));

   /*
    * Ensure the total size of the error packet is less than the
    * maximum allowed size
    */
   NtStatus = RtlULongAdd(sizeof(IO_ERROR_LOG_PACKET),
                          DumpDataSize,
                          &PacketSize);

   if ( !NT_SUCCESS(NtStatus) || (PacketSize > ERROR_LOG_MAXIMUM_SIZE) )
   {
      /*
       * Reduce the size of the dump data to create a total packet size
       * of ERROR_LOG_MAXIMUM_SIZE
       */
      DumpDataSize = (ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET));
   }

   IoErrorPacket = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(FKxData.DriverObject,
                                                                 (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + DumpDataSize));

   if ( NULL != IoErrorPacket )
   {
      IoErrorPacket->ErrorCode        = ErrorCode;
      IoErrorPacket->UniqueErrorValue = UniqueErrorValue;
      IoErrorPacket->FinalStatus      = FinalStatus;

      if ( NULL != DumpData )
      {
         RtlCopyMemory(&IoErrorPacket->DumpData[0],
                       DumpData,
                       DumpDataSize);

         IoErrorPacket->DumpDataSize = DumpDataSize;
      }

      IoWriteErrorLogEntry(IoErrorPacket);
   }
}

/**********************************************************************

    FragFlt - Driver initialization support

 **********************************************************************/
NTSTATUS DriverEntry( 
   IN PDRIVER_OBJECT DriverObject, 
   IN PUNICODE_STRING RegistryPath 
)
{
   /*
      IRQL:           PASSIVE_LEVEL
      THREAD CONTEXT: SYSTEM
    */
   NTSTATUS             NtStatus;
   UNICODE_STRING       IoPortName;
   PSECURITY_DESCRIPTOR SecurityDescriptor;
   OBJECT_ATTRIBUTES    ObjectAttributes;

   DECLARE_STACKSIZEUSED(DriverEntry,
                         sizeof(NtStatus) + 
                         sizeof(IoPortName) + 
                         sizeof(SecurityDescriptor) + 
                         sizeof(ObjectAttributes));

#if DBG
   FKxProcessStartupSettings(RegistryPath,
                             TRUE);

   if ( FKX_DBGF_BREAK_ON_DRIVERENTRY & FKxData.DebugFlags )
   {
      KdBreakPoint();
   }
#endif /* DBG */

   /* 
    * Initialize locals 
    */
   NtStatus           = STATUS_DEVICE_DATA_ERROR;
   SecurityDescriptor = NULL;
   
   __try
   {
      /*
       * Initialize the smart lock support before anything that may
       * use it is executed
       */
      InitializeSmartLockSupport();
      
      FKxData.DriverObject         = DriverObject;
      FKxData.MaxRecordsToAllocate = FKX_RECORD_MAX;
      FKxData.RuntimeControls      = FKX_FLTFEATURE_DEFAULT;

      KeQueryTickCount(&FKxData.SystemStartTime);
      FKxData.SystemStartTime.QuadPart *= (LONGLONG)KeQueryTimeIncrement();

      /*
       * Initialize the filter data
       */
      InitializeListHead(&FKxData.KLogRecordList);
      KeInitializeSpinLock(&FKxData.KLogRecordListLock);
            
      ExInitializeNPagedLookasideList(&FKxData.KLogRecordFreeList,
                                      NULL,
                                      NULL,
                                      0,
                                      sizeof(FKX_KLOG_RECORD_LIST),
                                      FKX_TAG_KLOG_RECORD,
                                      0);

      /*
       * Register this filter with the system
       */
      NtStatus = FltRegisterFilter(DriverObject,
                                   &FKxFltRegistration,
                                   &FKxData.FilterObject);

      if ( !NT_SUCCESS(NtStatus) )
      {
         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
         /* Failure */
         __leave;
      }

      /*
       * Create the communication port for this filter using the default
       * security descriptor. It might be a good idea to change this to
       * a predefined SID shared between the usermode service and this 
       * filter, or to one specified as a registry setting
       */
      NtStatus = FltBuildDefaultSecurityDescriptor(&SecurityDescriptor,
                                                   FLT_PORT_ALL_ACCESS);

      if ( !NT_SUCCESS(NtStatus) )
      {
         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
         /* Failure */
         __leave;
      }

      RtlInitStaticUnicodeString(&IoPortName, 
                                 _LTEXT(KEFRAGMINI_FILTERPORTNAME));

      InitializeObjectAttributes(&ObjectAttributes,
                                 &IoPortName,
                                 OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 SecurityDescriptor);

      /*
       * Only 1 connection to this mini-filter
       */
      NtStatus = FltCreateCommunicationPort(FKxData.FilterObject,
                                            &FKxData.ServerPort,
                                            &ObjectAttributes,
                                            NULL,
                                            FKxFltPortConnect,
                                            FKxFltPortDisconnect,
                                            FKxFltMessageNotify,
                                            1/*MAX-1-CONNECTION*/);

      if ( !NT_SUCCESS(NtStatus) )
      {
         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
         /* Failure */
         __leave;
      }

      NtStatus = FltStartFiltering(FKxData.FilterObject);

      if ( !NT_SUCCESS(NtStatus) )
      {
         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
         /* Failure */
         __leave;
      }

      /*
       * Load and act on settings from the registry now, if any. This doesn't have a return value, but
       * if anything of importance fails, an event will be written to the system event log
       */
      FKxProcessStartupSettings(RegistryPath,
                                FALSE);
   }
   __finally
   {
      if ( NULL != SecurityDescriptor )
      {
         FltFreeSecurityDescriptor(SecurityDescriptor);
      }

      if ( !NT_SUCCESS(NtStatus) )
      {
         if ( NULL != FKxData.ServerPort )
         {
            FltCloseCommunicationPort(FKxData.ServerPort);
         }

         if ( NULL != FKxData.FilterObject )
         {
            FltUnregisterFilter(FKxData.FilterObject);
         }

         ExDeleteNPagedLookasideList(&FKxData.KLogRecordFreeList);
      }
   }

   /* Success / Failure */
   return ( NtStatus );
}   

VOID
FKxProcessStartupSettings(
   __in PUNICODE_STRING RegistryPath,
   __in BOOLEAN DebugFlagsOnly
)
{
   /*
      IRQL:             PASSIVE_LEVEL (via DriverEntry)
      THREAD CONTEXT:   SYSTEM
    */
   NTSTATUS                       NtStatus;
   OBJECT_ATTRIBUTES              ObjectAttributes;
   HANDLE                         RegistryKey;
   UNICODE_STRING                 ValueName;
   PKEY_VALUE_PARTIAL_INFORMATION ValuePartialInfo;
   ULONG                          ResultLength;   
   ULONG                          SettingValue;
   UNICODE_STRING                 VolumeName;
   size_t                         NameLength;

   enum
   {
      SIZEOF_VALUEPARTIALINFO = 1024 - sizeof(KEY_VALUE_PARTIAL_INFORMATION)
   };

   DECLARE_STACKSIZEUSED(FKxProcessStartupSettings,
                         sizeof(NtStatus) +
                         sizeof(ObjectAttributes) + 
                         sizeof(RegistryKey) +
                         sizeof(ValuePartialInfo) +
                         sizeof(SettingValue) +
                         sizeof(VolumeName) +
                         sizeof(NameLength));

   /*
    * Initialize locals
    */
   RegistryKey      = NULL;
   ValuePartialInfo = NULL;

   /*
    * Open the driver key for reading any startup parameters
    */
   InitializeObjectAttributes(&ObjectAttributes,
                              RegistryPath,
                              OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                              NULL,
                              NULL);

   NtStatus = ZwOpenKey(&RegistryKey,
                        KEY_READ,
                        &ObjectAttributes);

   if ( !NT_SUCCESS(NtStatus) )
   {
      FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
      /* Failure */
      return;
   }

   __try
   {
      /*
       * Allocate a decent sized buffer for the registry data
       */
      ValuePartialInfo = ExAllocatePoolWithTag(PagedPool,
                                               SIZEOF_VALUEPARTIALINFO,
                                               KFX_TAG_STARTUP_REGISTRY_DATA);
      if ( !ValuePartialInfo )
      {
         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
         /* Failure */
         __leave;
      }

   #if DBG
      /*
       * Get the DebugFlags value
       */
      RtlInitStaticUnicodeString(&ValueName,
                                 _LTEXT(KEFRAGMINI_REG_DEBUG_FLAGS));

      
      RtlZeroMemory(ValuePartialInfo,
                    SIZEOF_VALUEPARTIALINFO);

      NtStatus = ZwQueryValueKey(RegistryKey,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValuePartialInfo,
                                 SIZEOF_VALUEPARTIALINFO,
                                 &ResultLength);

      if ( NT_SUCCESS(NtStatus) && (REG_DWORD == ValuePartialInfo->Type) )
      {
         FKxData.DebugFlags = *((PULONG)&ValuePartialInfo->Data[0]);
      }

      if ( DebugFlagsOnly )
      {
         /* Success */
         __leave;
      }
   #else /* FKX_DBG */
      UNREFERENCED_PARAMETER(DebugFlagsOnly);
   #endif /* DBG */

      /*
       * Get the MaximumRecords value
       */
      RtlInitStaticUnicodeString(&ValueName,
                                 _LTEXT(KEFRAGMINI_REG_RECORD_MAX));

      RtlZeroMemory(ValuePartialInfo,
                    SIZEOF_VALUEPARTIALINFO);

      NtStatus = ZwQueryValueKey(RegistryKey,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValuePartialInfo,
                                 SIZEOF_VALUEPARTIALINFO,
                                 &ResultLength);

      if ( NT_SUCCESS(NtStatus) && (REG_DWORD == ValuePartialInfo->Type) )
      {
         SettingValue = *((PLONG)&ValuePartialInfo->Data[0]);
         if ( SettingValue > FKX_RECORD_MAX )
         {
            SettingValue = FKX_RECORD_MAX;
         }

         FKxData.MaxRecordsToAllocate = SettingValue;
      }
      
      /*
       * Get the FeatureControls value
       */
      RtlInitStaticUnicodeString(&ValueName,
                                 _LTEXT(KEFRAGMINI_REG_FEATURE_CONTROL));

      RtlZeroMemory(ValuePartialInfo,
                    SIZEOF_VALUEPARTIALINFO);

      NtStatus = ZwQueryValueKey(RegistryKey,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValuePartialInfo,
                                 SIZEOF_VALUEPARTIALINFO,
                                 &ResultLength);

      if ( NT_SUCCESS(NtStatus) && (REG_DWORD == ValuePartialInfo->Type) )
      {
         FKxData.RuntimeControls = *((PLONG)&ValuePartialInfo->Data[0]);
      }
      
      /*
       * Get the size of the InitialVolumes value 
       */
      RtlInitStaticUnicodeString(&ValueName,
                                 _LTEXT(KEFRAGMINI_REG_VOLUME_LIST));

      RtlZeroMemory(ValuePartialInfo,
                    SIZEOF_VALUEPARTIALINFO);

      NtStatus = ZwQueryValueKey(RegistryKey,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValuePartialInfo,
                                 SIZEOF_VALUEPARTIALINFO,
                                 &ResultLength);

      if ( (STATUS_BUFFER_OVERFLOW == NtStatus) || (STATUS_BUFFER_TOO_SMALL == NtStatus) )
      {
         /*
          * Include room for an extra UNICODE_NULL just to be sure we get a multi-sz string
          */
         ExFreePoolWithTag(ValuePartialInfo,
                           KFX_TAG_STARTUP_REGISTRY_DATA);

         ValuePartialInfo = ExAllocatePoolWithTag(PagedPool,
                                                  ResultLength + sizeof(UNICODE_NULL),
                                                  KFX_TAG_STARTUP_REGISTRY_DATA);
         if ( !ValuePartialInfo )
         {
            FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
            /* Failure */
            __leave;
         }

         RtlZeroMemory(ValuePartialInfo,
                       ResultLength + sizeof(UNICODE_NULL));

         NtStatus = ZwQueryValueKey(RegistryKey,
                                    &ValueName,
                                    KeyValuePartialInformation,
                                    ValuePartialInfo,
                                    ResultLength,
                                    &ResultLength);
      }

      if ( !NT_SUCCESS(NtStatus) )
      {
         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
         /* Failure */
         __leave;
      }

      /*
       * Ensure the value is of type REG_MULTI_SZ
       */
      if ( REG_MULTI_SZ != ValuePartialInfo->Type )
      {
         NtStatus = STATUS_OBJECT_TYPE_MISMATCH;

         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
         /* Failure */
         __leave;
      }

      /*
       * Use the value's DataLength to scan the value for null terminated
       * strings which we will interpret as volume names. 
       */
      ResultLength      = ValuePartialInfo->DataLength;
      VolumeName.Buffer = (PWCHAR)ValuePartialInfo->Data;

      /*
       * Determine the length of the volume name. We continue doing this
       * until the value is exhausted. 
       *
       * NOTE! The use of RtlStringCpLengthW() & ResultLength is crucial
       * so that the returned NameLength which is size_t can be safely
       * downcast to a ULONG.
       */
      NtStatus = RtlStringCbLengthW(VolumeName.Buffer,
                                    ResultLength,
                                    &NameLength);

      while ( NT_SUCCESS(NtStatus) && (NameLength > 0) )
      {
         /*
          * Setup the remaining VolumeName members and pass the name to our
          * attach routine.
          */
         VolumeName.Length        = (USHORT)NameLength;
         VolumeName.MaximumLength = (USHORT)NameLength;

         FKxFilterStartupVolume(&VolumeName);

         /*
          * Move the name pointer beyond the string we just processed. Also
          * update the remaining buffer length and get the next string's length.
          */
         VolumeName.Buffer += (NameLength / sizeof(WCHAR)) + 1;
         /* This can't underflow because NameLength is known to be <= ResultLength */
         ResultLength      -= (ULONG)NameLength;
         
         NtStatus = RtlStringCbLengthW(VolumeName.Buffer,
                                       ResultLength,
                                       &NameLength);
      }
   }
   __finally
   {
      ZwClose(RegistryKey);

      if ( NULL != ValuePartialInfo )
      {
         ExFreePoolWithTag(ValuePartialInfo,
                           KFX_TAG_STARTUP_REGISTRY_DATA);
      }
   }
}

VOID
FKxFilterStartupVolume(
   __in PUNICODE_STRING VolumeName
)
{
   /*
      IRQL:             PASSIVE_LEVEL (via DriverEntry)
      THREAD CONTEXT:   SYSTEM
    */
   NTSTATUS    NtStatus;
   PFLT_VOLUME FltVolume;

   DECLARE_STACKSIZEUSED(FKxFilterStartupVolume,
                         sizeof(NtStatus) +
                         sizeof(FltVolume));

   /*
    * Try to get a filter volume object for the specified volume name
    */
   NtStatus = FltGetVolumeFromName(FKxData.FilterObject,
                                   VolumeName,
                                   &FltVolume);

   if ( !NT_SUCCESS(NtStatus) )
   {
      FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));
      /* Failure */
      return;
   }

   __try
   {
      NtStatus = FltAttachVolume(FKxData.FilterObject,
                                 FltVolume,
                                 NULL,
                                 NULL);

      if ( !NT_SUCCESS(NtStatus) )
      {
         FKxDbgTrace(FKX_DBGTRACE_ERROR, (__FUNCTION__ "!failed @ %d!Status=%08lx\n", __LINE__, NtStatus));

         FKxLogErrorEntry((ULONG)(ULONG_PTR)&FKxFilterStartupVolume,
                          NtStatus,
                          0,
                          VolumeName->Buffer,
                          VolumeName->Length);
         /* Failure */
      }
   }
   __finally
   {
      FltObjectDereference(FltVolume);
   }
}

/**********************************************************************

    FragFlt - Mini-filter support

 **********************************************************************/
NTSTATUS
FKxFltUnload(
   FLT_FILTER_UNLOAD_FLAGS Flags
)
{
   /*
      IRQL:             <= APC_LEVEL
      THREAD CONTEXT:   SYSTEM
    */
   DECLARE_STACKSIZEUSED(FKxFltUnload,
                         0);
 
   UNREFERENCED_PARAMETER(Flags);

   PAGED_CODE();

#if DBG
   if ( FKX_DBGF_BREAK_ON_FLTUNLOAD & FKxData.DebugFlags )
   {
      KdBreakPoint();
   }
#endif /* DBG */

   /*
    * Close the server port
    */
   FltCloseCommunicationPort(FKxData.ServerPort);

   /*
    * Unregister the filter
    */
   FltUnregisterFilter(FKxData.FilterObject);

   /*
    * Empty and free the log record list
    */
   FKxFltRecordLogEmpty();

   ExDeleteNPagedLookasideList(&FKxData.KLogRecordFreeList);

   /* Success */
   return ( STATUS_SUCCESS );
}

NTSTATUS 
FKxFltPortConnect(
   __in PFLT_PORT ClientPort,
   __in_opt PVOID ServerPortCookie,
   __in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
   __in ULONG SizeOfContext,
   __deref_out_opt PVOID* ConnectionPortCookie
)
{
   /*
      IRQL:             ???
      THREAD CONTEXT:   ARBITRARY
    */
   DECLARE_STACKSIZEUSED(FKxFltPortConnect,
                         0);

   UNREFERENCED_PARAMETER( ServerPortCookie );
   UNREFERENCED_PARAMETER( ConnectionContext );
   UNREFERENCED_PARAMETER( SizeOfContext );
   UNREFERENCED_PARAMETER( ConnectionPortCookie );

   PAGED_CODE();

   ASSERT(!FKxData.ClientPort);

   /*
    * A maximum of 1 client connection was set when the port was created,
    * so we won't be called while a client port is connected
    */
   FKxData.ClientPort = ClientPort;

   /* Success */
   return ( STATUS_SUCCESS );
}

VOID
FKxFltPortDisconnect(
   __in_opt PVOID ConnectionCookie
)
{
   /*
      IRQL:             ???
      THREAD CONTEXT:   ARBITRARY
    */
   DECLARE_STACKSIZEUSED(FKxFltPortDisconnect,
                         0);

   UNREFERENCED_PARAMETER( ConnectionCookie );

   PAGED_CODE();

   /*
    * Close the client connection. No new connections will be allowed until
    * this routine returns
    */
   FltCloseClientPort(FKxData.FilterObject,
                      &FKxData.ClientPort);
}

NTSTATUS
FKxFltInstanceSetup(
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in FLT_INSTANCE_SETUP_FLAGS Flags,
   __in DEVICE_TYPE VolumeDeviceType,
   __in FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{
   /*
      IRQL:             <= APC_LEVEL
      THREAD CONTEXT:   ARBITRARY
    */
   DECLARE_STACKSIZEUSED(FKxFltInstanceSetup,
                         0);

   UNREFERENCED_PARAMETER(FltObjects);
   UNREFERENCED_PARAMETER(Flags);
   UNREFERENCED_PARAMETER(VolumeFilesystemType);

   PAGED_CODE();

   ASSERT(FltObjects->Filter == FKxData.FilterObject);

   /*
    * The only drives that can be defragmented are local disk drives
    */
   if ( FILE_DEVICE_DISK_FILE_SYSTEM != VolumeDeviceType )
   {
      return ( STATUS_FLT_DO_NOT_ATTACH );
   }

   return ( STATUS_SUCCESS );
}

NTSTATUS
FKxFltInstanceQueryTeardown(
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
{
   /*
      IRQL:             <= APC_LEVEL
      THREAD CONTEXT:   ARBITRARY
    */
   DECLARE_STACKSIZEUSED(FKxFltInstanceQueryTeardown,
                         0);

   UNREFERENCED_PARAMETER(FltObjects);
   UNREFERENCED_PARAMETER(Flags);

   PAGED_CODE();

   /*
    * We always return success, but this routine is required to
    * allow detaching of volumes
    */
   return ( STATUS_SUCCESS );
}

NTSTATUS
FKxFltMessageNotify(
   __in PVOID ConnectionCookie,
   __in_bcount_opt(InputBufferSize) PVOID InputBuffer,
   __in ULONG InputBufferSize,
   __out_bcount_part_opt(OutputBufferSize,*ReturnOutputBufferLength) PVOID OutputBuffer,
   __in ULONG OutputBufferSize,
   __out PULONG ReturnOutputBufferLength
)
{
   /*
      IRQL:             ???
      THREAD CONTEXT:   ARBITRARY
    */
   NTSTATUS             NtStatus;
   FKX_FLTCOMMAND_CODE  CommandCode;
   FKX_FLTCOMMAND_BLOCK CommandBlock;

   DECLARE_STACKSIZEUSED(FKxFltMessageNotify,
                         sizeof(NtStatus) +
                         sizeof(CommandCode) +
                         sizeof(CommandBlock));

   UNREFERENCED_PARAMETER(ConnectionCookie);

   PAGED_CODE();

   /*
    * Verify the caller passed a valid command block
    */
#ifdef _WIN64
   if ( FltIs32bitProcess(NULL) )
   {
      if ( !InputBuffer || (sizeof(FKX_FLTCOMMAND_BLOCK_32) > InputBufferSize) )
      {
         /* Failure */
         return ( STATUS_INVALID_PARAMETER );
      }

      /*
       * Also verify that the input buffer pointer is properly aligned
       */
      if ( !IS_ALIGNED(InputBuffer, sizeof(ULONG)) )
      {
         /* Failure */
         return ( STATUS_DATATYPE_MISALIGNMENT );
      }

      CommandBlock32 = (PFKX_FLTCOMMAND_BLOCK_32)InputBuffer;
   }
   else
#endif /* _WIN64 */
   {
      if ( !InputBuffer || (sizeof(FKX_FLTCOMMAND_BLOCK) > InputBufferSize) )
      {
         /* Failure */
         return ( STATUS_INVALID_PARAMETER );
      }

      if ( !IS_ALIGNED(InputBuffer, sizeof(PVOID)) )
      {
         /* Failure */         
         return ( STATUS_DATATYPE_MISALIGNMENT );
      }

#ifdef _WIN64
      CommandBlock32 = NULL;
#endif /* _WIN64 */
   }

   /*
    * Initialize locals
    */
   NtStatus    = STATUS_SUCCESS;
   CommandCode = (FKX_FLTCOMMAND_CODE)-1;

   __try
   {
      /*
       * Extract the command code. We've already determined that the alignment is OK, and
       * there is no thunking that needs to be done since this is the first member making
       * it OK to extract
       */
      CommandCode = ((PFKX_FLTCOMMAND_BLOCK)InputBuffer)->Command;
   }
   __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
   {
      NtStatus = GetExceptionCode();
   }

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      return ( NtStatus );
   }

   switch ( CommandCode )
   {
      case FKxCommandRegisterProcess:
      case FKxCommandUnregisterProcess:
         __try
         {
            /*
             * Extract the ProcessId from the command block
             */
#ifdef _WIN64
            if ( NULL != CommandBlock32 )
            {
               CommandBlock.ProcessId = CommandBlock32->ProcessId;
            }
            else
#endif /* _WIN64 */
            {
               CommandBlock.ProcessId = ((PFKX_FLTCOMMAND_BLOCK)InputBuffer)->ProcessId;
            }

            if ( FKxCommandRegisterProcess == CommandCode )
            {
               NtStatus = FKxFltRegisterProcessId(CommandBlock.ProcessId);
               //if ( NT_SUCCESS(NtStatus) )
               //{
               //   InterlockedOr((LONG*)&FKxData.RuntimeControls,
               //                  FKxFeatureProcessRegistration);
               //}
            }
            else
            {
               FKxFltUnregisterProcessId(CommandBlock.ProcessId);
               /* Success */
               NtStatus = STATUS_SUCCESS;
            }
         }
         __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
         {
            /* Failure */
            NtStatus = GetExceptionCode();
         }

         /* Success / Failure */
         break;

      case FKxCommandGetRecordLimit:
         /*
          * Make sure the ouput buffer is large enough and properly aligned
          */
         if ( !OutputBuffer || (OutputBufferSize < sizeof(ULONG)) )
         {
            NtStatus = STATUS_INVALID_PARAMETER;
            /* Failure */
            break;
         }
#ifdef _WIN64
         if ( NULL != CommandBlock32 )
         {
            if ( !IS_ALIGNED(OutputBuffer, sizeof(ULONG)) )
            {
               NtStatus = STATUS_DATATYPE_MISALIGNMENT;
               /* Failure */
               break;
            }
         }
         else
#endif /* _WIN64 */
         {
            if ( !IS_ALIGNED(OutputBuffer, sizeof(PVOID)) )
            {
               NtStatus = STATUS_DATATYPE_MISALIGNMENT;
               /* Failure */
               break;
            }
         }

         __try
         {          
            *((PULONG)OutputBuffer)     = FKxData.MaxRecordsToAllocate;
            *(ReturnOutputBufferLength) = sizeof(ULONG);
         
            /* Success */
            NtStatus = STATUS_SUCCESS;
         }
         __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
         {
            /* Failure */
            NtStatus = GetExceptionCode();
         }

         /* Success / Failure */
         break;

      case FKxCommandSetRecordLimit:
         __try
         {
            /*
             * Extract the record limit from the caller's command block
             */
#ifdef _WIN64
            if ( NULL != CommandBlock32 )
            {
               CommandBlock.RecordLimit = CommandBlock32->RecordLimit;
            }
            else
#endif /* _WIN64 */
            {
               CommandBlock.RecordLimit = ((PFKX_FLTCOMMAND_BLOCK)InputBuffer)->RecordLimit;
            }

            if ( CommandBlock.RecordLimit > FKX_RECORD_MAX )
            {
               NtStatus = STATUS_INVALID_PARAMETER;
               /* Failure */
               __leave;
            }

            /*
             * There is a chance here that there may already be more
             * records than the new value, but oh well 
             */
            InterlockedExchange((LONG*)&FKxData.MaxRecordsToAllocate,
                                CommandBlock.RecordLimit);

            /* Success */
            NtStatus = STATUS_SUCCESS;
         }
         __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
         {
            /* Failure */
            NtStatus = GetExceptionCode();
         }

         /* Success / Failure */
         break;

      case FKxCommandSetTriggerCount:
         __try
         {
            /*
             * Extract the notification info from the caller's command block
             */
#ifdef _WIN64
            if ( NULL != CommandBlock32 )
            {
               CommandBlock.NotifyInfo.TriggerEvent = (HANDLE)CommandBlock32->NotifyInfo.TriggerEvent;
               CommandBlock.NotifyInfo.TriggerCount = CommandBlock32->NotifyInfo.TriggerCount;
            }
            else
#endif /* _WIN64 */
            {
               CommandBlock.NotifyInfo.TriggerEvent = ((PFKX_FLTCOMMAND_BLOCK)InputBuffer)->NotifyInfo.TriggerEvent;
               CommandBlock.NotifyInfo.TriggerCount = ((PFKX_FLTCOMMAND_BLOCK)InputBuffer)->NotifyInfo.TriggerCount;
            }
            
            /*
             * Get the event object for the handle
             */
            EventObject = NULL;

            if ( NULL != CommandBlock.NotifyInfo.TriggerEvent )
            {
               NtStatus = ObReferenceObjectByHandle(CommandBlock.NotifyInfo.TriggerEvent,
                                                    EVENT_MODIFY_STATE,
                                                    *ExEventObjectType,
                                                    UserMode,
                                                    (PVOID*)&EventObject,
                                                    NULL);

               if ( !NT_SUCCESS(NtStatus) )
               {
                  /* Failure */
                  __leave;
               }
            }

            /*
             * Assign the new trigger event object and get the old one
             */
            EventObject = (PKEVENT)InterlockedExchangePointer((PVOID*)&FKxData.LoggerTriggerEvent,
                                                              (PVOID)EventObject);

            /*
             * If there was an event already there, free it
             */
            if ( NULL != EventObject )
            {
               ObDereferenceObject(EventObject);
            }
            
            /*
             * Now assign the new trigger count
             */
            InterlockedExchange((LONG*)&FKxData.LoggerTriggerCount,
                                CommandBlock.NotifyInfo.TriggerCount);

            /*
             * Check if the current count is above the new trigger and if so trigger the new event
             */
            if ( FKxData.RecordsAllocated >= FKxData.LoggerTriggerCount )
            {
               FKxNotifyLogger();
            }

            /* Success */
            NtStatus = STATUS_SUCCESS;
         }         
         __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
         {
            /* Failure */
            NtStatus = GetExceptionCode();
         }

         /* Success / Failure */
         break;

      case FKxCommandGetFeatureControls:
         /*
          * Make sure the ouput buffer is large enough and properly aligned
          */
         if ( !OutputBuffer || (OutputBufferSize < sizeof(ULONG)) )
         {
            NtStatus = STATUS_INVALID_PARAMETER;
            /* Failure */
            break;
         }
#ifdef _WIN64
         if ( NULL != CommandBlock32 )
         {
            if ( !IS_ALIGNED(OutputBuffer, sizeof(ULONG)) )
            {
               NtStatus = STATUS_DATATYPE_MISALIGNMENT;
               /* Failure */
               break;
            }
         }
         else
#endif /* _WIN64 */
         {
            if ( !IS_ALIGNED(OutputBuffer, sizeof(PVOID)) )
            {
               NtStatus = STATUS_DATATYPE_MISALIGNMENT;
               /* Failure */
               break;
            }
         }

         __try
         {          
            *((PULONG)OutputBuffer)     = FKxData.RuntimeControls;
            *(ReturnOutputBufferLength) = sizeof(ULONG);
         
            /* Success */
            NtStatus = STATUS_SUCCESS;
         }
         __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
         {
            /* Failure */
            NtStatus = GetExceptionCode();
         }

         /* Success / Failure */
         break;

      case FKxCommandSetFeatureControls:
         __try
         {
            /*
             * Extract the feature controls from the caller's command block
             */
#ifdef _WIN64
            if ( NULL != CommandBlock32 )
            {
               CommandBlock.FeatureInfo.FeatureMask     = CommandBlock32->FeatureInfo.FeatureMask;
               CommandBlock.FeatureInfo.FeatureControls = CommandBlock32->FeatureInfo.FeatureControls;
            }
            else
#endif /* _WIN64 */
            {
               CommandBlock.FeatureInfo.FeatureMask     = ((PFKX_FLTCOMMAND_BLOCK)InputBuffer)->FeatureInfo.FeatureMask;
               CommandBlock.FeatureInfo.FeatureControls = ((PFKX_FLTCOMMAND_BLOCK)InputBuffer)->FeatureInfo.FeatureControls;
            }

            /*
             * Build the clear bit set
             */
            CommandBlock.FeatureInfo.FeatureMask &= ~CommandBlock.FeatureInfo.FeatureControls;

            /*
             * Turn on/off the features
             */
            InterlockedSetClearBits((PLONG)&FKxData.RuntimeControls,
                                    CommandBlock.FeatureInfo.FeatureControls,
                                    CommandBlock.FeatureInfo.FeatureMask);
         
            /*
             * Install/uninstall the process notify routine 
             */
            //if ( BooleanFlagOn(FKxData.RuntimeControls, FKxFeatureProcessRegistration) )
            //{
            //   PsSetCreateProcessNotifyRoutine(&FKxFltCreateProcessNotifyRoutine,
            //                                   FKxData.ProcessNotifyInstalled);

            //   FKxData.ProcessNotifyInstalled = ;



            /* Success */
            NtStatus = STATUS_SUCCESS;
         }
         __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
         {
            /* Failure */
            NtStatus = GetExceptionCode();
         }

         /* Success / Failure */
         break;
         
      case FKxCommandQueryLogRecordEntries:
         /*
          * Do any initial check of the output buffer to make sure it 
          * exists and is properly aligned
          */
         if ( !OutputBuffer || (0 == OutputBufferSize) )
         {
            NtStatus = STATUS_INVALID_PARAMETER;
            /* Failure */
            break;
         }

#ifdef _WIN64
         if ( FltIs32bitProcess(NULL) )
         {
            if ( !IS_ALIGNED(OutputBuffer, sizeof(ULONG)) )
            {
               NtStatus = STATUS_DATATYPE_MISALIGNMENT;
               /* Failure */
               break;
            }
         }
         else
#endif /* _WIN64 */
         {
            if ( !IS_ALIGNED(OutputBuffer, sizeof(PVOID)) )
            {
               NtStatus = STATUS_DATATYPE_MISALIGNMENT;
               /* Failure */
               break;
            }
         }

         /*
          * The output buffer looks good enough to use, so forward to
          * the handler routine
          */
         NtStatus = FKxFltRecordLogRead((PUCHAR)OutputBuffer,
                                        OutputBufferSize,
                                        ReturnOutputBufferLength);

         /* Success / Failure */
         break;
      
      default:
         FKxDbgTrace(FKX_DBGTRACE_WARNING, (__FUNCTION__ "!Invalid filter command!%d\n", CommandCode));

         NtStatus = STATUS_INVALID_PARAMETER;
         /* Failure */
         break;
   }

   /* Success / Failure */
   return ( NtStatus );
}

VOID
FKxFltContextCleanup(
   __in PFLT_CONTEXT Context,
   __in FLT_CONTEXT_TYPE ContextType
)
{
   ASSERT(FLT_STREAM_CONTEXT == ContextType);

   switch ( ContextType )
   {
      case FLT_STREAM_CONTEXT:
         FKxFltDumpStreamContextReferenceCount(Context);
         FKxFltMarkStreamContextStale((PFKX_STREAM_CONTEXT)Context);
         break;

      default:
         break;
   }
}

FLT_PREOP_CALLBACK_STATUS
FKxFltPreCreate(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID *CompletionContext
)
{
   /*
      IRQL:             PASSIVE_LEVEL
      THREAD CONTEXT:   ARBITRARY
    */
   ACCESS_MASK RequiredAccess;

   DECLARE_STACKSIZEUSED(FKxFltPreCreate,
                         sizeof(RequiredAccess));

   UNREFERENCED_PARAMETER(FltObjects);
   UNREFERENCED_PARAMETER(CompletionContext);

   /*
    * Ignore requests for files which will be deleted when they're closed
    */
   if ( FILE_DELETE_ON_CLOSE & Data->Iopb->Parameters.Create.Options )
   {
      FKxDbgTrace(FKX_DBGTRACE_INFO, (__FUNCTION__ "!Ignoring file due to FILE_DELETE_ON_CLOSE (%.*ws)\n", Data->Iopb->TargetFileObject->FileName.Length, Data->Iopb->TargetFileObject->FileName.Buffer));
      return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
   }   

   /* 
    * Ignore requests from the usermode service app
    */
   if ( FKxFltIsRegisteredProcessId(FltGetRequestorProcessId(Data)) )
   {
      return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
   }

   RequiredAccess  = IsFeatureControlOn(FKxFeatureCaptureRead, &FKxData) ? FKX_READ_ACCESS : 0;
   RequiredAccess |= IsFeatureControlOn(FKxFeatureCaptureWrite, &FKxData) ? FKX_WRITE_ACCESS : 0;

   if ( Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & RequiredAccess )
   {     
      return ( FLT_PREOP_SUCCESS_WITH_CALLBACK );
   }

   return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
}

FLT_POSTOP_CALLBACK_STATUS
FKxFltPostCreate(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in_opt PVOID CompletionContext,
   __in FLT_POST_OPERATION_FLAGS Flags
)
{
   /*
      IRQL:             PASSIVE_LEVEL (Post-Create is the exception)
      THREAD CONTEXT:   ARBITRARY
    */
   NTSTATUS                   NtStatus;
   PFLT_FILE_NAME_INFORMATION NameInformation;
   ACCESS_MASK                GrantedAccess;
   ACCESS_MASK                RequiredAccess;
   PFKX_STREAM_CONTEXT        StreamContext;
   FILE_BASIC_INFORMATION     BasicInformation;
   ULONG                      ReturnLength;
   BOOLEAN                    ForceLogAsFile;
   
   DECLARE_STACKSIZEUSED(FKxFltPostCreate,
                         sizeof(NtStatus) +
                         sizeof(NameInformation) +
                         sizeof(GrantedAccess) +
                         sizeof(RequiredAccess) +
                         sizeof(StreamContext) + 
                         sizeof(ForceLogAsFile));

   UNREFERENCED_PARAMETER(CompletionContext);
   UNREFERENCED_PARAMETER(Flags);

   if ( FlagOn(FLTFL_POST_OPERATION_DRAINING, Flags) )
   {
      /*
       * FltMgr is running down its post callbacks so just return
       */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   /*
    * If the create failed, no logging is done
    */
   if ( !NT_SUCCESS(Data->IoStatus.Status) || (STATUS_REPARSE == Data->IoStatus.Status) )
   {
      /* Success */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }
   
   /*
    * Build the access mask granted to the creator
    */
   GrantedAccess  = Data->Iopb->Parameters.Create.SecurityContext->AccessState->OriginalDesiredAccess;
   GrantedAccess &= ~Data->Iopb->Parameters.Create.SecurityContext->AccessState->RemainingDesiredAccess;

   /*
    * Check if the granted access is one that should be logged
    */
   RequiredAccess  = IsFeatureControlOn(FKxFeatureCaptureRead, &FKxData) ? FKX_READ_ACCESS : 0;
   RequiredAccess |= IsFeatureControlOn(FKxFeatureCaptureWrite, &FKxData) ? FKX_WRITE_ACCESS : 0;

   if ( 0 == (GrantedAccess & RequiredAccess) )
   {
      /* Success */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   /*
    * Get the normalized file name. This is safe here in the post IRP_MJ_CREATE path
    */
   NtStatus = FltGetFileNameInformation(Data,
                                        FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
                                        &NameInformation);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure - Can't do anything without a filename */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   /*
    * Ensure that the name information is parsed. If this fails we have to assume that the filename is
    * bogus and can't be logged
    */
   NtStatus = FltParseFileNameInformation(NameInformation);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      goto __CLEANUP;
   }
   
   /*
    * Check if this file has already been recorded by testing for the existance
    * of a stream context and the LoggedCount
    */
   NtStatus = FltGetStreamContext(Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject,
                                  &StreamContext);

   if ( NT_SUCCESS(NtStatus) )
   {
      FKxFltDumpStreamContextReferenceCount(StreamContext);

      /*
       * If the filter is setup to capture read access, we log the file here regardless of the access mode 
       */
      if ( IsFeatureControlOn(FKxFeatureCaptureRead, &FKxData) )
      {
         if ( 0 == StreamContext->LoggedCount )
         {
            _ReadBarrier();

            if ( 1 == InterlockedIncrement(&StreamContext->LoggedCount) )
            {
               NtStatus = KFxFltLogStreamContext(StreamContext,
                                                 FALSE);
            }
         }
      }
   
      FKxFltDumpStreamContextReferenceCount(StreamContext);
      FltReleaseContext(StreamContext);

      /* Success */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }
   else if ( STATUS_NOT_FOUND != NtStatus )
   {
      /* An error other than the file not having a context occured, so just bail */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   /*
    * Get file times for the stream context
    */
   NtStatus = FltQueryInformationFile(FltObjects->Instance,
                                      FltObjects->FileObject,
                                      &BasicInformation,
                                      sizeof(BasicInformation),
                                      FileBasicInformation,
                                      &ReturnLength);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   ASSERT(sizeof(BasicInformation) == ReturnLength);


   /*
    * Allocate and assign a stream context for this file. 
    */
   NtStatus = FltAllocateContext(FltObjects->Filter,
                                 FLT_STREAM_CONTEXT,
                                 FKX_STREAM_CONTEXT_SIZEOF,
                                 PagedPool,
                                 &StreamContext);

   if ( NT_SUCCESS(NtStatus) )
   {
      FKxFltDumpStreamContextReferenceCount(StreamContext);

      /*
       * Initialize our parts of the stream context
       */
      NtStatus = FKxFltInitializeStreamContext(FltGetRequestorProcessId(Data),
                                               GrantedAccess,
                                               FltObjects->Volume,
                                               NameInformation,
                                               BasicInformation.LastWriteTime,
                                               StreamContext);

      if ( NT_SUCCESS(NtStatus) )
      {
         /*
          * Assign the stream context. If one got assigned before this one had a chance to
          * be then we'll release this one. 
          */
         NtStatus = FltSetStreamContext(Data->Iopb->TargetInstance,
                                        Data->Iopb->TargetFileObject,
                                        FLT_SET_CONTEXT_KEEP_IF_EXISTS,
                                        StreamContext,
                                        NULL);

         FKxFltDumpStreamContextReferenceCount(StreamContext);

         if ( !NT_SUCCESS(NtStatus) )
         {
            /*
             * FltSetStreamContext() always adds a reference on the context, but since the call failed it
             * effectively abandoned the reference and we're responsible for releasing it
             */
            FKxFltDumpStreamContextReferenceCount(StreamContext);
            FltReleaseContext(StreamContext);
         }
      }
   
      /*
       * Lose the allocation reference count. This closes all references added and invalidates
       * this routines access to the context
       */
      FKxFltDumpStreamContextReferenceCount(StreamContext);
      FltReleaseContext(StreamContext);
   }

   /*
    * If any of the stream actions failed, fallback to logging the file here
    */
   if ( !NT_SUCCESS(NtStatus) )
   {
      /*
       * This is for handling when a stream context can not be created to track the file during subsequent
       * creates/writes. The file has to be logged regardless of the access mode because this is as far as
       * we'll be able to follow it
       */
      NtStatus = FKxFltRecordLogWrite(FltGetRequestorProcessId(Data),
                                      GrantedAccess,
                                      FltObjects->Volume,
                                      NameInformation);
   }

__CLEANUP:
   /*
    * Release this routine's reference on the name-cache record
    */
   FltReleaseFileNameInformation(NameInformation);

   /* Success / Failure */
   return ( FLT_POSTOP_FINISHED_PROCESSING );
}

#if 0
FLT_PREOP_CALLBACK_STATUS
FKxFltPreWrite(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID *CompletionContext
)
{
   NTSTATUS             NtStatus;
   PFKX_STREAM_CONTEXT  StreamContext;

   UNREFERENCED_PARAMETER(FltObjects);

   ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

   NtStatus = FltGetStreamContext(Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject,
                                  &StreamContext);

   if ( !NT_SUCCESS(NtStatus) )
   {
      return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
   }

   FKxFltDumpStreamContextReferenceCount(StreamContext);

   /*
    * Post-Write cannot call FltGetStreamContext() so stash it in the completion context. This
    * implies that Post-Write is responsible for releasing the reference
    */
   (*CompletionContext) = StreamContext;

   return (  FLT_PREOP_SUCCESS_WITH_CALLBACK );
}

FLT_POSTOP_CALLBACK_STATUS
FKxFltPostWrite(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in_opt PVOID CompletionContext,
   __in FLT_POST_OPERATION_FLAGS Flags
)
{
   /*
      IRQL:             <= DISPATCH_LEVEL
      THREAD CONTEXT:   ARBITRARY
    */
   NTSTATUS                   NtStatus;
   PFKX_STREAM_CONTEXT        StreamContext;
   PFLT_GENERIC_WORKITEM      FltWorkItem;

   UNREFERENCED_PARAMETER(FltObjects);

   StreamContext = CompletionContext;
   /* It's impossible to see a Post callback without a Pre */
   ASSERT(NULL != StreamContext);
   if ( !StreamContext )
   {
      /* Success */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   FKxFltDumpStreamContextReferenceCount(StreamContext);

   /*
    * If this is the FltMgr calling an outstanding post completion request
    * then simply release the context and return
    */
   if ( FlagOn(FLTFL_POST_OPERATION_DRAINING, Flags) )
   {
      FKxFltDumpStreamContextReferenceCount(StreamContext);
      FltReleaseContext(StreamContext);

      /* Success */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   /*
    * If the write failed, there's no sense logging the file
    */
   if ( !NT_SUCCESS(Data->IoStatus.Status) )
   {
      FKxFltDumpStreamContextReferenceCount(StreamContext);
      FltReleaseContext(StreamContext);

      /* Success */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   /*
    * If this is the first thread to capture a write for this stream then 
    * it is responsible for logging the stream
    */
   if ( 0 == StreamContext->LoggedCount )
   {
      _ReadBarrier();

      if ( 1 == InterlockedIncrement((LONG volatile*)&StreamContext->LoggedCount) )
      {
         /*
          * If we're already at passive level, run the work routine directly
          */
         if ( PASSIVE_LEVEL == KeGetCurrentIrql() )
         {
            NtStatus = KFxFltLogStreamContext(StreamContext, 
                                              FALSE);
         }
         else
         {
            /*
             * We're not at PASSIVE_LEVEL so we need to queue logging to a system thread which will be
             */
            FltWorkItem = FltAllocateGenericWorkItem();
            
            if ( NULL != FltWorkItem )
            {
               NtStatus = FltQueueGenericWorkItem(FltWorkItem,
                                                  FltObjects->Instance,
                                                  KFxFltLogStreamGenericWorkRoutine,
                                                  DelayedWorkQueue,
                                                  StreamContext);

               if ( NT_SUCCESS(NtStatus) )
               {
                  /*
                   * The work routine is now responsible for releasing the reference on the stream context
                   */
                  return ( FLT_POSTOP_FINISHED_PROCESSING );
               }

               /*
                * Free the work item here
                */
               FltFreeGenericWorkItem(FltWorkItem);

               /*
                * Reset the LoggedCount so another thread may have a chance to log the file
                */
               InterlockedExchange((LONG volatile*)&StreamContext->LoggedCount, 0);
            }
         }
      }
   }

   /*
    * If this path is followed, nothing released the stream context so it has
    * to be done here
    */
   FKxFltDumpStreamContextReferenceCount(StreamContext);
   FltReleaseContext(StreamContext);

   /* Success */
   return ( FLT_POSTOP_FINISHED_PROCESSING );
}
#endif //0

FLT_PREOP_CALLBACK_STATUS
FKxFltPreSetInformation(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID *CompletionContext
)
{
   NTSTATUS                      NtStatus;
   PFKX_STREAM_CONTEXT           StreamContext;
   FILE_INFORMATION_CLASS        FileInformation;
   PFILE_DISPOSITION_INFORMATION FileDisposition;

   UNREFERENCED_PARAMETER(FltObjects);

   ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

   FileInformation = Data->Iopb->Parameters.SetFileInformation.FileInformationClass;

   if ( (FileRenameInformation != FileInformation) && (FileDispositionInformation != FileInformation) )
   {
      /* Success */
      return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
   }

   NtStatus = FltGetStreamContext(Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject,
                                  &StreamContext);

   if ( !NT_SUCCESS(NtStatus) )
   {
      return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
   }

   FKxFltDumpStreamContextReferenceCount(StreamContext);

   /*
    * If the file is being deleted, mark the stream context as logged so that it is ignored
    */
   if ( FileDispositionInformation == FileInformation )
   {
      FileDisposition = (PFILE_DISPOSITION_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
      if ( NULL != FileDisposition )
      {
         if ( FileDisposition->DeleteFile )
         {
            InterlockedIncrement(&StreamContext->LoggedCount);
         }
      }

      /*
       * There is no need to track the post-op for this
       */
      FltReleaseContext(StreamContext);

      /* Success */
      return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
   }

   /*
    * Post-SetInfo cannot call FltGetStreamContext() so stash it in the completion context. This
    * implies that Post-SetInfo is responsible for releasing the reference
    */
   (*CompletionContext) = StreamContext;

   return (  FLT_PREOP_SUCCESS_WITH_CALLBACK );
}

FLT_POSTOP_CALLBACK_STATUS
FKxFltPostSetInformation(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __in_opt PVOID CompletionContext,
   __in FLT_POST_OPERATION_FLAGS Flags
)
{
   /*
      IRQL:             <= DISPATCH_LEVEL
      THREAD CONTEXT:   ARBITRARY
    */
   NTSTATUS                   NtStatus;
   PFILE_RENAME_INFORMATION   RenameInformation;
   PFLT_FILE_NAME_INFORMATION NameInformation;
   PFKX_STREAM_CONTEXT        StreamContext;
   PFLT_VOLUME                Volume;

   UNREFERENCED_PARAMETER(FltObjects);

   /*
    * Initialize locals
    */
   NameInformation = NULL;   
   StreamContext   = CompletionContext;
   Volume          = NULL;

   /* It's impossible to see a Post callback without a Pre */
   ASSERT(NULL != StreamContext);
   if ( !StreamContext )
   {
      /* Success */
      return ( FLT_POSTOP_FINISHED_PROCESSING );
   }

   FKxFltDumpStreamContextReferenceCount(StreamContext);

   /*
    * If this is the FltMgr calling an outstanding post completion request
    * then simply release the context and return
    */
   if ( FlagOn(FLTFL_POST_OPERATION_DRAINING, Flags) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * If the operation failed, there's no sense logging the file
    */
   if ( !NT_SUCCESS(Data->IoStatus.Status) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   if ( FileRenameInformation != Data->Iopb->Parameters.SetFileInformation.FileInformationClass )
   {
      /* Failure */
      goto __CLEANUP;
   }

   RenameInformation = (PFILE_RENAME_INFORMATION)Data->Iopb->Parameters.SetFileInformation.InfoBuffer;
   
   NtStatus = FltGetDestinationFileNameInformation(FltObjects->Instance,
                                                   FltObjects->FileObject,
                                                   RenameInformation->RootDirectory,
                                                   RenameInformation->FileName,
                                                   RenameInformation->FileNameLength,
                                                   FLT_FILE_NAME_NORMALIZED|FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
                                                   &NameInformation);

   if ( NT_SUCCESS(NtStatus) )
   {
      NtStatus = FltParseFileNameInformation(NameInformation);
   }

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Make sure the context will have a volume
    */
   NtStatus = FltGetVolumeFromName(FltObjects->Filter,
                                   &NameInformation->Volume,
                                   &Volume);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Swap the name information records in the stream context
    */
   NameInformation = InterlockedExchangePointer(&StreamContext->FileNameInformation,
                                                NameInformation);

   Volume = InterlockedExchangePointer(&StreamContext->Volume,
                                       Volume);

   /*
    * Force the record to be scheduled for logging
    */
   InterlockedExchange(&StreamContext->LoggedCount,
                       0);

__CLEANUP:
   if ( NULL != NameInformation )
   {
      FltReleaseFileNameInformation(NameInformation);
   }

   if ( NULL != Volume )
   {
      FltObjectDereference(Volume);
   }

   FKxFltDumpStreamContextReferenceCount(StreamContext);
   FltReleaseContext(StreamContext);
   
   /* Success / Failure */
   return ( FLT_POSTOP_FINISHED_PROCESSING );
}

FLT_PREOP_CALLBACK_STATUS
FKxFltPreCleanup(
   __inout PFLT_CALLBACK_DATA Data,
   __in PCFLT_RELATED_OBJECTS FltObjects,
   __deref_out_opt PVOID* CompletionContext
)
{
   NTSTATUS                   NtStatus;
   PFKX_STREAM_CONTEXT        StreamContext;
   FILE_BASIC_INFORMATION     BasicInformation;
   ULONG                      ReturnLength;
   PFLT_GENERIC_WORKITEM      FltWorkItem;

   UNREFERENCED_PARAMETER(FltObjects);
   UNREFERENCED_PARAMETER(CompletionContext);

   ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
   
   NtStatus = FltGetStreamContext(Data->Iopb->TargetInstance,
                                  Data->Iopb->TargetFileObject,
                                  &StreamContext);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
   }

   FKxFltDumpStreamContextReferenceCount(StreamContext);

   /*
    * Compare the file's times now to what was stored in the stream context
    */
   NtStatus = FltQueryInformationFile(FltObjects->Instance,
                                      FltObjects->FileObject,
                                      &BasicInformation,
                                      sizeof(BasicInformation),
                                      FileBasicInformation,
                                      &ReturnLength);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   ASSERT(sizeof(BasicInformation) == ReturnLength);

   //FKxDbgTrace(FKX_DBGTRACE_TRACE, (__FUNCTION__ "!Cleanup!LastWriteTime:%I64u/%I64u, Difference=%I64u\n", StreamContext->LastWriteTime, BasicInformation.LastWriteTime, BasicInformation.LastWriteTime.QuadPart - StreamContext->LastWriteTime.QuadPart));

   if ( StreamContext->LastWriteTime.QuadPart == BasicInformation.LastWriteTime.QuadPart )
   {
      /* Success */
      goto __CLEANUP;
   }

   /*
    * If this is the first thread to capture a write for this stream then 
    * it is responsible for logging the stream
    */
   if ( 0 == StreamContext->LoggedCount )
   {
      _ReadBarrier();

      if ( 1 == InterlockedIncrement((LONG volatile*)&StreamContext->LoggedCount) )
      {
         /*
          * If we're already at passive level, run the work routine directly
          */
         if ( PASSIVE_LEVEL == KeGetCurrentIrql() )
         {
            NtStatus = KFxFltLogStreamContext(StreamContext, 
                                              FALSE);
         }
         else
         {
            /*
             * We're not at PASSIVE_LEVEL so we need to queue logging to a system thread which will be
             */
            FltWorkItem = FltAllocateGenericWorkItem();
            
            if ( NULL != FltWorkItem )
            {
               NtStatus = FltQueueGenericWorkItem(FltWorkItem,
                                                  FltObjects->Instance,
                                                  KFxFltLogStreamGenericWorkRoutine,
                                                  DelayedWorkQueue,
                                                  StreamContext);

               if ( NT_SUCCESS(NtStatus) )
               {
                  /*
                   * The work routine is now responsible for releasing the reference on the stream context
                   */
                  return ( FLT_PREOP_SUCCESS_NO_CALLBACK );
               }

               /*
                * Free the work item here
                */
               FltFreeGenericWorkItem(FltWorkItem);

               /*
                * Reset the LoggedCount so another thread may have a chance to log the file
                */
               InterlockedExchange((LONG volatile*)&StreamContext->LoggedCount, 0);
            }
         }
      }
   }


__CLEANUP:
   FKxFltDumpStreamContextReferenceCount(StreamContext);
   FltReleaseContext(StreamContext);

   /* Success / Failure */
   return (  FLT_PREOP_SUCCESS_NO_CALLBACK );
}

VOID
KFxFltLogStreamGenericWorkRoutine(
   __in PFLT_GENERIC_WORKITEM FltWorkItem,
   __in PFLT_FILTER Filter,
   __in PVOID Context
)
{
   /*
      IRQL:             PASSIVE_LEVEL
      THREAD CONTEXT:   ARBITRARY (due to direct invocation)
    */

   NTSTATUS            NtStatus;
   PFKX_STREAM_CONTEXT StreamContext;

   UNREFERENCED_PARAMETER(Filter);

   StreamContext = Context;

   FKxFltDumpStreamContextReferenceCount(Context);

   NtStatus = KFxFltLogStreamContext(StreamContext,
                                     TRUE);
   
   FltFreeGenericWorkItem(FltWorkItem);
}

NTSTATUS
KFxFltLogStreamContext(
   __inout PFKX_STREAM_CONTEXT StreamContext,
   __in BOOLEAN ReleaseContext
)
{
   NTSTATUS NtStatus;

   /* If these assert then some thread has called FKxFltMarkStreamContextStale() followed
    * by a reset of the LoggedCount */
   ASSERT(NULL != StreamContext->Volume);
   ASSERT(NULL != StreamContext->FileNameInformation);

   FKxFltDumpStreamContextReferenceCount(StreamContext);

   /*
    * Log the file information
    */
   NtStatus = FKxFltRecordLogWrite(StreamContext->OriginatingProcess,
                                   StreamContext->AccessGranted,
                                   StreamContext->Volume,
                                   StreamContext->FileNameInformation);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /*
       * Reset the write count so that another request has a chance to log it
       */
      InterlockedExchange((LONG volatile*)&StreamContext->LoggedCount, 0);
   }
   else
   {
      FKxFltMarkStreamContextStale(StreamContext);
   }

   if ( ReleaseContext )
   {
      FKxFltDumpStreamContextReferenceCount(StreamContext);
      FltReleaseContext(StreamContext);
   }

   /* Success / Failure */
   return ( NtStatus );
}


/**********************************************************************

    Support routine implementations

 **********************************************************************/
NTSTATUS
FKxFltInitializeStreamContext(
   __in ULONG ProcessId,
   __in ULONG AccessMode,
   __in PFLT_VOLUME Volume,
   __in PFLT_FILE_NAME_INFORMATION FileNameInformation,
   __in LARGE_INTEGER LastWriteTime,
   __inout PFKX_STREAM_CONTEXT StreamContext
)
{
   /*
      IRQL:             <= DISPATCH_LEVEL
      THREAD CONTEXT:   ARBITRARY
    */

   NTSTATUS NtStatus;

   DECLARE_STACKSIZEUSED(FKxFltInitializeStreamContext,
                         sizeof(NtStatus));

   ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

   RtlZeroMemory(StreamContext,
                 FKX_STREAM_CONTEXT_SIZEOF);

   NtStatus = FltObjectReference(Volume);
   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      return ( NtStatus );
   }

   StreamContext->Volume = Volume;
   
   FltReferenceFileNameInformation(FileNameInformation);
   StreamContext->FileNameInformation = FileNameInformation;
   
   StreamContext->OriginatingProcess = ProcessId;
   StreamContext->AccessGranted      = AccessMode;
   StreamContext->LastWriteTime      = LastWriteTime;

   /*
    * Success
    */
   return ( STATUS_SUCCESS );
}

VOID
FKxFltMarkStreamContextStale(
   __in PFKX_STREAM_CONTEXT StreamContext
)
{
   /*
      IRQL:             <= APC_LEVEL
      THREAD CONTEXT:   ARBITRARY
    */
   PFLT_VOLUME                Volume;
   PFLT_FILE_NAME_INFORMATION NameInformation;

   DECLARE_STACKSIZEUSED(FKxFltMarkStreamContextStale,
                         0);

   /*
    * Initialize locals
    */
   Volume          = NULL;
   NameInformation = NULL;

   /*
    * Lose the references in the stream context that could potentially force their
    * associate object's resources to be locked. An example is the FileNameInformation
    * member. The log record also keeps a reference on the same structure, but once
    * a client has read the record the same reference is dropped. The stream context
    * may potentially outlive the log record and once the log record is done, the
    * stream context's reference will never be used again so keeping it around is
    * a waste of resources.
    *
    * Do _not_ alter the LoggedCount, because it will be used during the lifetime of
    * the stream context to test whether the file has been logged or not
    */
   Volume = InterlockedExchangePointer(&StreamContext->Volume,
                                       NULL);

   if ( NULL != Volume )
   {
      FltObjectDereference(Volume);
   }

   NameInformation = InterlockedExchangePointer(&StreamContext->FileNameInformation,
                                                NULL);

   if ( NULL != NameInformation )
   {
      FltReleaseFileNameInformation(NameInformation);
   }
}

NTSTATUS
FKxFltRecordAllocate(
   __out PFKX_KLOG_RECORD_LIST* KRecordList
)
{
   PFKX_KLOG_RECORD_LIST ListEntry;

   DECLARE_STACKSIZEUSED(FKxFltRecordAllocate,
                         sizeof(ListEntry));

   if ( FKxData.RecordsAllocated >= FKxData.MaxRecordsToAllocate )
   {
      FKxDbgTrace(FKX_DBGTRACE_WARNING, (__FUNCTION__ "!Maximum records allocated @ %u\n", FKxData.RecordsAllocated));

      /* Failure */
      return ( STATUS_LOG_CONTAINER_LIMIT_EXCEEDED );
   }

   InterlockedIncrement((LONG*)&FKxData.RecordsAllocated);

   ListEntry = (PFKX_KLOG_RECORD_LIST)ExAllocateFromNPagedLookasideList(&FKxData.KLogRecordFreeList);

   if ( !ListEntry )
   {
      InterlockedDecrement((LONG*)&FKxData.RecordsAllocated);

      /* Failure */
      return ( STATUS_INSUFFICIENT_RESOURCES );
   }

   (*KRecordList) = ListEntry;

   /* Success */
   return ( STATUS_SUCCESS );
}

VOID
FKxFltRecordFree(
   __in PFKX_KLOG_RECORD_LIST KRecordList
)
{
   DECLARE_STACKSIZEUSED(FKxFltRecordFree,
                         0);

   /*
    * Update the counter and free the memory
    */
   InterlockedDecrement((LONG*)&FKxData.RecordsAllocated);

   ASSERT(NULL != KRecordList->KLogRecord.FileNameInformation);
   FltReleaseFileNameInformation(KRecordList->KLogRecord.FileNameInformation);

   ExFreeToNPagedLookasideList(&FKxData.KLogRecordFreeList,
                               KRecordList);
}

VOID
FKxFltRecordLogEmpty(
)
{
   PLIST_ENTRY           ListEntry;
   PFKX_KLOG_RECORD_LIST KRecordList;
   KLOCK_QUEUE_HANDLE    LockHandle;

   DECLARE_STACKSIZEUSED(FKxFltRecordEmpty,
                         sizeof(ListEntry) +
                         sizeof(KRecordList) +
                         sizeof(LockHandle));
   
   PAGED_CODE();

   KeAcquireSmartSpinLock(&FKxData.KLogRecordListLock,
                          &LockHandle);

   while ( !IsListEmpty(&FKxData.KLogRecordList) )
   {
      ListEntry = RemoveHeadList(&FKxData.KLogRecordList);

      KeReleaseSmartSpinLock(&FKxData.KLogRecordListLock,
                             &LockHandle);

      KRecordList = CONTAINING_RECORD(ListEntry, 
                                      FKX_KLOG_RECORD_LIST, 
                                      List);

      FKxFltRecordFree(KRecordList);

      KeAcquireSmartSpinLock(&FKxData.KLogRecordListLock,
                             &LockHandle);
   }

   KeReleaseSmartSpinLock(&FKxData.KLogRecordListLock,
                          &LockHandle);
}

NTSTATUS
FKxFltRecordLogRead(
   __out_bcount_part(OutputBufferLength, *ReturnOutputBufferLength) PUCHAR OutputBuffer,
   __in ULONG OutputBufferLength,
   __out PULONG ReturnOutputBufferLength
)
{
   NTSTATUS                   NtStatus;
   PLIST_ENTRY                ListEntry;
   KLOCK_QUEUE_HANDLE         LockHandle;
   PFKX_LOG_RECORD            ULogRecord;
   PULONG                     UNextEntryOffset;
   PFKX_KLOG_RECORD           KLogRecord;
   PFKX_KLOG_RECORD_LIST      KRecordList;
   PUCHAR                     WriteBuffer;
   ULONG                      Length;
   ULONG                      BytesWritten;
   ULONG                      BytesRequired;
   ULONG                      BytesPadding;
   PFLT_FILE_NAME_INFORMATION NameInformation;

   DECLARE_STACKSIZEUSED(FKxFltRecordLogRead,
                         sizeof(NtStatus) +
                         sizeof(ListEntry) +
                         sizeof(LockHandle) +
                         sizeof(ULogRecord) +
                         sizeof(UNextEntryOffset) +
                         sizeof(KLogRecord) +
                         sizeof(KRecordList) +
                         sizeof(WriteBuffer) +
                         sizeof(Length) +
                         sizeof(BytesWritten) +
                         sizeof(BytesRequired) +
                         sizeof(BytesPadding) +
                         sizeof(NameInformation));

   /*
    * Initialize locals
    */
   ULogRecord       = (PFKX_LOG_RECORD)OutputBuffer;
   UNextEntryOffset = (PULONG)OutputBuffer;
   BytesWritten     = 0;
   BytesRequired    = 0;

   KeAcquireSmartSpinLock(&FKxData.KLogRecordListLock,
                          &LockHandle);

   while ( !IsListEmpty(&FKxData.KLogRecordList) && (OutputBufferLength > 0) )
   {
      ListEntry     = RemoveHeadList(&FKxData.KLogRecordList);
      KRecordList   = CONTAINING_RECORD(ListEntry, FKX_KLOG_RECORD_LIST,  List);      
      KLogRecord    = &KRecordList->KLogRecord;
      BytesRequired = FKxCalculateLogRecordSize(KLogRecord);

      if ( OutputBufferLength < BytesRequired )
      {
         InsertHeadList(&FKxData.KLogRecordList,
                        ListEntry);

         /* Success */
         break;
      }

      KeReleaseSmartSpinLock(&FKxData.KLogRecordListLock,
                             &LockHandle);

      __try
      {
         /*
          * Copy the base record members
          */
         UNextEntryOffset               = &ULogRecord->NextEntryOffset;
         ULogRecord->NextEntryOffset    = 0;
         ULogRecord->OriginatingProcess = KLogRecord->OriginatingProcess;
         ULogRecord->TimeOfCapture      = KLogRecord->TimeOfCapture;
         ULogRecord->AccessGranted      = KLogRecord->AccessGranted;

         
         /*
          * Build the file name
          */
         WriteBuffer     = (PUCHAR)ULogRecord->FileName;
         NameInformation = KLogRecord->FileNameInformation;

         /*
          * Copy the volume guid name
          */
         Length = FKxCalculateVolumeGuidNameLength(BytesRequired, 
                                                   NameInformation);

         RtlCopyMemory(WriteBuffer,
                       KLogRecord->VolumeGuidName,
                       Length);

         WriteBuffer += Length;

         /*
          * Now copy anything after the volume name, if it's set otherwise
          * copy the entire name
          */
         Length = NameInformation->Volume.Length;
            
         RtlCopyMemory(WriteBuffer,
                       (PUCHAR)NameInformation->Name.Buffer + Length,
                       NameInformation->Name.Length - Length);

         WriteBuffer += (NameInformation->Name.Length - Length);

         /*
          * Ensure the string is null terminated
          */
         ((PWCHAR)WriteBuffer)[0] = L'\0';
         WriteBuffer             += sizeof(L'\0');

         /*
          * Update the remaining buffer size and the caller's return length. It is also known that neither
          * of these can overflow/underflow because they are subsets of the output buffer length
          */
         OutputBufferLength -= BytesRequired;
         BytesWritten       += BytesRequired;
         
         /*
          * Zero fill the remaining record bytes to properly align the next record 
          */
         NtStatus = RtlUlongAlignUp(BytesRequired,
                                    sizeof(PVOID),
                                    &BytesPadding);

         if ( NT_SUCCESS(NtStatus) )
         {
            BytesPadding -= BytesRequired;
            
            if ( (BytesPadding > 0) && (OutputBufferLength > BytesPadding) )
            {
               RtlZeroMemory(WriteBuffer,
                             BytesPadding);

               /*
                * Update the buffer size and result length. These can't overflow/underflow
                */
               OutputBufferLength -= BytesPadding;
               BytesWritten       += BytesPadding;
               BytesRequired      += BytesPadding;
            }
         }

         /*
          * Finally set the offset for the next record
          */
         ULogRecord->NextEntryOffset = BytesRequired;
         
         /*
          * Move the usermode log record pointer up to the next one
          */
         ULogRecord = (PFKX_LOG_RECORD)((PUCHAR)ULogRecord + ULogRecord->NextEntryOffset);
      }
      __except( FKX_LOG_EXCEPTION, EXCEPTION_EXECUTE_HANDLER )
      {
         NtStatus = GetExceptionCode();

         /*
          * Put the record back on the list
          */
         KeAcquireSmartSpinLock(&FKxData.KLogRecordListLock,
                                &LockHandle);

         InsertHeadList(&FKxData.KLogRecordList,
                        ListEntry);
         /* Failure */
         break;
      }

      FKxFltRecordFree(KRecordList);

      KeAcquireSmartSpinLock(&FKxData.KLogRecordListLock,
                             &LockHandle);
   }

   KeReleaseSmartSpinLock(&FKxData.KLogRecordListLock,
                          &LockHandle);

   /*
    * Clear the last offset in the caller's buffer
    */
   (*UNextEntryOffset) = 0;

   /*
    * Setup the output status
    */
   if ( (0 == BytesWritten) && (BytesRequired > 0) )
   {
      /* Failure */
      NtStatus = STATUS_BUFFER_TOO_SMALL;
   }
   else if ( BytesRequired > 0 )
   {  
      /* Success */
      NtStatus = STATUS_MORE_ENTRIES;
   }
   else
   {
      /* Success */
      NtStatus = STATUS_SUCCESS;
   }

   (*ReturnOutputBufferLength) = BytesWritten;

   return ( NtStatus );
}

NTSTATUS
FKxFltRecordLogWrite(
   __in ULONG ProcessId,
   __in ULONG AccessMode,
   __in PFLT_VOLUME Volume,
   __in PFLT_FILE_NAME_INFORMATION FileNameInformation
)
{
   NTSTATUS              NtStatus;
   PFKX_KLOG_RECORD_LIST KRecordList;
   PFKX_KLOG_RECORD      KLogRecord;
   KLOCK_QUEUE_HANDLE    LockHandle;
   ULONG                 SizeRequired;
   UNICODE_STRING        VolumeGuidName;

   DECLARE_STACKSIZEUSED(FKxFltRecordLogWrite,
                         sizeof(NtStatus) +
                         sizeof(KRecordList) +
                         sizeof(KLogRecord) +
                         sizeof(LockHandle) +
                         sizeof(SizeRequired) +
                         sizeof(VolumeGuidName));

   /*
    * Allocate a new record for this file 
    */
   NtStatus = FKxFltRecordAllocate(&KRecordList);

   if ( !NT_SUCCESS(NtStatus) )
   {
      /* Failure */
      return ( NtStatus );
   }

   KLogRecord = &KRecordList->KLogRecord;

   /*
    * Retrive the volume GUID name for the file's volume
    */
   VolumeGuidName.Length        = 0;
   VolumeGuidName.MaximumLength = sizeof(KLogRecord->VolumeGuidName);
   VolumeGuidName.Buffer        = KLogRecord->VolumeGuidName;

   NtStatus = FltGetVolumeGuidName(Volume,
                                   &VolumeGuidName,
                                   &SizeRequired);

   if ( !NT_SUCCESS(NtStatus) )
   {
      RtlCopyMemory(KLogRecord->VolumeGuidName,
                    FKX_UNKNOWN_VOLUMEGUIDNAME,
                    SizeOfVolumeGuidName);
   }

   /*
    * Hang onto the FlgMgr's name cache record
    */
   FltReferenceFileNameInformation(FileNameInformation);
   KLogRecord->FileNameInformation = FileNameInformation;

   /*
    * Setup the remaining members for this record
    */
   KLogRecord->OriginatingProcess = ProcessId;
   KLogRecord->AccessGranted      = AccessMode;

   KeQuerySystemTime(&KLogRecord->TimeOfCapture);
   
   /*
    * Tack the record onto the output list
    */
   KeAcquireSmartSpinLock(&FKxData.KLogRecordListLock, 
                          &LockHandle);

   InsertTailList(&FKxData.KLogRecordList, 
                  &KRecordList->List);
   
   KeReleaseSmartSpinLock(&FKxData.KLogRecordListLock,
                          &LockHandle);

   FKxDbgTrace(FKX_DBGTRACE_INFO, (__FUNCTION__ "!Logged file!Records=%u, Time=%.16I64u, Process=%08lx, Mode=%08lx, Name=%.*ws\n", FKxData.RecordsAllocated, KLogRecord->TimeOfCapture.QuadPart, ProcessId, AccessMode, FileNameInformation->Name.Length / sizeof(WCHAR), FileNameInformation->Name.Buffer));

   /*
    * Trigger the user event, if there is one and we hit the highwater mark
    */
   if ( (NULL != FKxData.LoggerTriggerEvent) && (FKxData.RecordsAllocated >= FKxData.LoggerTriggerCount) )
   {
      FKxNotifyLogger();
   }

   /* Success */
   return ( STATUS_SUCCESS );
}

FKX_FORCEINLINE
VOID
FKxNotifyLogger(
)
{
   PFLT_GENERIC_WORKITEM WorkItem;

   DECLARE_STACKSIZEUSED(FKxNotifyLogger,
                         sizeof(WorkItem));

   if ( !IsFeatureControlOn(FKxFeatureRecordNotification, &FKxData) )
   {
      /* Success */
      return;
   }

   if ( PASSIVE_LEVEL == KeGetCurrentIrql() )
   {
      FKxNotifyLoggerWorkRoutine(NULL,
                                 NULL,
                                 NULL);
   }
   else
   {
      FKxDbgTrace(FKX_DBGTRACE_INFO, (__FUNCTION__ "!Queuing workitem to notify logger due to high IRQL:%u\n", (ULONG)KeGetCurrentIrql()));

      WorkItem = FltAllocateGenericWorkItem();

      if ( NULL != WorkItem )
      {
         FltQueueGenericWorkItem(WorkItem,
                                 FKxData.FilterObject,
                                 &FKxNotifyLoggerWorkRoutine,
                                 DelayedWorkQueue,
                                 NULL);
      }
   }
}

VOID
FLTAPI
FKxNotifyLoggerWorkRoutine(
   IN PFLT_GENERIC_WORKITEM FltWorkItem,
   IN PFLT_FILTER Filter,
   IN PVOID Context
)
{
   /*
      IRQL:             PASSIVE_LEVEL
      THREAD CONTEXT:   SYSTEM
    */
   PKEVENT EventObject;

   DECLARE_STACKSIZEUSED(FKxNotifyLoggerWorkRoutine,
                         sizeof(EventObject));

   UNREFERENCED_PARAMETER(Filter);
   UNREFERENCED_PARAMETER(Context);

   /*
    * So that there is no chance of the event object being reset while this routine is
    * accessing it, we always swap it out then back in when we're done
    */
   EventObject = (PKEVENT)InterlockedExchangePointer((PVOID*)&FKxData.LoggerTriggerEvent,
                                                     NULL);

   if ( NULL != EventObject )
   {
      KeSetEvent(EventObject,
                 IO_NO_INCREMENT,
                 FALSE);

      /*
       * Put the event object back into the global copy. Since we put NULL in when we swapped it out
       * we will assume that if it is not NULL now, then another thread must have changed it so this
       * routine owns the outstanding reference
       */
      if ( NULL != (PKEVENT)InterlockedCompareExchangePointer((PVOID*)&FKxData.LoggerTriggerEvent,
                                                              EventObject,
                                                              NULL) )
      {
         /*
          * This is an outstanding reference because the global one is different. Get rid of it
          */
         ObDereferenceObject(EventObject);
      }
   }

   /*
    * Get rid of the work item now, if there is one
    */
   if ( NULL != FltWorkItem )
   {
      FltFreeGenericWorkItem(FltWorkItem);
   }
}

ULONG
FKxCalculateLogRecordSize(
   __in PFKX_KLOG_RECORD KLogRecord
)
{
   ULONG                      RecordSize;
   PFLT_FILE_NAME_INFORMATION FileNameInformation;

   DECLARE_STACKSIZEUSED(FKxCalculateLogRecordSize,
                         sizeof(RecordSize) +
                         sizeof(FileNameInformation));
   
   ASSERT(NULL != KLogRecord->FileNameInformation);
   FileNameInformation = KLogRecord->FileNameInformation;

   /*
    * The final file name will be formatted as follows
    *    \??\Volume{GUID}\ParentDirectory\FileName.Ext:Stream
    */
   RecordSize  = offsetof(FKX_LOG_RECORD, FileName);
   RecordSize += (ULONG)(sizeof(WCHAR) * wcslen(KLogRecord->VolumeGuidName));
   RecordSize += FileNameInformation->Name.Length;
   RecordSize += sizeof(L'\0');
   RecordSize -= FileNameInformation->Volume.Length;

   return ( RecordSize );
}

NTSTATUS
FKxFltRegisterProcessId(
   __in ULONG ProcessId
)
{
   ULONG Index;

   /*
    * There is no locking done on the entire list to ensure that duplicate entries do not
    * exist. This should be fine for most cases because registration is generally controlled
    * by this driver and the service. On the off chance that it does occur, when the process
    * is later unregistered all duplicates will be removed. This saves having to deal with
    * a lock on the list.
    */
   for ( Index = 0; Index < FKX_RPMAX; Index++ )
   {
      if ( ProcessId == FKxData.ProcessIdList[Index] )
      {
         /* Success */
         return ( STATUS_SUCCESS );
      }
   }

   for ( Index = 0; Index < FKX_RPMAX; Index++ )
   {
      if ( 0 == FKxData.ProcessIdList[Index] )
      {
         _ReadBarrier();

         if ( 0 == InterlockedCompareExchange((LONG volatile*)&FKxData.ProcessIdList[Index],
                                              ProcessId,
                                              0) )
         {
            /* Success */
            return ( STATUS_SUCCESS );
         }
      }
   }

   /* Failure */
   return ( STATUS_NO_MEMORY );
}

VOID
FKxFltUnregisterProcessId(
   __in ULONG ProcessId
)
{
   ULONG Index;
   
   for ( Index = 0; Index < FKX_RPMAX; Index++ )
   {
      if ( ProcessId == FKxData.ProcessIdList[Index] )
      {
         _ReadBarrier();

         InterlockedCompareExchange((LONG volatile*)&FKxData.ProcessIdList[Index],
                                    0,
                                    ProcessId);

         /* Success */
         break;
      }
   }
}

BOOLEAN
FKxFltIsRegisteredProcessId(
   __in ULONG ProcessId
)
{
   ULONG Index;

   for ( Index = 0; Index < FKX_RPMAX; Index++ )
   {
      if ( ProcessId == FKxData.ProcessIdList[Index] )
      {
         return ( TRUE );
      }
   }

   return ( FALSE );
}
