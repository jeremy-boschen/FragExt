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
 *    Include file for standard system include files, or project specific 
 *    include files that are used frequently, but are changed infrequently.
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
#include <tchar.h>

#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>

/**********************************************************************

	Project Specific

 **********************************************************************/
#define PRJ_WPP_CONTROL_GUIDS \
   WPP_CONTROL_GUID_FRAGLIB   \
   WPP_CONTROL_GUID_FRAGENG

#include <WppConfig.h>

#include <DbgUtil.h>
#include <RegUtil.h>
#include <Utility.h>

#include "Resource.h"

