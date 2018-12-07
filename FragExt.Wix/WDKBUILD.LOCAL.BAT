SET VS_UNICODE_OUTPUT=
CALL D:\DEVKIT\DDK\6001.18002\bin\setenv.bat D:\DEVKIT\DDK\6001.18002 CHK x64 WNET
CD D:\Projects\Programming\Active\FragExt.WDK\FragExt.Wix
DEL /Q buildchk_wnet_amd64.err 2>nul
DEL /Q buildchk_wnet_amd64.wrn 2>nul
DEL /Q buildchk_wnet_amd64.log 2>nul
SET PATH=%PATH%;%WSKROOT%\bin;%WIXROOT%
BUILD -begFW -j buildchk_wnet_amd64 -Z
