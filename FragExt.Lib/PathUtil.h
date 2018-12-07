/* FragExt - Windows defragmenter
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
 
/* PathUtil.h
 *    Useful implementations of path functions like those from SHLWAPI.DLL,
 *    to avoid the dependency and with length checking and handling of
 *    paths longer than MAX_PATH
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#pragma once

#ifndef __PATHUTIL_H__
#define __PATHUTIL_H__

#define PATHCCH_MAXLENGTH  (32767U)
#define PATHCB_MAXLENGTHA  (PATHCCH_MAXLENGTH * sizeof(CHAR))
#define PATHCB_MAXLENGTHW  (PATHCCH_MAXLENGTH * sizeof(WCHAR))

#define PATHCCH_NULTERM    USHRT_MAX

#define PATH_TYPE_UNKNOWN        0
#define PATH_TYPE_DOSDEVICE      1/*UNSUPPORTED*/
#define PATH_TYPE_NT             2/*UNSUPPORTED*/
#define PATH_TYPE_DOS            3
#define PATH_TYPE_VOLUMEGUID     4
#define PATH_TYPE_UNC            5


#define IsValidPathType( Type )        \
   (((Type) != PATH_TYPE_UNKNOWN) && ((Type) != PATH_TYPE_DOSDEVICE) && ((Type) != PATH_TYPE_NT))

typedef struct _PATH_INFORMATIONA {
   ULONG          Type                    : 3;
   
   ULONG          IsSystemParsingDisabled : 1;

   USHORT         BasePathLength;
   LPCSTR         BasePath;
   union {
      struct {
         USHORT   VolumeLength;
         LPCSTR   Volume;
      };
      struct {
         USHORT   ShareLength;
         LPCSTR   Share;
      };
   };
   USHORT         ParentPathLength;
   LPCSTR         ParentPath;

   USHORT         FinalNameLength;
   LPCSTR         FinalName;

   USHORT         ExtensionLength;
   LPCSTR         Extension;

   USHORT         StreamLength;
   LPCSTR         Stream;
}PATH_INFORMATIONA, *PPATH_INFORMATIONA;
typedef const struct _PATH_INFORMATIONA* PCPATH_INFORMATIONA;

typedef struct _PATH_INFORMATIONW {
   ULONG          Type                    : 3;
   
   ULONG          IsSystemParsingDisabled : 1;

   USHORT         BasePathLength;
   LPCWSTR        BasePath;
   union {
      struct {
         USHORT   VolumeLength;
         LPCWSTR  Volume;
      };
      struct {
         USHORT   ShareLength;
         LPCWSTR  Share;
      };
   };
   USHORT         ParentPathLength;
   LPCWSTR        ParentPath;

   USHORT         FinalNameLength;
   LPCWSTR        FinalName;

   USHORT         ExtensionLength;
   LPCWSTR        Extension;

   USHORT         StreamLength;
   LPCWSTR        Stream;
}PATH_INFORMATIONW, *PPATH_INFORMATIONW;
typedef const struct _PATH_INFORMATIONW* PCPATH_INFORMATIONW;

BOOLEAN
APIENTRY
PathCchParseInformationA(
   __in_ecount_z(MaxLength) LPCSTR Path,
   __in USHORT MaxLength,
   __out PPATH_INFORMATIONA PathInformation
);

BOOLEAN
APIENTRY
PathCchParseInformationW(
   __in_ecount_z(MaxLength) LPCWSTR Path,
   __in USHORT MaxLength,
   __out PPATH_INFORMATIONW PathInformation
);

BOOLEAN
APIENTRY
PathCchBuildPathA(
   __in PCPATH_INFORMATIONA PathInformation,
   __out_ecount_z(PathLength) LPSTR Path,
   __in size_t PathLength
);

BOOLEAN
APIENTRY
PathCchBuildPathW(
   __in PCPATH_INFORMATIONW PathInformation,
   __out_ecount_z(PathLength) LPWSTR Path,
   __in size_t PathLength
);

__nullterminated LPSTR
APIENTRY
PathCchAddBackslashA(
   __inout_ecount_z(PathLength) LPSTR Path,
   __in size_t PathLength
);

