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

/* RegUtil.cpp
 *    Registry utility function implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include <RegUtil.h>

typedef struct _HKEY_MAP
{
   BYTE Bit;
   HKEY Key;
}HKEY_MAP;

#ifndef AlignUp
   #define AlignUp( Size, Align ) \
      (((Size) + ((Align) - 1)) & ~((Align) - 1))      
#endif /* AlignUp */

#define DecodeRegRootKey( Flags ) \
   ((Flags) & 0x0000000f)

#define DecodeRegType( Flags ) \
   (((Flags) & 0xff000000) >> 24)

DWORD 
REGUTILAPI
GetRegistryValue( 
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_bcount_part(ValueLength, *ValueLength) LPBYTE Value,
   __inout DWORD* ValueLength
)
{
   LSTATUS  lStatus;

   HKEY     hKey;
   DWORD    dwType;
   DWORD    cbData;

   int      idx;
   HKEY_MAP rgRootKey[] = 
   {
      {1, HKEY_CLASSES_ROOT},
      {2, HKEY_CURRENT_USER},
      {4, HKEY_LOCAL_MACHINE}
   };

   lStatus = ERROR_FILE_NOT_FOUND;

   for ( idx = 0; idx < ARRAYSIZE(rgRootKey); idx++ )
   {
      if ( 0 == (rgRootKey[idx].Bit & DecodeRegRootKey(Flags)) )
      {
         continue;
      }

      hKey = NULL;
      
      lStatus = RegOpenKeyExW(rgRootKey[idx].Key,
                              SubKey,
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

      if ( ERROR_SUCCESS != lStatus )
      {
         continue;
      }

      dwType = REG_NONE;
      cbData = (*ValueLength);

      lStatus = RegQueryValueExW(hKey,
                                 ValueName,
                                 NULL,
                                 &dwType,
                                 Value,
                                 &cbData);

      RegCloseKey(hKey);
      hKey = NULL;
      
      if ( (ERROR_SUCCESS == lStatus) || (ERROR_MORE_DATA == lStatus) )
      {
         (*ValueLength) = cbData;

         /*
          * If the data type is wrong, fail the call
          */
         if ( dwType != DecodeRegType(Flags) )
         {
            lStatus = ERROR_INVALID_DATATYPE;
         }
         
         /* Success / Failure */
         break;
      }
   }

   /* Success / Failure */
   return ( static_cast<DWORD>(lStatus) );
}

DWORD
REGUTILAPI
GetRegistryValueBinary(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_bcount_part(ValueLength, *ValueLength) LPBYTE Value,
   __inout DWORD* ValueLength
)
{
   DWORD dwRet;

   dwRet = GetRegistryValue(Flags|EncodeRegType(REG_BINARY),
                            SubKey,
                            ValueName,
                            Value,
                            ValueLength);

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
GetRegistryValueString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_ecount_z(ValueLength) LPWSTR Value,
   __in DWORD ValueLength
)
{
   DWORD dwRet;

   DWORD cbData;
   DWORD cchData;

   cbData = (ValueLength * sizeof(WCHAR));

   dwRet = GetRegistryValue(Flags|EncodeRegType(REG_SZ),
                            SubKey,
                            ValueName,
                            reinterpret_cast<LPBYTE>(Value),
                            &cbData);
   
   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   /*
    * Ensure the data is null terminated
    */
   if ( ValueLength && ValueName )
   {
      /*
       * Ensure cbData is properly aligned and convert it to a character
       * length
       */
      cchData = (AlignUp(cbData, sizeof(WCHAR)) / sizeof(WCHAR));
      _ASSERTE(cchData <= ValueLength);

      if ( 0 == cchData )
      {
         /*
          * The registry value is NULL
          */

         Value[0] = L'\0';

         /* Success */
         return ( NO_ERROR );
      }

      /*
       * Check if the string contains the null terminator. If it does not,
       * and the length of the string does not allow room to store the 
       * extra null terminator then fail the call
       */
      if ( L'\0' != Value[cchData - 1] )
      {
         if ( (cchData + 1) <= ValueLength )
         {
            /*
             * We still have room to store an extra null terminator, so
             * just bump up the data length
             */
            cchData += 1;
         }
         else
         {
            /*
             * There is no extra room for a null terminator
             */
            
            /* Failure */
            return ( ERROR_MORE_DATA );
         }
      }

      Value[cchData - 1] = L'\0';
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
GetRegistryValueMultiString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_ecount(ValueLength) LPWSTR Value,
   __in DWORD ValueLength
)
{
   DWORD dwRet;

   DWORD cbData;
   DWORD cchData;
   DWORD cchExtra;

   cbData = (ValueLength * sizeof(WCHAR));

   dwRet = GetRegistryValue(Flags|EncodeRegType(REG_MULTI_SZ),
                            SubKey,
                            ValueName,
                            reinterpret_cast<LPBYTE>(Value),
                            &cbData);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }
   
   /*
    * Ensure the data is double null terminated
    */
   if ( ValueLength && Value )
   {
      /*
       * Ensure cbData is properly aligned and convert it to a character
       * length
       */
      cchData = (AlignUp(cbData, sizeof(WCHAR)) / sizeof(WCHAR));
      _ASSERTE(cchData <= ValueLength);

      /*
       * Handle the edge case where the value is NULL or an empty string
       *
       * If the value is NULL, cchData will be equal to 0. If the value 
       * is empty, cchData will be equal to 1. To have reached this point 
       * the caller could have passed a buffer length of 1 or greater, 
       * but the code below requires a buffer of length 2 or greater.
       *
       * For cchData to be equal to 1, the string must be empty because
       * RegQueryValueEx will require 2 terminating null characters for
       * a multi-string with at least 1 character.
       *
       * So basically, cchData must be >= 2 to continue past this point.
       */
      if ( cchData < 2 )
      {
         /*
          * REG_MULTI_SZ value is NULL or an empty string ""
          */

         Value[0] = L'\0';

         /* Success */
         return ( NO_ERROR );
      }

      cchExtra = 0;

      /*
       * Check if the string contains the two null terminators. If it does not,
       * and the length of the string does not allow room to store the 
       * extra null terminators then fail the call
       */
      if ( L'\0' != Value[cchData - 2] )
      {
         cchExtra += 1;
      }

      if (  L'\0' != Value[cchData - 1] )
      {
         cchExtra += 1;
      }

      if ( (cchData + cchExtra) <= ValueLength )
      {
         /*
          * We still have room to store an extra null terminator, if needed, so
          * bump up the data length
          */
         cchData += cchExtra;
      }
      else
      {
         /*
          * There is no extra room for the two null terminators
          */

         /* Failure */
         return ( ERROR_MORE_DATA );
      }

      Value[cchData - 2] = L'\0';
      Value[cchData - 1] = L'\0';
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
GetRegistryValueExpandString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out_ecount(ValueLength) LPWSTR Value,
   __in DWORD ValueLength
)
{
   DWORD  dwRet;
   
   DWORD  cbData;
   DWORD  cchData;
   
   WCHAR  chWorkSpace[128];
   DWORD  cchWorkSpace;   
   LPWSTR pszWorkSpace;

   DWORD  cchExpanded;
   
   cbData = (ValueLength * sizeof(WCHAR)) - sizeof(WCHAR);

   dwRet = GetRegistryValue(Flags|EncodeRegType(REG_EXPAND_SZ),
                            SubKey,
                            ValueName,
                            reinterpret_cast<LPBYTE>(Value),
                            &cbData);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }
   
   /*
    * Ensure the data is null terminated
    */
   if ( ValueLength && ValueName )
   {
      /*
       * Ensure cbData is properly aligned and convert it to a character
       * length
       */
      cchData = (AlignUp(cbData, sizeof(WCHAR)) / sizeof(WCHAR));
      _ASSERTE(cchData <= ValueLength);

      /*
       * Handle edge case where registry value is NULL or an empty string
       */
      if ( cchData < 1 )
      {
         Value[0] = L'\0';

         /* Success */
         return ( NO_ERROR );
      }

      /*
       * Check if the string contains the null terminator. If it does not,
       * and the length of the string does not allow room to store the 
       * extra null terminator then fail the call
       */
      if ( L'\0' != Value[cchData - 1] )
      {
         if ( (cchData + 1) <= ValueLength )
         {
            /*
             * We still have room to store an extra null terminator, so
             * just bump up the data length
             */
            cchData += 1;
         }
         else
         {
            /*
             * There is no extra room for a null terminator
             */
            
            /* Failure */
            return ( ERROR_MORE_DATA );
         }
      }

      Value[cchData - 1] = L'\0';
   }

   if ( !(Flags & RFV_NOEXPAND) )
   {
      /*
       * If necessary, allocate a buffer to expand the string into. We have to copy the
       * expanded string into the output buffer, so we cannot expand into anything larger
       * than the output buffer
       */
      cchWorkSpace = ValueLength;
      
      if ( cchWorkSpace > ARRAYSIZE(chWorkSpace) )
      {
         pszWorkSpace = reinterpret_cast<LPWSTR>(malloc(cchWorkSpace * sizeof(WCHAR)));
         if ( !pszWorkSpace )
         {
            /* Failure */
            return ( ERROR_NOT_ENOUGH_MEMORY );
         }
      }
      else
      {
         pszWorkSpace = &(chWorkSpace[0]);
      }

      ZeroMemory(pszWorkSpace,
                 cchWorkSpace * sizeof(WCHAR));

      cchExpanded = ExpandEnvironmentStringsW(Value,
                                              pszWorkSpace,
                                              cchWorkSpace);

      if ( cchExpanded > cchWorkSpace )
      {
         /*
          * The output buffer will not hold the expanded string 
          */
         dwRet = ERROR_MORE_DATA;
      }
      else
      {
         /*
          * Copy the expanded string into the output buffer. The output buffer
          * will be large enough because we use its size as our max for the
          * expansion buffer
          */
         StringCchCopyW(Value,
                        ValueLength,
                        pszWorkSpace);
      }

      /*
       * If we allocated a buffer, free it
       */
      if ( pszWorkSpace != &(chWorkSpace[0]) )
      {
         free(pszWorkSpace);
         pszWorkSpace = NULL;
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
GetRegistryValueDword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out DWORD* Value
)
{
   DWORD dwRet;
   DWORD cbData;

   cbData = sizeof(*Value);

   dwRet = GetRegistryValue(Flags|EncodeRegType(REG_DWORD),
                            SubKey,
                            ValueName,
                            reinterpret_cast<LPBYTE>(Value),
                            &cbData);

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
GetRegistryValueQword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __out ULONGLONG* Value
)
{
   DWORD dwRet;
   DWORD cbData;

   cbData = sizeof(*Value);

   dwRet = GetRegistryValue(Flags|EncodeRegType(REG_QWORD),
                            SubKey,
                            ValueName,
                            reinterpret_cast<LPBYTE>(Value),
                            &cbData);

   /* Success / Failure */
   return ( dwRet );
}


DWORD 
REGUTILAPI
SetRegistryValue( 
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_bcount(ValueLength) const BYTE* Value,
   __in DWORD ValueLength
)
{
   LSTATUS  lStatus;

   HKEY     hKey;

   int      idx;
   HKEY_MAP rgRootKey[] = 
   {
      {1, HKEY_CLASSES_ROOT},
      {2, HKEY_CURRENT_USER},
      {4, HKEY_LOCAL_MACHINE}
   };

   lStatus = ERROR_FILE_NOT_FOUND;

   for ( idx = 0; idx < ARRAYSIZE(rgRootKey); idx++ )
   {
      if ( 0 == (rgRootKey[idx].Bit & DecodeRegRootKey(Flags)) )
      {
         continue;
      }

      hKey = NULL;
      
      lStatus = RegCreateKeyExW(rgRootKey[idx].Key,
                                SubKey,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_SET_VALUE,
                                NULL,
                                &hKey,
                                NULL);

      if ( ERROR_SUCCESS != lStatus )
      {
         continue;
      }

      lStatus = RegSetValueEx(hKey,
                              ValueName,
                              0,
                              DecodeRegType(Flags),
                              Value,
                              ValueLength);

      RegCloseKey(hKey);
      hKey = NULL;

      if ( lStatus == ERROR_SUCCESS )
      {
         /* Success */
         break;
      }
   }

   /* Success / Failure */
   return ( static_cast<DWORD>(lStatus) );
}

DWORD
REGUTILAPI
SetRegistryValueBinary(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_bcount(ValueLength) const BYTE* Value,
   __in DWORD ValueLength
)
{
   DWORD dwRet;

   dwRet = SetRegistryValue(Flags|EncodeRegType(REG_BINARY),
                            SubKey,
                            ValueName,
                            Value,
                            ValueLength);

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
SetRegistryValueString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_z LPCWSTR Value
)
{
   DWORD  dwRet;

   size_t cchLength;
   DWORD  cbLength;

   if ( Value )
   {
      cchLength = wcslen(Value) + 1;

      if ( cchLength > static_cast<size_t>(MAXDWORD / sizeof(WCHAR)) )
      {
         /* Failure */
         return ( ERROR_INVALID_DATA );
      }

      cbLength = (static_cast<DWORD>(cchLength) * sizeof(WCHAR));
   }
   else
   {
      cbLength = 0;
   }

   dwRet = SetRegistryValue(Flags|EncodeRegType(REG_SZ),
                            SubKey,
                            ValueName,
                            reinterpret_cast<const BYTE*>(Value),
                            cbLength);

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
SetRegistryValueMultiString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_z LPCWSTR Value
)
{
   DWORD   dwRet;

   LPCWSTR pszString;
   size_t  cchString;

   size_t  cchLength;
   DWORD   cbLength;

   if ( Value )
   {
      /*
       * Calculate the length of the multi-string
       */
      pszString = Value;
      cchLength = 0;

      while ( L'\0' != pszString[0] )
      {
         cchString = wcslen(pszString) + 1;
         
         cchLength += cchString;
         pszString += cchString;
      }

      cchLength += 1;

      if ( cchLength > static_cast<size_t>(MAXDWORD / sizeof(WCHAR)) )
      {
         /* Failure */
         return ( ERROR_INVALID_DATA );
      }

      cbLength = (static_cast<DWORD>(cchLength) * sizeof(WCHAR));
   }
   else
   {
      cbLength = 0;
   }
   
   dwRet = SetRegistryValue(Flags|EncodeRegType(REG_MULTI_SZ),
                            SubKey,
                            ValueName,
                            reinterpret_cast<const BYTE*>(Value),
                            cbLength);

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
SetRegistryValueExpandString(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in_z LPCWSTR Value
)
{
   DWORD  dwRet;

   size_t cchLength;
   DWORD  cbLength;

   if ( Value )
   {
      cchLength = wcslen(Value) + 1;

      if ( cchLength > static_cast<size_t>(MAXDWORD / sizeof(WCHAR)) )
      {
         /* Failure */
         return ( ERROR_INVALID_DATA );
      }

      cbLength = (static_cast<DWORD>(cchLength) * sizeof(WCHAR));
   }
   else
   {
      cbLength = 0;
   }

   dwRet = SetRegistryValue(Flags|EncodeRegType(REG_EXPAND_SZ),
                            SubKey,
                            ValueName,
                            reinterpret_cast<const BYTE*>(Value),
                            cbLength);

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
SetRegistryValueDword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in DWORD Value
)
{
   DWORD dwRet;

   dwRet = SetRegistryValue(Flags|EncodeRegType(REG_DWORD),
                            SubKey,
                            ValueName,
                            reinterpret_cast<const BYTE*>(&Value),
                            sizeof(Value));

   /* Success / Failure */
   return ( dwRet );
}

DWORD
REGUTILAPI
SetRegistryValueQword(
   __in DWORD Flags, 
   __in_z LPCWSTR SubKey, 
   __in_z LPCWSTR ValueName, 
   __in ULONGLONG Value
)
{
   DWORD dwRet;

   dwRet = SetRegistryValue(Flags|EncodeRegType(REG_QWORD),
                            SubKey,
                            ValueName,
                            reinterpret_cast<const BYTE*>(&Value),
                            sizeof(Value));

   /* Success / Failure */
   return ( dwRet );
}