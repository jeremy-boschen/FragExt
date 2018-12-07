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

/* FragMgx.h
 *    Export helpers for launching FragMgx.exe
 *
 * Copyright (C) 2008 Jeremy Boschen
 */

#pragma once

#include <PathUtil.h>

/**********************************************************************

    

 **********************************************************************/

/**
 * LaunchDefragManager
 *    Starts the FragExt defrag manager application.
 *
 * Parameters:
 *    [in] hInstance
 *       Module instance of a well known library that exists in the
 *       same directory as FRAGMGX.EXE
 */
inline DWORD LaunchDefragManager( HINSTANCE hInstance ) throw()
{
   DWORD             dwRet;
   WCHAR             chPath[MAX_PATH];
   WCHAR             chDirectory[MAX_PATH];
   SHELLEXECUTEINFO  ExecuteInfo;
   BOOL              bRet;

   /* Initialize locals.. */
   ZeroMemory(chPath,
              sizeof(chPath));

   ZeroMemory(chDirectory,
              sizeof(chDirectory));

   if ( 0 == GetModuleFileName(hInstance,
                               chPath,
                               _countof(chPath)) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   dwRet = NO_ERROR;

   _PathRemoveFileSpec(chPath);

   StringCchCopy(chDirectory,
                 _countof(chDirectory),
                 chPath);
   
   if ( !_PathAppend(chPath,
                     _countof(chPath),
                     L"FRAGMGX.EXE") )
   {
      /* Failure */
      return ( ERROR_INSUFFICIENT_BUFFER );
   }

   ZeroMemory(&ExecuteInfo,
              sizeof(SHELLEXECUTEINFO));

   ExecuteInfo.cbSize      = sizeof(SHELLEXECUTEINFO);
   ExecuteInfo.fMask       = SEE_MASK_UNICODE;
   ExecuteInfo.lpFile      = chPath;
   ExecuteInfo.nShow       = SW_SHOWNORMAL;
   ExecuteInfo.lpDirectory = chDirectory;

   bRet = ShellExecuteEx(&ExecuteInfo);
   if ( !bRet )
   {
      dwRet = GetLastError();
   }

   /* Success / Failure */
   return ( dwRet );
}
