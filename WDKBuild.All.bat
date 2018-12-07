PUSHD . 
%COMSPEC% /C "WDKBuild.X86.bat"
POPD 

PUSHD .
%COMSPEC% /C "WDKBuild.X64.bat"
POPD

PUSHD .
%COMSPEC% /C "MakeSRCPKG.bat /srcfile:SourceListing.txt /outfile:out\FragExt-Source.zip" 1>nul
POPD
