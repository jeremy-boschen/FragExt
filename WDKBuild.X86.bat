PUSHD . 
CALL D:\DEVKIT\DDK\6001.18000\bin\setenv.bat D:\DEVKIT\DDK\6001.18000 FRE X86 W2K
POPD 
CD D:\Projects\Programming\Active\FragExt.WDK
DEL /Q buildfre_w2k_x86.err 2>nul
DEL /Q buildfre_w2k_x86.wrn 2>nul
DEL /Q buildfre_w2k_x86.log 2>nul
SET PATH=%PATH%;%WSKROOT%\bin;%WIXROOT%
BUILD -cbegFW -j buildfre_w2k_x86
IF EXIST buildfre_w2k_x86.wrn TYPE buildfre_w2k_x86.wrn
