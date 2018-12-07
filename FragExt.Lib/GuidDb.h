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
 
/* GuidDb.h
 *    In memory GUID database primarily for COM debugging
 */

#pragma once

#ifndef __GUIDDB_H__
#define __GUIDDB_H__

#define GDB_CCH_MAXGUIDNAME \
   (255 + 42 + 1)

#define GDB_CB_MAXGUIDNAMEA \
   (GDB_CCH_MAXGUIDNAME * sizeof(char))

#define GDB_CB_MAXGUIDNAMEW \
   (GDB_CCH_MAXGUIDNAME * sizeof(WCHAR))

DWORD
APIENTRY
LookupNameOfGuidA(
   __in REFGUID riid,
   __out_ecount(cchNameLength) LPSTR Name,
   __in size_t cchNameLength
);

DWORD
APIENTRY
LookupNameOfGuidW(
   __in REFGUID riid,
   __out_ecount(cchNameLength) LPWSTR Name,
   __in size_t cchNameLength
);

#if defined(_UNICODE) || defined(UNICODE)
   #define LookupNameOfGuid LookupNameOfGuidW
#else /* defined(_UNICODE) || defined(UNICODE) */
   #define LookupNameOfGuid LookupNameOfGuidA
#endif /* defined(_UNICODE) || defined(UNICODE) */

#endif /* __GUIDDB_H__ */
