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

/* DefragMgr.cpp
 *    Implements the DefragManager object, exposed via COM
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "DefragMgr.h"
#include "TaskMgr.h"

/**********************************************************************

   CDefragManager : ATL

 **********************************************************************/
HRESULT CDefragManager::FinalConstruct( ) throw()
{
   return ( S_OK );
}

void CDefragManager::FinalRelease( ) throw()
{
}

/**********************************************************************

   CDefragManager : IDefragManager

 **********************************************************************/
HRESULT STDMETHODCALLTYPE CDefragManager::QueueFile( __RPC__in_string LPCWSTR pwszFile ) throw()
{
   HRESULT hr;

   _dTrace(eTraceInfo, L"File!%s\n", pwszFile);

   hr = __pTaskManager->InsertQueueFile(pwszFile);

   return ( hr );
}
