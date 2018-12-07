@set @CMDDUMMYVAR=1 /*
@ECHO OFF

REM This block will be parsed by CMD.EXE, but ignored by JSCRIPT

SET @CMDDUMMYVAR=

CSCRIPT.EXE //E:JSCRIPT //NOLOGO "%~dpnx0" %*

GOTO :EOF

----------------------------------------------------------------
   
MakeSRCPKG.js
   Script to create source install zip file

Parameters:
   /srcfile
      File containing source listing. One file, per line
   /outfile
      Zip file to be created
*/

var __oShell      = WScript.CreateObject('WScript.Shell');
var __oFileSys    = WScript.CreateObject('Scripting.FileSystemObject');

function GetCommandLineVariable( szName, szDefault ) {
   if ( WScript.Arguments.Named.Exists(szName) ) {
      return ( WScript.Arguments.Named.Item(szName) );
   }
   return ( szDefault );
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

/* File extensions for files which should be parsed */ 
var rgExt = [
   /\.sln$/, 
   /\.vcproj$/
];  

/* Start, End tags to search for in each file */
var rgFnd = [
   {Start : /[\t]GlobalSection\(SourceCodeControl\) \= preSolution\r\n/, End : /EndGlobalSection\r\n/, EndLength : 'EndGlobalSection\r\n'.length},
   {Start : /[\t]SccProjectName\=/,  End : /\r\n/, EndLength: 2},
   {Start : /[\t]SccAuxPath\=/,      End : /\r\n/, EndLength: 2},
   {Start : /[\t]SccLocalPath\=/,    End : /\r\n/, EndLength: 2},
   {Start : /[\t]SccProvider\=/,     End : /\r\n/, EndLength: 2}
];

/* Removes items in rgFnd from oFile */
function SccProcessFile( oFile ) {
   WScript.Echo('Removing SCC information from file "' + oFile.Path + '"');
   
   var oStream   = __oFileSys.OpenTextFile(oFile.Path, 1);
   var szFileTxt = oStream.ReadAll();
   oStream.Close();
   
   for ( var i = 0; i < rgFnd.length; i++ ) {
      var iB = szFileTxt.search(rgFnd[i].Start);
      if ( -1 != iB ) {
         var iE = szFileTxt.substr(iB).search(rgFnd[i].End);
         if ( -1 != iE && Number.NaN != iE ) {
            var sB = szFileTxt.substr(0, iB);
            var sE = szFileTxt.substr(iB + iE + rgFnd[i].EndLength);
            szFileTxt = sB + sE;
         }
      }
   }
   
   /* Turn off the read-only bit on the file */
   //oFile.Attributes = oFile.Attributes & ~1;
   /* Overwrite the original file with the new text */
   oFile = __oFileSys.CreateTextFile(oFile.Path, true, false);
   oFile.Write(szFileTxt);
   oFile.Close();   
}

/* Enumerates over all files in a folder, then all subfolders */
function SccProcessFolder( oFolder ) {
   var oEnum = new Enumerator(oFolder.Files);
   
   while ( !oEnum.atEnd() ) {
      var oFile = oEnum.item();
      /* Check if the file's extention matches those in rgExt */
      for ( var i = 0; i < rgExt.length; i++ ) {
         if ( rgExt[i].test(oFile.Name) ) {
            SccProcessFile(oFile);
         }
      }
      oEnum.moveNext();
   }
   
   oEnum = new Enumerator(oFolder.SubFolders);
   while ( !oEnum.atEnd() ) {
      SccProcessFolder(oEnum.item());
      oEnum.moveNext();
   }
}

function DumpStandardStreams( oObj ) {
   if ( !oObj.StdOut.AtEndOfStream ) {
      WScript.StdOut.Write(oObj.StdOut.ReadAll());
   }
   
   if ( !oObj.StdErr.AtEndOfStream ) {
      WScript.StdErr.Write(oObj.StdErr.ReadAll());
   }
}

function ExecCommand( szCmd ) {
   WScript.Echo('Running command line: ' + szCmd);
   var oExecObj = __oShell.Exec(szCmd);
   while ( 1 ) {
      DumpStandardStreams(oExecObj);
      
      if ( 1 == oExecObj.Status ) {
         break;
      }
      
      WScript.Sleep(100);
   }
   
   DumpStandardStreams(oExecObj);
   
   return ( oExecObj.ExitCode );
}
   

var SZ_ZIPUTIL = __oFileSys.BuildPath(__oShell.ExpandEnvironmentStrings('%PROGRAMFILES%'), '7-Zip\\7z.exe');
if ( !__oFileSys.FileExists(SZ_ZIPUTIL) ) {
   SZ_ZIPUTIL = __oFileSys.BuildPath(__oShell.ExpandEnvironmentStrings('%PROGRAMFILES(x86)%'), '7-Zip\\7z.exe');
   if ( !__oFileSys.FileExists(SZ_ZIPUTIL) ) {
      SZ_ZIPUTIL = __oFileSys.BuildPath(__oShell.ExpandEnvironmentStrings('%PROGRAMFILES%'), '7-Zip\\7z.exe').replace(' (x86)', '');
      if ( !__oFileSys.FileExists(SZ_ZIPUTIL) ) {
         WScript.StdErr.WriteLine('MakeSRCPKG.BAT - Failed to locate zip utility');
         WScript.Quit(-1);
      }
   }
}

var szSrcFile = GetCommandLineVariable('srcfile', null);
var szOutFile = GetCommandLineVariable('outfile', null);

if ( !szSrcFile || !__oFileSys.FileExists(szSrcFile) || !szOutFile ) {
   WScript.StdErr.WriteLine('MakeSRCPKG.BAT - Invalid or missing command line parameter');
   WScript.Quit(-1);
}

/* Build a temporary directory for copying the source files to.. */
var szTempDir = __oFileSys.BuildPath(__oFileSys.GetSpecialFolder(2/*TemporaryFolder*/), __oFileSys.GetTempName());
if ( !__oFileSys.CreateFolder(szTempDir) ) {
   WScript.StdErr.WriteLine('MakeSRCPKG.BAT - Unable to create temporary directory');
   WScript.Quit(-1);
}

WScript.Echo('MakeSRCPKG.BAT - Copying source tree to ' + szTempDir);

var iExitCode = -1;
var oSrcFile  = null;

do {
   szSrcFile = __oFileSys.GetAbsolutePathName(szSrcFile);   
   oSrcFile  = __oFileSys.OpenTextFile(szSrcFile, 1, 0);
   if ( !oSrcFile ) {
      break;
   }
   
   var szSrcDir = __oFileSys.GetParentFolderName(szSrcFile);
   
   while ( !oSrcFile.AtEndOfStream ) {
      var szFile = oSrcFile.ReadLine();
      
      var szSrc = __oFileSys.BuildPath(szSrcDir, szFile);
      var szDst = __oFileSys.BuildPath(szTempDir, szFile);
      
      /* Walk the destination path and ensure that the folders exist */
      var szFolder = __oFileSys.GetParentFolderName(szDst);
      if ( !__oFileSys.FolderExists(szFolder) ) {
         var rgFolder = szFolder.split('\\');
         var szFolder = '';
         
         for ( var idx = 0; idx < rgFolder.length; idx++ ) {
            szFolder += rgFolder[idx] + '\\';
            if ( !__oFileSys.FolderExists(szFolder) ) {
               __oFileSys.CreateFolder(szFolder);
            }           
         }         
      }
      WScript.Echo('Copying ' + szSrc);
      __oFileSys.CopyFile(szSrc, szDst, true);
   }
   
   /* Clear read-only attributes on everything in the directory */
   var szAttribCmd = 'ATTRIB.EXE -R \"' + __oFileSys.BuildPath(szTempDir, '\\*') + '\" /S';
   WScript.Echo('Clearing read-only attributes');
   iExitCode = ExecCommand(szAttribCmd);
   
   /* Strip all source provider info from the project files.. */
   SccProcessFolder(__oFileSys.GetFolder(szTempDir));   
   
   /* Zip everything up.. */
   var szZipCmd = '\"' + SZ_ZIPUTIL + '\" a -tzip ' + szOutFile + ' \"' + __oFileSys.BuildPath(szTempDir, '\\*') + '\"';
   WScript.Echo('Zipping files');
   iExitCode = ExecCommand(szZipCmd);
} while ( 0 );

if ( null != oSrcFile ) {
   oSrcFile.Close();
}

__oFileSys.DeleteFolder(szTempDir, true);
WScript.Quit(iExitCode);
