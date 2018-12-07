
/*++
   
   Internal

 --*/

#pragma push_macro( "AWSUFFIX" )
#pragma push_macro( "XTEXT" )
#pragma push_macro( "XCHAR" )
#pragma push_macro( "LPXSTR" )
#pragma push_macro( "LPCXSTR" )
#pragma push_macro( "CCHLENGTH" )

#ifdef UNICODE
   #define AWSUFFIX( name ) \
      name##W
   
   #define XTEXT( text ) \
      L##text

   #define XCHAR   WCHAR
   #define LPXSTR  LPWSTR
   #define LPCXSTR LPCWSTR
#else
   #define AWSUFFIX( name ) \
      name##A

   #define XTEXT( text ) \
      text

   #define XCHAR   CHAR
   #define LPXSTR  LPSTR
   #define LPCXSTR LPCSTR
#endif

#define CCHLENGTH( sz ) \
   ((sizeof( sz ) / sizeof(XCHAR)) - 1)

inline
BOOLEAN
AWSUFFIX(IsCharInRange)(
   __in XCHAR Char,
   __in XCHAR First,
   __in XCHAR Last
)
{
   return ( (Char >= First) && (Char <= Last) );
}

inline
BOOLEAN
AWSUFFIX(IsHexDigit)(
   __in XCHAR Digit
)
{
   if ( ((Digit >= XTEXT('0')) && (Digit <= XTEXT('9'))) ||
        ((Digit >= XTEXT('A')) && (Digit <= XTEXT('F'))) ||
        ((Digit >= XTEXT('a')) && (Digit <= XTEXT('f'))) ) {
      return ( TRUE );
   }

   return ( FALSE );
}

inline
BOOLEAN
AWSUFFIX(IsDOSDriveLetter)(
   __in XCHAR DriveLetter
)
{
   if ( AWSUFFIX(IsCharInRange)(DriveLetter, XTEXT('A'), XTEXT('Z')) || 
        AWSUFFIX(IsCharInRange)(DriveLetter, XTEXT('a'), XTEXT('z')) ) {
      return ( TRUE );
   }

   return ( FALSE );
}

inline
LPCXSTR
AWSUFFIX(FindDOSVolumeName)(
   __in_z LPCXSTR Name
)
{
   if ( (XTEXT('\\') == Name[0]) &&
        (XTEXT('\\') == Name[1]) &&
        (XTEXT('?')  == Name[2]) &&
        (XTEXT('\\') == Name[3]) ) {
      Name += 4;
   }

   if ( AWSUFFIX(IsDOSDriveLetter)(Name[0]) && (XTEXT(':') == Name[1]) ) {
      return ( &(Name[0]) );
   }

   return ( NULL );
}

inline
BOOLEAN
AWSUFFIX(IsDOSDeviceName)(
   __in_z LPCXSTR Name
)
{
   if ( (XTEXT('\\') == Name[0]) &&
        (XTEXT('\\') == Name[1]) &&
        (XTEXT('.')  == Name[2]) &&
        (XTEXT('\\') == Name[3]) ) {
      return ( TRUE );
   }

   return ( FALSE );
}

inline
LPCXSTR
AWSUFFIX(FindUNCShareName)(
   __in_z LPCXSTR Name
)
{
   if ( (XTEXT('\\') == Name[0]) &&
        (XTEXT('\\') == Name[1]) ) {
      if ( XTEXT('?') == Name[2] ) {
         if ( (XTEXT('\\') == Name[3]) &&
              (XTEXT('U')  == Name[4]) &&
              (XTEXT('N')  == Name[5]) &&
              (XTEXT('C')  == Name[6]) && 
              (XTEXT('\\') == Name[7]) ) {
            /*
             * Name is \\?\UNC\
             */
            return ( &(Name[8]) );
         }
      }
      else if ( XTEXT('.') != Name[2] ) {
         /*
          * Name is \\
          */
         return ( &(Name[2]) );
      }
   }

   return ( NULL );
}

inline
LPCXSTR
AWSUFFIX(FindVolumeGUIDName)(
   __in_z LPCXSTR Name
)
{
   LPCXSTR pszGuidFormat;

   /*
    * Any X in the format string accepts any hexidecimal digit
    */
   pszGuidFormat = XTEXT("\\\\?\\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}");

   while ( XTEXT('\0') != pszGuidFormat[0] ) {
      /*
       * If the format character is 0, allow any hexidecimal character,
       * otherwise require an exact match
       */
      if ( XTEXT('X') == pszGuidFormat[0] ) {
         if ( !AWSUFFIX(IsHexDigit)(Name[0]) ) {
            /* Failure */
            return ( NULL );
         }
      }
      else if ( Name[0] != pszGuidFormat[0] ) {
         /* Failure */
         return ( NULL );
      }

      Name          += 1;
      pszGuidFormat += 1;
   }

   /* Success */
   return ( Name - CCHLENGTH(XTEXT("\\\\?\\Volume{00000000-0000-0000-0000-000000000000}")) );
}

inline
BOOLEAN
AWSUFFIX(IsNTNativePath)(
   __in_z LPCXSTR Path
)
{
   if ( (XTEXT('\\') == Path[0]) &&
        (XTEXT('?')  == Path[1]) &&
        (XTEXT('?')  == Path[2]) &&
        (XTEXT('\\') == Path[3]) ) {
      return ( TRUE );
   }

   return ( FALSE );
}

