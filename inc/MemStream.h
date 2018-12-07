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

/* MemStream.h
 *    IStream interface on memory
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __MEMSTREAM_H__
#define __MEMSTREAM_H__

#ifndef PAGE_SIZE
   #define PAGE_SIZE 4096
#endif /* PAGE_SIZE */

/**
 * VirtualMemoryHeap
 *    A generic memory allocator which wraps the VirtualXxx API
 */
struct VirtualMemoryHeap
{
   static 
   void* 
   APIENTRY
   MemAllocate( 
      __in size_t cbSize 
   )
   {
      return ( VirtualAlloc(NULL, 
                            (SIZE_T)cbSize, 
                            MEM_COMMIT, 
                            PAGE_READWRITE) );
   }            

   static void* __stdcall MemReallocate( void* pv, size_t cbSizeOld, size_t cbSizeNew )
   {
      if ( !pv )
      {
         return ( VirtualMemoryHeap::MemAllocate(cbSizeNew) );
      }

      /* Like realloc, if size equals 0 and pv is not NULL, pv is freed and NULL
       * is returned */
      if ( (0 == cbSizeNew) && (NULL != pv) )
      {
         VirtualMemoryHeap::MemFree(pv);
         return ( NULL );
      }

      LPVOID pTmp = NULL;
      /* pv needs to be returned in the event of failure at this point because
       * both it and cbSizeNew are not NULL or 0 */
      LPVOID pRet = pv;

      __try
      {
         if ( NULL != (pTmp = VirtualMemoryHeap::MemAllocate(cbSizeNew)) )
         {
            /* ...contents of the block are unchanged up to the shorter of the new and old sizes */
            CopyMemory(pTmp, pv, min(cbSizeOld, cbSizeNew));
            
            /* Return the new block and setup the old one to be freed */
            pRet = pTmp;
            pTmp = pv;
         }
      }
      __finally
      {
         if ( NULL != pTmp )
         {
            VirtualMemoryHeap::MemFree(pTmp);
         }
      }

      return ( pRet );
   }

   static void __stdcall MemFree( void* pv )
   {
      VirtualFree(pv, 0, MEM_RELEASE);
   }
};

/**
 * CComMemHeap
 *    Generic allocator that uses the CoTaskMemXxx api.
 */
struct ComMemHeap
{
   static void* __stdcall MemAllocate( size_t cbSize )
   {
      return ( CoTaskMemAlloc((SIZE_T)cbSize) );
   }

   static void* __stdcall MemReallocate( void* pv, size_t /*cbSizeOld*/, size_t cbSizeNew )
   {
      return ( CoTaskMemRealloc(pv, (SIZE_T)cbSizeNew) );
   }

   static void __stdcall MemFree( void* pv )
   {
      CoTaskMemFree(pv);
   }
};


/**
 * CRTMemoryHeap
 *    Generic allocator that uses CRT memory API
 */
struct CRTMemoryHeap
{
   static void* __stdcall MemAllocate( size_t cbSize )
   {
      return ( malloc(cbSize) );
   }

   static void* __stdcall MemReallocate( void* pv, size_t /*cbSizeOld*/, size_t cbSizeNew )
   {
      return ( realloc(pv, cbSizeNew) );
   }

   static void __stdcall MemFree( void* pv )
   {
      free(pv);
   }
};

/**
 * IID_IMemoryStream
 *    {F823C533-D357-4635-BBF2-993B00D7BD5B}
 */
static const IID IID_IMemoryStream = {0xF823C533, 0xD357, 0x4635, {0xBB, 0xF2, 0x99, 0x3B, 0x0, 0xD7, 0xBD, 0x5B}};

class __declspec(novtable) __declspec(uuid("F823C533-D357-4635-BBF2-993B00D7BD5B"))
IMemoryStream : public IStream
{
public:
   STDMETHOD(GetMemoryInfo)( LPVOID* ppAddress, ULONG* pcbAllocated ) = 0;
};

/**
 * CMemoryStreamT
 *    A minimal IMemoryStream implementation.
 *
 * Parameters:
 *    _Allocator
 *       A class which implements 3 basic functions. MemAllocate, MemReallocate 
 *       and MemFree. The class must also define a typedef _ThreadModel to be
 *       used as the threading model for CComObjectRootEx<>
 *
 */
