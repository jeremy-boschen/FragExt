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
 
/* Stdafx.h
 *    Include file for standard system include files, and project specific 
 *    files that are used frequently, but are changed infrequently.
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

/**********************************************************************

	Platform SDK & C/C++ Runtime Specific

 **********************************************************************/
#include <stddef.h>
#include <stdlib.h>
#include <crtdbg.h>

#include <process.h>
#include <tchar.h>

#include <windows.h>
#include <winnt.h>
#include <strsafe.h>

#include <objbase.h>

/**********************************************************************

	Project Specific

 **********************************************************************/

/* nonstandard extension used: specifying underlying type for enum */
#pragma warning( disable : 4480 )

#include "NTVersion.h"

#include "DbgUtil.h"
#include "MemUtil.h"

#define PRJ_WPP_CONTROL_GUIDS \
   WPP_CONTROL_GUID_FRAGLIB

#include <Utility.h>
#include <WppConfig.h>

