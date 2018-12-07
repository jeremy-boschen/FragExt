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
 
/* NTVersion.h
 *    General OS version testing functions
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#pragma pack( push, 4 )
typedef union _PACKEDNTVERSION
{
   DWORD    NTVersion;
   struct
   {
      DWORD SpMinorVersion : 8;
      DWORD SpMajorVersion : 8;
      DWORD OSMinorVersion : 8;
      DWORD OSMajorVersion : 8;
   };
}PACKEDNTVERSION;
#pragma pack( pop )

/*++
   GetNTVersion
      Retrieves the current operating system version packed into a
      DWORD value compatible with SDKDDKVER.H NTDDI macros
 --*/
inline
DWORD
GetNTVersion( )
{
   PACKEDNTVERSION PackedVersion;

   OSVERSIONINFOEX VersionInfo;

   ZeroMemory(&VersionInfo,
              sizeof(VersionInfo));

   VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);

   if ( !GetVersionEx(reinterpret_cast<LPOSVERSIONINFO>(&VersionInfo)) )
   {
      /* Failure */
      return ( 0 );
   }

   /*
    * Pack the version information we care about. This allows us to do
    * a quick comparison to check for what features we can use
    */
   PackedVersion.OSMajorVersion = static_cast<BYTE>(VersionInfo.dwMajorVersion);
   PackedVersion.OSMinorVersion = static_cast<BYTE>(VersionInfo.dwMinorVersion);
   PackedVersion.SpMajorVersion = static_cast<BYTE>(VersionInfo.wServicePackMajor);
   PackedVersion.SpMinorVersion = static_cast<BYTE>(VersionInfo.wServicePackMinor);

   /* Success */
   return ( PackedVersion.NTVersion );
}

/*
 * Global OS platform packed version, initialized by CRT initialization phase
 */
__declspec(selectany) DWORD g_NTVersion = GetNTVersion();

/*++
   IsNTDDIVersion
  		Checks whether the current OS is greater than a specified NTDDI 
      macro version
  
   Return Value
  		TRUE if the system is running the specified NTDDI version or greater, 
      otherwise FALSE.

   Remarks
      Common NTDDI macros are:
         NTDDI_WIN2K, NTDDI_WINXP, NTDDI_WINXPSP1, NTDDI_WINXPSP2, NTDDI_VISTA, 
         NTDDI_WIN7
  --*/
inline
BOOLEAN
IsNTDDIVersion(
   __in DWORD NTVersion,
   __in DWORD NTDDIVersion
)
{
   return ( NTVersion >= NTDDIVersion ? TRUE : FALSE );
}

/*++
   IsNTVersion
  		Checks whether the current OS is greater than a specified version
  
   Return Value
  		TRUE if the system is running the specified NT version or greater, 
      otherwise FALSE
 --*/
inline 
BOOLEAN 
IsNTVersion(
   __in DWORD NTVersion,
   __in BYTE OSMajorVersion,
   __in BYTE OSMinorVersion,
   __in BYTE SpMajorVersion,
   __in BYTE SpMinorVersion 
)
{
   PACKEDNTVERSION PackedVersion;

   PackedVersion.OSMajorVersion = OSMajorVersion;
   PackedVersion.OSMinorVersion = OSMinorVersion;
   PackedVersion.SpMajorVersion = SpMajorVersion;
   PackedVersion.SpMinorVersion = SpMinorVersion;

   return ( NTVersion >= PackedVersion.NTVersion ? TRUE : FALSE );
}
