;/* FragExt - Shell extension for providing file fragmentation
; * information.
; *
; * Copyright (C) 2004-2008 Jeremy Boschen. All rights reserved.
; *
; * This software is provided 'as-is', without any express or implied
; * warranty. In no event will the authors be held liable for any damages
; * arising from the use of this software. 
; *
; * Permission is granted to anyone to use this software for any purpose,
; * including commercial applications, and to alter it and redistribute it
; * freely, subject to the following restrictions:
; *
; * 1. The origin of this software must not be misrepresented; you must not
; * claim that you wrote the original software. If you use this software in
; * a product, an acknowledgment in the product documentation would be
; * appreciated but is not required.
; *
; * 2. Altered source versions must be plainly marked as such, and must not
; * be misrepresented as being the original software.
; *
; * 3. This notice may not be removed or altered from any source
; * distribution.
; */

;/* Messages.mc
; *    Message text file to build the message table
; *
; * Copyright (C) 2004-2008 Jeremy Boschen
; */


;----------------------------------------------------------------------
;
;	Header Section
;
;----------------------------------------------------------------------

MessageIdTypedef = DWORD
SeverityNames    = 
(
   Success       =  0x0:STATUS_SEVERITY_SUCCESS
   Informational =  0x1:STATUS_SEVERITY_INFORMATIONAL
   Warning       =  0x2:STATUS_SEVERITY_WARNING
   Error         =  0x3:STATUS_SEVERITY_ERROR
)
FacilityNames    =
(
   System        = 0x0:FACILITY_SYSTEM
   Runtime       = 0x2:FACILITY_RUNTIME
   Application   = 0x3:FACILITY_NULL
)
LanguageNames    =
(
   English       = 0x409:MSG00409
)
LanguageNames    =
(
   Japanese      = 0x411:MSG00411
)

;----------------------------------------------------------------------
;
;	Message Definitions
;
;----------------------------------------------------------------------

MessageId=100
Severity=Success
Facility=Application
SymbolicName=MSG_DEFRAGLOG_INITIALIZING
Language=English
Defragmenting %1%0
.

MessageId