template < class _Allocator >
class ATL_NO_VTABLE CMemoryStreamT : public CComObjectRootEx<typename _Allocator::_ThreadModel>,
                                     public IMemoryStream
{
   /* ATL Interface */
public:	
   DECLARE_NOT_AGGREGATABLE(CMemoryStreamT<_Allocator>)

   BEGIN_COM_MAP(CMemoryStreamT<_Allocator>)
      COM_INTERFACE_ENTRY(IStream)
      COM_INTERFACE_ENTRY(IMemoryStream)
      COM_INTERFACE_ENTRY(ISequentialStream)
   END_COM_MAP()

   void FinalRelease( )
   {
      _Allocator::MemFree(_pbData);
      _pbData = NULL;
   }

   /* ISequentialStream */
public:
   STDMETHOD(Read)( void* pv, ULONG cb, ULONG* pcbRead )
   {
      HRESULT  hr;
      ULONG    cbRead;

      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("!%0.8lx, %u, %0.8lx\n"), this, pv, cb, pcbRead);

      if ( NULL != pcbRead )
      {
         (*pcbRead) = 0;
      }

      cbRead = 0;

      Lock();
      {
         hr = _Read(pv, 
                    cb, 
                    &cbRead);

         if ( SUCCEEDED(hr) && (NULL != pcbRead) )
         {
            (*pcbRead) = cbRead;
         }
      }
      Unlock();

      /* Success / Failure */
      return ( hr );
   }

   STDMETHOD(Write)( const void* pv, ULONG cb, ULONG* pcbWritten )
   {
      HRESULT  hr;
      ULONG    iPosNew;

      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);

      if ( NULL != pcbWritten )
      {
         (*pcbWritten) = 0;
      }

      /* Initialize locals */
      hr = S_OK;
      
      Lock();
      do
      {
         iPosNew = _iPosCur + cb;

         if ( iPosNew < cb )
         {
            hr = STG_E_INVALIDFUNCTION;
            /* Failure */
            break;
         }

         if ( iPosNew > _cbData )
         {
            hr = _EnsureBufferSize(iPosNew);
            if ( FAILED(hr) )
            {
               /* Failure */
               break;
            }
         }

         CopyMemory(_pbData + _iPosCur, 
                    pv, 
                    cb);

         _iPosCur = iPosNew;
         if ( _iPosCur > _cbSize )
         {
            _cbSize = _iPosCur;
         }
      }
      while ( 0 );
      Unlock();

      if ( NULL != pcbWritten )
      {
         (*pcbWritten) = cb;
      }

      /* Success / Failure */
      return ( hr );
   }

   /* IStream */
