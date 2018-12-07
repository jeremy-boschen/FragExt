/* FragExt - Shell extension for providing file fragmentation
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

/* NumberFmt.cpp
 *   CNUMBERFMT
 *
 * Copyright (C) 2006-2009 Jeremy Boschen
 */

#include "Stdafx.h"

#include <NumberFmt.h>

CNUMBERFMT::CNUMBERFMT( 
) throw() 
{
   Reset();
}

CNUMBERFMT::CNUMBERFMT(
   __in LCID Locale
) throw() 
{
   Reset();
   Initialize(Locale);
}

void
CNUMBERFMT::Reset(
) throw()
{
   ZeroMemory(static_cast<NUMBERFMT*>(this),
              sizeof(NUMBERFMT));

   ZeroMemory(&NumDigits,
              sizeof(NUMBERFMT));

   ZeroMemory(DecimalSeperator,
              sizeof(DecimalSeperator));

   ZeroMemory(ThousandSeperator,
              sizeof(ThousandSeperator));
  
   lpDecimalSep  = DecimalSeperator;
   lpThousandSep = ThousandSeperator;
}

BOOLEAN 
CNUMBERFMT::Initialize( 
   __in LCID Locale 
) throw()
{
   UINT     uGroup;
   TCHAR    chGroup[10];
   LPTSTR   pszChar;
   
   if ( !GetLocaleInformation(Locale,
                              LOCALE_IDIGITS|LOCALE_RETURN_NUMBER, 
                              reinterpret_cast<LPTSTR>(&NumDigits),
                              sizeof(NumDigits))            ||
        !GetLocaleInformation(Locale,
                              LOCALE_ILZERO|LOCALE_RETURN_NUMBER, 
                              reinterpret_cast<LPTSTR>(&LeadingZero),
                              sizeof(LeadingZero))          ||
        !GetLocaleInformation(Locale,
                              LOCALE_INEGNUMBER|LOCALE_RETURN_NUMBER, 
                              reinterpret_cast<LPTSTR>(&NegativeOrder),
                              sizeof(NegativeOrder))        ||
        !GetLocaleInformation(Locale,
                              LOCALE_SGROUPING, 
                              chGroup, 
                              _countof(chGroup))            ||
        !GetLocaleInformation(Locale, 
                              LOCALE_SDECIMAL, 
                              DecimalSeperator, 
                              _countof(DecimalSeperator))   ||
        !GetLocaleInformation(Locale, 
                              LOCALE_STHOUSAND, 
                              ThousandSeperator,
                              _countof(ThousandSeperator)) )
   {
      IsValid = FALSE;
      /* Failure */
      return ( FALSE );
   }
   
   /*
    * Convert the grouping from characters to its numeric form 
    */
   uGroup  = 0;
   pszChar = chGroup;

   while ( (_T('\0') != (*pszChar)) && (pszChar < &(chGroup[_countof(chGroup)])) )
   {
      if ( (_T('0') <= (*pszChar)) && (_T('9') >= (*pszChar)) )
      {
         uGroup = (uGroup * 10) + ((*pszChar) - _T('0'));
      }

      pszChar += 1;
   }

   /* The grouping string uses a trailing 0 to specify a repeat of the
    * previous value, whereas the numeric form uses an implicit repeat
    * and 0 to specify disabling repeating groups, so we need to drop
    * any trailing 0 */
   if ( 0 == (uGroup % 10) )
   {
      uGroup /= 10;
   }

   Grouping = uGroup;
   IsValid  = TRUE;

   /* Success */
   return ( TRUE );
}

BOOLEAN 
CNUMBERFMT::GetLocaleInformation( 
   __in LCID Locale, 
   __in LCTYPE LCType, 
   __out_ecount_opt(cchData) LPTSTR lpLCData, 
   __in int cchData 
) throw()
{
   if ( 0 == ::GetLocaleInfo(Locale,
                             LCType,
                             lpLCData,
                             cchData) )
   {
      /* Failure */
      return ( FALSE );
   }

   /* Success */
   return ( TRUE );
}

int 
__cdecl 
GetLocaleFormattedNumber( 
   __in_z __format_string LPCTSTR pszFormat, 
   __out_ecount_opt(cchBuf) LPTSTR lpNumberStr, 
   __in int cchNumber, 
   __in_opt const NUMBERFMT* lpFormat, 
   ... 
)
{
	_ASSERTE(NULL != pszFormat);
	
	int     nRet;
	va_list args;
	TCHAR   chNumber[512];

   /* Initialize locals.. */
   nRet = 0;

	ZeroMemory(chNumber,
              sizeof(chNumber));
	
	va_start(args, 
            lpFormat);

   if ( SUCCEEDED(StringCchVPrintf(chNumber, 
                                   _countof(chNumber), 
                                   pszFormat, 
                                   args)) )
	{
	   nRet = ::GetNumberFormat(LOCALE_USER_DEFAULT, 
                               0, 
                               chNumber, 
                               lpFormat, 
                               lpNumberStr, 
                               cchNumber);
	}

	va_end(args);

	return ( nRet );
}
