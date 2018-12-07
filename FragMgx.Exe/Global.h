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
 
/* Global.h
 *    Global program declarations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

/*++ ++

Shared Global Declarations

 -- --*/

/*
 * Generic Win32 failure macro for functions that return NO_ERROR,
 * ERROR_SUCCESS or SEC_E_OK on success
 */
#define WINERROR( dw ) \
   (0 != (dw))

/*++ ++

MgxEntry.cpp Declarations

 -- --*/
#pragma pack( push, 4 )
typedef union _MGXFLAGS
{
   DWORD    Flags;
   struct
   {
      /*
       * Specifies whether the COM SCM started the application or not
       */
      DWORD IsCOMActivation : 1;
   };
}MGXFLAGS;
#pragma pack( pop )

/*
 * Global flags / settings
 */
extern MGXFLAGS g_ControlFlags;

#endif /* __GLOBAL_H__ */
