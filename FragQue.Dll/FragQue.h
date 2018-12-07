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

/* FragQue.h
 *    FragExt file queue management DLL
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**********************************************************************

   Public

 **********************************************************************/

#ifdef _WIN64
   #define QUEUEAPI
#else /* _WIN64 */
   #define QUEUEAPI __stdcall 
#endif /* _WIN64 */

enum
{
   FQ_FILENAME_MAX_CCH  = (sizeof("\\??\\Volume{00000000-0000-0000-0000-000000000000}") + (2 * MAX_PATH) + 36),
   FQ_FILENAME_MAX_CB   = (FQ_FILENAME_MAX_CCH * sizeof(WCHAR))
};

typedef 
BOOL 
(QUEUEAPI* 
PENUMQUEUEPROC)( 
   __in HANDLE     hQueueCtx, 
   __in_z LPCWSTR  pwszFileName, 
   __in PLONGLONG pLastModified, 
   __in_opt PVOID  pParameter 
);

typedef 
VOID 
(QUEUEAPI* 
PQUEUENOTIFICATIONPROC)( 
   __in HANDLE     hQueueCtx, 
   __in HANDLE     hNotifyCtx, 
   __in LPCWSTR    pwszFileName, 
   __in PLONGLONG pLastModified, 
   __in_opt PVOID  pParameter 
);

DWORD 
QUEUEAPI 
FqInitialize( 
   __out PHANDLE phQueueCtx 
);

VOID 
QUEUEAPI 
FqUninitialize( 
   __in HANDLE hQueueCtx 
);

DWORD 
QUEUEAPI 
FqInsertFile( 
   __in HANDLE    hQueueCtx, 
   __in_z LPCWSTR pwszFileName 
);

DWORD 
QUEUEAPI 
FqRemoveFile( 
   __in HANDLE    hQueueCtx, 
   __in_z LPCWSTR pwszFileName 
);

DWORD 
QUEUEAPI 
FqEnumerateQueue( 
   __in HANDLE         hQueueCtx, 
   __in PENUMQUEUEPROC pCallback, 
   __in_opt PVOID      pParameter, 
   __in_opt PLONGLONG  pLastModified
);

DWORD 
QUEUEAPI 
FqRegisterQueueNotification( 
   __in HANDLE                 hQueueCtx, 
   __in PQUEUENOTIFICATIONPROC pCallback, 
   __in_opt PVOID              pParameter, 
   __in_opt PLONGLONG          pLastModified,
   __out PHANDLE               phNotifyCtx
);

VOID 
QUEUEAPI 
FqUnregisterQueueNotification( 
   __in HANDLE hNotifyCtx 
);

VOID
QUEUEAPI
FqGetLastModifiedTime(
   __out PLONGLONG pLastModified
);

#ifdef __cplusplus
}; /* extern "C" */
#endif /* __cplusplus */
