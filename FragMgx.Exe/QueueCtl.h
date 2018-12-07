/* FragExt - Shell extension for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2010 Jeremy Boschen. All rights reserved.
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
 
/* QueueCtl.h
 *    IQueueController definitions
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#pragma once

#include <ObjBase.h>

/* 
 * Callback used by CQueueController to post defrag updates to its client 
 */
typedef enum _DEFRAGUPDATECODE
{
   DC_INITIALIZING   = 0x00000001,
   DC_DEFRAGMENTING  = 0x00000002,
   DC_COMPLETED      = 0x00000003
}DEFRAGUPDATECODE, *PDEFRAGUPDATECODE;

typedef 
DWORD
(CALLBACK* PQC_DEFRAGUPDATE_ROUTINE)( 
   __in DEFRAGUPDATECODE UpdateCode, 
   __in DWORD Status, 
   __in_z LPCWSTR FileName, 
   __in LONG PercentComplete, 
   __in_opt PVOID Parameter 
);



/* CQueueController
 *    
 */
class CQueueController 
{
public:
   CQueueController( 
   ) throw();

   ~CQueueController(
   ) throw();

   DWORD   
   Initialize( 
   );

   DWORD
   Uninitialize( 
   );

   DWORD
   SetQueueUpdateRoutine( 
      __in_opt PFQ_UPDATE_ROUTINE pCallback, 
      __in_opt PVOID pParameter 
   );

   DWORD
   SetDefragUpdateRoutine( 
      __in_opt PQC_DEFRAGUPDATE_ROUTINE pCallback, 
      __in_opt PVOID pParameter 
   );

   DWORD
	InsertQueueFile( 
      __in_z LPCWSTR FileName 
   );

   DWORD
	DeleteQueueFile( 
      __in_z LPCWSTR FileName 
   );

   DWORD
	FlushQueue( 
   );

   DWORD
	SuspendDefragmenter( 
      DWORD dwTimeout 
   );

   DWORD
	ResumeDefragmenter(
   );

private:
   CQueueController( const CQueueController& );
   operator =( const CQueueController& );
};
