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
 
/* DbgUtil.h
 *    Debugging utilities
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include <tchar.h>
#include <crtdbg.h>
#include <strsafe.h>

#define DBGF_ERROR         1
#define DBGF_WARNING       2
#define DBGF_INFORMATION   3
#define DBGF_VERBOSE       4

/*
 * Global DbgPrintfEx level. Calls to DbgPrintfEx with a level greater than
 * this value will not be processed. When it is equal to 0, no calls are
 * processed
 */
extern UCHAR g_DbgPrintFilter;

/*++
   DbgPrint variants
      OutputDebugString wrapper with support for printf style formatting
 
 --*/
void
__cdecl 
DbgPrintfExA( 
   __in UCHAR Level,
   __in_z LPCSTR Format, 
   ... 
);

void 
__cdecl 
DbgPrintfExW( 
   __in UCHAR Level,
   __in_z LPCWSTR Format, 
   ... 
);

#define DbgPrintfW(Format, ...) \
   DbgPrintfExW(DBGF_VERBOSE, Format, __VA_ARGS__)

#define DbgPrintfA(Format, ...) \
   DbgPrintfExA(DBGF_VERBOSE, Format, __VA_ARGS__)

#if defined(UNICODE) || defined(_UNICODE)
   #define DbgPrintfEx DbgPrintfExW
   #define DbgPrintf   DbgPrintfW
#else /* defined(UNICODE) || defined(_UNICODE) */
   #define DbgPrintfEx DbgPrintfExA
   #define DbgPrintf   DbgPrintfA
#endif /* defined(UNICODE) || defined(_UNICODE) */

/**
 * CSystemMessage
 *		Wrapper of the FormatMessage function.
 */
template < SIZE_T cchMsgBuf = 512 > 
class CSystemMessage
{
	/* Interface */
public:
	CSystemMessage( 
   ) throw()
	{
	    ZeroMemory(_chMsgBuf, 
                  cchMsgBuf);
	}

	CSystemMessage( 
      __in_opt LPCVOID lpSource, 
      __in DWORD dwMessageId, 
      __in DWORD dwLanguageId = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
      __in_opt va_list* Arguments = NULL 
   ) throw()
	{
      LPTSTR pszChar;

      ZeroMemory(_chMsgBuf, 
                 cchMsgBuf);

      if ( 0 != Format(lpSource, 
                       dwMessageId, 
                       dwLanguageId, 
                       Arguments) )
      {
         pszChar = _chMsgBuf;

         while ( _T('\0') != (*pszChar) )
         {
            pszChar += 1;
         }

         while ( pszChar != _chMsgBuf )
         {
            if ( (_T('\n') == (*pszChar)) || (_T('\r') == (*pszChar)) )
            {
               (*pszChar) = _T('\0');
            }
            
            pszChar--;
          }
      }
   }

	/**
	 *  Format
	 *      Wraps FormatMessage.
	 */
   DWORD 
   Format( 
      __in_opt LPCVOID lpSource, 
      __in DWORD dwMessageId,
      __in DWORD dwLanguageId, 
      __in_opt va_list* Arguments
   ) throw()
	{        
	   return ( ::FormatMessage((lpSource ? 0 : FORMAT_MESSAGE_FROM_SYSTEM)|(Arguments ? 0 : FORMAT_MESSAGE_IGNORE_INSERTS), 
                               lpSource, 
                               dwMessageId, 
                               dwLanguageId, 
                               _chMsgBuf, 
                               static_cast<DWORD>(cchMsgBuf), 
                               Arguments) );
	}

	operator LPTSTR( ) throw()
	{
	    return ( &(_chMsgBuf[0]) );
	}

	TCHAR _chMsgBuf[cchMsgBuf];

	/* Restricted */
private:    
	/* Copy construction and copy-assignment are not supported */
	CSystemMessage( const CSystemMessage& );
	CSystemMessage& operator =( const CSystemMessage& );
};

/**
 * SysErrorMsg / OleErrMsg / OleAutoErrorMsg
 *		Generic wrappers of CSystemMessage
 */
#define SysErrorMsg(c, m) \
	((LPTSTR)CSystemMessage<>((LPCVOID)(m), (DWORD)(c)))

#define OleErrorMsg(c) \
	((LPTSTR)CSystemMessage<>((LPCVOID)::GetModuleHandle(_T("ole32")), (DWORD)(c)))

