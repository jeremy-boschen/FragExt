
#
#  These will cause BUILD to run the custom targets defined in the build directory's
#  makefile.inc file
#
!if exist(makefile.inc)
NTTARGETFILE0 = CustomPreBuild_Target CustomPreBuildLocal_Target
NTTARGETFILE1 = CustomBuild_Target CustomBuildLocal_Target
NTTARGETFILE2 = CustomPostBuild_Target CustomPostBuildLocal_Target
!else
NTTARGETFILE0 = CustomPreBuildLocal_Target
NTTARGETFILE1 = CustomBuildLocal_Target
NTTARGETFILE2 = CustomPostBuildLocal_Target
!endif
