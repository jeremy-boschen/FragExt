
WSK_INC_PATH      = $(WSKROOT)\Include
WTL_INC_PATH      = $(WTLROOT)\Include

!if "$(_BUILDARCH)"=="AMD64"
WSK_LIB_PATH      = $(WSKROOT)\LIB\x64
!else
WSK_LIB_PATH      = $(WSKROOT)\LIB
!endif

INCLUDES = \
   $(INCLUDES);            \
   $(WSK_INC_PATH);        \
   $(WTL_INC_PATH);        \
   $(WSKROOT)\VC\Include;  \
   ..\inc\;                \
   ..\FragExt.Lib\;        \
   ..\FragEng.Dll\;        \
   ..\CoFrag.Dll\;         \
   ..\FragShx.Dll\;        \
   ..\FragMgx.Exe\;        \
   ..\FragSvx.Exe\;        \
   $(DDK_INC_PATH);        \

!if $(FREEBUILD)
USE_MSVCRT        = 1
!else
MSC_OPTIMIZATION  = /Odi
USE_LIBCMT        = 1
DEBUG_CRTS        = 1
C_DEFINES         = $(C_DEFINES) /D_CRTDBG_MAP_ALLOC
# /D_ATL_DEBUG_INTERFACES /D_ATL_DEBUG_QI
!endif

C_DEFINES         = $(C_DEFINES) /D_WDKBUILD /D_UNICODE /DUNICODE /DSTRICT /DSTRSAFE_NO_DEPRECATE /DISOLATION_AWARE_ENABLED=1 /DNO_SHLWAPI_PATH /DGUIDDB_LIB_PATH="$(SDK_LIB_PATH:\=\\)" /DFRAGEXT_NTDDI_MINIMUM=NTDDI_WINXP
USER_C_FLAGS      = $(USER_C_FLAGS) /wd4201 /wd4505

LINKER_FLAGS      = $(LINKER_FLAGS) /IGNORE:4505

!if "$(_BUILDARCH)" == "AMD64"
!message Setting LTCG linker option for AMD64 build
LINKER_FLAGS      = $(LINKER_FLAGS) /LTCG /OPT:LBR
!endif

LINK_ICF_COUNT    = 32

MSC_WARNING_LEVEL = /W4

UNICODE           = 1
UMTYPE            = windows

# This is set explicitly so we can build for previous versions using new features in the headers,
# but still use the downlevel linker settings

!if "$(_BUILDARCH)" == "X86"
_NT_TARGET_VERSION=$(_NT_TARGET_VERSION_WINXP)
!elseif "$(_BUILDARCH)" == "AMD64"
_NT_TARGET_VERSION=$(_NT_TARGET_VERSION_WS03)
!endif
WIN32_WINNT_VERSION=$(LATEST_WIN32_WINNT_VERSION)
NTDDI_VERSION=$(LATEST_NTDDI_VERSION)

PROJECT_ROOT      = ..\

RUN_WPP = \
   -scan:..\inc\WppConfig.h   \
   -ext:.cpp.c.h              \
   -dll                       \
   $(SOURCES)
