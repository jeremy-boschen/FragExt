
!INCLUDE ..\common.inc

CustomPreBuild_Target: DeleteTarget CustomPreBuildLocal_Target

CustomBuild_Target: CustomBuildLocal_Target

CustomPostBuild_Target: CopyToSharedOut CustomPostBuildLocal_Target
