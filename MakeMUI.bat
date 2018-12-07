@set @CMDDUMMYVAR=1 /*
@ECHO OFF

REM This block will be parsed by CMD.EXE, but ignored by JSCRIPT

SET @CMDDUMMYVAR=

DEL /Q MAKEMUI.LOCAL.BAT 2>nul

CSCRIPT.EXE //E:JSCRIPT //NOLOGO "%~dpnx0" %*

IF ERRORLEVEL=0 (
   MAKEMUI.LOCAL.BAT
)

GOTO :EOF

----------------------------------------------------------------
MakeMUI.bat
   Builds MUI resource files for supported languages
    
Parameters:
   /platform
      One of x86, x64, IA64
   /out
      Output directory
   /cultures
      File with cultures to build, one per line
   /target
      Target file name
   /rcbase
      Base resource file name used to construct full language 
      resource file names, of basename-language.rc
   /rcconfig
      Name of .rcconfig file to be passed to MUIRCT.EXE
   /rcoptions
      Options to be passed to the resource compiler
   
Example:
   To compile X86 MUI resources:
      MAKEMUI.BAT /platform:x86 /out:x86\Debug /cultures:MUI.languages /target:MyTarget.dll /rcbase:MyResFile /rcconfig:MUI.rcconfig /rcoptions:"-I.\some_include_dir"      
 */

var SZ_MUIRCT_CMD = 'MUIRCT.EXE';
var SZ_RC_CMD     = 'RC.EXE';
var SZ_LINK_CMD   = 'LINK.EXE';

var __oShell      = WScript.CreateObject('WScript.Shell');
var __oFileSys    = WScript.CreateObject('Scripting.FileSystemObject');

function GetDirectory( szName ) {
    return ( __oFileSys.GetAbsolutePathName(szName) );
}

function GetEnvironmentVariable(szName) {
   var oUser = __oShell.Environment('Process');
   var oSys = __oShell.Environment('System');

   var szVar = oUser(szName);
   if (!szVar) {
      szVar = oSys(szName);
   }
   return (szVar);
}

function GetCommandLineVariable( szName, szDefault ) {
    if ( WScript.Arguments.Named.Exists(szName) ) {
        return ( WScript.Arguments.Named.Item(szName) );
    }
    return ( szDefault );
}

function Echo( szMsg ) {
   WScript.Echo('MakeMUI.BAT - ' + szMsg);
}

/* Get command line parameters.. */
var szPlatform     = GetCommandLineVariable('platform', 'x86');
var szBuildDir     = GetCommandLineVariable('out', '.');
var szCultureFile  = GetCommandLineVariable('cultures', null);
var szTargetFile   = GetCommandLineVariable('target', null);
var szRCBaseFile   = GetCommandLineVariable('rcbase', null);
var szRCConfigFile = GetCommandLineVariable('rcconfig', 'MUI.rcconfig');
var szRCParameters = GetCommandLineVariable('rcoptions', '');

/* Validate parameters.. */
if ( !szCultureFile || !__oFileSys.FileExists(szCultureFile) || !szTargetFile || !szRCBaseFile ) {
   Echo('Missing command line parameters');
   WScript.Quit(-1);
}

szBuildDir = GetDirectory(szBuildDir);

/* We build up a file called MAKEMUI.LOCAL.BAT then run it */
var szBuildCmd = '';

/* Create the language neutral file.. */
var szLNFile = __oFileSys.BuildPath(szBuildDir, szTargetFile);

szBuildCmd += 'DEL /Q \"' + szLNFile + '.ln\" 2>nul\r\n';
szBuildCmd += 'DEL /Q \"' + szLNFile + '.mui\" 2>nul\r\n';
szBuildCmd += SZ_MUIRCT_CMD + ' -q ' + szRCConfigFile + ' \"' + szLNFile + '\" \"' + szLNFile + '.ln\" \"' + szLNFile + '.mui\"\r\n';
szBuildCmd += 'IF ERRORLEVEL=0 (\r\n';
szBuildCmd += '\tMOVE \"' + szLNFile + '.ln\" \"' + szLNFile + '\"\r\n';
szBuildCmd += ')\r\n';
szBuildCmd += '\r\n';

/* Generate the statements for each culture.. */
var oCultureFile = __oFileSys.OpenTextFile(szCultureFile, 1, 0);
if ( !oCultureFile ) {
   Echo('Unable to open culture file - ' + szCultureFile);
   WScript.Quit(-1);
}

while ( !oCultureFile.AtEndOfStream ) {
   var szCulture = ('' + oCultureFile.ReadLine()).replace(' ', '');
   if ( '' != szCulture ) {
      var szMUIDir    = __oFileSys.BuildPath(szBuildDir, szCulture);
      var szMUIFile   = __oFileSys.BuildPath(szMUIDir, szRCBaseFile + '-' + szCulture);
      var szMUITarget = __oFileSys.BuildPath(szMUIDir, szTargetFile);
      
      szBuildCmd += 'MKDIR \"' + szMUIDir + '\" 2>nul\r\n';
      szBuildCmd += SZ_RC_CMD + ' -q ' + szRCConfigFile + ' -fm\"' + szMUIFile + '.mui.res\" -fo\"' + szMUIFile + '.res\" ' + szRCParameters + ' ' + szRCBaseFile + '-' + szCulture + '.rc\r\n';
      szBuildCmd += 'IF ERRORLEVEL=0 (\r\n';
      szBuildCmd += '\t' + SZ_LINK_CMD + ' /DLL /NOENTRY /MACHINE:' + szPlatform + ' /OUT:\"' + szMUITarget + '.mui\" \"' + szMUIFile + '.mui.res\"\r\n';
      szBuildCmd += '\tIF ERRORLEVEL=0 (\r\n';
      szBuildCmd += '\t\t' + SZ_MUIRCT_CMD + ' -c \"' + szLNFile + '\" -e \"' + szMUITarget + '.mui\"\r\n';
      szBuildCmd += '\t)\r\n';
      szBuildCmd += ')\r\n';
      szBuildCmd += '\r\n';
   }
}

var szBuildPath= __oFileSys.BuildPath('.', 'MAKEMUI.LOCAL.BAT');
var oBuildFile = __oFileSys.CreateTextFile(szBuildPath, true, false);
if ( null != oBuildFile ) {
    oBuildFile.Write(szBuildCmd);
    oBuildFile.Close();
    
    WScript.Quit(0);
}

WScript.Quit(-1);