public:
   STDMETHOD(Seek)( LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition )
   {
      HRESULT        hr;
      ULONG          iPosStart;
      ULONGLONG      iPosNew;
      ULARGE_INTEGER cbNewSize;

      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("!%I64u, %d, %0.8lx\n"), this, dlibMove.QuadPart, dwOrigin, plibNewPosition);

      /* Initialize locals */
      hr        = S_OK;
      iPosStart = 0;

      Lock();
      do
      {
         switch ( dwOrigin )
         {
            case STREAM_SEEK_SET:
               iPosStart = 0;
               break;
            case STREAM_SEEK_CUR:
               iPosStart = _iPosCur;
               break;
            case STREAM_SEEK_END:
               iPosStart = _cbSize;
               break;
            default:
               hr = STG_E_INVALIDFUNCTION;
               break;
         }

         if ( FAILED(hr) )
         {
            /* Failure */
            break;
         }

         iPosNew = iPosStart + dlibMove.QuadPart;

         /* COM says it's an error to seek before the beginning */
         if ( iPosNew < 0 )
         {
            hr = STG_E_INVALIDFUNCTION;
            /* Failure */
            break;
         }

         /* COM says seeking past the end grows the backing storage */
         if ( iPosNew > _cbSize )
         {            
            cbNewSize.QuadPart = iPosNew;

            hr = _SetSize(cbNewSize);
            if ( FAILED(hr) )
            {
               /* Failure */
               break;
            }
         }

         _iPosCur = (ULONG)iPosNew;
      }
      while ( 0 );
      Unlock();

      if ( SUCCEEDED(hr) && (NULL != plibNewPosition) )
      {
         plibNewPosition->QuadPart = _iPosCur;
      }

      /* Success / Failure */
      return ( hr );
   }

   STDMETHOD(SetSize)( ULARGE_INTEGER libNewSize )
   {
      HRESULT hr;

      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);

      Lock();
      {
         hr = _SetSize(libNewSize);
      }
      Unlock();

      /* Success / Failure */
      return ( hr );
   }

   STDMETHOD(CopyTo)( IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten )
   {
      HRESULT     hr;
      IUnknown*   pUnk;
      
      LPVOID      pBuff;
      ULONG       cbBuff;
      BYTE        bBuff[512];
      
      ULONG       cbTotal;
      ULONG       cbRead;
      ULONG       cbWritten;
      ULONG       cbTmp;

      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);

      if ( !pstm )
      {
         /* Failure */
         return ( E_POINTER );
      }

      /* Initialize locals */
      pUnk  = NULL;
      pBuff = bBuff;

      hr = pstm->QueryInterface(IID_IUnknown, 
                                reinterpret_cast<void**>(&pUnk));
      if ( FAILED(hr) )
      {
         /* Failure */
         return ( hr );
      }

      Lock();
      do
      {
         cbTotal   = min(cb.LowPart, (_cbSize - _iPosCur));
         cbRead    = 0;
         cbWritten = 0;

         if ( pUnk == GetUnknown() )
         {
            /* This is a local copy */
            hr = _Read(_pbData + _iPosCur + cbWritten,
                       cbTotal,
                       &cbRead);

            cbWritten = cbRead;
            /* Success / Failure */
            goto __CLEANUP;
         }
            
         cbBuff = PAGE_SIZE;
         pBuff  = _Allocator::MemAllocate(cbBuff);
         if ( !pBuff )
         {
            /* Use the stack buffer */
            pBuff  = bBuff;
            cbBuff = sizeof(bBuff);
         }

         while ( cbTotal )
         {
            cbTmp = 0;
            hr    = _Read(pBuff, 
                          cbBuff, 
                          &cbTmp);

            if ( FAILED(hr) ) 
            {
               /* Failure */
               goto __CLEANUP;
            }

            if ( cbTmp > cbTotal )
            {
               hr = STG_E_INVALIDPOINTER;
               /* Failure */
               goto __CLEANUP;
            }
            
            cbRead  += cbTmp;
            cbTotal -= cbTmp;

            hr = pstm->Write(pBuff, 
                             cbTmp, 
                             &cbTmp);

            if ( FAILED(hr) ) 
            {
               /* Failure */
               goto __CLEANUP;
            }

            cbWritten += cbTmp;
         }
      }
      while ( 0 );
      Unlock();
      
      if ( NULL != pcbRead )
      {
         pcbRead->QuadPart = cbRead;
      }

      if ( NULL != pcbWritten )
      {
         pcbWritten->QuadPart = cbWritten;
      }

   __CLEANUP:
      if ( pBuff != bBuff )
      {
         _Allocator::MemFree(pBuff);
      }

      ATLASSERT(NULL != pUnk);
      pUnk->Release();

      /* Success / Failure */
      return ( hr );
   }

   STDMETHOD(Commit)( DWORD /*grfCommitFlags*/ )
   {
      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);
      return ( S_OK );
   }

   STDMETHOD(Revert)( )
   {
      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);
      return ( E_NOTIMPL );
   }

   STDMETHOD(LockRegion)( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/ )
   {
      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);
      return ( E_NOTIMPL );
   }

   STDMETHOD(UnlockRegion)( ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/ )
   {
      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);
      return ( E_NOTIMPL );
   }

   STDMETHOD(Stat)( STATSTG* pstatstg, DWORD /*grfStatFlag*/ )
   {
      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);

      if ( !pstatstg )
      {
         return ( E_POINTER );
      }

      Lock();
      {
         ZeroMemory(pstatstg, 
                    sizeof(STATSTG));

         pstatstg->cbSize.LowPart  = _cbSize;
         pstatstg->cbSize.HighPart = 0;
         pstatstg->type            = STGTY_STREAM;
      }
      Unlock();

      return ( S_OK );
   }

   STDMETHOD(Clone)( IStream** /*ppstm*/ )
   {
      return ( E_NOTIMPL );
   }

   /* IMemoryStream */
