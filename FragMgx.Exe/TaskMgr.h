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

/* TaskMgr.h
 *    Implements the CTaskManager object, which exposes the task queue
 *    and defrag wrappers for CTaskDlg and CDefragManager
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include <List.h>
#include <SpinLock.h>
#include <FragSvx.h>

#define _TX_CREATECRTTHREAD
#include <ThreadUtil.h>

#include <DirUtil.h>

typedef enum _QUEUEUPDATECODE
{
   QC_FILEINSERTED,
   QC_FILEDELETED
}QUEUEUPDATECODE, *PQUEUEUPDATECODE;

typedef enum _DEFRAGUPDATECODE
{
   DC_INITIALIZING,
   DC_DEFRAGMENTING,
   DC_COMPLETED
}DEFRAGUPDATECODE, *PDEFRAGUPDATECODE;

/* Callback used by ITaskManager to post queue updates to the client */
typedef VOID (CALLBACK* PQUEUEUPDATEPROC)( QUEUEUPDATECODE eUpdateCode, __in_z LPCWSTR pwszFileName, __in_opt PVOID pParameter );

/* Callback used by ITaskManager to post defrag updates to the client */
typedef HRESULT (CALLBACK* PDEFRAGUPDATEPROC)( DEFRAGUPDATECODE eUpdateCode, DWORD dwStatus, __in_z LPCWSTR pwszFileName, LONG iPercentComplete, __in_opt PVOID pParameter );

class __declspec(novtable) __declspec(uuid("1B31293C-CE09-41EC-A9D0-42E5ACE26597"))
ITaskManager : public IUnknown
{
public:
   STDMETHOD(Initialize)( ) = 0;
   STDMETHOD(Uninitialize)( ) = 0;

   STDMETHOD(SetQueueUpdateRoutine)( __in_opt PQUEUEUPDATEPROC pCallback, __in_opt PVOID pParameter ) = 0;
   STDMETHOD(SetDefragUpdateRoutine)( __in_opt PDEFRAGUPDATEPROC pCallback, __in_opt PVOID pParameter ) = 0;

   STDMETHOD(InsertQueueFile)( __in_z LPCWSTR pwszFileName ) = 0;
   STDMETHOD(DeleteQueueFile)( __in_z LPCWSTR pwszFileName ) = 0;
   STDMETHOD(FlushQueue)( ) = 0;

   STDMETHOD(Suspend)( DWORD dwTimeout ) = 0;
   STDMETHOD(Resume)( ) = 0;   
};

