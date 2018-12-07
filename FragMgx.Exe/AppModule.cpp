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

/* AppModule.pp
 *    
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

/*++
   WPP
 *--*/
#include "AppModule.tmh"

// x32 == 13 bytes
// x64 == 22 bytes


#if defined(_M_IX86)

#pragma pack(push, 1)

struct WndProcAdapter
{
   DWORD Mov;
   DWORD This;
   BYTE  Jmp;
   DWORD RelativeAddress;

   void
   Initialize(
      __in WNDPROC
      __in void* pThis
   )
   {
#if 0
      m_mov = 0x042444C7;  //C7 44 24 0C
		m_this = PtrToUlong(pThis);
		m_jmp = 0xe9;
		m_relproc = DWORD((INT_PTR)proc - ((INT_PTR)this+sizeof(_stdcallthunk)));
		// write block from data cache and
		//  flush from instruction cache
		FlushInstructionCache(GetCurrentProcess(), this, sizeof(_stdcallthunk));
#endif //0
      Mov  = 0x042444C7;
      This = (DWORD)(ULONG_PTR)pThis;
      Jmp  = 0xE9;
      RelativeAddress = (DWORD)((INT_PTR));
   }

   static
   LRESULT
   CALLBACK
   WndProc(
      __in HWND hWnd,
      __in UINT Msg,
      __in WPARAM wParam,
      __in LPARAM lParam
   )
   {

   }      
};

#pragma pack(pop)

#elif defined(_M_AMD64)

#pragma pack(push, 2)

struct WndProcAdapter
{
};

#pragma pack(pop)

#endif

#if 0
/**********************************************************************

	CLibraryModule : CLibraryModule

 **********************************************************************/

#ifdef _DEBUG
void _dbgQueueFilesInApplicationDirectory( UINT iIter )
{
   WCHAR           chPath[MAX_PATH];
   HANDLE          hFind;
   WIN32_FIND_DATA FindData;

   while ( iIter-- )
   {
      GetModuleFileName(NULL,
                        chPath,
                        _countof(chPath));

      PathReplaceFileSpec(chPath,
                          _countof(chPath),
                          L"*");

      hFind = FindFirstFile(chPath,
                            &FindData);

      if ( INVALID_HANDLE_VALUE != hFind )
      {
         do
         {
            if ( !PathIsDotDirectory(FindData.cFileName) )
            {
               PathReplaceFileSpec(chPath,
                                   _countof(chPath),
                                   FindData.cFileName);

               __pTaskManager->InsertQueueFile(chPath);
            }
         }
         while ( FindNextFile(hFind,
                              &FindData) );

         FindClose(hFind);
      }
   }
}
#endif /* _DEBUG */

CLibraryModule::CLibraryModule( ) throw() : CAtlExeModuleT<CLibraryModule>(), CMUIModule()
{
   _hInstanceFile = INVALID_HANDLE_VALUE;
}

HRESULT CLibraryModule::InitializeCom() throw()
{
   HRESULT hr;

   hr = _AtlExeModuleT::InitializeCom();
   if ( SUCCEEDED(hr) )
   {
      hr = CoInitializeSecurity(NULL,
                                -1, 
                                NULL, 
                                NULL, 
                                RPC_C_AUTHN_LEVEL_PKT, 
                                RPC_C_IMP_LEVEL_IMPERSONATE,
                                NULL, 
                                EOAC_STATIC_CLOAKING|EOAC_DISABLE_AAA|EOAC_NO_CUSTOM_MARSHAL, 
                                NULL);
   }

   /* Success/Failure */
   return ( hr );
}

HRESULT CLibraryModule::Run( int nShowCmd ) throw()
{
   HRESULT  hr;
   DWORD    dwRet;
   HWND     hWnd;
   
   /* Initialize locals */
   hr = E_FAIL;
   
   /* Load up the MUI module and overwrite the resource instance used by the rest
    * of the application */
   dwRet = LoadMUIModule();
   if ( NO_ERROR != dwRet )
   {
      hr = __HRESULT_FROM_WIN32(dwRet);
      /* Failure */
      goto __CLEANUP;
   }

   _AtlBaseModule.SetResourceInstance(GetMUIInstance());

   /* Ensure that this is the only instance running for the current user */
   hr = _CreateUserInstanceLock();

   if ( (__HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr) || (__HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) == hr) )
   {
      /* There appears to be another instance of the application running that already
       * owns the instance lock, so show it. The likely case for this is that a user
       * launched us after an instance was already started. Another possible case is
       * that COM is launching us again to handle an activation when another instance
       * is shutting down. */
      hWnd = FindWindow(_T(FRAGMGXDLGCLS),
                        NULL);

      //TODO:Consider waiting for a a few seconds and trying to acquire the lock file if NULL == hWnd

      if ( hWnd )
      {
         //TODO:Change this to a custom window message that will cause the window to foreground itself
         ShowWindow(hWnd,
                    SW_SHOWNORMAL);

         SetForegroundWindow(hWnd);
      }

      /* Nothing else to do, so bail out */
      goto __CLEANUP;
   }

   /* Kick up the TaskManager.. */
   hr = ATLEx::CreateObjectInstance<CTaskManager>(__uuidof(ITaskManager),
                                                  reinterpret_cast<void**>(&__pTaskManager),
                                                  NULL);

   if ( SUCCEEDED(hr) )
   {
      hr = __pTaskManager->Initialize();
   }

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Try to create the TaskDlg. We have to do this before PreMessageLoop() runs because it will
    * register the class object with COM, and without the TaskDlg, client calls could be lost */
   if ( _TaskDlg.Create(NULL,
                                0) )
   {
      /* Try to start up the application.. */
      hr = PreMessageLoop(nShowCmd);
      
      /* We only pump messages if PreMessageLoop returns S_OK */
      if ( S_OK == hr )
      {         
         /* Everything must be up and running, so show the dialog and start pumping messages */
         _TaskDlg.ShowWindow(SW_SHOWNORMAL);

#ifdef _DEBUGX
         /* Add some files to the file queue.. */
         _dbgQueueFilesInApplicationDirectory(1);
#endif /* _DEBUG */
      
         RunMessageLoop();
      }

      if ( SUCCEEDED(hr) )
      {
         hr = PostMessageLoop();
      }
   }

