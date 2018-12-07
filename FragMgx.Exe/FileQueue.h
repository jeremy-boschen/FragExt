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
 
/* FileQueue.h
 *    IMgxFileQueue and CMgxFileQueue definitions
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#pragma once

#include <ObjBase.h>
#include "COMLib.h"

/* 
 * Callback used by CQueueController to post file queue updates to its client
 */
typedef enum _FQUPDATECODE
{
   FileQueued     = 0x00000001,
   FileDequeued   = 0x00000002
}FQUPDATECODE, *PFQUPDATECODE;

typedef 
void
(CALLBACK* PFQ_UPDATE_ROUTINE)( 
   __in FQUPDATECODE UpdateCode, 
   __in_z LPCWSTR FileName, 
   __in_opt PVOID Parameter 
);

class
DECLSPEC_NOVTABLE DECLSPEC_UUID("DBC917E8-3717-44FB-92D4-5D03E5310847")
IMgxFileQueueMonitor : public IUnknown
{
public:
   STDMETHOD(OnQueueUpdate)(
      __in FQUPDATECODE UpdateCode,
      __in_z LPCWSTR FileName
   ) = 0;
};

class
DECLSPEC_NOVTABLE DECLSPEC_UUID("DA654E18-F7E3-4429-BDD8-0C11E6C9AAD4")
IMgxFileQueue : public IUnknown
{
public:
   STDMETHOD(QueueFile)( 
      __in_z LPCWSTR FileName
   ) = 0;

   STDMETHOD(DequeueFile)(
      __out_ecount_z(FileNameLength) LPCWSTR FileName,
      __in size_t FileNameLength
   ) = 0;

   STDMETHOD(SetMonitorInterface)(
      __in ULONG Flags,
      __in_opt IMgxFileQueueMonitor* Monitor
   ) = 0;
};

class DECLSPEC_NOVTABLE
CMgxFileQueue : public CCoUnknownT<CCoNoLockModel>,
                public IMgxFileQueue
{
   /* CCoUnknownT */
public:
   BEGIN_QI_MAP(CMgxFileQueue)
      QI_ENTRY(IMgxFileQueue)
   END_QI_MAP()

   /* Overrides */
   HRESULT 
   InternalConstruct( 
   ) throw();
   
   void 
   InternalDestruct( 
   ) throw();
   
   /* IMgxFileQueue */
public:
   STDMETHODIMP
	QueueFile( 
      __in_z LPCWSTR FileName
   );

   STDMETHODIMP
	DequeueFile( 
      __out_ecount_z(FileNameLength) LPCWSTR FileName,
      __in size_t FileNameLength
   );

/* CMgxFileQueue */
public:
   static
   HRESULT
   CreateFileQueue(
      __in PFQ_UPDATE_ROUTINE UpdateRoutine, 
      __in_opt PVOID UpdateRoutineParameter 
   );

   static
   HRESULT
   DestroyFileQueue(
   );

private:
   CMgxFileQueue(
   ) throw();

   /*++
      TYPES
    --*/
   typedef struct _QUEUEFILE
   {      
      LIST_ENTRY  Entry;
      USHORT      Flags;
      WCHAR       FileName[1];
   }QUEUEFILE, *PQUEUEFILE;

   enum : unsigned long {
      FQ_FILENAME_MAX_CCH        = UNICODE_STRING_MAX_CHARS,
      FQ_FILENAME_MAX_CB         = FQ_FILENAME_MAX_CCH * sizeof(WCHAR),
      FQ_FILENAME_MIN_CCH        = 64,
      FQ_FILENAME_MIN_CB         = FQ_FILENAME_MIN_CCH * sizeof(WCHAR),

      FQ_QUEUEFILE_MIN_CB        = Align<FQ_FILENAME_MIN_CB + sizeof(QUEUEFILE)>::Up,
      FQ_QUEUEFILE_ISDIRECTORY   = 0x0001,
      FQ_QUEUEFILE_ISAUTOFILE    = 0x0002
   };

   /*++
      FUNCTION PROTOTYPES
    --*/
   static
   PQUEUEFILE 
   CreateQueueFileRecord( 
      __in_z LPCWSTR FileName, 
      __in USHORT Flags 
   );

   static
   void
   DestroyQueueFileRecord(
      __in PQUEUEFILE QueueFile
   );

   /*++
      CLASS DATA
    --*/
   CRITICAL_SECTION     _cxDataLock;
   LIST_ENTRY           _
   PFQ_UPDATE_ROUTINE   _QueueUpdateRouteine;
};
