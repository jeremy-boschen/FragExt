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
 
/* Module.cpp
 *    CLibraryModule implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"

/**********************************************************************

   CLibraryModule : ATL

 **********************************************************************/

BOOL WINAPI CLibraryModule::DllMain( DWORD dwReason, LPVOID lpReserved ) throw()
{
   return ( _AtlDllModuleT::DllMain(dwReason,
                                    lpReserved) );
}

HRESULT CLibraryModule::DllCanUnloadNow() throw()
{
   HRESULT hr;

   /* We want to clean up the MUI library if it was loaded, but being
    * a DLL we can't do that in DllMain() because of the loader lock,
    * so if it looks safe to unload then we'll do that here */
   hr = _AtlDllModuleT::DllCanUnloadNow();

   if ( S_OK == hr )
   {
      FreeMUIModule();
   }

   /* Success */
   return ( hr );
}

HINSTANCE CLibraryModule::GetMUIInstance( ) const throw()
{
   if ( !_hMUIModule )
   {
      const_cast<CLibraryModule*>(this)->LoadMUIModule();
   }

   return ( _MuiModule::GetMUIInstance() );
}
