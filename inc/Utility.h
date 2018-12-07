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

/* Utility.h
 *   Generic utility code declarations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __UTILITY_H__
#define __UTILITY_H__

/**********************************************************************
	
	Compile-time Miscellany

 **********************************************************************/

/**
 * _L / _LTEXT
 *    Like _T/_TEXT but always UNICODE
 */
#ifndef _LTEXT
   #define _LTEXTI( sz ) L##sz
   #define _LTEXT( sz ) _LTEXTI(sz)
#endif /* _LTEXT */

#ifndef _L
   #define _L _LTEXT
#endif /* _L */

/**
 * _countof
 *   Number of elements in a static array.  
 */
#ifndef _countof
   #define _countof(rg) ( sizeof( rg ) / sizeof( (rg)[0] ) )
#endif

/**
 * _charsof
 *   Number of characters in a static string, not including the null
 *   terminator. 
 */
#ifndef _charsofA
   #define _charsofA( sz ) (sizeof(sz) - 1)
#endif /* _charsofA */

#ifndef _charsofW
   #define _charsofW( sz ) ((sizeof(sz) / sizeof(wchar_t)) - 1)
#endif /* _charsofW */

#ifndef _charsof
   #if defined(_UNICODE) || defined(UNICODE)
      #define _charsof _charsofA
   #else
      #define _charsof _charsofW
   #endif
#endif

/**
 * _sizeoffield
 *		The size of a type's field.
 */
#ifndef _sizeoffield
	#define _sizeoffield(t, f) ( sizeof(((t *)0)->f) )
#endif /* _sizeoffield */

/**
 * _ansiunicode
 *    Assigns a generic API name its A or W suffix based on the
 *    presence of the _UNICODE macro.
 */
#ifndef _ansiunicode
   #if defined(_UNICODE) || defined(UNICODE)
      #define _ansiunicode(sz) sz "W"
   #else
      #define _ansiunicode(sz) sz "A"
   #endif
#endif /* _ansiunicode */

/**
 * _tostring
 *    Macro string`izer
 */
#ifndef _tostring   
   #define _tostringi( i ) #i
   #define _tostring( i ) _tostringi(i)
#endif /* _tostring */

/*++
   CInitializeString[A|W]
      Meta-template for performing an unrolled-initialization of a
      string buffer.
  --*/
template < SIZE_T _cch > 
class CInitializeStringA
{
public:
	static inline void initialize( char rg[], LPCSTR psz ) throw()
	{
	    rg[_cch] = psz[_cch];
	    /* 
        * When _cch reaches 0, the specialization below will be used
        * to end the recursion 
        */
	    CInitializeStringA<_cch - 1>::initialize(rg, psz);
	}
};

template < SIZE_T _cch > 
class CInitializeStringW
{
public:
   static inline void initialize( wchar_t rg[], LPCWSTR psz ) throw()
	{
	    rg[_cch] = psz[_cch];
	    /* 
        * When _cch reaches 0, the specialization below will be used
	     * to end the recursion 
        */
	    CInitializeStringW<_cch - 1>::initialize(rg, psz);
	}
};

/* 
 * Specialization to end recursion  
 */
template <> class CInitializeStringA<0>
{
public:
	static inline void initialize( char rg[], LPCSTR psz ) throw()
	{
	    rg[0] = psz[0];
	}
};

template <> class CInitializeStringW<0>
{
public:
	static inline void initialize( wchar_t rg[], LPCWSTR psz ) throw()
	{
	    rg[0] = psz[0];
	}
};

/*++
   InitializeString[A|W]
  		Wrapper for the CInitializeString[A|W] meta-templates
  
   Parameters
  		buf
  		    [out] The buffer to initialize.
  		sz
  		    [in] The string to initialize with.
  
   Return Value
  		None
  
   Remarks
  		buf must be large enough to accomidate sz, including the null-
  		terminator.
 --*/
#define InitializeStringA(buf, sz) \
	C_ASSERT(sizeof(buf) >= sizeof(sz)); \
	CInitializeStringA<(sizeof(sz) / sizeof(char)) - 1>::initialize(buf, sz)

#define InitializeStringW(buf, sz) \
	C_ASSERT(sizeof(buf) >= sizeof(sz)); \
	CInitializeStringW<(sizeof(sz) / sizeof(wchar_t)) - 1>::initialize(buf, sz)

#if defined(_UNICODE) || defined(UNICODE)
   #define InitializeString InitializeStringW
#else
   #define InitializeString InitializeStringA
#endif

/*++
   Align
  		Compile time alignment templates
  
   Usage
  		char buf1[ Align<sizeof(SOME_TYPE1)>:Up ];
  		char buf2[ Align<sizeof(SOME_TYPE2)>:Up ];
  
 --*/
template < size_t _Size, size_t _Align = MEMORY_ALLOCATION_ALIGNMENT >
class Align
{
public:
#pragma warning( suppress : 4480 )
   enum : size_t
   {
      Raw  = _Size,
      Down = _Size & ~(_Align - 1),
      Up   = (_Size + (_Align - 1)) & ~(_Align - 1),
   };
};

#ifndef MAX_PATH_STREAM
   #define MAX_PATH_STREAM (MAX_PATH + 36)
#endif /* MAX_PATH_STREAM */

#ifndef MAX_PATH_NATIVE
   /* MAX_PATH for \??\Volume{GUID}\devicename\filename:StreamName:$DATA syntax */
   #define MAX_PATH_NATIVE (sizeof("\\??\\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}") + MAX_PATH + MAX_PATH_STREAM)
#endif /* MAX_PATH_NATIVE */

#ifndef FlagOn
   #define FlagOn(_F, _SF)  \
      ((_F) & (_SF))
#endif

#ifndef BooleanFlagOn
   #define BooleanFlagOn(_F, _SF)\
      ((BOOLEAN)(0 != ((_F) & (_SF))))
#endif

/**********************************************************************
	
	Runtime Miscellany

 **********************************************************************/

#ifdef _DEBUG
   #define MEMORY_FILL_CHAR ((BYTE)0xFE)
#else
   #define MEMORY_FILL_CHAR ((BYTE)0x00)
#endif

#define InitializeMemory( Destination, Length ) \
   FillMemory(Destination, Length, MEMORY_FILL_CHAR)
//
///**
// * IsVersionNT
// *		Checks whether the current OS is a specified version of WinNT. 
// *
// * Return Value
// *		TRUE if the system is running the specified NT version, FALSE
// *		otherwise.
// */
//inline __declspec(deprecated) BOOL IsVersionNT( DWORD Major, DWORD Minor, WORD SpMajor, WORD SpMinor ) throw()
//{
//   OSVERSIONINFOEX osix;
//   DWORDLONG       dwMask;
//
//   ZeroMemory(&osix,
//              sizeof(osix));
//
//   osix.dwOSVersionInfoSize = sizeof(osix);
//   osix.dwMajorVersion      = Major;
//   osix.dwMinorVersion      = Minor;
//   osix.wServicePackMajor   = SpMajor;
//   osix.wServicePackMinor   = SpMinor;
//
//   /* Build the condition mask */
//   dwMask = 0;
//   VER_SET_CONDITION(dwMask, VER_MAJORVERSION,     VER_GREATER_EQUAL);
//   VER_SET_CONDITION(dwMask, VER_MINORVERSION,     VER_GREATER_EQUAL);
//   VER_SET_CONDITION(dwMask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
//   VER_SET_CONDITION(dwMask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);
//
//   return ( VerifyVersionInfo(&osix,
//                              VER_MAJORVERSION|VER_MINORVERSION|VER_SERVICEPACKMAJOR|VER_SERVICEPACKMINOR,
//                              dwMask) );
//}

//#ifndef InterlockedExchangeAdd64
//inline LONGLONG __cdecl InterlockedExchangeAdd64( LONGLONG volatile* Addend, LONGLONG Value ) throw()
//{
//   LONGLONG Old;
//
//   do
//   {
//      Old = (*Addend);
//   }
//   /* Explicitly use the intrinsic definition */
//   while ( Old != _InterlockedCompareExchange64(Addend, Old + Value, Old) );
//
//   return ( Old );
//}
//#endif /* InterlockedExchangeAdd64 */

__forceinline
LONG
InterlockedSetClearBits(
   __inout LONG volatile* Flags,
   __in LONG SetFlags,
   __in LONG ClearFlags
)
{
   LONG OldFlags;
   LONG NewFlags;
   
   do
   {
      OldFlags = (*Flags);
      NewFlags = (OldFlags & ~ClearFlags);
      NewFlags = (NewFlags | SetFlags);

   }
   while ( OldFlags != InterlockedCompareExchange(Flags,
                                                  NewFlags,
                                                  OldFlags) );

   return ( OldFlags );
}

__forceinline
SHORT
InterlockedSetClearBits16(
   __inout SHORT volatile* Flags,
   __in SHORT SetFlags,
   __in SHORT ClearFlags
)
{
   SHORT OldFlags;
   SHORT NewFlags;

   do
   {
      OldFlags = (*Flags);
      NewFlags = (OldFlags & ~ClearFlags);
      NewFlags = (NewFlags | SetFlags);
   }
   while ( OldFlags != InterlockedCompareExchange16(Flags,
                                                    NewFlags,
                                                    OldFlags) );

   return ( OldFlags );
}

/**********************************************************************
    
    Version helpers to avoid version.dll

 **********************************************************************/

/* Ignore alignment warnings */
#pragma warning( push )
#pragma warning( disable : 4324 )

/* Controls the alignment of a version resource structure which
 * must be aligned on 32-bit boundries */
#define __versionAlign __declspec(align(4))

/**********************************************************************
 *  VsInfoItemT
 *      In memory representation of a version resource structure. 
 */
