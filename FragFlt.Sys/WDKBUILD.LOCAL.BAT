PUSHD . 
CALL D:\DEVKIT\DDK\6000\bin\setenv.bat D:\DEVKIT\DDK\6000 CHK AMD64 WNET
POPD 
CD D:\Projects\Programming\Active\FragExt.WDK\FragFlt.Sys
BUILD -beFW -cZ