inline
LPCXSTR
AWSUFFIX(FindFinalComponentName)(
   __in_z LPCXSTR ParentPath
)
{
   XCHAR   wcPath;
   LPCXSTR pszFinalName;

   pszFinalName = ParentPath;

   for ( ;; ) {
      wcPath = ParentPath[0];

      /*
       * We have to continue scanning the entire path because directories can have
       * stream names too
       */
      if ( (XTEXT('\0') == wcPath) ) {
         break;
      }

      if ( XTEXT('\\') == wcPath ) {
         /*
          * The name starts after the \
          */
         pszFinalName = &(ParentPath[1]);
      }

      ParentPath += 1;
   }

   if ( pszFinalName ) {
      if ( (XTEXT('\0') == pszFinalName[0]) || (XTEXT(':') == pszFinalName[0]) ) {
         /*
          * Name is "" or ":Stream:$DATA", return nothing
          */
         pszFinalName = NULL;
      }
      else if ( (XTEXT('.')  == pszFinalName[0]) && (XTEXT('\0') == pszFinalName[1]) ) {
         /*
          * Name is ".", return nothing
          */
         pszFinalName = NULL;
      }
      else if ( (XTEXT('.')  == pszFinalName[0]) && (XTEXT('.')  == pszFinalName[1]) && (XTEXT('\0') == pszFinalName[2]) ) {
         /*
          * Name is "..", return nothing
          */
         pszFinalName = NULL;
      }
   }

   return ( pszFinalName );
}

LPCXSTR
AWSUFFIX(FindExtensionName)(
   __in_z LPCXSTR ParentPath
)
{
   XCHAR   wcPath;
   LPCXSTR pszExtension;

   pszExtension = NULL;

   for ( ;; ) {
      wcPath = ParentPath[0];

      /*
       * The end of an extension is marked by either the end of the
       * string or the start of a stream name
       */
      if ( (XTEXT('\0') == wcPath) || (XTEXT(':') == wcPath) ) {
         break;
      }

      if ( XTEXT('.') == wcPath ) {
         pszExtension = ParentPath;
      }
      else if ( (XTEXT('\\') == wcPath) || (XTEXT(' ') == wcPath) ) {
         /*
          * Neither \ or spaces are allowed in an extension
          */
         pszExtension = NULL;
      }
      
      ParentPath += 1;
   }
   
   if ( pszExtension ) {
      /*
       * Make sure the extension has at least one character. We know that any
       * character other than the two we check are valid because these two
       * are the only ones we exit the loop above for
       */
      if ( (XTEXT('\0') == pszExtension[1]) || (XTEXT(':') == pszExtension[1]) ) {
         /*
          * Path is either "File." or "File.:StreamName:$DATA"
          */
         pszExtension = NULL;
      }
   }

   return ( pszExtension );
}

LPCXSTR
AWSUFFIX(FindStreamName)(
   __in_z LPCXSTR ParentPath
)
{
   XCHAR   wcPath;
   LPCXSTR pszStreamName;

   pszStreamName = NULL;

   for ( ;; ) {
      wcPath = ParentPath[0];

      if ( XTEXT('\0') == wcPath ) {
         break;
      }
   
      if ( XTEXT(':') == wcPath ) {
         pszStreamName = ParentPath;
         break;
      }

      ParentPath += 1;
   }

   return ( pszStreamName );
}

