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
 
/* DiskDefrag.cpp
 *    CFSxDefragmenter implementation
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#include "Stdafx.h"
#include "DiskDefrag.h"
/*++
	WPP
 --*/
#include "DiskDefrag.tmh"

#ifndef WINERROR
	#define WINERROR( dw ) \
		(0 != (dw))
#endif /* WINERROR */

HRESULT
CFSxDefragmenter::InternalConstruct( 
) throw()
{
	DWORD dwRet;

	dwRet = NO_ERROR;

	__try {
		InitializeCriticalSection(&(_cxDataLock));
	}
	__except( STATUS_NO_MEMORY == GetExceptionCode() ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH ) {
		dwRet = ERROR_NOT_ENOUGH_MEMORY;
	}

	if ( WINERROR(dwRet) ) {
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"InitializeCriticalSectionAndSpinCount failed, error = %!WINERROR!, exiting\n",
				  dwRet);

		/* Failure */
		return ( __HRESULT_FROM_WIN32(dwRet) );
	}

	/* Success */
	return ( S_OK );
}

void
CFSxDefragmenter::InternalDestruct( 
)
{
	InternalSetCallbackInterface(NULL);
	DeleteCriticalSection(&_cxDataLock);
}


/*++

	CFSxDefragmenter : CFSxDefragmenter

 --*/

CFSxDefragmenter::CFSxDefragmenter( 
)
{
	_pDefragmentFileCallback = NULL;

	ZeroMemory(&_cxDataLock,
				  sizeof(_cxDataLock));
}

STDMETHODIMP 
CFSxDefragmenter::DefragmentFile( 
	__RPC__in_string LPCWSTR FileName, 
	__RPC__in ULONG Flags,
	__RPC__in ULONG_PTR Parameter
)
{
	HRESULT hr;

	/*
	 * Check if we can allow this call at the COM server level
	 */
	if ( !CoEnterExternalCall() ) {
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"CoEnterExternalCall failed, CO_E_SERVER_STOPPING\n");
		
		/* Failure */
		return ( CO_E_SERVER_STOPPING );
	}

	__try {
		/*
		 * Forward to the worker but wrap the call in an exception handler so we
		 * can cleanup the external call state
		 */
		hr = InternalDefragmentFile(FileName,
											 Flags,
											 Parameter);

		if ( FAILED(hr) ) {
			FpTrace(TRACE_LEVEL_ERROR,
						SvxFileDefragmenter,
						L"InternalDefragmentFile failed, FileName = %p, Flags = %08lx, Parameter = %p, hr = %!HRESULT!\n",
						FileName,
						Flags,
						reinterpret_cast<void*>(Parameter),
						hr);
		}
	}
	__finally {
		/*
		 * Don't do anything else in this termination handler
		 */
		CoLeaveExternalCall();
	}

	/* Success / Failure */
	return ( hr );   
}

STDMETHODIMP
CFSxDefragmenter::SetCallbackInterface(
	__RPC__in_opt IUnknown* Callback
)
{
	HRESULT hr;

	if ( !CoEnterExternalCall() ) {
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"CoEnterExternalCall failed, CO_E_SERVER_STOPPING\n");

		/* Failure */
		return ( CO_E_SERVER_STOPPING );
	}

	__try {
		hr = InternalSetCallbackInterface(Callback);

		if ( FAILED(hr) ) {
			FpTrace(TRACE_LEVEL_ERROR,
						SvxFileDefragmenter,
						L"InternalSetCallbackInterface failed, Callback = %p, hr = %!HRESULT!\n",
						Callback,
						hr);
		}
	}
	__finally {
		/*
		 * Don't do anything else in this termination handler
		 */
		CoLeaveExternalCall();
	}

	/* Success / Failure */
	return ( hr );
}

