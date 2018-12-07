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

/* MUIModule.h
 *    MUI resource loader
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __MUIMODULE_H__
#define __MUIMODULE_H__

#include "MUIload.h"
#pragma comment(lib, "MUIload")

class CMUIModule
{
public:
   CMUIModule( ) throw() : _hMUIModule(NULL)
   {
   }

   HINSTANCE GetMUIInstance( ) const throw()
   {
      return ( _hMUIModule );
   }

   DWORD LoadMUIModule( ) throw() 
   {
      DWORD   dwRet;
      TCHAR   chPath[MAX_PATH];
      HMODULE hMUIModule;

      dwRet = GetModuleFileName(_AtlBaseModule.GetModuleInstance(),
                                chPath,
                                _countof(chPath));

      if ( 0 == dwRet )
      {
         dwRet = GetLastError();
         /* Failure */
         return ( dwRet );
      }
      else if ( _countof(chPath) == dwRet )
      {
         dwRet = GetLastError();
         if ( ERROR_INSUFFICIENT_BUFFER == dwRet )
         {
            /* Failure */
            return ( ERROR_INSUFFICIENT_BUFFER );
         }
      }

      hMUIModule = ::LoadMUILibrary(chPath,
                                    MUI_LANGUAGE_NAME,
                                    0);

      if ( !hMUIModule )
      {
         dwRet = GetLastError();
         /* Failure */
         return ( dwRet );
      }

      if ( InterlockedCompareExchangePointer(reinterpret_cast<volatile PVOID*>(&_hMUIModule),
                                                     hMUIModule,
                                                     NULL) )
      {
         /* Another thread beat us to the assignment, so we need to free this handle */
         ::FreeMUILibrary(hMUIModule);
      }

      /* Success */
      return ( NO_ERROR );
   }

   void FreeMUIModule( ) throw()
   {
      HMODULE hMUIModule;

      hMUIModule = reinterpret_cast<HMODULE>(InterlockedExchangePointer(reinterpret_cast<PVOID*>(&_hMUIModule),
                                                                        NULL));

      if ( hMUIModule )
      {
         ::FreeMUILibrary(hMUIModule);
      }
   }

protected:
   HMODULE _hMUIModule;
};

#endif /* __MUIMODULE_H__ */