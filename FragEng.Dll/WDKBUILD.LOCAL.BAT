SET VS_UNICODE_OUTPUT=
CALL D:\DEVKIT\DDK\7600.16385.1\bin\setenv.bat D:\DEVKIT\DDK\7600.16385.1 CHK x86 WXP no_oacr 
CD D:\Projects\Programming\Active\FragExt.WDK\FragEng.Dll
DEL /Q buildchk_wxp_x86.err 2>nul
DEL /Q buildchk_wxp_x86.wrn 2>nul
DEL /Q buildchk_wxp_x86.log 2>nul
SET PATH=%PATH%;%WSKROOT%\bin;%WIXROOT%
BUILD -begFW -j buildchk_wxp_x86
