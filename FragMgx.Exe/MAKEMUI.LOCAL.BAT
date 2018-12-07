DEL /Q "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe.ln" 2>nul
DEL /Q "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe.mui" 2>nul
MUIRCT.EXE -q MUI.rcconfig "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe" "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe.ln" "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe.mui"
IF ERRORLEVEL=0 (
	MOVE "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe.ln" "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe"
)

MKDIR "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\en-US" 2>nul
RC.EXE -q MUI.rcconfig -fm"D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\en-US\FragMgx-en-US.mui.res" -fo"D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\en-US\FragMgx-en-US.res" -l 409 -DSXS_ASSEMBLY_NAME=jBoschen.FragExt.FragMgx -DSXS_LANGUAGE=0000 -DSXS_ASSEMBLY_LANGUAGE=0000 -DSXS_PROCESSOR_ARCHITECTURE=AMD64 -DSXS_TARGET=FragMgx.exe -DSYSTEM_COMPATIBLE_ASSEMBLY_NAME=Microsoft.Windows.SystemCompatible -DLSYSTEM_COMPATIBLE_ASSEMBLY_NAME=LMicrosoft.Windows.SystemCompatible -DSXS_ASSEMBLY_VERSION=1.0.0.0 -DSXS_MANIFEST_OBJ1=\d:/projects/programming/active/fragext.wdk/fragmgx.exe/objfre_wnet_amd64/amd64/FragMgx.o_manifest\ -Iobjfre_wnet_amd64\amd64;D:\DEVKIT\WSK\61\Include;D:\DEVKIT\WTL\80\Include;D:\DEVKIT\WSK\61\VC\Include;..\inc\;..\FragEng.Dll\;..\CoFrag.Dll\;..\FragShx.Dll\;..\FragMgx.Exe\;..\FragSvx.Exe\;D:\DEVKIT\DDK\6001.18002\inc\ddk; FragMgx-en-US.rc
IF ERRORLEVEL=0 (
	LINK.EXE /DLL /NOENTRY /MACHINE:AMD64 /OUT:"D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\en-US\FragMgx.exe.mui" "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\en-US\FragMgx-en-US.mui.res"
	IF ERRORLEVEL=0 (
		MUIRCT.EXE -c "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\FragMgx.exe" -e "D:\Projects\Programming\Active\FragExt.WDK\FragMgx.Exe\objfre_wnet_amd64\amd64\en-US\FragMgx.exe.mui"
	)
)

