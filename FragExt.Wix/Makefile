
#
# DO NOT EDIT THIS FILE!!!  Edit .\sources. if you want to add a new source
# file to this component.  This file merely indirects to the real make file
# that is shared by all the driver components of the Windows NT DDK
#

!INCLUDE $(NTMAKEENV)\makefile.def

#
#  WiX build 
#

.SUFFIXES : .wxs .wixobj

WIX_COMPILER   = $(WIXROOT)\CANDLE.EXE
WIX_LINKER     = $(WIXROOT)\LIGHT.EXE
WIXC_FLAGS     = -sw1044 -nologo -w0 -v -pedantic -d_BUILDARCH=$(_BUILDARCH) -dBUILDENV=$(DDKBUILDENV) -dO=$O -ext WixUtilExtension 
WIXL_FLAGS     = -v -w0 -nologo -ext WixUIExtension -ext WixUtilExtension $(WIXL_FLAGS)
!IF !$(FREEBUILD)
WIXL_FLAGS     = $(WIXL_FLAGS) -sval
!ENDIF

.wxs.obj:
   $(WIX_COMPILER) $(WIXC_FLAGS) /out $O\ $<

.wxs{$O}.obj:
   $(WIX_COMPILER) $(WIXC_FLAGS) /out $O\ $<
   
.wxs{$(OBJ_PATH)\$O}.obj:
   $(WIX_COMPILER) $(WIXC_FLAGS) /out $O\ $<

$(OBJ_PATH)\$O\$(TARGETNAME).msi:
   $(WIX_LINKER) $(WIXL_FLAGS) /out $O\$(TARGETNAME).msi $(OBJ_PATH)\$O\*.wixobj