HRESULT
CFSxDefragmenter::InternalDefragmentFile(
	__RPC__in_string LPCWSTR FileName, 
	__RPC__in ULONG Flags,
	__RPC__in ULONG_PTR CallbackParameter 
)
{
	HRESULT     hr;
	DWORD       dwRet;

	HANDLE      hFile;
	
	ULONG       cFragCount;
	
	DEFRAGCTX   DefragCtx;

	BOOLEAN     bInBackgroundMode;

	/* Validate parameters */
	if ( !FileName ) {
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"Invalid parameter, FileName = NULL, exiting\n");

		/* Failure */
		return ( E_POINTER );
	}

	/* Initialize locals */
	hFile             = INVALID_HANDLE_VALUE;
	bInBackgroundMode = TRUE;

	ZeroMemory(&DefragCtx,
				  sizeof(DefragCtx));

	/*
	 * If the caller has requested an increased priority, raise to it now
	 */
	if ( FlagOn(Flags, DFF_NOTBACKGROUNDPRIORITY) ) {
		if ( !SetThreadPriority(GetCurrentThread(),
										THREAD_MODE_BACKGROUND_END) )
		{
			dwRet = GetLastError();
			if ( dwRet != ERROR_THREAD_MODE_NOT_BACKGROUND ) {
				FpTrace(TRACE_LEVEL_WARNING,
						  SvxFileDefragmenter,
						  L"Unable to exit background mode for current thread, dwRet = %!WINERROR!\n",
						  dwRet);
			}
		}
		else {
			/*
			 * This thread has exited background mode so we need to restore when we exit
			 */
			bInBackgroundMode = FALSE;
		}
	}

	/*
	 * Open the file with the client's security context. If they don't have access
	 * to the file, or if the path is not a valid disk file then this will fail and
	 * we can exit
	 */
	hr = OpenFileAsClient(FileName,
								 &hFile);

	if ( FAILED(hr) ) {
		_ASSERTE(INVALID_HANDLE_VALUE == hFile);

		FpTrace(TRACE_LEVEL_INFORMATION,
				  SvxFileDefragmenter,
				  L"Failed to open file with client's security context, FileName = %p, hr = %!HRESULT!, exiting\n",
				  FileName,
				  hr);

		/* Failure */
		goto __CLEANUP;
	}

	if ( FlagOn(Flags, DFF_IGNORECONTIGUOUSFILES) ) {
		/*
		 * Caller doesn't want to defragment a file that doesn't have any fragments, 
		 * so we'll check if this file has any first
		 */
		cFragCount = 0;
			
		dwRet = GetFileFragmentCount(hFile,
											  &cFragCount);
			
		if ( WINERROR(dwRet) ) {
			FpTrace(TRACE_LEVEL_ERROR,
						SvxFileDefragmenter,
						L"GetFileFragmentCount failed, FileName = %p, dwRet = %!WINERROR!, exiting\n",
						FileName,
						dwRet);

			hr = __HRESULT_FROM_WIN32(dwRet);
			/* Failure */
			goto __CLEANUP;
		}
		
		if ( cFragCount <= 1 ) {
			FpTrace(TRACE_LEVEL_INFORMATION,
						SvxFileDefragmenter,
						L"File is contiguous, FileName = %p, cFragCount = %u, sending client notification, exiting\n",
						FileName,
						cFragCount);

			/*
			 * The file has 0 or 1 fragment, so it isn't a candidate for defragmenting. We'll
			 * inform the client by posting a single update to them, mimicing completion. 
			 */
			hr = SendDefragmentFileUpdate(FileName,
													100,
													CallbackParameter);

			if ( FAILED(hr) ) {
				FpTrace(TRACE_LEVEL_WARNING,
							SvxFileDefragmenter,
							L"SendDefragmentFileUpdate failed, FileName = %p, hr = %!HRESULT!, exiting\n",
							FileName,
							dwRet);
			}

			/*
			 * Always succeed this code path
			 */
			hr = S_OK;

			/* Success */
			goto __CLEANUP;
		}
	}

	/*
	 * We're going to defragment this file, so setup a DEFRAGCTX to keep track of the progress
	 */
	DefragCtx.Result    = S_OK;
	DefragCtx.State     = 0;
	DefragCtx.Percent   = 0;
	DefragCtx.CallTime  = 0;
	DefragCtx.Parameter = CallbackParameter;
	DefragCtx.FileName  = FileName;      
	InternalGetCallbackInterface(&(DefragCtx.Callback));

	/*
	 * Strip out flags the client may have set that we need to override, and override them
	 */
	Flags &= ~(FPF_DEFRAG_CALLBACKINCREMENTMASK);
	Flags |= EncodeCallbackIncrement(2);
			
	/* 
	 * Defragment the file. If there's no client callback, we still use a local one to test the 
	 * service running state so we can abort if it is shutting down
	 */
	dwRet = FpDefragmentFileEx(hFile,
										Flags,
										&CFSxDefragmenter::DefragmentFileRoutine,
										&(DefragCtx));
			
	if ( WINERROR(dwRet) && (ERROR_CANCELLED != dwRet) ) {
		FpTrace(TRACE_LEVEL_WARNING,
					SvxFileDefragmenter,
					L"FpDefragmentFileEx failed, FileName = %p, dwRet = %!WINERROR!, exiting\n",
					FileName,
					dwRet);

		/* This is a failure by FpDefragmentFile or an abort by DefragmentFileRoutine */
		hr = __HRESULT_FROM_WIN32(dwRet);
	}
	else {
		/* Success / Failure - Via the callback */
		hr = DefragCtx.Result;
	}

