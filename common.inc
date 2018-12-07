
#
# Generic build target for executing the MAKEMUI.BAT script on the build output
#
MakeMUI:
   @ECHO Splitting MUI binaries
   ..\MAKEMUI.BAT /platform:$(_BUILDARCH) /out:$O /cultures:MUI.languages /target:$(TARGETNAME).$(TARGETEXT) /rcbase:$(TARGETNAME) /rcconfig:MUI.rcconfig /rcoptions:"$(RCOPTIONS) -I$O$(INCLUDES)"

#
# Custom build target to force deletion of the target file and any associated
# resource files
#
DeleteTarget:
   @ECHO Deleting target file
   DEL /Q $O\$(TARGETNAME).$(TARGETEXT) 2>nul
   DEL /Q $O\*.res 2>nul

#
# Custom target to copy all build files necessary to run the application to a shared output
# directory.
#
CopyToSharedOut:
   echo $(SHARED_OUT_PATHx)
   @ECHO Copying targets to shared out directory
   -SC stop FragSvx
   -CHOICE /N /C y /T 1 /D y
   -MKDIR ..\out\$O\ 
   -COPY /Y $O\$(TARGETNAME).$(TARGETEXT) ..\out\$O\ 
   -COPY /Y $O\$(TARGETNAME).pdb ..\out\$O\ 
   -FOR /F %I IN (MUI.languages) DO MKDIR ..\out\$O\%I & COPY /Y $O\%I\$(TARGETNAME).$(TARGETEXT).mui ..\out\$O\%I\$(TARGETNAME).$(TARGETEXT).mui 

#
# Custom COM registration target that will only register AMD64 debug builds on an AMD64 machine,
# and X86 debug builds on an X86 machine
#
REGSVR_CMD = $(OBJ_PATH)\$O\RegComServer.bat

RegisterTargetSharedOut:
	@type <<$(REGSVR_CMD)
IF "%~x1"==".dll" ( REGSVR32.EXE /c /s "%~f1" ) ELSE IF "%~x1"==".exe" ( "%~1" /REGSERVER )
<<$(BUILD_NOKEEP)
!if ("$(PROCESSOR_ARCHITECTURE)" == "AMD64" || "$(PROCESSOR_ARCHITEW6432)" == "AMD64") && ("$(_BUILDARCH)" == "AMD64")
!if !$(FREEBUILD)
   @ECHO Registering X64 target
   $(REGSVR_CMD) ..\out\$O\$(TARGETNAME).$(TARGETEXT)
!endif
!elseif ("$(PROCESSOR_ARCHITECTURE)" != "AMD64") && ("$(PROCESSOR_ARCHITEW6432)" != "AMD64") && ("$(_BUILDARCH)" != "AMD64") 
!if !$(FREEBUILD)
   @ECHO Registering X86 target
   $(REGSVR_CMD) ..\out\$O\$(TARGETNAME).$(TARGETEXT)
!endif
!endif

#
# These are targets which pull in a local file that isn't included with the project source, to do something 
# specific on the build machine. If the local file exists, it must define a target that matches the one for 
# that build pass. If it does not exist, a default one is used that does nothing
#
!if ("$(BUILD_PASS)" == "PASS0") && exist($(MAKEDIR)\prebuildlocal)
!include $(MAKEDIR)\prebuildlocal
!else
CustomPreBuildLocal_Target:
!endif

!if ("$(BUILD_PASS)" == "PASS1") && exist($(MAKEDIR)\buildlocal)
!include $(MAKEDIR)\buildlocal
!else
CustomBuildLocal_Target:
!endif

!if ("$(BUILD_PASS)" == "PASS2") && exist($(MAKEDIR)\postbuildlocal)
!include $(MAKEDIR)\postbuildlocal
!else
CustomPostBuildLocal_Target:
!endif

#
# These are default targets for the NTTARGETFILES[0|1|2] that a source directory can
# override in its makefile.inc file, to provide custom build rules for each build
# phase. The default ones here do nothing.
#
CustomPreBuild_Target:

CustomBuild_Target:

CustomPostBuild_Target:
