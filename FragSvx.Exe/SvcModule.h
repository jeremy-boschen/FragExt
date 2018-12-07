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
 
/* Module.h
 *    Module definitions
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include <FragSvx.h>

class CServiceModule
{
public:

   static DWORD Initialize( ) throw();
   static DWORD Uninitialize( ) throw();

private:

   //static DWORD 
};

#if 0
#include <AtlEx.h>
#include <MUIModule.h>
#include <FragSvx.h>

/**
 *  CServiceModule
 *      Required wrapper for CAtlServiceModuleExT
 */
class CServiceModule : public ATLEx::CAtlServiceModuleExT<CServiceModule, IDS_SERVICENAME, IDS_DISPLAYNAME, IDS_DESCRIPTION>,
                       public CMUIModule
                     
{
   /* ATL */
public:
#ifdef _DEBUG
	DECLARE_LIBID(LIBID_FragSvx)
   DECLARE_REGISTRY_APPID_RESOURCEID(IDR_FRAGSVX, "{7C1A3EB5-37A0-4BC2-B0E5-F3C5BF1FCB5D}")
#else /* _DEBUG */
   DECLARE_NO_REGISTRY()
#endif /* _DEBUG */

   /* CAtlServiceModuleT */
public:
   HRESULT Start( int nShowCmd ) throw();
   void ServiceMain( DWORD dwArgc, LPTSTR* lpszArgv ) throw();
   HRESULT Run( int nShowCmd ) throw();
	HRESULT InitializeSecurity( ) throw();
   
   /* These get removed for release builds as all the registration information is in the MSI file */
   bool ParseCommandLine(LPCTSTR lpCmdLine, HRESULT* pnRetCode) throw();
   HRESULT RegisterAppId( bool bService = false ) throw();
   
   /* CAtlServiceModuleExT */
public:
   DWORD HandlerEx( DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext ) throw();

   /* CServiceModule */
public:
   CServiceModule( ) throw();
   ~CServiceModule( ) throw();

private:
   enum { eTraceLevelCOM = 0 };
};

__declspec(selectany) CServiceModule _AtlModule;
#endif //0