__CLEANUP:
	if ( INVALID_HANDLE_VALUE != hFile ) {
		CloseHandle(hFile);
	}

	if ( DefragCtx.Callback ) {
		DefragCtx.Callback->Release();
	}

	if ( !bInBackgroundMode ) {
		/*
		 * This thread needs to return itself to background mode, so do so now
		 */
		SetThreadPriority(GetCurrentThread(),
								THREAD_MODE_BACKGROUND_BEGIN);
	}

	/* Success / Failure */
	return ( hr );
}

HRESULT
CFSxDefragmenter::InternalSetCallbackInterface(
	__RPC__in_opt IUnknown* Callback
)
{
	HRESULT                       hr;

	IFSxDefragmentFileCallback*   pCloakedCallback;
	IFSxDefragmentFileCallback*   pPreviousCallback;

	/* Initialize locals */
	pCloakedCallback  = NULL;
	pPreviousCallback = NULL;

	if ( Callback ) {
		/*
		 * This is a bi-directional component, so we need to capture the caller's security
		 * context and ensure that COM is using it instead of ours when we send our updates
		 * to the caller
		 */
		hr = CloakClientCallbackInterface(Callback,
													 &pCloakedCallback);

		if ( FAILED(hr) ) {
			_ASSERTE(NULL == pCloakedCallback);

			FpTrace(TRACE_LEVEL_ERROR,
					  SvxFileDefragmenter,
					  L"CloakClientCallbackInterface failed, hr = %!HRESULT!, exiting\n",
					  hr);

			/* Failure */
			return ( hr );
		}
	}

	/*
	 * We need to swap out _pDefragmentFileCallback in a guarded region because other
	 * methods of this class will call AddRef() on it before making a local copy. So
	 * if another thread has a copy of the one we're about to remove, it will use it 
	 * for the lifetime of its call. Any new ones will use the one we're setting.
	 */
	EnterCriticalSection(&_cxDataLock); {
		/*
		 * Save the current callback, if any, so we can release it
		 */
		pPreviousCallback = _pDefragmentFileCallback;

		/*
		 * Set the new callback, if any. _pDefragmentFileCallback takes ownership of the 
		 * ref count added on pCloakedCallback by CloakClientCallbackInterface above
		 */
		_pDefragmentFileCallback = pCloakedCallback;
	}
	LeaveCriticalSection(&_cxDataLock);

	/*
	 * If we previously had a callback, we need to give up the reference count we put on 
	 * it in a previous call to this function
	 */
	if ( pPreviousCallback ) {
		pPreviousCallback->Release();
	}

	/* Success */
	return ( S_OK );
}

void
CFSxDefragmenter::InternalGetCallbackInterface(
	__deref_out IFSxDefragmentFileCallback** Callback
)
{
	EnterCriticalSection(&_cxDataLock); {
		if ( _pDefragmentFileCallback ) {
			/*
			 * Add a reference on the copy we're handing out
			 */
			_pDefragmentFileCallback->AddRef();
		}
			
		(*Callback) = _pDefragmentFileCallback;
	}
	LeaveCriticalSection(&_cxDataLock);
}

