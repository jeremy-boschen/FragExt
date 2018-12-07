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

/* StreamUtil.h
 *    NTFS alternate data stream support
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __STREAMUTIL_H__
#define __STREAMUTIL_H__

/* Stream flags */
#define FSX_STREAM_NODEFAULTDATASTREAM    0x00000001


#ifndef MAX_PATH_STREAM
   #define MAX_PATH_STREAM (MAX_PATH + 36)
#endif /* MAX_PATH_STREAM */

typedef 
BOOLEAN 
(APIENTRY* 
PENUMFILESTREAMINFORMATIONROUTINE)( 
   __in ULONG StreamCount, 
   __in ULONGLONG StreamSize, 
   __in ULONGLONG StreamSizeOnDisk,
   __in_z LPCWSTR StreamName, 
   __in_opt PVOID CallbackParameter
);

DWORD 
APIENTRY 
FpEnumFileStreamInformation( 
   __in LPCWSTR FileName, 
   __in DWORD Flags, 
   __callback PENUMFILESTREAMINFORMATIONROUTINE Callback, 
   __in_opt PVOID CallbackParameter
);

DWORD 
APIENTRY 
FpGetFileStreamCount( 
   __in_z LPCWSTR FileName,
   __in DWORD Flags, 
   __out PULONG StreamCount
);

#endif /* __STREAMUTIL_H__ */