public:
   STDMETHOD(GetMemoryInfo)( LPVOID* ppAddress, ULONG* pcbAllocated )
   {
      HRESULT hr;

      ATLTRACE2(atlTraceCOM, eTraceCOMLevel, _T("%0.8lx!") _T(__FUNCTION__) _T("\n"), this);

      if ( !ppAddress && !pcbAllocated )
      {
         /* Failure */
         return ( E_POINTER );
      }

      Lock();
      do
      {
         /* We're about to give out a memory address and range so make sure the current size
          * is actually backed up by memory. It is possible for _cbSize to be valid without
          * a committed block of memory via "SetSize()...GetMemoryInfo()" so we ensure the
          * greater of the two. */
         hr = _EnsureBufferSize(max(_cbData, _cbSize));
         if ( FAILED(hr) )
         {
            /* Failure */
            break;
         }

         if ( NULL != ppAddress )
         {
            (*ppAddress) = _pbData;
         }

         if ( NULL != pcbAllocated )
         {
            (*pcbAllocated) = _cbSize;
         }
      }
      while ( 0 );
      Unlock();

      /* Success / Failure */
      return ( hr );
   }

   /* CMemoryStream */
public:
   enum { eTraceCOMLevel = 4 };

   BYTE* _pbData;
   ULONG _cbData;
   ULONG _cbSize;
   ULONG _iPosCur;

   CMemoryStreamT( ) throw() : _pbData(NULL), _cbData(0), _cbSize(0), _iPosCur(0)
   {
   }

   HRESULT _EnsureBufferSize( ULONG cbNewSize )
   {
      ULONG    cbData;
      BYTE*    pbNewData;

      if ( cbNewSize > _cbData )
      {
      #ifdef _DEBUG         
         cbData;
         /* For _DEBUG builds we allocate exactly what was requested and no more
          * so that any access outside the expected size should be caught */
         cbNewSize = cbNewSize;
      #else /* _DEBUG */
         cbData = _cbData;
         cbData = min(2 * cbData, cbData + (cbData / 4) + 0x100000);
         cbData = max(cbData, 0x1000);

         cbNewSize = max(cbNewSize, cbData);
      #endif /* _DEBUG */
      }
      else if ( cbNewSize > (_cbData / 4) )
      {
         /* The new size is at least 1/4 of the current allocated size,
          * so we'll keep the buffer around for a bit longer */

         /* Success */
         return ( S_OK );
      }

      pbNewData = reinterpret_cast<BYTE*>(_Allocator::MemReallocate(_pbData, 
                                                                    _cbData, 
                                                                    cbNewSize));
      
      
      /* If reallocation failed and cbNewSize is not 0 then _pbData is still valid,
       * otherwise we need to fall through and clear it because MemReallocate will 
       * have freed it */
      if ( !pbNewData && (cbNewSize > 0) )
      {
         /* Failure */
         return ( E_OUTOFMEMORY );
      }

      _cbData = cbNewSize;
      _pbData = pbNewData;

      /* Success */
      return ( S_OK );
   }

   
   HRESULT _Read( void* pv, ULONG cb, ULONG* pcbRead )
   {
      ULONG iPosNew;
      ULONG cbData;

      iPosNew = (_iPosCur + cb);
      /* Check for overflow */
      if ( iPosNew < cb )
      {
         iPosNew = ULONG_MAX;
      }
         
      /* Two things are happening here. The first is if Read was called when
       * both the size and the backing data have been allocated. In this case
       * the read is being done from that memory. The second is if Read was
       * called when the size is set but data has not been allocated. In this
       * case the data is simulated by filling the read buffer with 0 rather
       * than allocating backing storage to be cleared and copied. */
      iPosNew = min(iPosNew, _cbSize);      
      cbData  = min(iPosNew, _cbData);

      if ( cbData > _iPosCur )
      {
         CopyMemory(pv, 
                    _pbData + _iPosCur, 
                    cbData - _iPosCur);
      }

      if ( iPosNew > cbData )
      {
         ZeroMemory(reinterpret_cast<BYTE*>(pv) + (cbData - _iPosCur), 
                    iPosNew - cbData);
      }

      (*pcbRead) = (iPosNew - _iPosCur);
      _iPosCur   = iPosNew;
      
      /* Success */
      return ( S_OK );
   }
   
   HRESULT _SetSize( ULARGE_INTEGER libNewSize )
   {
      /* Don't support any size larger than ULONG_MAX */
      if ( 0 != libNewSize.HighPart )
      {
         /* Failure */
         return ( E_OUTOFMEMORY );
      }

      _cbSize = libNewSize.LowPart;

      if ( _cbSize < _cbData )
      {
         /* Success / Failure */
         return ( _EnsureBufferSize(_cbSize) );
      }

      /* Success */
      return( S_OK );
   }
};