HRESULT
CFSxDefragmenter::CloakClientCallbackInterface(
	__in IUnknown* CallbackProxy,
	__deref_out IFSxDefragmentFileCallback** Callback
)
{
	HRESULT                       hr;

	HANDLE                        hClientToken;
	SECURITY_IMPERSONATION_LEVEL  ImpersonationLevel;
	BOOL                          bImpersonating;

	_ASSERTE(!IsThreadImpersonating(GetCurrentThread()));

	/* Initialize outputs */
	(*Callback) = NULL;

	/* Initialize locals */
	hClientToken   = NULL;
	bImpersonating = FALSE;

	/*
	 * Open the client's access token so we can determine if they have
	 * the correct impersonation level before we try to cloak them
	 */
	hr = CoGetClientToken(TOKEN_QUERY|TOKEN_IMPERSONATE,
								 &hClientToken);

	if ( FAILED(hr) ) {
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"CoGetClientToken failed, DesiredAccess = TOKEN_QUERY|TOKEN_IMPERSONATE, hr = %!HRESULT!, exiting\n",
				  hr);

		/* Failure */
		goto __CLEANUP;
	}

	/*
	 * We require at least SecurityImpersonation so that we can cloak the client
	 */
	ImpersonationLevel = GetTokenImpersonationLevel(hClientToken);
	if ( !VALID_IMPERSONATION_LEVEL(ImpersonationLevel) ) {
		hr = CoGetLastError();

		FpTrace(TRACE_LEVEL_ERROR,
					SvxFileDefragmenter,
					L"GetTokenImpersonationLevel returned an invalid impersonation level, ImpersonationLevel = %d, hr = %!HRESULT!, exiting\n",
					ImpersonationLevel,
					hr);

		/* Failure */
		goto __CLEANUP;
	}

	if ( ImpersonationLevel < SecurityImpersonation ) {
		hr = __HRESULT_FROM_WIN32(ERROR_CANNOT_IMPERSONATE);

		FpTrace(TRACE_LEVEL_ERROR,
					SvxFileDefragmenter,
					L"Unable to impersonate client, ImersonationLevel = %d, exiting\n",
					ImpersonationLevel);

		/* Failure */
		goto __CLEANUP;
	}

	/*
	 * The client gave us a token that we can impersonate so do so now and cloak
	 * the interfaces. 
	 */
	if ( !SetThreadToken(NULL,
								hClientToken) ) {
		hr = CoGetLastError();
			
		FpTrace(TRACE_LEVEL_ERROR,
					SvxFileDefragmenter,
					L"SetThreadTokenFailed, hr = %!HRESULT!, exiting\n",
					hr);
			
		/* Failure */
		goto __CLEANUP;
	}

	bImpersonating = TRUE;

	/*
	 * This will set static cloaking on both the proxy manager, which is responsible for the
	 * IUnknown calls to the client, and the IFSxDefragmentFileCallback callback. If we didn't
	 * do this COM would use our service identity, which is the SYSTEM account. Because we're
	 * impersonating the client now, when CoCloakedQueryInterface() calls SetBlanket() COM will
	 * use that identity instead.
	 */
	hr = CoCloakedQueryInterface(CallbackProxy,
										  Callback);

	if ( FAILED(hr) ) {
		FpTrace(TRACE_LEVEL_ERROR,
					SvxFileDefragmenter,
					L"CoCloakedQueryInterface failed, hr = %!HRESULT!, exiting\n",
					hr);
	}

__CLEANUP:
	if ( bImpersonating ) {
		/*
		 * Reset the thread token. For this to fail, we'd have to somehow lose
		 * our SE_IMPERSONATE privilege, and we never do that so the only way
		 * it could happen is through tampering
		 */
		SetThreadToken(NULL,
							NULL);
	}

	if ( hClientToken ) {
		CloseHandle(hClientToken);
	}

	/* Success / Failure */
	return ( hr );
}