BOOLEAN
APIENTRY
AWSUFFIX(PathCchParseInformation)(
   __in_ecount_z(MaxLength) LPCXSTR Path,
   __in USHORT MaxLength,
   __out AWSUFFIX(PPATH_INFORMATION) PathInformation
)
{
   HRESULT  hr;
   LPCXSTR  pszTail;
   LPCXSTR  pszVolume;
   LPCXSTR  pszShare;
   LPCXSTR  pszParentPath;
   LPCXSTR  pszFinalName;
   LPCXSTR  pszExtension;
   LPCXSTR  pszStream;
   size_t   cchLength;
   USHORT   PathType;

   /* Initialize outputs */
   ZeroMemory(PathInformation,
              sizeof(*PathInformation));

   /* Initialize locals */
   pszTail       = NULL;
   pszVolume     = NULL;
   pszShare      = NULL;
   pszParentPath = NULL;
   pszFinalName  = NULL;
   pszExtension  = NULL;
   pszStream     = NULL;
   cchLength     = 0;
   PathType      = PATH_TYPE_UNKNOWN;

   /*
    * Populate the PATH_INFORMATION structure
    */
   hr = AWSUFFIX(StringCchLength)(Path,
                                  PATHCCH_MAXLENGTH,
                                  &cchLength);
   if ( FAILED(hr) || (cchLength > (size_t)MaxLength) ) {
      /*
       * Path is too long
       */
      return ( FALSE );
   }

   /*
    * Save the end of the string for calculating lengths later
    */
   pszTail = &(Path[cchLength]);

   PathInformation->BasePathLength = (USHORT)cchLength;
   PathInformation->BasePath       = Path;
   
   if ( 0 == cchLength ) {
      /* 
       * Path is empty
       */
      return ( FALSE );
   }

   /*
    * Check for path types that aren't supported
    */
   if ( AWSUFFIX(IsDOSDeviceName)(Path) ) {
      PathType              = PATH_TYPE_DOSDEVICE;
      PathInformation->Type = PATH_TYPE_DOSDEVICE;

      /*
       * Unsupported path type
       */
      return ( FALSE );
   }
   
   if ( AWSUFFIX(IsNTNativePath)(Path) ) {
      PathType              = PATH_TYPE_NT;
      PathInformation->Type = PATH_TYPE_NT;

      /*
       * Unsupported path type
       */
      return ( FALSE );
   }

   /*
    * Determine the drive, volume, or share name
    */
   if ( NULL != (pszVolume = AWSUFFIX(FindDOSVolumeName)(Path)) ) {
      PathType                = PATH_TYPE_DOS;
      PathInformation->Type   = PATH_TYPE_DOS;
      PathInformation->Volume = pszVolume;
   }
   else if ( NULL != (pszShare = AWSUFFIX(FindUNCShareName)(Path)) ) {
      PathType               = PATH_TYPE_UNC;
      PathInformation->Type  = PATH_TYPE_UNC;
      PathInformation->Share = pszShare;
   }
   else if ( NULL != (pszVolume = AWSUFFIX(FindVolumeGUIDName)(Path)) ) {
      PathType                = PATH_TYPE_VOLUMEGUID;
      PathInformation->Type   = PATH_TYPE_VOLUMEGUID;
      PathInformation->Volume = pszVolume;
   }   
   else {
      /*
       * Assume this is a DOS path, relative or rooted
       */
      PathType              = PATH_TYPE_DOS;
      PathInformation->Type = PATH_TYPE_DOS;
   }

   /*
    * Determine if Win32 path parsing is disabled for this path
    */
   if ( (XTEXT('\\') == Path[0]) &&
        (XTEXT('\\') == Path[1]) &&
        (XTEXT('?')  == Path[2]) &&
        (XTEXT('\\') == Path[3]) ) {
      PathInformation->IsSystemParsingDisabled = TRUE;
   }

   /*
    * Find the start of the parent path
    */
   if ( PATH_TYPE_DOS == PathInformation->Type ) {
      /*
       * This could be a relative or rooted path
       */
      if ( PathInformation->Volume ) {
         pszParentPath = PathInformation->Volume + CCHLENGTH(XTEXT("A:"));
      }
      else {
         pszParentPath = PathInformation->BasePath;
      }
   }
   else if ( PATH_TYPE_VOLUMEGUID == PathInformation->Type ) {
      pszParentPath = PathInformation->Volume + CCHLENGTH(XTEXT("\\\\?\\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"));
   }
   else {
      _ASSERTE(PATH_TYPE_UNC == PathInformation->Type);

      pszShare      = PathInformation->Share;
      pszParentPath = NULL;

      /*
       * For UNC paths the sub path starts after the share name, if present
       */
      while ( XTEXT('\0') != pszShare[0] ) {
         if ( XTEXT('\\') == pszShare[0] ) {
            pszParentPath = pszShare;
            pszShare      += 1;

            if ( XTEXT('\0') == pszShare[0] ) {
               /*
                * Path is "server\"
                */
               break;
            }

            /*
             * We've found the end of the server component, so find the end of the share, if present
             */
            for ( ;; ) {
               if ( XTEXT('\0') == pszShare[0] ) {
                  /*
                   * Path is "server\share", so there is no parent path
                   */
                  pszParentPath = NULL;

                  break;
               }

               if ( XTEXT('\\') == pszShare[0] ) {
                  /*
                   * Path is "server\share\"
                   */
                  pszParentPath = pszShare;

                  break;
               }

               pszShare += 1;               
            }

            break;
         }

         pszShare += 1;
      }
   }

   if ( pszParentPath && (XTEXT('\0') != pszParentPath[0]) ) {
      /*
       * Find the remaining path components
       */
      pszFinalName = AWSUFFIX(FindFinalComponentName)(pszParentPath);
      
      if ( pszFinalName ) {
         pszExtension = AWSUFFIX(FindExtensionName)(pszFinalName);
      
         if ( pszExtension ) {
            pszStream = AWSUFFIX(FindStreamName)(pszExtension);
         }
         else {
            pszStream = AWSUFFIX(FindStreamName)(pszFinalName);
         }
      }

      /*
       * A few of the functions above cannot determine if the parts they find
       * are valid without additional information obtained in other functions,
       * so apply some rules now based on everything we've gathered 
       */
      if ( pszStream ) {
         /*
          * A parent path must come before a stream name
          */
         if ( pszParentPath >= pszStream ) {
            pszParentPath = NULL;
         }

         /*
          * A final name must come before a stream name
          */
         if ( pszFinalName && (pszFinalName >= pszStream) ) {
            pszFinalName = NULL;
         }

         /*
          * An extension must come before a stream name
          */
         if ( pszExtension && (pszExtension >= pszStream) ) {
            pszExtension = NULL;
         }
      }

      if ( pszExtension ) {
         /*
          * A parent path must come before an extension
          */
         if ( pszParentPath >= pszExtension ) {
            pszParentPath = NULL;
         }

         /*
          * A final name must come before an extension
          */
         if ( pszFinalName && (pszFinalName >= pszExtension) ) {
            pszFinalName = NULL;
         }         
      }

      if ( pszFinalName ) {
         /*
          * A parent path must come before a final name
          */
         if ( pszParentPath >= pszFinalName ) {
            pszParentPath = NULL;
         }
      }

      PathInformation->ParentPath = pszParentPath;
      PathInformation->FinalName  = pszFinalName;
      PathInformation->Extension  = pszExtension;
      PathInformation->Stream     = pszStream;
   }

   /*
    * Now that everything is set, calculate the lengths of each component
    */
   if ( pszStream ) {
      PathInformation->StreamLength = (USHORT)(pszTail - pszStream);      
   }

   if ( pszExtension ) {
      if ( pszStream ) {
         /* Path is: Name.Ext:Stream:$DATA */
         PathInformation->ExtensionLength = (USHORT)(pszStream - pszExtension);
      } 
      else {
         /* Path is: Name:Stream:$DATA */
         PathInformation->ExtensionLength = (USHORT)(pszTail - pszExtension);
      }
   }

   if ( pszFinalName ) {
      if ( pszExtension ) {
         /* Path is: Name.Ext */
         PathInformation->FinalNameLength = (USHORT)(pszExtension - pszFinalName);
      }
      else if ( pszStream ) {
         /* Path is: Name:Stream:$DATA */
         PathInformation->FinalNameLength = (USHORT)(pszStream - pszFinalName);
      }
      else {
         /* Path is: Name */
         PathInformation->FinalNameLength = (USHORT)(pszTail - pszFinalName);
      }
   }

   if ( pszParentPath ) {
      if ( pszFinalName ) {
         /* Path is: \Directory\Name */
         PathInformation->ParentPathLength = (USHORT)(pszFinalName - pszParentPath);
      }
      else if ( pszExtension ) {
         /* Path is: \Directory.Ext\ */
         PathInformation->ParentPathLength = (USHORT)(pszExtension - pszParentPath);
      }
      else if ( pszStream ) {
         /* Path is: \Directory:Stream:$DATA\ */
         PathInformation->ParentPathLength = (USHORT)(pszStream - pszParentPath);
      }
      else {
         /* Path is: \Directory\ */
         PathInformation->ParentPathLength = (USHORT)(pszTail - pszParentPath);
      }
   }

   if ( pszVolume ) {
      if ( PATH_TYPE_DOS == PathInformation->Type ) {
         /* Path is: C: */
         PathInformation->VolumeLength = (USHORT)CCHLENGTH(XTEXT("A:"));
      }
      else {
         /* Path is: \\?\Volume{00000000-0000-0000-0000-000000000000} */
         _ASSERTE(PATH_TYPE_VOLUMEGUID == PathInformation->Type);
         PathInformation->VolumeLength = (USHORT)CCHLENGTH(XTEXT("\\\\?\\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}"));
      }
   }
   else if ( pszShare ) {
      _ASSERTE(PATH_TYPE_UNC == PathInformation->Type);

      if ( pszParentPath ) {
         /* Path is: \\server\share\ */
         PathInformation->ShareLength = (USHORT)(pszParentPath - pszShare);
      }
      else {
         /* Path is: \\server\share */
         PathInformation->ShareLength = (USHORT)(pszTail - pszShare);
      }
   }

   /* Success */
   return ( TRUE );
}


