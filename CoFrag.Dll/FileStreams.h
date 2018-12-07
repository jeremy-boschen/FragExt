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

/* FileStreams.cpp
 *    CFileStream and CFileStreams implementations
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 10/31/2004 - Created
 */

#pragma once

#include <AtlEx.h>
using namespace ATLEx;
#include <FragEng.h>

#ifndef MAX_STREAM_PATH
   #define MAX_STREAM_PATH (MAX_PATH + 36)
#endif /* MAX_STREAM_PATH */

/**********************************************************************

    CFileStream

 **********************************************************************/

/**
 * CFileStream
 */
class ATL_NO_VTABLE CFileStream : public  CComObjectRootEx<CComMultiThreadModelNoCS>,
                                  public  IDispatchImpl<IFileStream, &IID_IFileStream, &LIBID_FragEng, 1, 0>,
                                  public  CoFreeThreadedMarshalerImpl<CFileStream>
{
   /* ATL */
public:
   DECLARE_NOT_AGGREGATABLE(CFileStream)

   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(CFileStream)
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IFileStream)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, reinterpret_cast<_ATL_CREATORARGFUNC*>(CoFTMImpl::QueryMarshalInterface))
   END_COM_MAP()

   /* IFileStream */
public:
   STDMETHOD(get_Name)( BSTR* pbszName );
   STDMETHOD(get_Size)( double* pdblSize );

   /* CFileStream */
public:
   static HRESULT CreateFromStreamInfo( LPCWSTR pszName, ULONGLONG cbStream, REFIID riid, void** ppObject ) throw();

protected:
   CFileStream( ) throw();

   ULONGLONG _cbSize;
   WCHAR     _chName[MAX_STREAM_PATH];
};

typedef CComObject<CFileStream> CoFileStream;

/**********************************************************************

    CFileStreams

 **********************************************************************/

/**
 * CFileStreams
 */
class ATL_NO_VTABLE CFileStreams : public CComObjectRootEx<CComMultiThreadModel>,
                                   public CComCoClass<CFileStreams, &CLSID_FileStreams>,
                                   public IDispatchImpl<IFileStreams, &IID_IFileStreams, &LIBID_FragEng, 1, 0>,
                                   public CoFreeThreadedMarshalerImpl<CFileStreams>
{
   /* ATL */
public:
   DECLARE_NO_REGISTRY()

   DECLARE_NOT_AGGREGATABLE(CFileStreams)

   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(CFileStreams)
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IFileStreams)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, reinterpret_cast<_ATL_CREATORARGFUNC*>(CoFTMImpl::QueryMarshalInterface))
   END_COM_MAP()

   void FinalRelease( );

   /* IFileStreams */
public:
   STDMETHOD(get__NewEnum)( IUnknown** ppEnum ) throw();
   STDMETHOD(get_Item)( long iIndex, IDispatch** ppStream ) throw();
   STDMETHOD(get_Count)( long* plResult ) throw();
   STDMETHOD(OpenFile)( BSTR bszPath, VARIANT_BOOL* pbOpened ) throw();

   /* CFileStreams */
public:
   CFileStreams( ) throw();

   /* !!!MUST BE CALLED INSIDE A LOCK!!! */
   void _ResetData( ) throw();
   static DWORD FRAGAPI _EnumFileStreamRoutine( ULONG StreamCount, ULONGLONG StreamSize, ULONGLONG StreamSizeOnDisk, LPCWSTR StreamName, PVOID Parameter ) throw();

   typedef struct _ENUMCTX
   {
      ULONG    Count;
      ULONG    Index;
      VARIANT* Streams;
   }ENUMCTX, *PENUMCTX;

   ULONG    _cElem;
   VARIANT* _rgStream;
};

OBJECT_ENTRY_AUTO(CLSID_FileStreams, CFileStreams)

/**********************************************************************

    CEnumFileStreams

 **********************************************************************/

class ATL_NO_VTABLE CEnumFileStreams : public CComObjectRootEx<CComMultiThreadModelNoCS>,
                                       public IEnumVARIANT,
                                       public CoFreeThreadedMarshalerImpl<CEnumFileStreams>
{
   /* ATL */
public:
   DECLARE_NOT_AGGREGATABLE(CEnumFileStreams)
   
   DECLARE_GET_CONTROLLING_UNKNOWN()

   BEGIN_COM_MAP(CEnumFileStreams)
      COM_INTERFACE_ENTRY(IEnumVARIANT)
      COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, reinterpret_cast<_ATL_CREATORARGFUNC*>(CoFTMImpl::QueryMarshalInterface))
   END_COM_MAP()

   void FinalRelease( );

   /* IEnumVARIANT */
public:
   STDMETHOD(Next)( ULONG celt, VARIANT* rgelt, ULONG* peltFetched ) throw();
   STDMETHOD(Skip)( ULONG celt ) throw();
   STDMETHOD(Reset)( ) throw();
   STDMETHOD(Clone)( IEnumVARIANT** ppEnum ) throw();

   /* CEnumFileStreams */
public:
   static HRESULT CreateInstance( VARIANT* rgStreams, ULONG cElem, ULONG cIter, REFIID riid, void** ppObject ) throw();

protected:   
   CEnumFileStreams( ) throw();
   void _ResetData( ) throw();

   typedef CComObject<CEnumFileStreams> CoEnumFileStreams;

private:
   ULONG    _cIter;
   ULONG    _cElem;
   VARIANT* _rgStream;
};