HRESULT
CFSxDefragmenter::OpenFileAsClient(
	__in_z LPCWSTR FileName,
	__deref_out PHANDLE FileHandle
)
{
	HRESULT           hr;
	DWORD             dwRet;

	IServerSecurity*  pServerSecurity;
	BOOL              bImpersonating;

	UINT              uErrorMode;
	DWORD             dwAttributes;
	DWORD             dwAccessMode;
	DWORD             dwShareMode;
	HANDLE            hFile;
	DWORD             dwFileType;

	/* Initialize outputs */
	(*FileHandle) = INVALID_HANDLE_VALUE;

	/* Initialize locals */
	dwRet           = NO_ERROR;
	pServerSecurity = NULL;
	bImpersonating  = FALSE;
	hFile           = INVALID_HANDLE_VALUE;

	_ASSERTE(NULL != FileName);
	_ASSERTE(NULL != FileName);

	/*
	 * Impersonate the client and open a handle to the file. We do this under impersonation
	 * so we can ensure the client has access to the file. We use CreateFile as opposed to 
	 * using AccessCheck as CreateFile is going to do an access check anyway, so we can avoid
	 * doing it twice. Another benefit to using CreateFile is that it will validate the path.
	 */
	hr = CoGetCallContext(__uuidof(IServerSecurity),
								 reinterpret_cast<void**>(&pServerSecurity));

	if ( FAILED(hr) ) {
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"CoGetCallContext failed, riid = IServerSecurity, hr = %!HRESULT!, exiting\n",
				  hr);

		/* Failure */
		goto __CLEANUP;
	}

	if ( !pServerSecurity->IsImpersonating() ) {
		hr = pServerSecurity->ImpersonateClient();
		if ( FAILED(hr) ) {
			FpTrace(TRACE_LEVEL_ERROR,
						SvxFileDefragmenter,
						L"IServerSecurity::ImpersonateClient failed, hr = %!HRESULT!, exiting\n",
						hr);

			/* Failure */
			goto __CLEANUP;
		}

		bImpersonating = TRUE;
	}

	/*
	 * Retrieve the file's attributes so we can determine the appropriate access rights required
	 * to defragment it
	 */
	uErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX); {
		dwAttributes = GetFileAttributes(FileName);      
		if ( INVALID_FILE_ATTRIBUTES == dwAttributes ) {
			hr = CoGetLastError();
		}
	}
	SetErrorMode(uErrorMode);

	if ( FAILED(hr) ) {      
		FpTrace(TRACE_LEVEL_ERROR,
					SvxFileDefragmenter,
					L"GetFileAttributes failed, FileName = %p, hr = %!HRESULT!, exiting\n",
					FileName,
					hr);

		/* Failure */
		goto __CLEANUP;
	}

	dwAccessMode = GetDefragmentRightsRequired(dwAttributes);

	/*
	 * Open the file/directory with enough rights to allow defragmenting whether the file is encrypted
	 * or not. We don't care what the path is, as long as it represents a file, local and accessable
	 * by the client so we'll let CreateFile() handle validating the path. 
	 *
	 * We also set SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION|SECURITY_EFFECTIVE_ONLY in the flags
	 * so that if a client passed us a pipe or some other object that allows impersonation, then the
	 * caller cannot impersonate us. As we're already impersonating the client, they would effectively
	 * be impersonating themselves but it's an extra step that doesn't cost us anything
	 */
	dwShareMode = FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE;

	while ( 0 != dwShareMode ) {
		hFile = CreateFile(FileName,
								 dwAccessMode,
								 dwShareMode,
								 NULL,
								 OPEN_EXISTING,
								 FILE_FLAG_BACKUP_SEMANTICS|/*FILE_FLAG_OVERLAPPED|*/SECURITY_SQOS_PRESENT|SECURITY_IDENTIFICATION|SECURITY_EFFECTIVE_ONLY,
								 NULL);

		if ( INVALID_HANDLE_VALUE != hFile ) {
			/* Success */
			break;
		}
			
		if ( ERROR_SHARING_VIOLATION != dwRet ) {
			/* Failure - Some error that we're not prepared to handle */
			break;
		}

		/* 
		 * Lower the sharing mode we're requesting and try again. This will cause us to 
		 * request sharing rights in the following order..
		 *
		 *    FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE
		 *    FILE_SHARE_READ|FILE_SHARE_WRITE
		 *    FILE_SHARE_READ
		 */
		dwShareMode >>= 1;
	}

	/*
	 * If we get here without a valid file handle, we were unable to open
	 * the file due to a sharing violation or some other error
	 */
	if ( INVALID_HANDLE_VALUE == hFile ) {
		hr = __HRESULT_FROM_WIN32(dwRet);

		FpTrace(TRACE_LEVEL_ERROR,
					SvxFileDefragmenter,
					L"CreateFile failed, FileName = %p, hr = %!HRESULT!, exiting\n",
					FileName,
					hr);

		/* Failure */
		goto __CLEANUP;
	}

	/*
	 * Make sure we actually opened a file on a disk drive
	 */
	dwFileType = GetFileType(hFile);

	if ( FILE_TYPE_DISK != dwFileType ) {
		hr = __HRESULT_FROM_WIN32(ERROR_BAD_FILE_TYPE);

		FpTrace(TRACE_LEVEL_ERROR,
					SvxFileDefragmenter,
					L"File is not a disk file, FileName = %p, dwFileType = %u, exiting\n",
					FileName,
					dwFileType);

		/* Failure */
		goto __CLEANUP;
	}
			
	/*
	 * Assign the disk file to the caller
	 */
	(*FileHandle) = hFile;

	/*
	 * Clear the local alias so we don't close it when cleaning up
	 */
	hFile = INVALID_HANDLE_VALUE;
		  
	/* Success */
	hr = S_OK;

