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

/* FragFlt.h
 *    Shared (user/kernel) declarations for FragFlt.sys
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#pragma once

/**********************************************************************

    Filter Services

 **********************************************************************/

typedef enum _FKX_FLTCOMMAND_CODE
{
   FKxCommandRegisterProcess           = 0,
   FKxCommandUnregisterProcess         = 1,

   FKxCommandGetRecordLimit            = 2,
   FKxCommandSetRecordLimit            = 3,
   
   FKxCommandSetTriggerCount           = 4,
   
   FKxCommandGetFeatureControls        = 5,
   FKxCommandSetFeatureControls        = 6,

   FKxCommandQueryLogRecordEntries     = 7
}FKX_FLTCOMMAND_CODE, *PFKX_FLTCOMMAND_CODE;

typedef struct _FKX_FLTCOMMAND_BLOCK
{
   FKX_FLTCOMMAND_CODE  Command;
   union
   {
      ULONG             ProcessId;
      ULONG             RecordLimit;
      ULONG             TriggerCount;
      struct
      {
         ULONG          FeatureMask;
         ULONG          FeatureControls;
      }FeatureInfo;
   };
}FKX_FLTCOMMAND_BLOCK, *PFKX_FLTCOMMAND_BLOCK;

/**
 * FKX_FLTFEATURE_CODE
 *    Runtime feature managment.
 *
 * Remarks
 *    Some of these are enabled/disabled by default and cannot
 *    be changed in the current version.
 */
typedef enum _FKX_FLTFEATURE_CODE
{
   FKxFeatureProcessRegistration          = 0x00000001,
   FKxFeatureRecordLimits                 = 0x00000002,
   FKxFeatureRecordNotification           = 0x00000004,
   FKxFeatureCaptureRead                  = 0x00000008,
   FKxFeatureCaptureWrite                 = 0x00000010,
   FKxFeatureAutoAttachVolumes            = 0x00000020
}FKX_FLTFEATURE_CODE, *PFKX_FLTFEATURE_CODE;

#define FKX_FLTFEATURE_DEFAULT (FKxFeatureRecordLimits|FKxFeatureRecordNotification|FKxFeatureCaptureWrite)

typedef struct _FKX_LOG_RECORD
{
   ULONG          NextEntryOffset;     /* Offset of the next record, in bytes */
   ULONG          OriginatingProcess;  /* ID of user process which created file */
   LARGE_INTEGER  TimeOfCapture;       /* System time the record was logged */
   ULONG          AccessGranted;       /* Access mode granted to the creator */
   WCHAR          FileName[1];         /* The file name post-creation */
}FKX_LOG_RECORD, *PFKX_LOG_RECORD;

/* Volume guid name for unknown volumes */
#define FKX_UNKNOWN_VOLUMEGUIDNAME  L"\\??\\Volume{00000000-0000-0000-0000-000000000000}"
/* Typical size of a log record */
#define FKX_LOG_RECORD_SIZE (sizeof(FKX_LOG_RECORD) + sizeof(FKX_UNKNOWN_VOLUMEGUIDNAME) + (MAX_PATH * sizeof(WCHAR)))

__forceinline
PFKX_LOG_RECORD
FKxGetNextLogRecord(
   __in const FKX_LOG_RECORD* LogRecord
)
{
   return ( LogRecord->NextEntryOffset ? (PFKX_LOG_RECORD)((ULONG_PTR)LogRecord + LogRecord->NextEntryOffset) : NULL );
}

/* AccessMasks used by FragFlt */
#define FKX_READ_ACCESS  (READ_CONTROL|STANDARD_RIGHTS_READ|FILE_READ_DATA|FILE_EXECUTE|FILE_READ_EA|FILE_READ_ATTRIBUTES|GENERIC_READ)
#define FKX_WRITE_ACCESS (STANDARD_RIGHTS_WRITE|FILE_WRITE_DATA|FILE_APPEND_DATA|FILE_WRITE_EA|GENERIC_WRITE)

__inline
BOOLEAN
FKxIsReadAccess(
   __in DWORD AccessMode
)
{
   return ( AccessMode & FKX_READ_ACCESS ? TRUE : FALSE );
}

__inline
BOOLEAN
FKxIsWriteAccess(
   __in DWORD AccessMode
)
{
   return ( AccessMode & FKX_WRITE_ACCESS ? TRUE : FALSE );
}

/**********************************************************************

    Device and Registry info

 **********************************************************************/
#define KEFRAGMINI_FILENAME            "FragFlt.sys"           /* Driver filename */
#define KEFRAGMINI_FILTERNAME          "FragFlt"               /* User-mode filter name */
#define KEFRAGMINI_FILTERPORTNAME      "\\FragFltIoPort"       /* Filter communication port object path */
#define KEFRAGMINI_REG_RECORD_MAX      "MaximumRecords"        /* Max number of records to allocate (REG_DWORD) */
#define KEFRAGMINI_REG_FEATURE_CONTROL "FeatureControls"       /* Any of FKX_FLTFEATURE_CODE (REG_DWORD) */
#define KEFRAGMINI_REG_VOLUME_LIST     "InitialVolumes"        /* Initial list of volumes to be attached (REG_MULTI_SZ) */
#define KEFRAGMINI_REG_DEBUG_FLAGS     "DebugFlags"            /* Debugging flags used in DBG builds (REG_DWORD) */

/* Registry values should be stored in HKEY_LOCAL_MACHINE under the following subkey */
#define KEFRAGMINI_REG_PATH_SUBKEY     "SYSTEM\\CurrentControlSet\\Services\\FragFlt"
