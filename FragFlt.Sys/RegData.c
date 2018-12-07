/* FragExt - File defragmentation software utility toolkit.
 *
 * Copyright (C) 2004-2008 Jeremy Boschen. All rights reserved.
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

/* KeRegData.c
 *    Mini-filter registration declarations. These need to be in a separate
 *    file because of the xxx_seg( "INIT" ) usage.
 *
 * Copyright (C) 2004-2008 Jeremy Boschen
 *
 * Version History
 *    0.0.001 - 12/30/2005 - Created
 */

#include "KeFragFlt.h"

/*
 * This will cause the registration data to be merged into INIT section
 */
#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg( "INIT" )
   #pragma const_seg( "INIT" )
#endif

const FLT_OPERATION_REGISTRATION FKxFltOperationRegistration[] = 
{
   {
      IRP_MJ_CREATE,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      FKxFltPreCreate,
      FKxFltPostCreate
   },
   //{
   //   IRP_MJ_WRITE,
   //   FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
   //   FKxFltPreWrite,
   //   FKxFltPostWrite
   //},
   {
      IRP_MJ_SET_INFORMATION,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      FKxFltPreSetInformation,
      FKxFltPostSetInformation
   },
   {
      IRP_MJ_CLEANUP,
      FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO,
      FKxFltPreCleanup,
      NULL
   },
   {
      IRP_MJ_OPERATION_END
   }
};

const FLT_CONTEXT_REGISTRATION FKxFltContextRegistration[] =
{
   {
      FLT_STREAM_CONTEXT,
      0,
      FKxFltContextCleanup,
      FKX_STREAM_CONTEXT_SIZEOF,
      FKX_TAG_STREAM_CONTEXT,
      NULL,
      NULL,
      NULL
   },
   {
      FLT_CONTEXT_END
   }
};


const FLT_REGISTRATION FKxFltRegistration =
{
    sizeof(FLT_REGISTRATION),          /*  Size                         */
    FLT_REGISTRATION_VERSION,          /*  Version                      */
    0,                                 /*  Flags                        */
    FKxFltContextRegistration,         /*  Context Registration         */
    FKxFltOperationRegistration,       /*  Operation callbacks          */
    FKxFltUnload,                      /*  FilterUnload                 */
    FKxFltInstanceSetup,               /*  InstanceSetup                */
    FKxFltInstanceQueryTeardown,       /*  InstanceQueryTeardown        */
    NULL,                              /*  InstanceTeardownStart        */
    NULL,                              /*  InstanceTeardownComplete     */
    NULL,                              /*  GenerateFileName             */
    NULL,                              /*  GenerateDestinationFileName  */
    NULL                               /*  NormalizeNameComponent       */
};

#ifdef ALLOC_DATA_PRAGMA
   #pragma data_seg()
   #pragma const_seg()
#endif