BOOLEAN
APIENTRY
AWSUFFIX(PathCchBuildPath)(
   __in AWSUFFIX(PCPATH_INFORMATION) PathInformation,
   __out_ecount_z(PathLength) LPXSTR Path,
   __in size_t PathLength
)
{
   HRESULT  hr;
   
   LPXSTR   pszDestination;
   size_t   cchRemaining;

   size_t   cchMaxLength;
     
   LPCXSTR  pszSource;
   size_t   cchToCopy;

   if ( !IsValidPathType(PathInformation->Type) ) {
      /* Failure */
      return ( FALSE );
   }

   /* Initialize locals */
   pszDestination = Path;
   cchRemaining   = PathLength;

   cchMaxLength   = (size_t)PathInformation->BasePathLength;
   if ( cchMaxLength >  (size_t)PATHCCH_MAXLENGTH ) {
      cchMaxLength = (size_t)PATHCCH_MAXLENGTH;
   }

   pszSource = NULL;
   cchToCopy = 0;

   if ( PATH_TYPE_DOS == PathInformation->Type ) {
      pszSource = PathInformation->Volume;
      cchToCopy = (size_t)PathInformation->VolumeLength;
   }
   else if ( PATH_TYPE_VOLUMEGUID == PathInformation->Type ) {
      pszSource = PathInformation->Volume;
      cchToCopy = (size_t)PathInformation->VolumeLength;
   }
   else {
      _ASSERTE(PATH_TYPE_UNC == PathInformation->Type);
      pszSource = PathInformation->Share;
      cchToCopy = (size_t)PathInformation->ShareLength;
   }

   if ( pszSource ) {
      if ( PATHCCH_NULTERM == (USHORT)cchToCopy ) {
         hr = AWSUFFIX(StringCchLength)(pszSource,
                                        cchMaxLength,
                                        &cchToCopy);
         
         if ( FAILED(hr) ) {
            /* Failure */
            return ( FALSE );
         }
      }
         
      hr = AWSUFFIX(StringCchCopyNEx)(pszDestination,
                                      cchRemaining,
                                      pszSource,
                                      cchToCopy,
                                      &pszDestination,
                                      &cchRemaining,
                                      0);

      if ( FAILED(hr) ) {
         /* Failure */
         return ( FALSE );
      }
   }

   pszSource = PathInformation->ParentPath;
   cchToCopy = (size_t)PathInformation->ParentPathLength;

   if ( pszSource ) {
   if ( PATHCCH_NULTERM == (USHORT)cchToCopy ) {
         hr = AWSUFFIX(StringCchLength)(pszSource,
                                        cchMaxLength,
                                        &cchToCopy);
         
         if ( FAILED(hr) ) {
            /* Failure */
            return ( FALSE );
         }
      }
         
      hr = AWSUFFIX(StringCchCopyNEx)(pszDestination,
                                      cchRemaining,
                                      pszSource,
                                      cchToCopy,
                                      &pszDestination,
                                      &cchRemaining,
                                      0);

      if ( FAILED(hr) ) {
         /* Failure */
         return ( FALSE );
      }
   }
   
   pszSource = PathInformation->FinalName;
   cchToCopy = (size_t)PathInformation->FinalNameLength;

   if ( pszSource ) {
   if ( PATHCCH_NULTERM == (USHORT)cchToCopy ) {
         hr = AWSUFFIX(StringCchLength)(pszSource,
                                        cchMaxLength,
                                        &cchToCopy);
         
         if ( FAILED(hr) ) {
            /* Failure */
            return ( FALSE );
         }
      }
         
      hr = AWSUFFIX(StringCchCopyNEx)(pszDestination,
                                      cchRemaining,
                                      pszSource,
                                      cchToCopy,
                                      &pszDestination,
                                      &cchRemaining,
                                      0);

      if ( FAILED(hr) ) {
         /* Failure */
         return ( FALSE );
      }
   }
   
   pszSource = PathInformation->Extension;
   cchToCopy = (size_t)PathInformation->ExtensionLength;

   if ( pszSource ) {
   if ( PATHCCH_NULTERM == (USHORT)cchToCopy ) {
         hr = AWSUFFIX(StringCchLength)(pszSource,
                                        cchMaxLength,
                                        &cchToCopy);
         
         if ( FAILED(hr) ) {
            /* Failure */
            return ( FALSE );
         }
      }
         
      hr = AWSUFFIX(StringCchCopyNEx)(pszDestination,
                                      cchRemaining,
                                      pszSource,
                                      cchToCopy,
                                      &pszDestination,
                                      &cchRemaining,
                                      0);

      if ( FAILED(hr) ) {
         /* Failure */
         return ( FALSE );
      }
   }

   pszSource = PathInformation->Stream;
   cchToCopy = (size_t)PathInformation->StreamLength;

   if ( pszSource ) {
   if ( PATHCCH_NULTERM == (USHORT)cchToCopy ) {
         hr = AWSUFFIX(StringCchLength)(pszSource,
                                        cchMaxLength,
                                        &cchToCopy);
         
         if ( FAILED(hr) ) {
            /* Failure */
            return ( FALSE );
         }
      }
         
      hr = AWSUFFIX(StringCchCopyNEx)(pszDestination,
                                      cchRemaining,
                                      pszSource,
                                      cchToCopy,
                                      &pszDestination,
                                      &cchRemaining,
                                      0);

      if ( FAILED(hr) ) {
         /* Failure */
         return ( FALSE );
      }
   }

   /* Success */
   return ( TRUE );
}

