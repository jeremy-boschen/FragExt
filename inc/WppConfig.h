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
 
/* WppConfig.h
 *    WPP tracing configuration file
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __WPPCONFIG_H__
#define __WPPCONFIG_H__

#define FRAGEXT_TRACING_ID \
   L"jBoschen\\FragExt"

#define FRAGSVX_TRACING_ID \
   FRAGEXT_TRACING_ID

#define FRAGMGX_TRACING_ID \
   FRAGEXT_TRACING_ID

#define WPP_CONTROL_GUID_FRAGLIB \
   WPP_DEFINE_CONTROL_GUID(FragLibTraceGuid, (ADF4C350,1551,4C9D,A92D,D40183C0B24A), \
      WPP_DEFINE_BIT(FxlDebug)            \
      WPP_DEFINE_BIT(FpDirectory)         \
      WPP_DEFINE_BIT(FxlPathing)          \
      WPP_DEFINE_BIT(FxlLocking)          \
      WPP_DEFINE_BIT(FxlSecurity)         \
      WPP_DEFINE_BIT(FxlThreading)        \
      WPP_DEFINE_BIT(FpDataStream)        \
      WPP_DEFINE_BIT(FpCacheList)         \
      WPP_DEFINE_BIT(FpCOMLibrary)        \
   )

#define WPP_CONTROL_GUID_FRAGENG \
   WPP_DEFINE_CONTROL_GUID(FragEngTraceGuid, (ADF4C351,1551,4C9D,A92D,D40183C0B24A), \
      WPP_DEFINE_BIT(FpFragContext)       \
   )

#define WPP_CONTROL_GUID_COFRAG \
   WPP_DEFINE_CONTROL_GUID(FragEngTraceGuid, (ADF4C352,1551,4C9D,A92D,D40183C0B24A), \
      WPP_DEFINE_BIT(FpFragContext)       \
   )

#define WPP_CONTROL_GUID_FRAGSVX \
   WPP_DEFINE_CONTROL_GUID(FragSvxTraceGuid, (ADF4C353,1551,4C9D,A92D,D40183C0B24A), \
      WPP_DEFINE_BIT(SvxDebug)            \
      WPP_DEFINE_BIT(SvxStartup)          \
      WPP_DEFINE_BIT(SvxShutdown)         \
      WPP_DEFINE_BIT(SvxSecurity)         \
      WPP_DEFINE_BIT(SvxMemory)           \
      WPP_DEFINE_BIT(SvxService)          \
      WPP_DEFINE_BIT(SvxCOM)              \
      WPP_DEFINE_BIT(SvxServiceManager)   \
      WPP_DEFINE_BIT(SvxFileDefragmenter) \
   )

#define WPP_CONTROL_GUID_FRAGMGX \
   WPP_DEFINE_CONTROL_GUID(FragMgxTraceGuid, (ADF4C354,1551,4C9D,A92D,D40183C0B24A), \
      WPP_DEFINE_BIT(FpDebug)       \
      WPP_DEFINE_BIT(FpStartup)     \
      WPP_DEFINE_BIT(FpShutdown)    \
      WPP_DEFINE_BIT(FpSecurity)    \
      WPP_DEFINE_BIT(FpMemory)      \
      WPP_DEFINE_BIT(FpFileQueue)   \
   )

/*
 * Stdafx.h for each project contains a PRJ_WPP_CONTROL_GUIDS macro that expands
 * to the WPP control guids used
 */
#define WPP_CONTROL_GUIDS  \
   PRJ_WPP_CONTROL_GUIDS

#define WPP_FLAG_LEVEL_LOGGER(_Flag, _Level) \
   WPP_LEVEL_LOGGER(_Flag)

#define WPP_FLAG_LEVEL_ENABLED(_Flag, _Level) \
   (WPP_LEVEL_ENABLED(_Flag) && WPP_CONTROL(WPP_BIT_ ## flag).Level >= _Level)

#define WPP_LEVEL_FLAGS_LOGGER(_Level, _Flags) \
   WPP_LEVEL_LOGGER(_Flags)
   
#define WPP_LEVEL_FLAGS_ENABLED(_Level, _Flags) \
   (WPP_LEVEL_ENABLED(_Flags) && WPP_CONTROL(WPP_BIT_ ## _Flags).Level >= _Level)


/*++
   
   WPP Configuration Section

 --*/

//
// Custom type configuration
//
// begin_wpp config
// CUSTOM_TYPE(SVCSTATE,ItemListLong(UNDEFINED,SERVICE_STOPPED,SERVICE_START_PENDING,SERVICE_STOP_PENDING,SERVICE_RUNNING,SERVICE_CONTINUE_PENDING,SERVICE_PAUSE_PENDING,SERVICE_PAUSED) );
// CUSTOM_TYPE(SVCCTL,ItemListLong(SERVICE_CONTROL_STOP,SERVICE_CONTROL_PAUSE,SERVICE_CONTROL_CONTINUE,SERVICE_CONTROL_INTERROGATE,SERVICE_CONTROL_SHUTDOWN,SERVICE_CONTROL_PARAMCHANGE,SERVICE_CONTROL_NETBINDADD,SERVICE_CONTROL_NETBINDREMOVE,SERVICE_CONTROL_NETBINDENABLE,SERVICE_CONTROL_NETBINDDISABLE,SERVICE_CONTROL_DEVICEEVENT,SERVICE_CONTROL_HARDWAREPROFILECHANGE,SERVICE_CONTROL_POWEREVENT,SERVICE_CONTROL_SESSIONCHANGE,SERVICE_CONTROL_PRESHUTDOWN,SERVICE_CONTROL_TIMECHANGE,SERVICE_CONTROL_TRIGGEREVENT) );
// end_wpp
//

//
// Trace function configuration
//
// begin_wpp config
// FUNC FpTrace(LEVEL,FLAGS,MSG,...);
// FUNC SvxTrace(LEVEL,FLAGS,MSG,...);
// end_wpp
//

#ifdef _FX_WPPDEBUG
#define WPP_DEBUG( _Msg ) \
   _WPP_DbgPrintf _Msg

__inline
VOID
_cdecl
_WPP_DbgPrintf(
   LPCWSTR Format,
   ...
)
{
   va_list args;
   WCHAR   buff[512];
   
   va_start(args,
            Format);
   
   if ( SUCCEEDED(StringCchVPrintfW(buff,
                                    ARRAYSIZE(buff),
                                    Format,
                                    args)) )
   {
      OutputDebugStringW(buff);
   }

   va_end(args);
}
#endif /* _FX_WPPDEBUG */

#endif /* __WPPCONFIG_H__ */
