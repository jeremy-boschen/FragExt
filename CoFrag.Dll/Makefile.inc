
!INCLUDE ..\common.inc

CustomPreBuild_Target: DeleteTarget CustomPreBuildLocal_Target

CustomBuild_Target: CustomBuildLocal_Target
    -COPY /Y $O\CoFrag.h .\CoFrag.h
    
CustomPostBuild_Target: CopyToSharedOut RegisterTargetSharedOut CustomPostBuildLocal_Target
