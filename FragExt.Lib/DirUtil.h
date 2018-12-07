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

/* DirUtil.h
 *    Directory utility functions
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __DIRUTIL_H__
#define __DIRUTIL_H__

/**
 * Directory enumeration
 **/

#define EDF_DIRECTORIESONLY         0x00000001
#define EDF_INCLUDEDATASTREAMS      0x00000002
#define EDF_THREADEDENUMERATION     0x00000004

#define EDF_TRAVERSEDEPTHCOUNTMASK  0x7fff0000
#define EDF_TRAVERSEDEPTHCOUNTSHIFT 16
#define EDF_TRAVERSALDEPTHMAX       32767

__inline
DWORD 
EncodeMaxTraverseDepth( 
   __in USHORT Depth 
)
{
   return ( (((ULONG)Depth) << EDF_TRAVERSEDEPTHCOUNTSHIFT) & EDF_TRAVERSEDEPTHCOUNTMASK );
}

typedef struct _DIRECTORYENTRY_INFORMATION
{
   /*
    * File times
    */
   FILETIME  CreationTime;
   FILETIME  LastAccessTime;
   FILETIME  LastWriteTime;

   /*
    * Size of the file, in bytes
    */
   ULONGLONG FileSize;
   
   /*
    * File attributes
    */
   DWORD     FileAttributes;
   
   /*
    * Length of FilePath, in characters
    */
   USHORT    FilePathLength;
   WCHAR     FilePath[1];
}DIRECTORYENTRY_INFORMATION, *PDIRECTORYENTRY_INFORMATION;
typedef const struct _DIRECTORYENTRY_INFORMATION* PCDIRECTORYENTRY_INFORMATION;

typedef
BOOLEAN
(APIENTRY* PENUMDIRECTORYROUTINE)(
   __in PCDIRECTORYENTRY_INFORMATION EntryInformation,
   __in DWORD Depth,
   __in_opt PVOID CallbackParameter
);

DWORD 
APIENTRY
FpEnumDirectoryInformation( 
   __in_z LPCWSTR DirectoryPath, 
   __in DWORD Flags, 
   __callback PENUMDIRECTORYROUTINE Callback, 
   __in_opt PVOID CallbackParameter 
);

#endif /* __DIRUTIL_H__ */