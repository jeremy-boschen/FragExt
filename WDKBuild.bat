@set @CMDDUMMYVAR=1 /*
@ECHO OFF

REM This block will be parsed by CMD.EXE, but ignored by JSCRIPT

SET @CMDDUMMYVAR=

DEL /Q WDKBUILD.LOCAL.BAT 2>nul

IF EXIST WDKBUILD.ENV.BAT (
   CALL WDKBUILD.ENV.BAT
) ELSE IF EXIST ..\WDKBUILD.ENV.BAT (
   CALL ..\WDKBUILD.ENV.BAT
) ELSE IF EXIST ..\..\WDKBUILD.ENV.BAT (
	CALL ..\..\WDKBUILD.ENV.BAT
)

CSCRIPT.EXE //E:JSCRIPT //NOLOGO "%~dpnx0" %*

IF ERRORLEVEL=0 (
   CALL WDKBUILD.LOCAL.BAT
)

GOTO :EOF

----------------------------------------------------------------
WDKBuild.js - Script for generating the WDKBUILD.LOCAL.BAT file
which invokes the WDK BUILD utility.
 */

var szWDKRoot;
var szBuildDir;
var szBuildType;
var szPlatform;
var szTargetOS;
var szBuildCmd;
var szLogName;
var bPrefast;
var bOACR;

WScript.Echo('\nWDKBuild.js - Building WDKBUILD.LOCAL.BAT');

var __oShell = WScript.CreateObject('WScript.Shell');
var __oFileSys = WScript.CreateObject('Scripting.FileSystemObject');
var __rgWDKVer = [
    'HKLM\\SOFTWARE\\Microsoft\\KitSetup\\configured-kits\\{B4285279-1846-49B4-B8FD-B9EAF0FF17DA}\\{515A5454-555D-5459-5B5D-616264656660}\\setup-install-location',
    'HKLM\\SOFTWARE\\Microsoft\\WINDDK\\6001.18000\\Setup\\BUILD',
    'HKLM\\SOFTWARE\\Microsoft\\WINDDK\\6001.17121\\Setup\\BUILD'];

function GetDirectory(szName) {
	return (__oFileSys.GetAbsolutePathName(szName));
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

function GetCommandLineVariable(szName, szDefault) {
	if (WScript.Arguments.Named.Exists(szName)) {
		return (WScript.Arguments.Named.Item(szName));
	}
	return (szDefault);
}

function IsValidFolder(szFolder) {
	return ((null != szFolder) && ('' != szFolder) && __oFileSys.FolderExists(szFolder) ? true : false);
}

/* Load command options */
szWDKRoot = GetEnvironmentVariable('WDKROOT');

if (!IsValidFolder(szWDKRoot)) {
	/* Try to get the WDK path from the registry.. */
	for (var idx = 0; idx < __rgWDKVer.length; idx++) {
		try {
			szWDKRoot = __oShell.RegRead(__rgWDKVer[idx]);
		} catch (e) {
			szWDKRoot = null;
		}

		if (IsValidFolder(szWDKRoot)) {
			WScript.Echo('WDKBuild.js - Located WDK root @ ' + __rgWDKVer[idx]);
			break;
		}
	}

	if (!IsValidFolder(szWDKRoot)) {
		WScript.Echo('WDKBuild.js - Error: Unable to determine WDK BUILD directory from ENVIRONMENT or REGISTRY.');
		WScript.Quit(-1);
	}
}

szBuildDir = GetDirectory('.');
szBuildType = GetCommandLineVariable('build', '');
szPlatform = GetCommandLineVariable('platform', 'X86');
szTargetOS = GetCommandLineVariable('target', 'WNET');
bPrefast = GetCommandLineVariable('prefast', false);
bOACR = GetCommandLineVariable('oacr', false);
bSeparateObjRoot = GetCommandLineVariable('objs', false);

szLogName = new String('build' + szBuildType + '_' + szTargetOS + '_' + szPlatform).toLowerCase().replace('x64', 'amd64');

WScript.Echo('WDKBuild.js - WDKROOT found at: ' + szWDKRoot);

szBuildCmd =  'SET VS_UNICODE_OUTPUT=\r\n';
szBuildCmd += 'CALL ' + szWDKRoot + '\\bin\\setenv.bat ' + szWDKRoot + ' ' + szBuildType + ' ' + szPlatform + ' ' + szTargetOS + (bOACR ? ' oacr ' : ' no_oacr ') + (bSeparateObjRoot ? ' objs ' : '') + '\r\n';
szBuildCmd += 'CD ' + szBuildDir + '\r\n';
szBuildCmd += 'DEL /Q ' + szLogName + '.err 2>nul\r\n';
szBuildCmd += 'DEL /Q ' + szLogName + '.wrn 2>nul\r\n';
szBuildCmd += 'DEL /Q ' + szLogName + '.log 2>nul\r\n';
szBuildCmd += 'SET PATH=%PATH%;%WSKROOT%\\bin;%WIXROOT%\r\n';
if (bPrefast) {
	szBuildCmd += 'PREFAST /list /filter /FilterPreset=\"(all defects)\" /StackHogThreshold=4096 /reset /log=\"' + __oFileSys.BuildPath(szBuildDir, '\\defectlog.xml') + '\" ';
}
szBuildCmd += 'BUILD -begFW -j ' + szLogName;
var rgCmd = WScript.Arguments.Unnamed;
for (var i = 0; i < rgCmd.length; i++) {
	szBuildCmd += ' ' + rgCmd.Item(i);
}
szBuildCmd += '\r\n';
//szBuildCmd += 'IF EXIST ' + szLogName + '.wrn TYPE ' + szLogName + '.wrn\r\n';

var szBuildPath = __oFileSys.BuildPath('.\\', 'WDKBUILD.LOCAL.BAT');
var oBuildFile = __oFileSys.CreateTextFile(szBuildPath, true, false);
if (null != oBuildFile) {
	oBuildFile.Write(szBuildCmd);
	oBuildFile.Close();

	WScript.Quit(0);
}

WScript.Quit(-1);