/*++
 
   Exports

 --*/

__nullterminated LPXSTR
APIENTRY
APIENTRY
AWSUFFIX(PathCchAddBackslash)( 
   __inout_ecount_z(PathLength) LPXSTR Path,
   __in size_t PathLength
)
{
   LPXSTR pszChar;

   _ASSERTE(NULL != Path);
   _ASSERTE(0 != PathLength);
   _ASSERTE(PathLength <= PATHCCH_MAXLENGTH);

   if ( XTEXT('\0') == Path[0] ) {
      return ( Path );
   }

   pszChar = Path;

   for ( ;; ) {
      pszChar += 1;

      if ( XTEXT('\0') == pszChar[0] ) {
         if ( XTEXT('\\') != pszChar[-1] ) {
            /*
             * Ensure we have room in the path to add a \
             */
            if ( &(pszChar[1]) >= (Path + PathLength) ) {
               /* Failure */
               return ( NULL );
            }

	         pszChar[0] = XTEXT('\\');
            pszChar   += 1;
            pszChar[0] = XTEXT('\0');
         }
         
         /* Success */
	      return ( pszChar );
      }
   }
}

__nullterminated LPXSTR 
APIENTRY
APIENTRY
AWSUFFIX(PathCchRemoveBackslash)( 
   __inout_z LPXSTR Path
)
{
   _ASSERTE(NULL != Path);

   /*
    * If the path is empty, return it
    */
   if ( XTEXT('\0') == Path[0] ) {
      return ( Path );
   }

   for ( ;; ) {
      Path += 1;

      if ( XTEXT('\0') == Path[0] ) {
         /*
          * If the last character was a \, strip it
          */
         if ( XTEXT('\\') == Path[-1] ) {
            Path[-1] = XTEXT('\0');
         }

         /*
          * Return either the address of the last character or the null terminator
          * that replaced it
          */
         Path -= 1;

         /* Success */
         return ( Path );
      }
   }
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchAppend)( 
   __inout_ecount_z(PathLength) LPXSTR Path, 
   __in size_t PathLength, 
   __in_z LPCXSTR PathToAppend 
)
{
   HRESULT hr;
   LPXSTR  pszTail;

   _ASSERTE(NULL != Path);
   _ASSERTE(NULL != PathToAppend);
   _ASSERTE(PathLength <= PATHCCH_MAXLENGTH);

   pszTail = AWSUFFIX(PathCchAddBackslash)(Path, 
                                           PathLength);

   if ( pszTail ) {
      PathLength -= static_cast<size_t>(pszTail - Path);

      if ( PathLength > 0 ) {      
         if ( XTEXT('\\') == PathToAppend[0] ) {
            PathToAppend += 1;
         }

         hr = AWSUFFIX(StringCchCopy)(pszTail, 
                                      PathLength, 
                                      PathToAppend);

         return ( SUCCEEDED(hr) );
      }
   }

   return ( FALSE );
}     

