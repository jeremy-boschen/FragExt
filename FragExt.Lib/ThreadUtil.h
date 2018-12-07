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

/* ThreadUtil.h
 *    Generic thread creation routines which manage creating threads inside
 *    a DLL
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 */

#pragma once

#ifndef __THREADUTIL_H__
#define __THREADUTIL_H__

/*
 * Use _beginthreadex to create the thread, otherwise CreateThread. This
 * flag is combined with standard CreateThread flags
 */
#define TXF_CRTTHREAD   0x80000000U

DWORD 
APIENTRY
TxCreateThread(
   __in_opt LPSECURITY_ATTRIBUTES ThreadAttributes,
   __in SIZE_T StackSize,
   __in LPTHREAD_START_ROUTINE StartRoutine, 
   __in_opt PVOID StartParameter, 
   __in DWORD Flags,
   __out_opt PHANDLE ThreadHandle, 
   __out_opt PDWORD ThreadId
);

void 
APIENTRY
TxSetThreadInitComplete( 
   __in DWORD CompleteStatus
);

#endif /* __THREADUTIL_H__ */