/**
 * CreateMemoryStreamT
 *    Creates an instance of CMemoryStreamT<>
 *
 * Parameters:
 *    _MemStreamT
 *       A typed CMemoryStreamT class
 *    ppStream
 *       [out] The resultant stream
 */
template < class _MemStreamT >
static HRESULT CreateMemoryStreamT( IStream** ppStream )
{
   typedef CComObject<_MemStreamT> CoMemStream;

   HRESULT      hr;
   CoMemStream* pStream;

   ATLASSERT(NULL != ppStream);

   pStream = NULL;
   hr = CoMemStream::CreateInstance(&pStream);
   if ( FAILED(hr) )
   {
      /* Failure */
      return ( hr );
   }

   pStream->AddRef();
#ifdef _ATL_DEBUG_INTERFACES
   /* Need to use QI here so we get the debug thunk */
   pStream->QueryInterface(IID_IStream, reinterpret_cast<void**>(ppStream));
   pStream->Release();
#else
   (*ppStream) = static_cast<IStream*>(pStream);
#endif

   /* Success */
   return ( S_OK );
}

/**
 * CreateByteStream
 *    Creates an instance CMemoryStreamT using the
 *    ComMemHeap allocator.
 *
 * The stream created with this function is _not_ thread safe.
 */
inline HRESULT __stdcall CreateByteStream( IStream** ppStream )
{
   return ( CreateMemoryStreamT< CMemoryStreamT<ComMemHeap> >(ppStream) );
}

/**
 * CreateVirtualStream
 *    Creates an instance CMemoryStreamT using the
 *    VirtualMemoryHeap allocator.
 *
 * The stream created with this function is thread safe.
 */
inline HRESULT __stdcall CreateVirtualStream( IStream** ppStream )
{
   return ( CreateMemoryStreamT< CMemoryStreamT<VirtualMemoryHeap> >(ppStream) );
}

/**
 * CreateVerifierStream
 *    Creates an instance of CMemoryStreamT using the
 *    VerifierMemHeap allocator.
 *
 * The stream created with this function is thread safe.
 */
inline HRESULT __stdcall CreateVerifierStream( IStream** ppStream )
{
   return ( CreateMemoryStreamT< CMemoryStreamT<VerifierMemHeap> >(ppStream) );
}

typedef HRESULT (__stdcall* PCREATESTREAMROUTINE)( IStream** );

/**
 * CopyStream
 *    Copies a stream from one to another using CopyTo() if
 *    possible and falling back to Read()\Write() if not.
 *
 * Remarks
 *    The seek pointers of both streams are undefined when
 *    this function returns.
 */