__nullterminated LPWSTR
APIENTRY
PathCchAddBackslashW(
   __inout_ecount_z(PathLength) LPWSTR Path,
   __in size_t PathLength
);

__nullterminated LPSTR
APIENTRY
PathCchRemoveBackslashA(
   __inout_z LPSTR Path
);

__nullterminated LPWSTR
APIENTRY
PathCchRemoveBackslashW(
   __inout_z LPWSTR Path
);

BOOLEAN
APIENTRY
PathCchAppendA(
   __inout_ecount_z(PathLength) LPSTR Path,
   __in size_t PathLength,
   __in_z LPCSTR PathToAppend
);

BOOLEAN
APIENTRY
PathCchAppendW(
   __inout_ecount_z(PathLength) LPWSTR Path,
   __in size_t PathLength,
   __in_z LPCWSTR PathToAppend
);

__nullterminated LPSTR
APIENTRY
PathCchFindExtensionA(
   __in_z LPCSTR Path
);

__nullterminated LPWSTR
APIENTRY
PathCchFindExtensionW(
   __in_z LPCWSTR Path
);

__nullterminated LPSTR
APIENTRY
PathCchFindFileNameA(
   __in_z LPCSTR Path
);

__nullterminated LPWSTR
APIENTRY
PathCchFindFileNameW(
   __in_z LPCWSTR Path
);

void
APIENTRY
PathCchRemoveExtensionA(
   __in_z LPSTR Path
);

void
APIENTRY
PathCchRemoveExtensionW(
   __in_z LPWSTR Path
);

BOOLEAN
APIENTRY
PathCchIsRootA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchIsRootW(
   __in_z LPCWSTR Path
);

void
APIENTRY
PathCchRemoveFileSpecA(
   __inout_z LPSTR Path
);

void
APIENTRY
PathCchRemoveFileSpecW(
   __inout_z LPWSTR Path
);

void
APIENTRY
PathCchRemoveStreamSpecA(
   __inout_z LPSTR Path
);

void
APIENTRY
PathCchRemoveStreamSpecW(
   __inout_z LPWSTR Path
);

BOOLEAN
APIENTRY
PathCchReplaceFileSpecA(
   __inout_ecount_z(PathLength) LPSTR Path,
   __in size_t PathLength,
   __in_z LPCSTR FileSpec
);

BOOLEAN
APIENTRY
PathCchReplaceFileSpecW(
   __inout_ecount_z(PathLength) LPWSTR Path,
   __in size_t PathLength,
   __in_z LPCWSTR FileSpec
);

BOOLEAN
APIENTRY
PathCchStripToRootA(
   __inout_z LPSTR Path
);

BOOLEAN
APIENTRY
PathCchStripToRootW(
   __inout_z LPWSTR Path
);

BOOLEAN
APIENTRY
PathCchIsUNCA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchIsUNCW(
   __in_z LPCWSTR Path
);

BOOLEAN
APIENTRY
PathCchIsDOSA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchIsDOSW(
   __in_z LPCWSTR Path
);

BOOLEAN
APIENTRY
PathCchIsVolumeGUIDA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchIsVolumeGUIDW(
   __in_z LPCWSTR Path
);

BOOLEAN 
APIENTRY
PathCchIsDOSDeviceA( 
   __in_z LPCSTR Path
);

BOOLEAN 
APIENTRY
PathCchIsDOSDeviceW( 
   __in_z LPCWSTR Path
);

BOOLEAN
APIENTRY
PathCchIsNTA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchIsNTW(
   __in_z LPCWSTR Path
);

BOOLEAN
APIENTRY
PathCchIsExtendedLengthA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchIsExtendedLengthW(
   __in_z LPCWSTR Path
);

BOOLEAN
APIENTRY
PathCchIsDotDirectoryA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchIsDotDirectoryW(
   __in_z LPCWSTR Path
);

BOOLEAN
APIENTRY
PathCchFileExistsA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchFileExistsW(
   __in_z LPCWSTR Path
);

BOOLEAN
APIENTRY
PathCchDirectoryExistsA(
   __in_z LPCSTR Path
);

BOOLEAN
APIENTRY
PathCchDirectoryExistsW(
   __in_z LPCWSTR Path
);

