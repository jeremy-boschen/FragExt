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

/* Module.h
 *    CLibraryModule definitions
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include <MUIModule.h>

#include "FragMgx.h"
#include "TaskDlg.h"

/**********************************************************************

    

 **********************************************************************/

class CLibraryModule : public CAtlExeModuleT<CLibraryModule>,
                       public CMUIModule
{
   /* CAtlExeModuleT<> */
public:
   typedef CLibraryModule                 _LibraryModule;
   typedef CAtlExeModuleT<CLibraryModule> _AtlExeModuleT;

   /* ATL */
public:   
#ifdef _DEBUG
	DECLARE_LIBID(LIBID_FragMgx)
   DECLARE_REGISTRY_APPID_RESOURCEID(IDR_FRAGMGX, "{5B2A3C76-4C9F-4ABF-8E50-D806008C0B47}")
#else /* _DEBUG */
   DECLARE_NO_REGISTRY()
#endif /* _DEBUG */

   /* CLibraryModule */
public:
   CLibraryModule( ) throw();

   static HRESULT InitializeCom( ) throw();
   HRESULT Run( int nShowCmd = SW_HIDE ) throw();   
   HRESULT PreMessageLoop(int nShowCmd ) throw();
   void RunMessageLoop( ) throw();
   HRESULT PostMessageLoop( ) throw();

private:
   HRESULT _CreateUserInstanceLock( ) throw();
   void _CloseUserInstanceLock( ) throw();

   CTaskDlg _TaskDlg;
   HANDLE   _hInstanceFile;
};

__declspec(selectany) CLibraryModule _AtlModule;