__CLEANUP:
   /* By this point, the dialog should be destroyed */
   ATLASSERT(!::IsWindow(_TaskDlg));

   _CloseUserInstanceLock();

   if ( __pTaskManager )
   {
      __pTaskManager->Uninitialize();
      __pTaskManager->Release();
   }

   FreeMUIModule();

   /* Success / Failure */
   return ( hr );
}

HRESULT CLibraryModule::PreMessageLoop( int nShowCmd ) throw()
{
   HRESULT hr;

   hr = _AtlExeModuleT::PreMessageLoop(nShowCmd);
 
   /* Success / Failure */
   return ( hr );
}

void CLibraryModule::RunMessageLoop( ) throw()
{
   _TaskDlg.RunMessageLoop();
}

HRESULT CLibraryModule::PostMessageLoop( ) throw()
{
   HRESULT hr;

   /* Close the file used as the instance lock if it was opened */
   hr = _AtlExeModuleT::PostMessageLoop();

   /* Success / Failure */
   return ( hr );
}

HRESULT CLibraryModule::_CreateUserInstanceLock( ) throw()
{
   HRESULT hr;
   DWORD   dwRet;
   DWORD   dwFlagsAndAttributes;
   WCHAR   chPath[MAX_PATH];
   LPCWSTR rgSubDir[] = {L"jBoschen", L"FragExt"};
   int     idx;

   /* The instance lock is a file in the launching user's settings directory
    * that has DELETE_ON_CLOSE and exlusive access. It is kept open while the
    * application is running so that the user can not run another instance */
   hr = SHGetFolderPath(NULL,
                        CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,
                        NULL,
                        SHGFP_TYPE_CURRENT,
                        chPath);

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   /* Build up the path for our application */
   for ( idx = 0; idx < _countof(rgSubDir); idx++ )
   {
      if ( !PathAppend(chPath,
                       _countof(chPath),
                       rgSubDir[idx]) )
      {
         hr = E_FAIL;
         /* Failure */
         break;
      }

      if ( !CreateDirectory(chPath,
                            NULL) )
      {
         dwRet = GetLastError();
         if ( ERROR_ALREADY_EXISTS != dwRet )
         {
            hr = __HRESULT_FROM_WIN32(dwRet);
            /* Failure */
            break;
         }
      }
   }

   if ( FAILED(hr) )
   {
      /* Failure */
      goto __CLEANUP;
   }

   if ( !PathAppend(chPath,
                    _countof(chPath),
                    L"FragMgx.loc") )
   {
      hr = E_FAIL;
      /* Failure */
      goto __CLEANUP;
   }

   dwFlagsAndAttributes  = FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE|FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH;
#ifndef _DEBUG
   dwFlagsAndAttributes |= FILE_ATTRIBUTE_HIDDEN;
#endif /* _DEBUG */

   _hInstanceFile = CreateFile(chPath,
                               GENERIC_READ,
                               0,
                               NULL,
                               OPEN_ALWAYS,
                               dwFlagsAndAttributes,
                               NULL);

   if ( INVALID_HANDLE_VALUE == _hInstanceFile )
   {
      dwRet = GetLastError();
      /* Failure */
      hr = __HRESULT_FROM_WIN32(dwRet);
   }
   else
   {
      /* Success */
      hr = S_OK;
   }

__CLEANUP:
   if ( FAILED(hr) )
   {
      /* Force a closure of the lock if anything failed */
      _CloseUserInstanceLock();
   }

   /* Success / Failure */
   return ( hr );
}

void CLibraryModule::_CloseUserInstanceLock( )
{
   if ( INVALID_HANDLE_VALUE != _hInstanceFile )
   {
      CloseHandle(_hInstanceFile);
      _hInstanceFile = INVALID_HANDLE_VALUE;
   }
}
#endif //0