/*
 * PathCchCreateDirectory
 *    Creates a directory tree
 *
 * Notes:
 *    If the path does not end in a \, the final component is considered to
 *    be a file
 */
BOOLEAN
APIENTRY
PathCchCreateDirectoryA( 
   __in_z LPCSTR Path,
   __in_opt LPSECURITY_ATTRIBUTES SecurityAttributes
);

BOOLEAN
APIENTRY
PathCchCreateDirectoryW( 
   __in_z LPCWSTR Path,
   __in_opt LPSECURITY_ATTRIBUTES SecurityAttributes
);

#if defined(UNICODE) || defined(_UNICODE)
   #define PATH_INFORMATION         PATH_INFORMATIONW
   #define PPATH_INFORMATION        PPATH_INFORMATIONW
   #define PCPATH_INFORMATION       PCPATH_INFORMATIONW
   
   #define PathCchParseInformation  PathCchParseInformationW
   #define PathCchBuildPath         PathCchBuildPathW
   #define PathCchAddBackslash      PathCchAddBackslashW
   #define PathCchRemoveBackslash   PathCchRemoveBackslashW
   #define PathCchAppend            PathCchAppendW
   #define PathCchFindExtension     PathCchFindExtensionW
   #define PathCchFindFileName      PathCchFindFileNameW
   #define PathCchRemoveExtension   PathCchRemoveExtensionW
   #define PathCchIsRoot            PathCchIsRootW
   #define PathCchRemoveFileSpec    PathCchRemoveFileSpecW
   #define PathCchRemoveStreamSpec  PathCchRemoveStreamSpecW
   #define PathCchReplaceFileSpec   PathCchReplaceFileSpecW
   #define PathCchStripToRoot       PathCchStripToRootW
   #define PathCchIsUNC             PathCchIsUNCW
   #define PathCchIsDOS             PathCchIsDOSW
   #define PathCchIsDOSDevice       PathCchIsDOSDeviceW
   #define PathCchIsNT              PathCchIsNTW
   #define PathCchIsExtendedLength  PathCchIsExtendedLengthW
   #define PathCchIsDotDirectory    PathCchIsDotDirectoryW
   #define PathCchFileExists        PathCchFileExistsW
   #define PathCchDirectoryExists   PathCchDirectoryExistsW
   #define PathCchCreateDirectory   PathCchCreateDirectoryW
#else /* defined(UNICODE) || defined(_UNICODE) */
   #define PATH_INFORMATION         PATH_INFORMATIONA
   #define PPATH_INFORMATION        PPATH_INFORMATIONA
   #define PCPATH_INFORMATION       PCPATH_INFORMATIONA
   
   #define PathCchParseInformation  PathCchParseInformationA
   #define PathCchBuildPath         PathCchBuildPathA
   #define PathCchAddBackslash      PathCchAddBackslashA
   #define PathCchRemoveBackslash   PathCchRemoveBackslashA
   #define PathCchAppend            PathCchAppendA
   #define PathCchFindExtension     PathCchFindExtensionA
   #define PathCchFindFileName      PathCchFindFileNameA
   #define PathCchRemoveExtension   PathCchRemoveExtensionA
   #define PathCchIsRoot            PathCchIsRootA
   #define PathCchRemoveFileSpec    PathCchRemoveFileSpecA
   #define PathCchRemoveStreamSpec  PathCchRemoveStreamSpecA
   #define PathCchReplaceFileSpec   PathCchReplaceFileSpecA
   #define PathCchStripToRoot       PathCchStripToRootA
   #define PathCchIsUNC             PathCchIsUNCA
   #define PathCchIsDOS             PathCchIsDOSA
   #define PathCchIsDOSDevice       PathCchIsDOSDeviceA
   #define PathCchIsNT              PathCchIsNTA
   #define PathCchIsExtendedLength  PathCchIsExtendedLengthA
   #define PathCchIsDotDirectory    PathCchIsDotDirectoryA
   #define PathCchFileExists        PathCchFileExistsA
   #define PathCchDirectoryExists   PathCchDirectoryExistsA
   #define PathCchCreateDirectory   PathCchCreateDirectoryA
#endif /* defined(UNICODE) || defined(_UNICODE) */

#endif /* __PATHUTIL_H__ */
