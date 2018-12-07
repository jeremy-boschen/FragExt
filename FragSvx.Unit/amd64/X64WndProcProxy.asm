; FragExt - Shell extension for providing file fragmentation
; information.
;
; Copyright (C) 2004-2010 Jeremy Boschen. All rights reserved.
;
; This software is provided 'as-is', without any express or implied
; warranty. In no event will the authors be held liable for any damages
; arising from the use of this software. 
;
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
; claim that you wrote the original software. If you use this software in
; a product, an acknowledgment in the product documentation would be
; appreciated but is not required.
;
; 2. Altered source versions must be plainly marked as such, and must not
; be misrepresented as being the original software.
;
; 3. This notice may not be removed or altered from any source
; distribution.
;

; X64WndProcProxy.asm
;    Proxy function for transferring from a static WndProc to a class member
;    function of the same type.
;
; Copyright (C) 2004-2010 Jeremy Boschen
;

OPTION CASEMAP:none

.CODE _TEXT

PUBLIC ?X64WndProcProxy@@YA_JPEAUHWND__@@I_K_J@Z
?X64WndProcProxy@@YA_JPEAUHWND__@@I_K_J@Z PROC FRAME
   sub      rsp, 56
   .ALLOCSTACK 56
.ENDPROLOG

;
; Shift the parameters passed to this function up one slot so
; we can insert the 'this' pointer as the first argument to the
; target function
;
; R11 and R10 are loaded by __X64WndProcThunk
;

   mov      QWORD PTR [rsp+32], r9
   mov      r9, r8
   mov      r8d, edx
   mov      rdx, rcx
   mov      rcx, r11
   call     r10

   add      rsp, 56
   ret      0
?X64WndProcProxy@@YA_JPEAUHWND__@@I_K_J@Z ENDP
END
