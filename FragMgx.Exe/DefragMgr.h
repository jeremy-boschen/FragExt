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

/* DefragMgr.h
 *    Implements the DefragManager object, exposed via COM
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#include "FragMgx.h"

class ATL_NO_VTABLE CDefragManager : public CComObjectRootEx<CComMultiThreadModel>,
                                     public CComCoClass<CDefragManager, &CLSID_DefragManager>,
                                     public IDefragManager
{
   /* ATL */
public:   
#ifdef _DEBUG
   DECLARE_REGISTRY_RESOURCEID(IDR_FRAGMGX)
#else /* _DEBUG */
   DECLARE_NO_REGISTRY()
#endif /* _DEBUG */

   DECLARE_NOT_AGGREGATABLE(CDefragManager);

   BEGIN_COM_MAP(CDefragManager)
      COM_INTERFACE_ENTRY(IDefragManager)
   END_COM_MAP()

   HRESULT FinalConstruct( ) throw();
   void FinalRelease( ) throw();

   /* IDefragManager */
public:
   HRESULT STDMETHODCALLTYPE QueueFile( __RPC__in_string LPCWSTR pwszFile ) throw();

   /* CDefragManager */
private:
   enum 
   {
      eTraceInfo = 1
   };
};

OBJECT_ENTRY_AUTO(CLSID_DefragManager, CDefragManager)
