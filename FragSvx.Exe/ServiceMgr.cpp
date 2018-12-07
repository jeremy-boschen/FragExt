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
 
/* ServiceMgr.cpp
 *    CFSxServiceManager implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "ServiceMgr.h"
#include "DiskDefrag.h"
/*++
   WPP
 --*/
#include "ServiceMgr.tmh"

#ifdef _DEBUG
#include <GuidDb.h>
#endif /* _DEBUG */

/*++

   IServiceProvider 

 --*/
STDMETHODIMP
CFSxServiceManager::QueryService( 
   REFGUID guidService,
   REFIID riid,
   void** ppvObject
)
{
   HRESULT hr;

   if ( !CoEnterExternalCall() ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxServiceManager,
              L"CoEnterExternalCall failed, CO_E_SERVER_STOPPING\n");

      /* Failure */
      return ( CO_E_SERVER_STOPPING );
   }

   __try {
      hr = InternalQueryService(guidService,
                                riid,
                                ppvObject);

      if ( FAILED(hr) ) {
         FpTrace(TRACE_LEVEL_ERROR,
                 SvxServiceManager,
                 L"InternalQueryService failed, guidService = %!GUID!, riid = %!GUID!, ppvObject = %p, hr = %!HRESULT!\n",
                 &guidService,
                 &riid,
                 ppvObject,
                 hr);
      }
   }
   __finally {
      CoLeaveExternalCall();
   }

   /* Success / Failure */
   return ( hr );
}

HRESULT 
CFSxServiceManager::InternalQueryService( 
   REFGUID guidService,
   REFIID riid,
   void** ppvObject
)
{   
   HRESULT hr;

   if ( !ppvObject ) {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxServiceManager,
              L"Invalid parameter, ppvObject = NULL, exiting\n");

      /* Failure */
      return ( E_POINTER );
   }

   /* Initialize outputs */
   (*ppvObject) = NULL;

   if ( InlineIsEqualGUID(guidService, 
                          __uuidof(FSxFileDefragmenter)) ) {
      /*
       * Create a new file defragmenter for the caller
       */
      hr = CFSxDefragmenter::CreateCoClassInstance(NULL,
                                                   riid,
                                                   ppvObject);

      if ( FAILED(hr) ) {
         FpTrace(TRACE_LEVEL_ERROR,
                 SvxServiceManager,
                 L"CFSxDefragmenter::CreateCoClassInstance failed, riid = %!GUID!, ppvObject = %p, hr = %!HRESULT!\n",
                 &riid,
                 ppvObject,
                 hr);
      }
   }
   else {
      FpTrace(TRACE_LEVEL_ERROR,
              SvxServiceManager,
              L"Unsupported interface requested, guidService = %!GUID!, riid = %!IID!\n",
              &guidService,
              &riid);
               
      /* Failure */
      hr = E_NOINTERFACE;
   }

   /* Success / Failure */
   return ( hr );
}
