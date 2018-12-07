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

; X86WndProcProxy.asm
;    Proxy function for transferring from a static WndProc to a class member
;    function of the same type.
;
; Copyright (C) 2004-2010 Jeremy Boschen
;

.386
.MODEL FLAT, STDCALL

OPTION CASEMAP:none
OPTION PROLOGUE:None
OPTION EPILOGUE:None

_TEXT SEGMENT

.CODE

EXTERN WndProc:PROC

PUBLIC X86WndProcProxy

X86WndProcProxy PROC

   pop   eax
   push  dword ptr 00112233h
   push  eax
   jmp   offset WndProc   

X86WndProcProxy ENDP

_TEXT ENDS

END