__nullterminated LPXSTR
APIENTRY
AWSUFFIX(PathCchFindExtension)( 
   __in_z LPCXSTR Path 
)
{   
	AWSUFFIX(PATH_INFORMATION) PathInformation;

	_ASSERTE(NULL != Path);

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);

   return ( const_cast<LPXSTR>(PathInformation.Extension) );
}

__nullterminated LPXSTR 
APIENTRY
AWSUFFIX(PathCchFindFileName)( 
   __in_z LPCXSTR Path 
)
{
	AWSUFFIX(PATH_INFORMATION) PathInformation;

	_ASSERTE(NULL != Path);

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);

   return ( const_cast<LPXSTR>(PathInformation.FinalName) );
}

void 
APIENTRY
AWSUFFIX(PathCchRemoveExtension)( 
   __inout_z LPXSTR Path 
)
{
   LPXSTR                     pszTail;
	AWSUFFIX(PATH_INFORMATION) PathInformation;

	_ASSERTE(NULL != Path);

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);
   
	if ( PathInformation.Extension ) {
      pszTail = const_cast<LPXSTR>(PathInformation.Extension);
   }
   else if ( PathInformation.Stream ) {
      pszTail = const_cast<LPXSTR>(PathInformation.Stream);
   }
   else {
      pszTail = NULL;
   }

   if ( pszTail ) {
      pszTail[0] = XTEXT('\0');
   }
}

BOOLEAN
APIENTRY
AWSUFFIX(PathCchIsRoot)( 
   __in_z LPCXSTR Path 
)
{
   AWSUFFIX(PATH_INFORMATION) PathInformation;

   _ASSERTE(NULL != Path);

	if ( XTEXT('\0') == Path[0] ) {
      /* Failure */
	   return ( FALSE );
	}

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);

   if ( !IsValidPathType(PathInformation.Type) ) {
      /* Failure */
      return ( FALSE );
   }

   if ( PATH_TYPE_UNC == PathInformation.Type ) {
      /*
       * Rooted UNC paths cannot have a trailing \, so if they have a parent path
       * then the path is not a root path 
       */
      if ( PathInformation.Share && !PathInformation.ParentPath ) {
         /*
          * Path is \\server, \\server\share, \\?\UNC\server, \\?\UNC\server\share
          */
         return ( TRUE );
      }
   }
   else {
      if ( PathInformation.ParentPath ) {
         if ( (XTEXT('\\') == PathInformation.ParentPath[0]) &&
              (XTEXT('\0') == PathInformation.ParentPath[1]) ) {
            /*
             * Parent path is \
             */
            return ( TRUE );
         }
      }
   }

   /* Failure */
   return ( FALSE );
}

void 
APIENTRY
AWSUFFIX(PathCchRemoveFileSpec)( 
   __inout_z LPXSTR Path 
)
{
   LPXSTR                     pszTail;
   AWSUFFIX(PATH_INFORMATION) PathInformation;

   _ASSERTE(NULL != Path);

	if ( XTEXT('\0') == Path[0] ) {
	   return;
	}

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);

   if ( !IsValidPathType(PathInformation.Type) ) {
      return;
   }

   if ( PathInformation.FinalName ) {
      pszTail = const_cast<LPXSTR>(PathInformation.FinalName);
   }
   else if ( PathInformation.Extension ) {
      pszTail = const_cast<LPXSTR>(PathInformation.Extension);
   }
   else if ( PathInformation.Stream ) {
      pszTail = const_cast<LPXSTR>(PathInformation.Stream);
   }
   else {
      pszTail = NULL;
   }

   if ( pszTail ) {
      pszTail[0] = XTEXT('\0');
   }
}

