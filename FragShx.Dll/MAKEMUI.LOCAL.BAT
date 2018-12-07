DEL /Q "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll.ln" 2>nul
DEL /Q "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll.mui" 2>nul
MUIRCT.EXE -q MUI.rcconfig "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll" "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll.ln" "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll.mui"
IF ERRORLEVEL=0 (
	MOVE "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll.ln" "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll"
)

MKDIR "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\en-US" 2>nul
RC.EXE -q MUI.rcconfig -fm"D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\en-US\FragShx-en-US.mui.res" -fo"D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\en-US\FragShx-en-US.res" -l 409 -DSXS_PROCESSOR_ARCHITECTURE=AMD64 -DSXS_TARGET=FragShx.dll -DSYSTEM_COMPATIBLE_ASSEMBLY_NAME=Microsoft.Windows.SystemCompatible -DLSYSTEM_COMPATIBLE_ASSEMBLY_NAME=LMicrosoft.Windows.SystemCompatible -DSXS_ASSEMBLY_VERSION= -Iobjchk_wnet_amd64\amd64;D:\DEVKIT\WSK\61\Include;D:\DEVKIT\WTL\80\Include;D:\DEVKIT\WSK\61\VC\Include;..\inc\;..\FragEng.Dll\;..\CoFrag.Dll\;..\FragShx.Dll\;..\FragMgx.Exe\;..\FragSvx.Exe\; FragShx-en-US.rc
IF ERRORLEVEL=0 (
	LINK.EXE /DLL /NOENTRY /MACHINE:AMD64 /OUT:"D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\en-US\FragShx.dll.mui" "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\en-US\FragShx-en-US.mui.res"
	IF ERRORLEVEL=0 (
		MUIRCT.EXE -c "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\FragShx.dll" -e "D:\Projects\Programming\Active\FragExt.WDK\FragShx.Dll\objchk_wnet_amd64\amd64\en-US\FragShx.dll.mui"
	)
)

