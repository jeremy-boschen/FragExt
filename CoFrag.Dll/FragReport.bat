@ECHO OFF
FOR %%I IN (%1) DO CSCRIPT.EXE //NOLOGO FragInfo.js /f:"%%I" %~2
