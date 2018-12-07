/* FragExt - Shell extension for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2006 Jeremy Boschen. All rights reserved.
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

/* DelayLoad.h
 *   Enhancements for delay-loading DLLs
 *
 * Copyright (C) 2004-2006 Jeremy Boschen
 *
 * Version History
 *   0.0.001 - 09/20/2005 - Created
 */

#pragma once

#include <delayimp.h>
#pragma comment(lib, "delayimp")

typedef struct _DELAYLOADENTRY
{
   LPCSTR  szDll;
   LPCSTR  szProc;
   FARPROC pfnFallback;
}DELAYLOADENTRY;

#pragma section("DLT$__a", read, shared)
#pragma section("DLT$__m", read, shared)
#pragma section("DLT$__z", read, shared)

extern "C"
{
   __declspec(selectany) __declspec(allocate("DLT$__a")) _DELAYLOADENTRY* __pdlemEntryFirst = NULL;
   __declspec(selectany) __declspec(allocate("DLT$__z")) _DELAYLOADENTRY* __pdlemEntryLast  = NULL;
}

#pragma comment(linker, "/merge:DLT=.rdata")

#ifdef _WIN64
   #define _DELAYLOAD_INCLUDE_SYMBOL( _procname ) \
      __pragma(comment(linker, "/include:__pdle" #_procname)); 
   #define _DELAYLOAD_SYSTEMDIR_TAGBIT \
      0x1000000000000000i64U
#else
   #define _DELAYLOAD_INCLUDE_SYMBOL( _procname ) \
      __pragma(comment(linker, "/include:___pdle" #_procname)); 
   #define _DELAYLOAD_SYSTEMDIR_TAGBIT \
      0x10000000U
#endif

#define _DELAYLOAD_DELAY_LIBRARY( _dllname ) \
   __pragma(comment(linker, "/DELAYLOAD:" #_dllname))

/**
 * DELAYLOAD_ENTRY_NAME
 */
#define DELAYLOAD_ENTRY_NAME( _dllname, _loadFromSystemDirectory, _procname, _fallback ) \
   __declspec(selectany) _DELAYLOADENTRY __dle##_procname = {#_dllname, #_procname, (FARPROC)((ULONG_PTR)_fallback|(_loadFromSystemDirectory?_DELAYLOAD_SYSTEMDIR_TAGBIT:0))}; \
   extern "C" __declspec(allocate("DLT$__m")) __declspec(selectany) _DELAYLOADENTRY* const __pdle##_procname = &__dle##_procname; \
   _DELAYLOAD_INCLUDE_SYMBOL(_procname) \
   _DELAYLOAD_DELAY_LIBRARY(_dllname)

/**
 * DELAYLOAD_ENTRY_ORDINAL
 */
#define DELAYLOAD_ENTRY_ORDINAL( _dllname, _loadFromSystemDirectory, _procname, _fallback ) \
   __declspec(selectany) _DELAYLOADENTRY __dle##_procname = {#_dllname, MAKEINTRESOURCE(_procname), (FARPROC)((ULONG_PTR)_fallback|(_loadFromSystemDirectory?_DELAYLOAD_SYSTEMDIR_TAGBIT:0))}; \
   extern "C" __declspec(allocate("DL$__m")) __declspec(selectany) _DELAYLOADENTRY* const __pdle##_procname = &__dle##_procname; \
   _DELAYLOAD_INCLUDE_SYMBOL(_procname)


/**
 * DECLARE_DELAYLOAD_NOTIFYHOOK
 */
#ifdef _delayimp_h
   #define __HOOKDECLTYPE const PfnDliHook
#else /* _delayimp_h */
   #define __HOOKDECLTYPE PfnDliHook
#endif /* _delayimp_h */

#define DECLARE_DELAYLOAD_NOTIFYHOOK( _pfn ) \
      __HOOKDECLTYPE __pfnDliNotifyHook2 = _pfn;

/** VC8 removed these from delayimp.h **/
#ifndef _delayimp_h
__inline unsigned IndexFromPImgThunkData( PCImgThunkData pitdCur, PCImgThunkData pitdBase )
{
	return ( (unsigned) (pitdCur - pitdBase) );
}

#if defined(__cplusplus)
template <class X>
X * PFromRva(RVA rva, const X *) {
    return (X*)(PBYTE(&__ImageBase) + rva);
    }
#else
__inline
void *
WINAPI
PFromRva(RVA rva, void *unused) {
    return (PVOID)(((PBYTE)&__ImageBase) + rva);
    }
#endif

#endif /* _DELAY_IMP_VER */

/**
 * CDelayLoadModuleT
 */
template < class T >
class CDelayLoadModuleT
{
public:
   static FARPROC WINAPI NotifyProcedure( unsigned dliNotify, PDelayLoadInfo pdli ) {
      return ( T::_DelayLoadNotifyProc(dliNotify, pdli) );
   }

protected:
   /**
    * _DelayLoadNotifyProc 
    *    Delay-load notification procedure.
    *
    * Remarks
    *   This implementation works in two parts. The first is the handling of the
    *   dliStartProcessing notification. When it is received the table of 
    *   _DELAYLOADENTRY structures built by the DELAYLOAD_ENTRY_XXX macros is 
    *   scanned for an entry that matches both the DLL name and the procedure 
    *   name or ordinal passed by __delayLoadHelper2(). If a match is found the 
    *   function calls LoadLibrary() and GetProcAddress() for the DLL and 
    *   procedure being requested. If both functions succeed NULL is returned to
    *   instruct __delayLoadHelper2() to go about its normal workflow. If either
    *   functions fail, the function first calls FreeLibrary() if necessary to 
    *   free the module loaded via LoadLibrary() then overwrites the IAT entry 
    *   and returns a fallback function that must EXACTLY match the function 
    *   signature of the function it is replacing. If the signatures do not 
    *   match, the results are undefined. Future calls through the IAT entry will 
    *   go directly to the fallback. If no fallback function was provided, NULL 
    *   is essentially returned and the delay-load mechanism moves forward with 
    *   its normal behavior for a missing DLL/procedure. 
    *        
    *   The second part is when the final dliNoteEndProcessing notification is 
    *   sent. For this notification the function checks the hmodCur and pfnCur 
    *   members of the PDelayLoadInfo structure to determine if they hold valid
    *   values. If they do the function then rescans the _DELAYLOADENTRY table 
    *   and if a matching entry is again found the function assumes that it must
    *   have been previously called with the dliStartProcessing notification 
    *   during which it successfully calld LoadLibrary() on the DLL, thereby 
    *   incrementing its reference count. To keep things it order it then calls 
    *   FreeLibrary() on the hmodCur value to decrement the reference count added.
    */
   static FARPROC WINAPI _DelayLoadNotifyProc( unsigned dliNotify, PDelayLoadInfo pdli ) throw() {
      FARPROC pfn = NULL;

      switch ( dliNotify ) {
         case dliStartProcessing: {
            for ( _DELAYLOADENTRY** ppEntry = &__pdlemEntryFirst + 1; ppEntry < &__pdlemEntryLast; ppEntry++ ) {
               if ( (NULL != (*ppEntry)) && IsMatchingEntry((*ppEntry), pdli) ) {
                  /* Look for the module name... */
                  HMODULE hModule = LoadModule(pdli->szDll, 0 != ((ULONG_PTR)(*ppEntry)->pfnFallback & _DELAYLOAD_SYSTEMDIR_TAGBIT) ? TRUE : FALSE);

                  /* Look for the procedure... */
                  if ( NULL != hModule ) {
                     if ( IsBoundProcedure(hModule, pdli) || (NULL != GetProcAddress(hModule, (*ppEntry)->szProc)) ) {
                        /* The module and proc are valid, let the delay-load helper
                         * do its normal work, but keep the module around to speed
                         * things up. FreeLibrary will be then be called when this
                         * procedure receives dliNoteEndProcessing */

                        /* Success - Allow procedure fixup */
                        break;
                     }

                     /* Lose the ref-count on the module... */
                     FreeLibrary(hModule);
                  }

                  /* At this point, the proc or module is invalid so the fallback
                   * is going to be used. To prevent this procedure from being
                   * called again, the fallback is also written into the UAT. If
                   * no fallback was provided, then this effectively does nothing. */
                  (*pdli->ppfn) = pfn = (FARPROC)((ULONG_PTR)(*ppEntry)->pfnFallback & ~_DELAYLOAD_SYSTEMDIR_TAGBIT);
                  
                  /* Success - fallback */
                  break;
               }
            }

            break;
         }
#if !defined(_USRDLL) && !defined(_WINDLL)
         /* THIS MUST BE IGNORED FOR DLL BUILDS! */
         case dliNoteEndProcessing: {
            if ( (NULL != pdli->hmodCur) && (NULL != pdli->pfnCur) ) {
               for ( _DELAYLOADENTRY** ppEntry = &__pdlemEntryFirst + 1; ppEntry < &__pdlemEntryLast; ppEntry++ ) {
                  if ( (NULL != (*ppEntry)) && IsMatchingEntry((*ppEntry), pdli) ) {
                     /* Decrement the reference count created for dliStartProcessing */
                     FreeLibrary(pdli->hmodCur);

                     /* Success */
                     break;
                  }
               }
            }

            break;
         }
#endif /* _USRDLL && _WINDLL */
      }
      return ( pfn );
   }

   static bool IsMatchingEntry( _DELAYLOADENTRY* pdle, PDelayLoadInfo pdli ) throw() {
      if ( 0 == T::CompareModuleString(pdle->szDll, pdli->szDll) ) {
         if ( pdli->dlp.fImportByName ) {
            return ( 0 == T::CompareProcedureString(pdle->szProc, pdli->dlp.szProcName) ? true : false );
         }

         return ( pdli->dlp.dwOrdinal == IMAGE_ORDINAL((DWORD_PTR)pdle->szProc) ? true : false );
      }

      return ( false );
   }

   static bool IsBoundProcedure( HMODULE hModule, PDelayLoadInfo pdli ) throw() {
      PCImgDelayDescr pidd = pdli->pidd;

      if ( pidd->rvaBoundIAT && pidd->dwTimeStamp ) {
         PIMAGE_NT_HEADERS pinh = PIMAGE_NT_HEADERS( PBYTE(hModule) + PIMAGE_DOS_HEADER(hModule)->e_lfanew );

         if ( (IMAGE_NT_SIGNATURE == pinh->Signature) && (pinh->FileHeader.TimeDateStamp == pidd->dwTimeStamp) && UINT_PTR(hModule) == pinh->OptionalHeader.ImageBase ) {
            PCImgThunkData pBoundIAT = PFromRva(pidd->rvaBoundIAT, (PCImgThunkData)NULL);
            const unsigned iIAT      = IndexFromPImgThunkData(PCImgThunkData(pdli->ppfn), pBoundIAT);

            return ( NULL != FARPROC(UINT_PTR(pBoundIAT[iIAT].u1.Function)) ? true : false );
         }
      }

      return ( false );
   }

   static int CompareModuleString( const char* sz1, const char* sz2 ) throw() {
      return ( lstrcmpiA(sz1, sz2) );
   }

   static int CompareProcedureString( const char* sz1, const char* sz2 ) throw() {
      return ( lstrcmpA(sz1, sz2) );
   }

   static HMODULE LoadModule( LPCSTR ModuleName, BOOLEAN FromSystemDirectory ) throw() {
      UINT cchRet;
      char chModulePath[MAX_PATH*2];

      if ( FromSystemDirectory ) {
         cchRet = GetSystemWindowsDirectoryA(chModulePath,
                                             ARRAYSIZE(chModulePath));

         if ( (cchRet > 0) && (cchRet < ARRAYSIZE(chModulePath)) ) {
            if ( '\\' != chModulePath[cchRet] ) {
               chModulePath[cchRet] = '\\';
               cchRet              += 1;
            }

            while ( (cchRet < ARRAYSIZE(chModulePath)) && ('\0' != *(ModuleName)) ) {
               chModulePath[cchRet] = (*ModuleName);
               cchRet += 1;
               ModuleName += 1;
            }

            if ( '\0' != (*ModuleName) ) {
               SetLastError(ERROR_INSUFFIENT_BUFFER);
               return ( NULL );
            }

            ModuleName = chModulePath;
         }
      }
   }
};