__CLEANUP:
	if ( INVALID_HANDLE_VALUE != hFile ) {
		CloseHandle(hFile);
	}

	if ( pServerSecurity ) {
		if ( bImpersonating ) {
			pServerSecurity->RevertToSelf();
		}

		pServerSecurity->Release();
	}

	/* Success / Failure */
	return ( hr );
}

DWORD
CFSxDefragmenter::GetFileFragmentCount(
	__in HANDLE FileHandle,
	__deref_out ULONG* FragmentCount
)
{
	DWORD  dwRet;
	HANDLE hFragCtx;

	_ASSERTE(INVALID_HANDLE_VALUE != FileHandle);
	_ASSERTE(NULL != FragmentCount);

	/* Initialize outputs */
	(*FragmentCount) = 0;

	/* Initialize locals */
	hFragCtx   = NULL;

	/*
	 * Load the file fragment data by creating a frag context for it
	 */
	dwRet = FpCreateContextEx(FileHandle,
									  0,
									  &hFragCtx);
	
	if ( WINERROR(dwRet) ) {
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"FpCreateContextEx failed, dwRet = %!WINERROR!, exiting\n",
				  dwRet);
	}
	else {
		dwRet = FpGetContextFragmentCount(hFragCtx,
													 FragmentCount);
	
		FpTrace(TRACE_LEVEL_ERROR,
				  SvxFileDefragmenter,
				  L"FpGetContextFragmentCount failed, dwRet = %!WINERROR!, exiting\n",
				  dwRet);

		FpCloseContext(hFragCtx);
	}

	/* Success / Failure */
	return ( dwRet );
}

HRESULT
CFSxDefragmenter::SendDefragmentFileUpdate(
	__in_z LPCWSTR FileName,
	__in ULONG PercentComplete,
	__in ULONG_PTR CallbackParameter
)
{
	HRESULT                     hr;
	IFSxDefragmentFileCallback* Callback;

	/* Initialize locals */
	hr       = S_OK;
	Callback = NULL;
		
	InternalGetCallbackInterface(&Callback);

	if ( Callback ) {
		hr = SendDefragmentFileUpdateEx(Callback,
												  FileName,
												  PercentComplete,
												  CallbackParameter);

		Callback->Release();
	}

	/* Success / Failure */
	return ( hr );
}

HRESULT
CFSxDefragmenter::SendDefragmentFileUpdateEx(
	__in IFSxDefragmentFileCallback* Callback,
	__in_z LPCWSTR FileName,
	__in ULONG PercentComplete,
	__in ULONG_PTR CallbackParameter
)
{
	HRESULT  hr;

	HANDLE   hCancelCtx;

	_ASSERTE(NULL != Callback);
	_ASSERTE(NULL != FileName);

	/* Initialize locals */
	hr = S_OK;

	FpTrace(TRACE_LEVEL_VERBOSE,
			  SvxFileDefragmenter,
			  L"Sending client notification, OnDefragmentFileUpdate, FileName = %p, PercentComplete = %u\n",
			  FileName,
			  PercentComplete);

	/*
	 * Register for call cancellation with the service before sending the update. This closes a hole that
	 * would allow a client to hang in their update handler while the service is trying to shutdown, thereby
	 * preventing the service from shutting down.
	 * 
	 * The service will cancel all registered outbound calls if it receives a stop/shutdown signal, then wait 
    * for all internal calls to complete. Once all internal calls complete, it will shutdown cleanly.
	 */
	hCancelCtx = CoRegisterCallCancelCtx((USHORT)CALLCANCELLATION_TIMEOUT); {
		hr = Callback->OnDefragmentFileUpdate(FileName,
														  PercentComplete,
														  CallbackParameter);
	}
	CoUnregisterCallCancelCtx(hCancelCtx);

	/* Success / Failure */
	return ( hr );
}