void 
APIENTRY
AWSUFFIX(PathCchRemoveStreamSpec)( 
   __inout_z LPXSTR Path 
)
{
   AWSUFFIX(PATH_INFORMATION) PathInformation;

   _ASSERTE(NULL != Path);

	if ( XTEXT('\0') == Path[0] ) {
	   return;
	}

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);

   if ( !IsValidPathType(PathInformation.Type) ) {
      return;
   }

   if ( PathInformation.Stream ) {
      const_cast<LPXSTR>(PathInformation.Stream)[0] = XTEXT('\0');
   }
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchReplaceFileSpec)( 
   __inout_ecount_z(PathLength) LPXSTR Path, 
   __in size_t PathLength, 
   __in_z LPCXSTR FileSpec 
)
{
   HRESULT                    hr;
   LPXSTR                     pszTail;
   AWSUFFIX(PATH_INFORMATION) PathInformation;

   _ASSERTE(NULL != Path);

	if ( XTEXT('\0') == Path[0] ) {
      /* Failure */
	   return ( FALSE );
	}

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);

   if ( !IsValidPathType(PathInformation.Type) ) {
      /* Failure */
      return ( FALSE );
   }

   if ( PathInformation.FinalName ) {
      pszTail = const_cast<LPXSTR>(PathInformation.FinalName);
   }
   else if ( PathInformation.Extension ) {
      pszTail = const_cast<LPXSTR>(PathInformation.Extension);
   }
   else if ( PathInformation.Stream ) {
      pszTail = const_cast<LPXSTR>(PathInformation.Stream);
   }
   else {
      pszTail = NULL;
   }

   if ( pszTail ) {
      PathLength -= static_cast<size_t>(pszTail - PathInformation.BasePath);

      hr = AWSUFFIX(StringCchCopy)(pszTail,
                                   PathLength,
                                   FileSpec);

      /* Success / Failure */
      return ( SUCCEEDED(hr) );
   }

   /* Failure */
   return ( FALSE );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchStripToRoot)( 
   __inout_z LPXSTR Path
)
{
   LPXSTR                     pszRootTail;
   AWSUFFIX(PATH_INFORMATION) PathInformation;

   _ASSERTE(NULL != Path);

	if ( XTEXT('\0') == Path[0] ) {
      /* Failure */
	   return ( FALSE );
	}

   AWSUFFIX(PathCchParseInformation)(Path,
                                     PATHCCH_MAXLENGTH,
                                     &PathInformation);

   if ( !IsValidPathType(PathInformation.Type) ) {
      /* Failure */
      return ( FALSE );
   }

   if ( PathInformation.ParentPath ) {
      pszRootTail = const_cast<LPXSTR>(PathInformation.ParentPath);

      if ( PATH_TYPE_UNC != PathInformation.Type ) {
         if ( XTEXT('\\') == pszRootTail[0] ) {
            pszRootTail += 1;
         }
      }
   }
   else if ( PathInformation.FinalName ) {
      pszRootTail = const_cast<LPXSTR>(PathInformation.FinalName);
   }
   else if ( PathInformation.Extension ) {
      pszRootTail = const_cast<LPXSTR>(PathInformation.Extension);
   }
   else if ( PathInformation.Stream ) {
      pszRootTail = const_cast<LPXSTR>(PathInformation.Stream);
   }
   else {
      pszRootTail = NULL;
   }

   if ( pszRootTail ) {
      pszRootTail[0] = XTEXT('\0');

      /* Success */
      return ( TRUE );
   }

   /* Failure */
   return ( FALSE );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchIsUNC)( 
   __in_z LPCXSTR Path
)
{
	_ASSERTE(NULL != Path);

   /* Success / Failure */
   return ( AWSUFFIX(FindUNCShareName)(Path) ? TRUE : FALSE );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchIsDOS)( 
   __in_z LPCXSTR Path
)
{
	_ASSERTE(NULL != Path);
   
   /* Failure */
   return ( AWSUFFIX(FindDOSVolumeName)(Path) ? TRUE : FALSE );
}

BOOLEAN
APIENTRY
AWSUFFIX(PathCchIsVolumeGUID)(
   __in_z LPCXSTR Path
)
{
	_ASSERTE(NULL != Path);
   
   /* Failure */
   return ( AWSUFFIX(FindVolumeGUIDName)(Path) ? TRUE : FALSE );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchIsDOSDevice)( 
   __in_z LPCXSTR Path
)
{
	_ASSERTE(NULL != Path);
   
   /* Failure */
   return ( AWSUFFIX(IsDOSDeviceName)(Path) );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchIsNT)( 
   __in_z LPCXSTR Path
)
{
   _ASSERTE(NULL != Path);
   
   /* Success / Failure */
   return ( AWSUFFIX(IsNTNativePath)(Path) );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchIsExtendedLength)( 
   __in_z LPCXSTR Path
)
{
   _ASSERTE(NULL != Path);

   if ( (XTEXT('\\') == Path[0]) && (XTEXT('\\') == Path[1]) && (XTEXT('?') == Path[2]) && (XTEXT('\\') == Path[3]) ) {
      /* Success */
      return ( TRUE );
   }

   /* Failure */
   return ( FALSE );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchIsDotDirectory)( 
   __in_z LPCXSTR Path 
)
{
   _ASSERTE(NULL != Path);

   if ( (XTEXT('.') == Path[0]) && (XTEXT('\0') == Path[1]) ) {
      /* Success */
      return ( TRUE );
   }

   if ( (XTEXT('.') == Path[0]) && (XTEXT('.') == Path[1]) && (XTEXT('\0') == Path[2]) ) {
      /* Success */
      return ( TRUE );
   }

   /* Success */
   return ( FALSE );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchFileExists)( 
   __in_z LPCXSTR Path 
)
{
   UINT  ErrorMode;
   DWORD dwAttributes;

   _ASSERTE(NULL != Path);

   ErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX); {
      dwAttributes = AWSUFFIX(GetFileAttributes)(Path);
   }   
   SetErrorMode(ErrorMode);

   if ( INVALID_FILE_ATTRIBUTES != dwAttributes ) {
      /* Success */
      return ( TRUE );
   }

   /* Failure */
   return ( FALSE );
}