static 
STDMETHODIMP 
CopyStream( 
   __in IStream* pStmSource, 
   __in IStream* pStmDest,
   __out_opt PULARGE_INTEGER pcbCopied
)
{
   if ( NULL != pcbCopied )
   {
      pcbCopied->QuadPart = NULL;
   }

   LARGE_INTEGER iPos = {0};
   HRESULT hr = pStmSource->Seek(LongLongToLargeInt(0), SEEK_CUR, (ULARGE_INTEGER*)&iPos);
   if ( FAILED(hr) )
   {
      return ( hr );
   }

   ULARGE_INTEGER iEnd = {0};
   hr = pStmSource->Seek(LongLongToLargeInt(0), SEEK_END, &iEnd);
   if ( FAILED(hr) )
   {
      return ( hr );
   }

   ULARGE_INTEGER cbSize;   
   cbSize.QuadPart = (iEnd.QuadPart - iPos.QuadPart);

   hr = pStmSource->Seek(iPos, SEEK_SET, NULL);
   if ( FAILED(hr) )
   {
      return ( hr );
   }

   iPos.QuadPart = 0;
   hr = pStmDest->Seek(LongLongToLargeInt(0), SEEK_CUR, (ULARGE_INTEGER*)&iPos);
   if ( FAILED(hr) )
   {
      return ( hr );
   }

   iEnd.QuadPart = 0;
   hr = pStmDest->Seek(LongLongToLargeInt(0), SEEK_END, &iEnd);
   if ( FAILED(hr) )
   {
      return ( hr );
   }

   cbSize.QuadPart += (iEnd.QuadPart - iPos.QuadPart);
   
   hr = pStmDest->Seek(iPos, SEEK_SET, NULL);
   if ( FAILED(hr) )
   {
      return ( hr );
   }

   if ( 0 == cbSize.QuadPart )
   {
      return ( S_OK );
   }

   hr = pStmDest->SetSize(cbSize);
   if ( FAILED(hr) )
   {
      return ( hr );
   }

   ULARGE_INTEGER cbRead    = {0};
   ULARGE_INTEGER cbCopied  = {0};

   hr = pStmSource->CopyTo(pStmDest, cbSize, &cbRead, &cbCopied);
   if ( SUCCEEDED(hr) && ((cbRead.QuadPart < cbSize.QuadPart) || (cbCopied.QuadPart < cbSize.QuadPart)) )
   {
      return ( STG_E_CANTSAVE );
   }
   else if ( E_NOTIMPL == hr )
   {
      hr = pStmSource->Seek(LongLongToLargeInt(0), SEEK_SET, NULL);
      if ( FAILED(hr) )
      {
         return ( hr );
      }

      hr = pStmDest->Seek(LongLongToLargeInt(0), SEEK_SET, NULL);
      if ( FAILED(hr) )
      {
         return ( hr );
      }

      /* Copy in PAGE_SIZE blocks */
      enum { cbBuf = PAGE_SIZE };

      LPVOID pbData = VirtualAlloc(NULL, cbBuf, MEM_COMMIT, PAGE_READWRITE);
      if ( !pbData )
      {
         return ( E_OUTOFMEMORY );
      }

      __try
      {
         hr = pStmSource->Read(pbData, cbBuf, &cbRead.LowPart);

         while ( SUCCEEDED(hr) && (cbRead.LowPart > 0) )
         {
            DWORD cbWritten = 0;
            hr = pStmDest->Write(pbData, cbRead.LowPart, &cbWritten);
            if ( cbWritten < cbRead.LowPart )
            {
               hr = STG_E_CANTSAVE;
               __leave;
            }

            cbCopied.QuadPart += cbWritten;

            hr = pStmSource->Read(pbData, cbBuf, &cbRead.LowPart);            
         }
      }
      __finally
      {
         VirtualFree(pbData, 0, MEM_RELEASE);
      }
   }

   if ( NULL != pcbCopied )
   {
      pcbCopied->QuadPart = cbCopied.QuadPart;
   }

   return ( hr );
}

/**
 * GetMemoryStreamForStream
 *    Retrieves an IMemoryStream object for a given stream, creating
 *    and copying the source stream to a new one if necessary.
 */