template < size_t cchKey > struct __versionAlign VsInfoItemT
{
    /* These fields exist for all version types */
    WORD  wLength;
    WORD  wValueLength;
    WORD  wType;
    /* This is variable but always terminated by a L'\0' */
    WCHAR szKey[cchKey];
    /* This is variable, to align the struct on a 32-bit boundry,
     * and the actual value data follows */
    WORD  Padding1;

    /* Calcuates the size without padding */
    size_t SizeOf( ) const throw()
    {
        return ( (size_t)(DWORD_PTR)&Padding1 - (DWORD_PTR)this );
    }

    /* Returns TRUE if a value follows this structure */
    BOOL HasValue( ) const throw()
    {
        return ( wValueLength != 0 ? TRUE : FALSE );
    }
};

/*  StringTable resource
 */
struct __versionAlign StringTable : public VsInfoItemT<_charsof(L"00000000")>
{
};

/*  Var resource
 */
struct __versionAlign Var : public VsInfoItemT<_charsof(L"Translation")>
{
};

/*  VarFileInfo resource
 */
struct __versionAlign VarFileInfo : public VsInfoItemT<_charsof(L"VarFileInfo")>
{
};

/* VS_VERSIONINFO resource. This is the root of a VT_RESOURCE type.
 */
struct __versionAlign VsVersionInfo : public VsInfoItemT<_charsof(L"VS_VERSION_INFO")>
{
    /* Next in line - Note that if wValueLength is 0, then this doesn't exist
     */
    VS_FIXEDFILEINFO Value;
    
    union
    {
        VsInfoItemT<_charsof(L"StringFileInfo")> StringFileInfo;
        VsInfoItemT<_charsof(L"VarFileInfo")>    VarFileInfo;
    };

    BOOL HasExtraInfo( ) const throw()
    {
        if ( wLength > (SizeOf() + sizeof(VS_FIXEDFILEINFO)) )
        {
            return ( TRUE );
        }
        return ( FALSE );
    }

    BOOL HasStringInfo( ) const throw()
    {
        return ( HasExtraInfo() && 0 == lstrcmpW(StringFileInfo.szKey, L"StringFileInfo") ? TRUE : FALSE );
    }

    BOOL HasVarInfo( ) const throw()
    {
        return ( HasExtraInfo() && 0 == lstrcmpW(VarFileInfo.szKey, L"VarFileInfo") ? TRUE : FALSE );
    }
};
#pragma warning( pop )

inline VsVersionInfo* GetModuleVersionInfo( HMODULE hModule ) throw()
{
   HRSRC          hResInfo;
   HGLOBAL        hResData;
   VsVersionInfo* pVerInfo;

   hResInfo = FindResource(hModule,
                           MAKEINTRESOURCE(VS_VERSION_INFO),
                           RT_VERSION);

   if ( !hResInfo )
   {
      /* Failure */
      return ( NULL );
   }

   hResData = LoadResource(hModule,
                           hResInfo);

   if ( !hResData )
   {
      /* Failure */
      return ( NULL );
   }

   pVerInfo = reinterpret_cast<VsVersionInfo*>(LockResource(hResData));
   /* Success */
   return ( pVerInfo );
}

inline PIMAGE_NT_HEADERS GetModuleImageNtHeaders( HMODULE hModule ) throw()
{
   PIMAGE_DOS_HEADER pDosHeader;
   PIMAGE_NT_HEADERS pNtHeaders;
   
   pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
   if ( IMAGE_DOS_SIGNATURE != pDosHeader->e_magic )
   {
      /* Failure */
      return ( NULL );
   }

   pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(reinterpret_cast<ULONG_PTR>(hModule) + pDosHeader->e_lfanew);
   if ( IMAGE_NT_SIGNATURE != pNtHeaders->Signature )
   {
      /* Failure */
      return ( NULL );
   }

#ifdef _WIN64
   if ( IMAGE_NT_OPTIONAL_HDR64_MAGIC != pNtHeaders->OptionalHeader.Magic )
#else
   if ( IMAGE_NT_OPTIONAL_HDR32_MAGIC != pNtHeaders->OptionalHeader.Magic )
#endif
   {
      /* Failure */
      return ( NULL );
   }

   /* Success */
   return ( pNtHeaders );
}

inline HMODULE GetModuleHandleFromAddress( PVOID pAddress ) throw()
{
   MEMORY_BASIC_INFORMATION MemInfo;

   VirtualQuery(pAddress, 
                &MemInfo, 
                sizeof(MemInfo));

   return ( static_cast<HMODULE>(MemInfo.AllocationBase) );
}

inline HMODULE LoadLibraryByAddress( PVOID pAddress ) throw()
{
   HMODULE hModule;
   WCHAR   chModule[MAX_PATH];

   hModule = NULL;
   ZeroMemory(chModule,
              sizeof(chModule));

   if ( 0 != GetModuleFileName(GetModuleHandleFromAddress(pAddress),
                               chModule,
                               _countof(chModule)) )
   {
      hModule = LoadLibrary(chModule);
   }

   /* Success / Failure */
   return ( hModule );
}

#endif /* __UTILITY_H__ */
