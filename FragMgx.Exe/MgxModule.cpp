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
 
/* MgxModule.cpp
 *    Controls the windows message pump
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#include "Stdafx.h"
#include "MgxModule.h"

#include <COMLib.h>
#include <PathUtil.h>

/*++
   WPP
 *--*/
#include "MgxModule.tmh"

// initialize COM
// create singleton lock file
// initialize MUI

DWORD
MgxCreateUserInstanceLock(
   __out HANDLE* InstanceLock
);

void
MgxCloseUserInstanceLock(
   __in HANDLE InstanceLock
);

void
MgxShowRunningInstance(
);

DWORD
APIENTRY
MgxModuleMain(
)
{
   DWORD    dwRet;

   HANDLE   hInstanceLock;

   BOOLEAN  bCOMInitialized;
   BOOLEAN  bBackgroundPriority;

   /* Initialize locals */
   dwRet               = NO_ERROR;
   hInstanceLock       = INVALID_HANDLE_VALUE;
   bCOMInitialized     = FALSE;
   bBackgroundPriority = FALSE;

   /*
    * Attempt to grab the instance lock so we're the only one executing
    */
   dwRet = MgxCreateUserInstanceLock(&hInstanceLock);
   if ( WINERROR(dwRet) ) {
      if ( (ERROR_ALREADY_EXISTS == dwRet) || (ERROR_SHARING_VIOLATION == dwRet) ) {
         /* 
          * There appears to be another instance of the application running that already
          * owns the instance lock, so show it. The only case for this is that a user
          * launched us after an instance was already started. We'll try to notify the
          * already running instance for this case.
          *
          * It is not possible for COM to activate this application again while the lock
          * file is held because the lock file is released right before active objects
          * are revoked.
          */
         MgxShowRunningInstance();
      }

      /* Failure */
      goto __CLEANUP;
   }

   /*
    * This is the singleton instance so get everything up and running now. We'll start with
    * the TaskManager because it must exist before COM activations start coming in
    */



__CLEANUP:
   if ( INVALID_HANDLE_VALUE != hInstanceLock ) {
      MgxCloseUserInstanceLock(hInstanceLock);
   }

   return ( dwRet );
}

DWORD
MgxCreateUserInstanceLock(
   __out HANDLE* InstanceLock
)
{
   DWORD    dwRet;

   HRESULT  hr; 
   WCHAR    chLockFilePath[MAX_PATH*2];
   
   int      idx;
   LPCWSTR  rgSubDirectory[] = {L"jBoschen", L"FragExt"};
   
   DWORD    dwFlagsAndAttributes;
   HANDLE   hLockFile;
   
   /* Initialize outputs */
   (*InstanceLock) = INVALID_HANDLE_VALUE;

   /* Initialize locals */
   dwRet     = NO_ERROR;
   hLockFile = INVALID_HANDLE_VALUE;

   ZeroMemory(chLockFilePath,
              sizeof(chLockFilePath));

   /* 
    * The instance lock is a file in the launching user's settings directory
    * that has DELETE_ON_CLOSE and exlusive access. It is kept open while the
    * application is running so that the user cannot run another instance 
    */
   hr = SHGetFolderPath(NULL,
                        CSIDL_FLAG_CREATE|CSIDL_LOCAL_APPDATA,
                        NULL,
                        SHGFP_TYPE_CURRENT,
                        chLockFilePath);

   if ( FAILED(hr) ) {
      dwRet = (DWORD)hr;
      /* Failure */
      goto __CLEANUP;
   }

   if ( !PathCchAppend(chLockFilePath,
                       ARRAYSIZE(chLockFilePath),
                       L"jBoschen\\FragExt\\FragMgx.Lock") ) {
      dwRet = ERROR_BAD_PATHNAME;
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Ensure the directory for the lock file exists. This function will skip the final
    * file name and create all the preceding directories
    */
   if ( !PathCchCreateDirectory(chLockFilePath,
                                NULL) ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }
   
   /*
    * Attempt to create the file. If another instance already has it open, then that instance
    * is the default one and this one will close
    */
   dwFlagsAndAttributes  = FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE|FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH;
#ifndef _DEBUG
   dwFlagsAndAttributes |= FILE_ATTRIBUTE_HIDDEN;
#endif /* _DEBUG */

   hLockFile = CreateFile(chLockFilePath,
                          GENERIC_READ,
                          0,
                          NULL,
                          OPEN_ALWAYS,
                          dwFlagsAndAttributes,
                          NULL);

   if ( INVALID_HANDLE_VALUE == hLockFile ) {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Give the file handle to the caller
    */
   (*InstanceLock) = hLockFile;
   
   /*
    * Clear the local alias so the cleanup code doesn't close it
    */
   hLockFile = INVALID_HANDLE_VALUE;

   /* Success */
   dwRet = NO_ERROR;

__CLEANUP:
   if ( INVALID_HANDLE_VALUE != hLockFile ) {
      CloseHandle(hLockFile);
   }

   /* Success / Failure */
   return ( dwRet );
}

void
MgxCloseUserInstanceLock(
   __in HANDLE InstanceLock
)
{
   _ASSERTE(INVALID_HANDLE_VALUE != InstanceLock);
   CloseHandle(InstanceLock);
}

void
MgxShowRunningInstance(
)
{
}