static 
STDMETHODIMP 
GetMemoryStreamForStream( 
   __in IStream* pStmSource, 
   __out IMemoryStream** ppMemStream,
   __out_opt PULARGE_INTEGER pcbSource,
   __in_opt PCREATESTREAMROUTINE pCreateStreamRoutine 
)
{
   ATLASSERT(NULL != pStmSource && NULL != ppMemStream);

   HRESULT        hr         = E_FAIL;
   IStream*       pStmBlock  = NULL;
   IMemoryStream* pMemStream = NULL;

   (*ppMemStream) = NULL;

   if ( NULL != pcbSource )
   {
      pcbSource->QuadPart = 0i64;
   }

   __try
   {
      hr = pStmSource->QueryInterface(IID_IMemoryStream, (void**)&pMemStream);
      if ( FAILED(hr) )
      {
         /* Make sure pMemStream is NULL */
         pMemStream = NULL;
      }

      /* Seek to the end of the source stream to determine the size */
      ULARGE_INTEGER cbSource = {0};
      hr = pStmSource->Seek(LongLongToLargeInt(0), SEEK_END, &cbSource);
      if ( FAILED(hr) )
      {
         /* Failure */
         ATLASSERT(SUCCEEDED(hr));
         __leave;
      }

      if ( NULL != pcbSource )
      {
         pcbSource->QuadPart = cbSource.QuadPart;
      }

      /* If the stream is a memory stream, return it now */
      if ( NULL != pMemStream )
      {
         /* Success */
         __leave;
      }

      /* No support for messages of size 0 or larger than 4GB */
      if ( (cbSource.HighPart > 0) || (0 == cbSource.LowPart) )
      {
         hr = STG_E_INVALIDPARAMETER;
         /* Failure */
         ATLASSERT(SUCCEEDED(hr));
         __leave;
      }

      /* If the source stream is less than 4MB use a byte stream, otherwise
       * use a virtual stream */
      if ( NULL != pCreateStreamRoutine )
      {
         hr = (pCreateStreamRoutine)(&pStmBlock);
      }
      else
      {
         if ( (1024 * 1024 * 4) >= cbSource.LowPart )
         {
            hr = CreateByteStream(&pStmBlock);
         }
         else
         {
            hr = CreateVirtualStream(&pStmBlock);
         }
      }

      if ( FAILED(hr) )
      {
         /* Failure */
         ATLASSERT(SUCCEEDED(hr));
         __leave;
      }

      /* Force the size of the text stream to match the source */
      hr = pStmBlock->SetSize(cbSource);
      if ( FAILED(hr) )
      {
         /* Failure */
         ATLASSERT(SUCCEEDED(hr));
         __leave;
      }

      /* Get the out interface */
      hr = pStmBlock->QueryInterface(IID_IMemoryStream, (void**)&pMemStream);
      ATLASSERT(SUCCEEDED(hr));

      /* Reset the source stream pointer */
      hr = pStmSource->Seek(LongLongToLargeInt(0), SEEK_SET, NULL);
      if ( FAILED(hr) )
      {
         /* Failure */
         ATLASSERT(SUCCEEDED(hr));
         __leave;
      }

      /* Copy the source stream into the memory stream */
      ULARGE_INTEGER cbCopied = {0};
      hr = pStmSource->CopyTo(pStmBlock, cbSource, NULL, &cbCopied);
      if ( FAILED(hr) && (E_NOTIMPL == hr) )
      {
         PBYTE pbData = NULL;
         hr = pMemStream->GetMemoryInfo((LPVOID*)&pbData, NULL);
         if ( FAILED(hr) )
         {
            hr = STG_E_CANTSAVE;
            ATLASSERT(SUCCEEDED(hr));
            __leave;
         }

         ATLASSERT(NULL != pbData);
         hr = pStmSource->Read(pbData, cbSource.LowPart, &cbCopied.LowPart);
      }

      if ( cbCopied.QuadPart != cbSource.QuadPart )
      {
         hr = STG_E_CANTSAVE;
      }
   }
   __finally
   {
      if ( NULL != pStmBlock )
      {
         pStmBlock->Release();
      }

      if ( SUCCEEDED(hr) )
      {
         (*ppMemStream) = pMemStream;
      }
      else if ( NULL != pMemStream )
      {
         pMemStream->Release();
      }
   }

   return ( hr );
}

#endif /* __MEMSTREAM_H__ */