#define OleAutoErrorMsg(c) \
	((LPTSTR)CSystemMessage<>((LPCVOID)::GetModuleHandle(_T("oleaut32")), (DWORD)(c)))

/**
 * CDbgTrace
 *		_RPT4 wrapper that includes file, function and line information
 *		in a format compatible with the VC++ line-jump format.
 *
 *		ie: c:\Path\File.ext(101) : xxx
 */
class CDbgTrace
{
public:
	CDbgTrace( LPCSTR pszFile, LPCSTR pszFunction, int iLine, bool bFileNameOnly ) : _pszFile(pszFile), _pszFunction(pszFunction), _iLine(iLine)
	{
	    if ( bFileNameOnly && pszFile )
	    {
	        /* Scan to the end... */
	        while ( '\0' != *_pszFile )
	        {
	            _pszFile++;
	        }
	        /* Back it up to the file spec... */
	        while ( '\\' != *_pszFile && pszFile != _pszFile )
	        {
	            _pszFile--;
	        }
	        /* If there was a \, we backed up one too many */
	        if ( pszFile != _pszFile )
	        {
	            _pszFile++;
	        }
	    }
	}

	void __cdecl operator ( )( LPCTSTR pszFormat, ... )
	{
      va_list args;

      va_start(args,
               pszFormat);

      _Report(pszFormat,
              args);

      va_end(args);
	}

   void __cdecl operator( )( int bOutput, LPCTSTR pszFormat, ... )
   {
      va_list args;

      if ( bOutput )
      {
         va_start(args,
                  pszFormat);

         _Report(pszFormat,
                 args);

         va_end(args);
      }
   }
      
   void _Report( LPCTSTR pszFormat, va_list args )
   {
      TCHAR chBuf[1024] = {_T('\0')};

      if ( SUCCEEDED(StringCchVPrintf(chBuf, _countof(chBuf), pszFormat, args)) )
      {
      #ifdef _UNICODE
         _RPT4(_CRT_WARN, "%hs(%d):%hs!%ls", _pszFile ? _pszFile : "", _iLine, _pszFunction ? _pszFunction : "", chBuf);
      #else
         _RPT4(_CRT_WARN, "%hs(%d):%hs!%s", _pszFile ? _pszFile : "", _iLine, _pszFunction ? _pszFunction : "", chBuf);
      #endif
      }
   }

private:
	LPCSTR _pszFile;
	LPCSTR _pszFunction;
	int    _iLine;

	/* Default construction, copy-construction and copy-assignment are not supported */
	CDbgTrace( );
	CDbgTrace( const CDbgTrace& );
	CDbgTrace& operator =( const CDbgTrace& );
};

/**
 * _Trace
 *		Emits output to the CRT debug device using only the filename 
 *		spec of the __FILE__
 */
#define _Trace \
	CDbgTrace(__FILE__, __FUNCTION__, __LINE__, true)

/**
 * _TraceEx
 *		Emits output to the CRT debug device using the full file path 
 *		spec of the __FILE__.
 */
#define _TraceEx \
	CDbgTrace(__FILE__, __FUNCTION__, __LINE__, false)

/**
 * TraceException
 *		Emits basic information about an SEH exception 
 */
inline 
void 
__stdcall 
_TraceException( 
   PEXCEPTION_POINTERS pxp, 
   LPCSTR pszFile, 
   LPCSTR pszFunction, 
   int iLine 
) throw()
{
	CDbgTrace(pszFile, 
             pszFunction, 
             iLine, 
             FALSE)(_T("Unhandled exception. Code(0x%08lx), Address(0x%p)\n"), 
                    pxp->ExceptionRecord->ExceptionCode, 
                    pxp->ExceptionRecord->ExceptionAddress);
}

inline 
void 
__stdcall 
_TraceException( 
) throw() 
{ 
}

/**
 * _DEBUG only versions of all the _TraceXXX macros. 
 */
#ifdef _DEBUG
	#define _dTrace            _Trace
	#define _dTraceEx          _TraceEx
	#define _dTraceException   _TraceException(GetExceptionInformation(), __FILE__, __FUNCTION__, __LINE__)
#else
	#define _dTrace            __noop
	#define _dTraceEx          __noop
	#define _dTraceException   _TraceException()
#endif

/*++
   GetPerformanceCounter
      Wrapper for QueryPerformanceCounter
  
    Return Value
      The lpPerformanceCount->QuadPart value of QueryPerformanceCounter.
 --*/
