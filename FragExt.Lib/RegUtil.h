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

/* RegUtil.h
 *    Registry utilities for the solution
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __REGUTIL_H__
#define __REGUTIL_H__

/* Main registry subkey for FragExt settings
 */
#define _FX_SUBKEY                        L"SOFTWARE\\jBoschen\\FragExt"

/********************************************************************************
   
   FragSvx.Exe

 ********************************************************************************/

/* Registry subkey for FragSvx settings */
#define FXSVCMGR_SUBKEY                   _FX_SUBKEY

/* Type:    REG_DWORD
 * Values:  0, 1
 */
#define FXSVCMGR_ENABLEFRAGFILTER         L"EnableFragFilter"

/* Type:    REG_DWORD
 * Values:  0, 1
 */
#define FXSVCMGR_AUTOATTACHVOLUMES        L"AutoAttachVolumes"

/* Type:    REG_DWORD
 * Values:  0, 1
 */
#define FXSVCMGR_USELOGSTREAMS            L"UseLogStreams"

/* Type:    REG_DWORD
 * Values:  0, 1
 */
#define FXSVCMGR_ALLOWPROCESSREGISTRATION L"AllowProcessRegistration"

/* Type:    REG_DWORD
 * Values:  1-255
 */
#define FXSVCMGR_CLUSTERBLOCKS            L"ClusterBlocks"


/********************************************************************************
   
   FragEng.Dll

 ********************************************************************************/

/* Registry subkey for FragEng settings */
#define FRAGENG_SUBKEY                    _FX_SUBKEY

/* Type:    REG_DWORD
 * Values:  0 - 4294967295
 *
 * Size, in bytes, of the minimum fragment size for partial defrags. 1 will be
 * added to this value so the actual range is 1 - 4294967296
 */
#define FRAGENG_MINIMUMFRAGMENTSIZE       L"MinimumFragmentSize"


/* REG_QWORD */
#define FRAGENG_FILEACCESSEDDELTA_FREQUENT   L"TimeAccessedFrequent"
#define FRAGENG_FILEACCESSEDDELTA_INFREQUENT L"TimeAccessedInfrequent"

#define FRAGENG_FILEMODIFIEDDELTA_FREQUENT   L"TimeModifiedFrequent"
#define FRAGENG_FILEMODIFIEDDELTA_INFREQUENT L"TimeModifiedInfrequent"


/********************************************************************************
   
   FragMgx.Exe

 ********************************************************************************/

/* Registry subkey for FragMgx settings */
#define FRAGMGX_SUBKEY                    _FX_SUBKEY

/* Type:    REG_DWORD
 * Values:  0 (no), 1 (yes)
 */
#define FRAGMGX_CLOSEONCOMPLETION         L"AutoClose"

/* Type:    REG_DWORD
 * Values:  0 (no), 1 (yes)
 */
#define FRAGMGX_INCLUDEALTDATESTREAMS     L"IncludeAltDataStreams"

/* Type:    REG_DWORD
 * Values:  0 - 32767
 *
 * Notes:   - 0 disables directory scanning
 */
#define FRAGMGX_DIRECTORYSCANDEPTH        L"DirectoryScanDepth"

/********************************************************************************
   
   FragShx.Dll

 ********************************************************************************/

/* Registry subkey for FragShx settings */
#define FRAGSHX_SUBKEY                    _FX_SUBKEY

/* Type:    REG_DWORD
 * Values:  0, 1
 */
#define FRAGSHX_SHOWCOMPRESSEDUNITS       L"ShowCompressed"

/* Type:    REG_DWORD
 * Values:  0, 1
 */
#define FRAGSHX_SHOWFILESTREAMS           L"ShowStreams"

/* Type:    REG_DWORD
 * Values:  Bitmask. Bit 1 = Size, Bit 2 = Percentage, Bit 3 = Clusters, Bit 4 = Extents
 */
#define FRAGSHX_PROPCOLUMNMASK            L"PropColumnMask"

/* Type:    REG_DWORD
 * Values:  0, 1
 */
#define FRAGSHX_COLUMNNOTEXTCONVERT       L"NoColumnTextConversion"

/********************************************************************************
   
   Shared

 ********************************************************************************/

#define RVF_HKCR     0x00000001
#define RVF_HKCU     0x00000002
#define RVF_HKLM     0x00000004

#define RFV_NOEXPAND 0x00000010

#define EncodeRegType( Type ) \
   (((Type) & 0x0000000f) << 24)

#ifndef REGUTILAPI
   #define REGUTILAPI APIENTRY
#endif /* REGUTILAPI */

DWORD
REGUTILAPI
GetRegistryValue( 
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_bcount_part(ValueLength, *ValueLength) LPBYTE Value,
   __inout DWORD* ValueLength
);

DWORD
REGUTILAPI
GetRegistryValueBinary(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_bcount_part(ValueLength, *ValueLength) LPBYTE Value,
   __inout DWORD* ValueLength
);

DWORD
REGUTILAPI
GetRegistryValueString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_ecount_z(ValueLength) LPWSTR Value,
   __in DWORD ValueLength
);

DWORD
REGUTILAPI
GetRegistryValueMultiString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_ecount(ValueLength) __nullnullterminated LPWSTR Value,
   __in DWORD ValueLength
);

DWORD
REGUTILAPI
GetRegistryValueExpandString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_ecount(ValueLength) LPWSTR Value,
   __in DWORD ValueLength
);

DWORD
REGUTILAPI
GetRegistryValueDword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out DWORD* Value
);

DWORD
REGUTILAPI
GetRegistryValueQword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out ULONGLONG* Value
);

DWORD 
REGUTILAPI
SetRegistryValue( 
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_bcount(ValueLength) const BYTE* Value,
   __in DWORD ValueLength
);

DWORD
REGUTILAPI
SetRegistryValueBinary(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_bcount(ValueLength) const BYTE* Value,
   __in DWORD ValueLength
);

DWORD
REGUTILAPI
SetRegistryValueString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_z LPCWSTR Value
);

DWORD
REGUTILAPI
SetRegistryValueMultiString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in __nullnullterminated LPCWSTR ValueName, 
   __in_z LPCWSTR Value
);

DWORD
REGUTILAPI
SetRegistryValueExpandString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_z LPCWSTR Value
);

DWORD
REGUTILAPI
SetRegistryValueDword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in DWORD Value
);

DWORD
REGUTILAPI
SetRegistryValueQword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in ULONGLONG Value
);

#endif /* __REGUTIL_H__ */