FragExt supports multiple languages through the MUI API added in the VISTA SDK. 

1) Only English (US) resources are enabled at the project level (Files named FragXxx-En-Us.rc). English (US) is the ultimate
   fallback language for all projects. Other languages resource DLLs are built after the project.
2) On build completion, a custom build step is run that calls MAKEMUI.BAT, which is located in the Common project. Parameters 
   to MAKEMUI.BAT are:
      a) The current platform (x86|x64)
      b) The output directory. eg. x86\Debug, x64\Release, etc
      c) The base resource script name. eg. FragShx
      d) The target module name. eg FragShx.dll
   
   MAKEMUI.BAT also assumes that a file named Mui.Languages.txt exists in the caller's directory which contains language
   names, one per line, which must match up to resource scripts in the caller's directory.
   
   MAKEMUI.BAT scans Mui.Languages.txt and does the following for each line.
      1) Build a target directory based on the language. eg. en-US, es-ES, de-DE, etc
      2) Compile the resource script for the language into LN and MUI .res files. The resource script must be named
      by the base name passed as parameter (c) and the language name. eg. FragShx-es-Es.rc, FragShx-en-US.rc, etc.
      3) Link the resulting MUI .res file into a resource only DLL file matching the target module name. eg, FragShx.dll.mui
      
   The result of all this is a set of directories containing MUI files which are compatible with the VISTA MUI API that get
   included in the installation package.
   
   
Notes:
VS2005 seems unable to load a DLL if there is more than 1 language subdirectory in the directory the DLL is located. An
error occurs along the lines of "The specified resource language ID cannot be found in the image file".   