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
 
/* DiskDefrag.h
 *    CFSxDefragmenter declarations
 */

#pragma once

#include "FragEng.h"
#include "COMLib.h"
#include "FragSvx.h"

class __declspec(novtable) 
CFSxDefragmenter : public CCoUnknownT<CCoMultiThreadModel>,
                   public CCoClassT< CCoHeapObjectT<CFSxDefragmenter>, CCoExeServerLockModel >,
                   public IFSxFileDefragmenter
{
   /* CCoUnknownT */
public:
   BEGIN_QI_MAP(CFSxDefragmenter)
      QI_ENTRY(IFSxFileDefragmenter)
   END_QI_MAP()

   /* Overrides */
   HRESULT 
   InternalConstruct( 
   ) throw();
   
   void 
   InternalDestruct( 
   ) throw();
   
   /* IFSxFileDefragmenter */
public:
   STDMETHODIMP 
   DefragmentFile(
      __RPC__in_string LPCWSTR FileName, 
      __RPC__in ULONG Flags,
      __RPC__in ULONG_PTR CallbackParameter 
   );

   STDMETHODIMP
   SetCallbackInterface(
      __RPC__in_opt IUnknown* Callback
   );

   /* CFSxDefragmenter */

protected:
   CFSxDefragmenter(
   ) throw();

private:
   /*++
      TYPES
    --*/
   enum : unsigned long {
      /* 
       * Time used for the callback delay, in nanoseconds 
       */
      ONE_MICROSECOND               = 10,
      ONE_MILISECOND                = ONE_MICROSECOND * 1000,
      ONE_SECOND                    = ONE_MILISECOND * 1000,
      CALLBACK_DELAY                = ONE_SECOND * 1,

      FILEPATH_MAX_CCH              = sizeof("\\??\\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}\\") + MAX_PATH + 36,
      FILEPATH_MAX_CB               = sizeof(WCHAR) * FILEPATH_MAX_CCH,

      CALLCANCELLATION_TIMEOUT      = 5
   };

   /*
    * DEFRAGCTX
    *    Tracks defrag state during defragmentation, separate from the class object
    */
   typedef struct _DEFRAGCTX {
      /* 
       * This is to store a result code for DefragmentFileRoutine() so that there is a clear separation 
       * of failure by it and failure by FpDefragmentFile() 
       */
      HRESULT                       Result;

      /*
       * State flags, unused
       */
      USHORT                        State;

      /* 
       * Current percentage of the file that has been defragmented, from 0 - 100 
       */      
      USHORT                        Percent;

      /* 
       * The defrag API exposed by FragEng posts very frequent updates which are overkill for this component,
       * so the percentage complete and a timer is used to either post an update when the percentage changes
       * or when the timer has expired. The timer interval is determined by the CALLBACK_DELAY value. This
       * field tracks the last time an update was posted to the client
       */
      ULONGLONG                     CallTime;

      /* 
       * Client's original CallbackParameter 
       */
      ULONG_PTR                     Parameter;

      /* 
       * Client's original pwszFileName 
       */
      LPCWSTR                       FileName;
      
      /* 
       * Proxy callback sink
       */
      IFSxDefragmentFileCallback*   Callback;
   }DEFRAGCTX, *PDEFRAGCTX;
   
   /*++
      FUNCTION PROTOTYPES
    --*/
   HRESULT
   InternalDefragmentFile(
      __RPC__in_string LPCWSTR FileName, 
      __RPC__in ULONG Flags,
      __RPC__in ULONG_PTR Parameter 
   );

   HRESULT
   InternalSetCallbackInterface(
      __RPC__in_opt IUnknown* Callback
   );

   void
   InternalGetCallbackInterface(
      __deref_out IFSxDefragmentFileCallback** Callback
   );

   static
   HRESULT
   CloakClientCallbackInterface(
      __in IUnknown* CallbackProxy,
      __deref_out IFSxDefragmentFileCallback** Callback
   );

   static
   HRESULT
   OpenFileAsClient(
      __in_z LPCWSTR FileName,
      __deref_out PHANDLE FileHandle
   );

   static
   DWORD
   GetFileFragmentCount(
      __in HANDLE FileHandle,
      __deref_out ULONG* FragmentCount
   );

   HRESULT
   SendDefragmentFileUpdate(
      __in_z LPCWSTR FileName,
      __in ULONG PercentComplete,
      __in ULONG_PTR CallbackParameter
   );

   static
   HRESULT
   SendDefragmentFileUpdateEx(
      __in IFSxDefragmentFileCallback* Callback,
      __in_z LPCWSTR FileName,
      __in ULONG PercentComplete,
      __in ULONG_PTR CallbackParameter
   );

   static 
   DWORD 
   FRAGAPI 
   DefragmentFileRoutine( 
      __in ULONGLONG ClustersTotal, 
      __in ULONGLONG ClustersMoved, 
      __in_opt PVOID Parameter 
   );

   /*++
      CLASS DATA
    --*/
   CRITICAL_SECTION            _cxDataLock;
   IFSxDefragmentFileCallback* _pDefragmentFileCallback;
};
