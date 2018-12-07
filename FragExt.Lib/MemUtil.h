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

/* MemUtil.h
 *   Various memory related utilities
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#ifndef __MEMUTIL_H__
#define __MEMUTIL_H__

#pragma once

extern "C" void _ReadBarrier();
#pragma intrinsic(_ReadBarrier)

extern "C" void _WriteBarrier();
#pragma intrinsic(_WriteBarrier)

extern "C" void _ReadWriteBarrier();
#pragma intrinsic(_ReadWriteBarrier)

/*
 * Waits for all previous memory reads to complete
 */
#define _ForceMemoryReadCompletion( ) \
   _ReadBarrier(); \
   MemoryBarrier()

/*
 * Waits for all previous memory writes to complete
 */
#define _ForceMemoryWriteCompletion( ) \
   _WriteBarrier(); \
   MemoryBarrier()

/*
 * Waits for all previous memory reads/writes to complete
 */
#define _ForceMemoryReadWriteCompletion( ) \
   _ReadWriteBarrier(); \
   MemoryBarrier()

#endif /* __MEMUTIL_H__ */