inline 
LONGLONG 
GetPerformanceCounter(
) throw()
{
   LARGE_INTEGER iCount = {0};
   ::QueryPerformanceCounter(&iCount);
   return ( iCount.QuadPart );
}

/*++
   GetPerformanceFrequency
      Wrapper for QueryPerformanceFrequency
  
   Return Value
      The lpFrequency->QuadPart value of QueryPerformanceFrequency.
 */
inline 
LONGLONG 
GetPerformanceFrequency(
) throw()
{
   LARGE_INTEGER iFreq = {0};
   ::QueryPerformanceFrequency(&iFreq);
   return ( iFreq.QuadPart );
}

class CPerfClock
{
public:
   LONGLONG StartTime;
   LONGLONG StopTime;
   double   TotalSeconds;
   double   TotalMiliseconds;
   double   TotalNanoseconds;

   CPerfClock( 
   ) throw()
   {
      StartTime        = 0;
      StopTime         = 0;
      TotalSeconds     = 0.0f;
      TotalMiliseconds = 0.0f;
   }

   void 
   Start(
   ) throw()
   {
      StartTime = GetPerformanceCounter();
   }

   void 
   Stop( 
   ) throw()
   {
      double dTotalTime;
      double dFrequency;

      StopTime   = GetPerformanceCounter();
      dTotalTime = static_cast<double>(StopTime) - static_cast<double>(StartTime);      
      dFrequency = static_cast<double>(GetPerformanceFrequency());
      
      TotalSeconds     = (dTotalTime / dFrequency);
      TotalMiliseconds = (dTotalTime / dFrequency) * 1000;
      TotalNanoseconds = (dTotalTime / dFrequency) * 1000000;
   }
};

class CPerfMonitor : public CPerfClock
{
public:
   CPerfMonitor(
      __in_z LPCTSTR pszFunction 
   ) throw() : _pszFunction(pszFunction)
   {
      Start();
   }

   ~CPerfMonitor(
   ) throw()
   {
      Stop();

      DbgPrintf(_T("%s!Ticks = %I64u, Miliseconds = %f\n"), 
                _pszFunction, 
                StopTime - StartTime, 
                TotalMiliseconds);
   }

   LPCTSTR _pszFunction;
};

class CPerfTracker
{
public:
   CPerfTracker( 
      __in_z LPCTSTR Function
   ) throw() : _pszFunction(Function)
   {
   }

   ~CPerfTracker( ) throw() 
   {
      DbgPrintf(_T("%s!Total = %f ms, Average = %f ms\n"), 
                _pszFunction, 
                (static_cast<double>(_TotalNanoseconds) / 1000000),
                (static_cast<double>(_TotalNanoseconds) / 1000000) / _HitCount);
   }

   LPCTSTR   _pszFunction;
   ULONGLONG _HitCount;
   ULONGLONG _TotalNanoseconds;
};

class CPerfHit : public CPerfClock
{
public:
   CPerfHit( CPerfTracker& Tracker ) throw() : _Tracker(Tracker)
   {
      Start();
   }

   ~CPerfHit( ) throw()
   {
      Stop();

      InterlockedExchangeAdd64(reinterpret_cast<volatile LONGLONG*>(&(_Tracker._HitCount)),
                               1);

      InterlockedExchangeAdd64(reinterpret_cast<volatile LONGLONG*>(&(_Tracker._TotalNanoseconds)),
                               static_cast<LONGLONG>(TotalNanoseconds));
   }

   CPerfTracker& _Tracker;

private:
   CPerfHit& operator=( const CPerfHit& );
};

void
APIENTRY
SetDebuggerThreadName( 
   __in DWORD dwThreadID, 
   __in LPCSTR pszThreadName 
);

BOOLEAN
APIENTRY
IsServiceExecutionMode(
);

/**
 * IsUnrestrictedProcess
 *    Checks if the current process is allowed to load this module.
 *
 * Parameters
 *    rgModules
 *       Pointer to an array of module names.
 *    cModules
 *       Number of elements in the array pointed to by rgModules
 *
 * Return Value
 *    Returns TRUE if any of the modules in the list are loaded in
 *    the current process, FALSE otherwise.
 *
 * Remarks
 *    None
 */
BOOLEAN
APIENTRY
IsUnrestrictedProcess( 
   LPCTSTR* rgModules, 
   size_t cModules 
);

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