class ATL_NO_VTABLE CTaskManager : CComObjectRootEx<CComMultiThreadModel>,
                                   public ITaskManager,
                                   public IFSxDefragmentFileCallback
{
   /* ATL */
public:
   DECLARE_NOT_AGGREGATABLE(CTaskManager);

   BEGIN_COM_MAP(CTaskManager)
      COM_INTERFACE_ENTRY(IFSxDefragmentFileCallback)
      COM_INTERFACE_ENTRY(ITaskManager)
   END_COM_MAP()

   HRESULT FinalConstruct( ) throw();
   void FinalRelease( ) throw();

   /* ITaskManager */
public:
   HRESULT STDMETHODCALLTYPE Initialize( ) throw();
   HRESULT STDMETHODCALLTYPE Uninitialize( ) throw();

   HRESULT STDMETHODCALLTYPE SetQueueUpdateRoutine( __in_opt PQUEUEUPDATEPROC pCallback, __in_opt PVOID pParameter ) throw();
   HRESULT STDMETHODCALLTYPE SetDefragUpdateRoutine( __in_opt PDEFRAGUPDATEPROC pCallback, __in_opt PVOID pParameter ) throw();

   HRESULT STDMETHODCALLTYPE InsertQueueFile( __in_z LPCWSTR pwszFileName ) throw();
   HRESULT STDMETHODCALLTYPE DeleteQueueFile( __in_z LPCWSTR pwszFileName ) throw();
   HRESULT STDMETHODCALLTYPE FlushQueue( ) throw();

   HRESULT STDMETHODCALLTYPE Suspend( DWORD dwTimeout ) throw();
   HRESULT STDMETHODCALLTYPE Resume( ) throw();

   /* IFSxDefragmentFileCallback */
public:
   HRESULT STDMETHODCALLTYPE OnDefragmentFileUpdate( __RPC__in_string LPCWSTR pwszFileName, LONG iPercentComplete, LONG_PTR dwParameter ) throw();

   /* CTaskManager */
protected:
   CTaskManager( ) throw();

private:
   typedef struct _QUEUEFILE
   {      
      LIST_ENTRY  Entry;
      USHORT      Flags;
      WCHAR       FileName[1];
   }QUEUEFILE, *PQUEUEFILE;

   enum
   {
      eTraceError       = 1,
      eTraceWarning     = 1,
      eTraceMessage     = 0,

      /* Values for _dwState */
      TMS_UNINITIALIZED = 0,
      TMS_INITIALIZED   = 1,
      TMS_TERMINATING   = 2,
      
      /* Timeout interval for the defrag thread's wait, in miliseconds */
      TM_WAIT_INTERVAL  = 30 * 1000,

      /* Delay between defragmenting files, in miliseconds */
      TM_DELAY_INTERVAL = 18 * 3,

      TM_FILENAME_MAX_CCH = UNICODE_STRING_MAX_CHARS,
      TM_FILENAME_MAX_CB  = TM_FILENAME_MAX_CCH * sizeof(WCHAR),
      TM_FILENAME_MIN_CCH = MAX_PATH,
      TM_FILENAME_MIN_CB  = TM_FILENAME_MIN_CCH * sizeof(WCHAR),

      TM_QUEUEFILE_MIN_CB      = Align<TM_FILENAME_MIN_CB + sizeof(QUEUEFILE)>::Up,
      TM_QUEUEFILE_ISDIRECTORY = 0x0001,
      TM_QUEUEFILE_ISAUTOFILE  = 0x0002
   };

   HANDLE            _hSuspend;
   volatile DWORD    _dwState;
   volatile DWORD    _dwWaitTimeout;
   
   PQUEUEUPDATEPROC  _pQueueCallback;
   PVOID             _pQueueParameter;
   SPINLOCK          _QueueCallbackLock;
   
   PDEFRAGUPDATEPROC _pDefragCallback;
   PVOID             _pDefragParameter;
   SPINLOCK          _DefragCallbackLock;
   
   HANDLE            _hDefragThread;
   HANDLE            _hDefragSignal;
   LIST_ENTRY        _DefragQueue;
   SPINLOCK          _DefragQueueLock;

   volatile LONG     _EnumDirState;
   HANDLE            _hEnumDirThread;
   HANDLE            _hEnumDirSignal;
   LIST_ENTRY        _DirectoryQueue;
   SPINLOCK          _DirectoryLock;
   
   PQUEUEFILE _CreateQueueFile( LPCWSTR pwszFileName, USHORT uFlags ) throw();
   void _DestroyQueueFile( PQUEUEFILE pQueFile ) throw();

   void _InsertDefragQueueFile( PQUEUEFILE pQueFile ) throw();
   void _FlushDefragQueue( bool bPostUpdates ) throw();


   DWORD _InitializeDefragThread( ) throw();
   void _UninitializeDefragThread( ) throw();
   DWORD _DefragThreadHandler( ) throw();
   void _ProcessDefragQueue( ) throw();  

   DWORD _InitializeEnumDirThread( ) throw();
   void _UninitializeEnumDirThread( ) throw();
   DWORD _EnumDirThreadHandler( ) throw();
   void _ProcessDirectoryQueue( ) throw();

   void _DoWaitResume( DWORD dwForceDelay ) throw();

   void _PostQueueUpdate( QUEUEUPDATECODE eUpdateCode, __in_z LPCWSTR pwszFileName ) throw();
   HRESULT _PostDefragUpdate( DEFRAGUPDATECODE eUpdateCode, DWORD dwStatus, __in_z LPCWSTR pwszFileName, LONG iPercentComplete ) throw();

   static DWORD WINAPI _DefragThreadRoutine( PVOID pParameter ) throw();
   static DWORD WINAPI _EnumDirThreadRoutine( PVOID pParameter ) throw();
   static BOOL FRAGAPI _EnumDirectoryCallback( __in PCDIRECTORYENTRY_INFORMATION FileInfo, __in DWORD Depth, __in_opt PVOID Parameter ) throw();
};

/* Single TaskManager instance. This is created in CModule::Run() */
__declspec(selectany) ITaskManager* __pTaskManager = NULL;