DWORD 
FRAGAPI 
CFSxDefragmenter::DefragmentFileRoutine( 
	__in ULONGLONG ClustersTotal, 
	__in ULONGLONG ClustersMoved, 
	__in_opt PVOID Parameter 
)
{
	HRESULT        hr;

	PDEFRAGCTX     pDefragCtx;
	
	double         iPercent;
	
	FILETIME       SysTime;
	ULARGE_INTEGER TimeDiff;
	
	BOOL           bUpdateClient;

	_ASSERTE(NULL != Parameter);

	/* Initialize locals */
	bUpdateClient = FALSE;
	pDefragCtx    = reinterpret_cast<PDEFRAGCTX>(Parameter);
	
	/* 
	 * Check if the service has received a shutdown notification and bail if it has
	 */
	if ( !CoServerIsActive() ) {
		FpTrace(TRACE_LEVEL_VERBOSE,
				  SvxFileDefragmenter,
				  L"CoServerIsActive returned TRUE, aborting defragmentation of file, FileName = %p\n",
				  pDefragCtx->FileName);

		pDefragCtx->Result = CO_E_SERVER_STOPPING;
		/* Failure - Abort so we know to pick up DEFRAGCTX::Result */
		return ( ERROR_CANCELLED );
	}

	FpTrace(TRACE_LEVEL_VERBOSE,
			  SvxFileDefragmenter,
			  L"Received defragmention status update, FileName = %p, ClustersTotal = %I64u, ClustersMoved = %I64u\n",
			  pDefragCtx->FileName,
			  ClustersTotal,
			  ClustersMoved);

	/* 
	 * If the client didn't provide a callback, just return 
	 */
	if ( !pDefragCtx->Callback ) {
		/* Success - Continue with defragmentation */
		return ( NO_ERROR );
	}

	/* 
	 * This can be called with ClustersTotal & ClustersMoved both being 0, when the file fits inside the MFT
	 * record and FpDefragmentFile just posts a single completion callback, so make sure we're not about
	 * to divide by 0 
	 */
	if ( 0 == ClustersTotal ) {
		ClustersMoved = 1;
		ClustersTotal = 1;
	}

	/* 
	 * Calculate the percentage completed 
	 */
	iPercent  = static_cast<double>(ClustersMoved) / static_cast<double>(ClustersTotal);
	iPercent *= 100;

	/* 
	 * Get the current system time so we can calculate if we've passed the delay interval
	 */
	GetSystemTimeAsFileTime(&SysTime);
	TimeDiff.LowPart  = SysTime.dwLowDateTime;
	TimeDiff.HighPart = SysTime.dwHighDateTime;

	if ( static_cast<USHORT>(iPercent) > pDefragCtx->Percent ) {
		/* 
		 * The new percent is greater than the last one, so we'll post a callback
		 */
		bUpdateClient = TRUE;
	}
	else if ( TimeDiff.QuadPart >= pDefragCtx->CallTime ) {
		/* 
		 * The calltime delay has expired, so we'll post a callback
		 */
		bUpdateClient = TRUE;
	}

	/* 
	 * If we're doing a callback for any reason update the tracking info
	 */
	if ( bUpdateClient ) {      
		pDefragCtx->Percent  = static_cast<USHORT>(iPercent);
		/* Set the next timeout.. */
		pDefragCtx->CallTime = TimeDiff.QuadPart + static_cast<ULONGLONG>(CALLBACK_DELAY);
	}

	if ( bUpdateClient ) {
		/*
		 * Either the percentage defragmented has changed or we haven't posted a callback
		 * in a bit so we'll send one out to the client
		 */
		hr = SendDefragmentFileUpdateEx(pDefragCtx->Callback,
												  pDefragCtx->FileName,
												  pDefragCtx->Percent,
												  pDefragCtx->Parameter);

		if ( FAILED(hr) ) {
			FpTrace(TRACE_LEVEL_ERROR,
					  SvxFileDefragmenter,
					  L"SendDefragmentFileUpdateEx failed, Callback = %p, FileName = %p, hr = %!HRESULT!\n",
					  pDefragCtx->Callback,
					  pDefragCtx->FileName,
					  hr);

			pDefragCtx->Result = hr;
			/* Failure - Abort so we know to pick up DEFRAGCTX::Result */
			return ( ERROR_CANCELLED );
		}
	}

	/* Success */
	return ( NO_ERROR );
}
