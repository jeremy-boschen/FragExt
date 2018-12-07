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
 */

/* Defrag.js
 *    Demo usage of FragExt.Defragmenter component
 *
 * Copyright (C) 2004-2009 Jeremy Boschen
 *
 * Usage:
 *    CSCRIPT.EXE //NOLOGO Defrag.js /f:C:\FragExt.pch /v
 *
 *    @ECHO OFF
 *     FOR %I IN (C:\*) DO CSCRIPT.EXE //NOLOGO Defrag.js /f:%I 
 *    @ECHO ON
 */

function _echo( sMsg ) {
   for ( var i = 1; i < arguments.length; i++ ) {
      sMsg += arguments[i];
   }   
   WScript.Echo(sMsg);
}

function _quit( iExit ) {
   WScript.Quit(iExit);
}

function _showUsage( ) {
   _echo('Defrag.js - Copyright (C) 2004-2009 Jeremy Boschen');
   _echo('Usage: CSCRIPT.EXE Defrag.js /f:FilePath');
   _echo('  /f - Required, path of file to query information of');
   _echo('  /p - Pause when done');
}

var rgArg = WScript.Arguments.Named;

if ( !rgArg || !rgArg.Exists('f') ) {
   _showUsage();
   _quit(1);
}

var oDefrag = null;

function _OnProgressUpdate( sPath, iCookie, eState, iPercent ) {
   switch ( eState ) {
      case 1:
         _echo('Initializing ' + sPath + '...');
         break;
      case 2:
         _echo('Defragmenting file ' + iPercent + '%...');
         break;
      case 3:
         if ( 0 != oDefrag.ResultCode ) {
            _echo('Failed to defragment file due to error ' + oDefrag.ResultCode);
         } else {
            _echo('Completed defragmenting file');
         }                     
         break;
   }
   
   /* true cancels */
   return ( false );
}

if ( null == (oDefrag = WScript.CreateObject('FragExt.FileDefragmenter')) ) {
   _echo('Failed to create Defragmenter instance');
   _quit(2);
}

var sPath = rgArg.Item('f');
oDefrag.OnProgressUpdate = _OnProgressUpdate;

if ( !oDefrag.DefragmentFile(sPath, 0) ) {
   _echo('Unable to defragment file ' + sPath);
   _quit(3);
}

delete oDefrag;

if ( rgArg.Exists('p') ) {
   _echo('\nPress ENTER to continue...');
   WScript.StdIn.Read(1);
}