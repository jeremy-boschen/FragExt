
.386
.MODEL FLAT, STDCALL
OPTION CASEMAP:none
OPTION PROLOGUE:None
OPTION EPILOGUE:None

_TEXT SEGMENT

.DATA

.CODE

extern WndProc : near

Thunk PROC

   pop   eax
   push  dword ptr 0x11223344
   push  eax
   jmp   WndProc
   
Thunk ENDP

_TEXT ENDS

END
