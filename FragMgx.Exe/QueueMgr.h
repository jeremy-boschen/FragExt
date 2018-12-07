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

/* QueueManager.h
 *    
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#pragma once

#include <FragQue.h>

#include <DbgUtil.h>

#include <List.h>
#include <RWLock.h>
#include <SpinLock.h>
#include <PathUtil.h>

typedef VOID (CALLBACK* PFILEQUEUEUPDATEPROC)( __in_z LPCWSTR pwszFileName, __in PLONGLONG pLastModified, __in_opt PVOID pParameter );

class CQueueManager
{
public:
   CQueueManager( ) throw();
   ~CQueueManager( ) throw();

   DWORD Initialize( __in PFILEQUEUEUPDATEPROC pCallback, __in_opt PVOID pParameter ) throw();
   VOID  Uninitialize( ) throw();

   DWORD InsertFile( __in_z LPCWSTR pwszFileName ) throw();
   DWORD RemoveFile( __in_z LPCWSTR pwszFileName ) throw();

private:
   HANDLE               _hQueueCtx;
   HANDLE               _hNotifyCtx;
   PFILEQUEUEUPDATEPROC _pCallback;
   PVOID                _pParameter;

   static BOOL QUEUEAPI _EnumerateQueueRoutine( __in HANDLE hQueueCtx, __in_z LPCWSTR pwszFileName, __in PLONGLONG pLastModified, __in_opt PVOID pParameter ) throw();
   static VOID QUEUEAPI _QueueNotificationRoutine( __in HANDLE hQueueCtx, __in HANDLE hNotifyCtx, __in_z LPCWSTR pwszFileName, __in PLONGLONG pLastModified, __in_opt PVOID pParameter ) throw(); 
};
