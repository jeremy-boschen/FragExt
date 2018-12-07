@set @CMDDUMMYVAR=1 /*
@ECHO OFF

REM This block will be parsed by CMD.EXE, but ignored by JSCRIPT

SET @CMDDUMMYVAR=

CSCRIPT.EXE //E:JSCRIPT //NOLOGO "%~dpnx0" %*

GOTO :EOF

----------------------------------------------------------------

UpdateVersion.js - Script for generating updating version numbers
   
 */
 
var szDirectory;
var szFileTypes;
var szPatternFile;

var rgPattern = 
[
   /* VS_VERSION_INFO version statements */
   {Expression : '(.*VERSION.*)(\d+)([\.,])(\d+)([\.,])(\d+)([\.,])(\d+)(.*)',        Modifier : '$1$2!major$3$4!minor$5$6!build$7$8!revision$9'},
   {Expression : '(.*VALUE.*Version.*)(\d+)([\.,])(\d+)([\.,])(\d+)([\.,])(\d+)(.*)', Modifier : '$1$2!major$3$4!minor$5$6!build$7$8!revision$9'},
   /* WiX */
   {Expression : '(.*ProductVersion.*)(\d+)([\.,])(\d+)([\.,])(\d+)([\.,])(\d+)(.*)', Modifier : '$1$2!major$3$4!minor$5$6!build$7$8!revision$9'}
];
 

var __oShell   = WScript.CreateObject('WScript.Shell');
var __oFileSys = WScript.CreateObject('Scripting.FileSystemObject');

function GetCommandLineVariable( szName, szDefault ) {
    if ( WScript.Arguments.Named.Exists(szName) ) {
        return ( WScript.Arguments.Named.Item(szName) );
    }
    return ( szDefault );
}

function IsKnownExtension( rgExt, szFile ) {   
   var iExt = szFile.lastIndexOf('.');
   if ( -1 != iExt ) {
      var szExt = szFile.substr(iExt);
      for ( var i = 0; i < rgExt.length; i++ ) {
         if ( szExt == rgExt[i] ) {
            return ( true );
         }
      }
   }
   return ( false );
}

function CReplacementType( szExp, szMod ) {
   this.Expression = szExp;
   this.Modifier   = szMod;
   return ( szMod );
}

function ProcessFile( szFile ) {
   var oFile = __oFileSys.OpenTextFile(szFile, 1, 0);
   if ( !oFile ) {
      return;
   }
   
   var szData = oFile.ReadAll();
   oFile.Close();
   
   for ( var i = 0; i < rgPattern.length; i++ ) {
      if ( rgPattern[i].test(szData) ) {
         WScript.Echo(rgPattern[i].exec(szData));
      }
   }
}

szDirectory   = GetCommandLineVariable('directory', null);
szFileTypes   = GetCommandLineVariable('filetypes', '.rc');
szPatternFile = GetCommandLineVariable('patterns', null);

if ( !szDirectory || !__oFileSys.FolderExists(szDirectory) ) {
   WScript.Echo('UpdateVersion.js - Invalid input directory');
   WScript.Quit(-1);
}

/* If a pattern file was given, open it up and append it to the known types */
if ( null != szPatternFile ) {
   var oFile = __oFileSys.OpenTextFile(szPatternFile, 1, 0);
   if ( null != oFile ) {
      while ( !oFile.AtEndOfStream ) {
         var szPattern = oFile.ReadLine();
         if ( (null != szPattern) && ('' != szPattern) ) {
            var szExp = szPattern.substr(0, szPattern.indexOf('\',\'') - 1);
            var szMod = szPattern.substr(szPattern.indexOf('\',\'') + 1);
            rgPattern.push(new CReplacementType(szExp, szMod));
         }
      }
      
      oFile.Close();
   }
}

var oFolder = __oFileSys.GetFolder(szDirectory);
if ( null != oFolder ) {
   var rgExt = szFileTypes.split(';');
   
   var oEnum = new Enumerator(oFolder.Files);
   while ( !oEnum.atEnd() ) {
      var szFile = oEnum.item();
      if ( IsKnownExtension(rgExt, szFile.Path) ) {
         ProcessFile(szFile.Path);
      }
      oEnum.moveNext();
   }
   
   delete oEnum;
   delete oFolder;
}