BOOLEAN 
APIENTRY
AWSUFFIX(PathCchDirectoryExists)( LPCXSTR Path )
{
   UINT  ErrorMode;
   DWORD dwAttributes;

   _ASSERTE(NULL != Path);

   ErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX); {
      dwAttributes = AWSUFFIX(GetFileAttributes)(Path);      
   }
   SetErrorMode(ErrorMode);

   if ( INVALID_FILE_ATTRIBUTES != dwAttributes ) {
      if ( FILE_ATTRIBUTE_DIRECTORY & dwAttributes ) {
         /* Success */
         return ( TRUE );
      }
   }

   /* Failure */
   return ( FALSE );
}

BOOLEAN
APIENTRY
AWSUFFIX(PathCchCreateDirectory)( 
   __in_z LPCXSTR Path,
   __in_opt LPSECURITY_ATTRIBUTES SecurityAttributes
)
{
   BOOLEAN                    bRet;
   HRESULT                    hr;
   DWORD                      dwRet;

   size_t                     cchPath;
   LPXSTR                     pszPath;
   XCHAR                      chPath[MAX_PATH*2];
   AWSUFFIX(PATH_INFORMATION) PathInformation;
   LPXSTR                     pszDirectory;
   LPXSTR                     pszTail;

   /* Initialize locals */
   bRet = FALSE;
   
   /*
    * We're going to copy the string to a work buffer so we can modify
    * its contents as necessary. Determine if we can use a stack buffer
    * or if we need to allocate one on the heap
    */
   hr = AWSUFFIX(StringCchLength)(Path,
                                  PATHCCH_MAXLENGTH,
                                  &cchPath);
   if ( FAILED(hr) ) {
      SetLastError(ERROR_BAD_PATHNAME);
      /* Failure */
      return ( FALSE );
   }

   if ( cchPath < ARRAYSIZE(chPath) ) {
      cchPath = ARRAYSIZE(chPath);
      pszPath = chPath;
   }
   else {
      cchPath += 1;
      pszPath  = reinterpret_cast<LPXSTR>(malloc(cchPath * sizeof(XCHAR)));
      if ( !pszPath ) {
         SetLastError(ERROR_NOT_ENOUGH_MEMORY);
         /* Failure */
         return ( FALSE );
      }
   }

   ZeroMemory(pszPath,
              cchPath * sizeof(XCHAR));

   hr = AWSUFFIX(StringCchCopy)(pszPath,
                                cchPath,
                                Path);

   if ( FAILED(hr) ) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      
      bRet = FALSE;
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * Parse the path to find where the directory starts and where
    * a file name may start as well
    */
   if ( !AWSUFFIX(PathCchParseInformation)(pszPath,
                                           (USHORT)cchPath,
                                           &PathInformation) ) {
      SetLastError(ERROR_BAD_PATHNAME);

      bRet = FALSE;
      /* Failure */
      goto __CLEANUP;
   }

   /*
    * If there is no parent path, then there are no directories to create
    */
   if ( !PathInformation.ParentPath ) {
      bRet = TRUE;
      /* Success */
      goto __CLEANUP;
   }
   
   pszDirectory = (LPXSTR)PathInformation.ParentPath;
   
   if ( PathInformation.FinalName ) {
      /*
       * There is a file name in this path, so we terminate the directory at the start of it.
       *
       * Note that directories can contain . extensions and stream names, so those being present
       * without a final name do not constitute the tail of the directory
       */
      pszTail    = (LPXSTR)PathInformation.FinalName;
      (*pszTail) = XTEXT('\0');
   }
   else {
      pszTail = pszPath + cchPath;
   }

   if ( XTEXT('\\') == (*pszDirectory) ) {
      /*
       * Skip over the first \ if it exists as this specifies the root and we don't want
       * to create that
       */
      pszDirectory += 1;
   }
   
   /*
    * Walk the path, terminate it at directory seperators and create the directory. Repeat
    * this until we reach the tail of the directory
    */
   while ( pszDirectory < pszTail ) {
      if ( XTEXT('\\') == (*pszDirectory) ) {
         (*pszDirectory) = XTEXT('\0');
        
         if ( !AWSUFFIX(CreateDirectory)(pszPath,
                                         SecurityAttributes) ) {
            dwRet = GetLastError();
            if ( ERROR_ALREADY_EXISTS != dwRet ) {
               /* Failure */
               goto __CLEANUP;
            }
         }

         (*pszDirectory) = XTEXT('\\');
      }

      pszDirectory += 1;
   }

   /* Success */
   bRet = TRUE;

__CLEANUP:
   if ( pszPath != chPath ) {
      free(pszPath);
   }

   return ( bRet );
}

#undef CCHLENGTH
#undef AWSUFFIX
#undef XTEXT
#undef XCHAR
#undef LPXSTR
#undef LPCXSTR

#pragma pop_macro( "CCHLENGTH" )
#pragma pop_macro( "AWSUFFIX" )
#pragma pop_macro( "XTEXT" )
#pragma pop_macro( "XCHAR" )
#pragma pop_macro( "LPXSTR" )
#pragma pop_macro( "LPCXSTR" )
