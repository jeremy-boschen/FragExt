/* FragExt - Shell extensions for providing file fragmentation
 * information.
 *
 * Copyright (C) 2004-2009 Jeremy Boschen. All rights reserved.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software. 
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in
 * a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 * be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 **/

/* FragInfo.js
 *    Demo usage of FragExt.FileFragments component
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Usage:
 *    CSCRIPT.EXE //NOLOGO FragInfo.js /f:C:\pagefile.sys /e /v /c
 *
 *    @ECHO OFF
 *     FOR %I IN (C:\*) DO CSCRIPT.EXE //NOLOGO FragInfo.js /f:%I /e /b 
 *    @ECHO ON
 */

function _echo( sMsg ) {
   for ( var i = 1; i < arguments.length; i++ ) {
      sMsg += arguments[i];
   }   
   WScript.Echo(sMsg);
}

function _padStr( sMsg, iWidth ) {
   sMsg = new String(sMsg);
   while ( sMsg.length < iWidth ) {
      sMsg += ' ';
   }
   if ( sMsg[sMsg.length] != ' ' ) {
      sMsg += ' ';
   }
   return ( sMsg );
}

function _quit( iExit ) {
   WScript.Quit(iExit);
}

function showUsage( ) {
   _echo('FragInfo.js - Copyright (C) 2004-2009 Jeremy Boschen');
   _echo('Usage: CSCRIPT.EXE FragInfo.js /f:FilePath [/e] [/v] [/b]');
   _echo('  /f - Required, path of file to query information of');
   _echo('  /s - Optional, output stream information');
   _echo('  /v - Optional, output detailed information');
   _echo('  /c - Optional, show compressed/sparse allocation units');
   _echo('  /b - Optional, omit headings in output');
   _echo('  /p - Optional, pause before exiting');
}

var rgArg = WScript.Arguments.Named;

if ( !rgArg || !rgArg.Exists('f') ) {
   showUsage();
   _quit(1);
}

var fVerbose     = rgArg.Exists('v') ? true  : false;
var fCompressed  = rgArg.Exists('c') ? true  : false;
var fStreams     = rgArg.Exists('s') ? true  : false;
var fFormatting  = rgArg.Exists('b') ? false : true;
var fDelayExit   = rgArg.Exists('p') ? true  : false;
var cchPad   = 12;

function _dumpStreams( sPath ) {
   var oStreams = null;
   var oEnum    = null;
   
   try {
      if ( null == (oStreams = WScript.CreateObject('FragExt.FileStreams')) ) {
         _echo('Failed to create FileStreams instance');
         _quit(2);
      }
      
      if ( !oStreams.OpenFile(sPath) ) {
         _echo('Unable to open file streams ' + sPath);
         _quit(3);
      }
      
      oEnum = new Enumerator(oStreams);
      for ( ; !oEnum.atEnd(); oEnum.moveNext() ) {
         var oItem = oEnum.item();  
         _dumpFile(sPath + oItem.Name);
         _echo('');
         oItem = null;
      }
      
      oEnum    = null;
      oStreams = null;      
   } catch ( e ) {
      oEnum    = null;
      oStreams = null;
      
      throw e;
   }
}

function _dumpFile( sPath ) {
   var oFile = null;
   var oEnum = null;
   
   try {
      if ( null == (oFile = WScript.CreateObject('FragExt.FileFragments')) ) {
         _echo('Failed to create FileFragments instance');
         _quit(2);
      }
      
      if ( !oFile.OpenFile(sPath, fCompressed) ) {
         _echo('Unable to open file ' + sPath);
         _quit(3);
      }
      
      if ( fFormatting ) {
         _echo(_padStr('Fragments', cchPad) + _padStr('Size on Disk', cchPad) + 'Path');
      }
      
      _echo(_padStr(oFile.FragmentCount, cchPad) + _padStr(oFile.FileSize, cchPad) + sPath);
      
      if ( fVerbose ) {   
         if ( fFormatting ) {
            _echo('\n' + _padStr('Vcn', cchPad) + _padStr('Lcn', cchPad) + _padStr('Size', cchPad) + _padStr('Extents', cchPad) + _padStr('Clusters', cchPad) + 'Percentage');
         }
         
         oEnum = new Enumerator(oFile);
         var ag = [];      
         for ( ; !oEnum.atEnd(); oEnum.moveNext() ) {
            var oFrag  = oEnum.item();
            var cbFrag = oFile.ClusterSize * oFrag.ClusterCount;
            var cbPerc = (-1 == oFrag.LogicalCluster ? 0 : ((cbFrag / oFile.FileSize) * 100));
            _echo(_padStr(oFrag.Sequence, cchPad) + _padStr(oFrag.LogicalCluster, cchPad) + _padStr(cbFrag, cchPad) + _padStr(oFrag.ExtentCount, cchPad) + _padStr(oFrag.ClusterCount, cchPad) + cbPerc.toPrecision(4));
            oFrag = null;
         }
      }
      
      oEnum = null;
      oFile = null;
   }
   catch ( e ) {
      oEnum = null;
      oFile = null;
      
      throw e;
   }
}

var sPath = rgArg.Item('f');

if ( fStreams ) {
   _dumpStreams(sPath);
} else {
   _dumpFile(sPath);
}

if ( fDelayExit ) {
   _echo('\nPress ENTER to continue...');
   WScript.StdIn.Read(1);
}