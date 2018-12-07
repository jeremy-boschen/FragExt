
!INCLUDE ..\common.inc

CustomPreBuild_Target: DeleteTarget CustomPreBuildLocal_Target

CustomBuild_Target: CustomBuildLocal_Target

CustomPostBuild_Target: \
!IF $(FREEBUILD)
   MakeMUI \
!ENDIF
   CopyToSharedOut RegisterTargetSharedOut CustomPostBuildLocal_Target
   COPY /Y "$(OBJ_PATH)\$O\FragMgx.h" "$(MAKEDIR_LOWERCASE:fragmgx.exe=)inc\FragMgx.h"

