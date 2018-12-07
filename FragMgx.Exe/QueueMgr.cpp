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

/* FileQue.cpp
 *    
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#include "Stdafx.h"
#include "QueueMgr.h"

/**********************************************************************

   CQueueManager : CQueueManager

 **********************************************************************/

CQueueManager::CQueueManager( ) throw()
{
   _hQueueCtx  = NULL;
   _hNotifyCtx = NULL;
}

CQueueManager::~CQueueManager( ) throw()
{
   ATLASSERT(!_hQueueCtx && !_hNotifyCtx);
}

DWORD CQueueManager::Initialize( PFILEQUEUEUPDATEPROC pCallback, PVOID pParameter ) throw()
{
   DWORD    dwRet;
   LONGLONG LastModified;

   ATLASSERT(!_hQueueCtx && !_hNotifyCtx);

   _pCallback  = pCallback;
   _pParameter = pParameter;

   dwRet = FqInitialize(&_hQueueCtx);
   if ( NO_ERROR == dwRet )
   {
      /* Enumerate the queue so the client has the full initial data set to sort on */
      dwRet = FqEnumerateQueue(_hQueueCtx,
                               &CQueueManager::_EnumerateQueueRoutine,
                               this,
                               NULL);

      /* Register for update notifications */
      LastModified = 0;
      dwRet = FqRegisterQueueNotification(_hQueueCtx,
                                          &CQueueManager::_QueueNotificationRoutine,
                                          this,
                                          &LastModified,
                                          &_hNotifyCtx);
   }

   if ( NO_ERROR != dwRet )
   {
      Uninitialize();
   }

   /* Success / Failure */
   return ( dwRet );
}

void CQueueManager::Uninitialize( ) throw()
{
   if ( NULL != _hNotifyCtx )
   {
      FqUnregisterQueueNotification(_hNotifyCtx);
      _hNotifyCtx = NULL;
   }

   if ( NULL != _hQueueCtx )
   {
      FqUninitialize(_hQueueCtx);
      _hQueueCtx = NULL;
   }

   _pCallback  = NULL;
   _pParameter = NULL;
}

DWORD CQueueManager::InsertFile( __in_z LPCWSTR pwszFileName ) throw()
{
   DWORD dwRet;

   dwRet = FqInsertFile(_hQueueCtx,
                        pwszFileName);

   return ( dwRet );
}

DWORD CQueueManager::RemoveFile( __in_z LPCWSTR pwszFileName ) throw()
{
   DWORD dwRet;

   dwRet = FqRemoveFile(_hQueueCtx,
                        pwszFileName);

   return ( dwRet );
}

BOOL QUEUEAPI CQueueManager::_EnumerateQueueRoutine( __in HANDLE /*hQueueCtx*/, __in_z LPCWSTR pwszFileName, __in PLONGLONG pLastModified, __in_opt PVOID pParameter ) throw()
{
   CQueueManager* pFileQue;

   /* Initialize locals.. */
   pFileQue = reinterpret_cast<CQueueManager*>(pParameter);

   pFileQue->_pCallback(pwszFileName,
                        pLastModified,
                        pFileQue->_pParameter);

   /* Continue the enumeration */
   return ( TRUE );                             
}

VOID QUEUEAPI CQueueManager::_QueueNotificationRoutine( __in HANDLE hQueueCtx, __in HANDLE /*hNotifyCtx*/, __in_z LPCWSTR pwszFileName, __in PLONGLONG pLastModified, __in_opt PVOID pParameter ) throw()
{
   _EnumerateQueueRoutine(hQueueCtx,
                          pwszFileName,
                          pLastModified,
                          pParameter);
}
