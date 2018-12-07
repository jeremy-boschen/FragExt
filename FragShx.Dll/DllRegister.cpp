/*	FragExt - Shell extension for providing file fragmentation
 *	information.
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

/*	DllRegister.cpp
 *	   Implementation of COM registration exports
 *
 *	Copyright (C) 2004-2009 Jeremy Boschen
 *
 *	Version History
 *		0.0.001 - 07/13/2004 - Created
 */

#include "Stdafx.h"

#ifdef _DEBUG
/**
 *	DllRegisterServer
 *    Standard COM export function.
 *
 * This is built only in DEBUG versions to aid in registration
 */
STDAPI DllRegisterServer( ) throw()
{
   #pragma comment(linker, "/export:" __FUNCTION__ "=" __FUNCDNAME__ ",PRIVATE")

   HRESULT hr;

   hr = _AtlModule.DllRegisterServer(FALSE);
   if ( SUCCEEDED(hr) )
   {
      SHChangeNotify(SHCNE_ASSOCCHANGED,
                     SHCNF_IDLIST,
                     NULL,
                     NULL);
   }

   /* Success / Failure */
   return ( hr );
}
#endif /* _DEBUG */

#ifdef _DEBUG
/**
 *	DllUnregisterServer
 *    Standard COM export function.
 *
 * This is built only in DEBUG versions to aid in registration
 */
STDAPI DllUnregisterServer( ) throw()
{
   #pragma comment(linker, "/export:" __FUNCTION__ "=" __FUNCDNAME__ ",PRIVATE")

   HRESULT hr;

   hr = _AtlModule.DllUnregisterServer(FALSE);
   if ( SUCCEEDED(hr) )
   {
      SHChangeNotify(SHCNE_ASSOCCHANGED,
                     SHCNF_IDLIST,
                     NULL,
                     NULL);
   }

   /* Success / Failure */
   return ( hr );
}
#endif /* _DEBUG */