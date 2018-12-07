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

/* NumberFmt.h
 *   Generic NUMBERFMT wrapper
 *
 * Copyright (C) 2006-2009 Jeremy Boschen
 */

#pragma once

#ifndef __NUMBERFMT_H__
#define __NUMBERFMT_H__

class CNUMBERFMT : public NUMBERFMT
{
public:
   CNUMBERFMT( 
   ) throw();

   CNUMBERFMT::CNUMBERFMT(
      __in LCID Locale
   ) throw();

   void
   Reset(
   ) throw();

   BOOLEAN
   Initialize( 
      __in LCID Locale
   ) throw();

   operator CONST NUMBERFMT*( ) throw()
   {
      return ( IsValid ? static_cast<CONST NUMBERFMT*>(this) : NULL );
   }

   operator NUMBERFMT*( ) throw()
   {
      return ( static_cast<NUMBERFMT*>(this) );
   }

   TCHAR    DecimalSeperator[4];
   TCHAR    ThousandSeperator[4];
   BOOLEAN  IsValid;

private:
   static 
   BOOLEAN 
   GetLocaleInformation( 
      __in LCID Locale, 
      __in LCTYPE LCType, 
      __out_ecount_opt(cchData) LPTSTR lpLCData, 
      __in int cchData 
   ) throw();
};

/*++
   GetLocaleFormattedNumber
  		Wrapper for GetNumberFormat to convert an integer/decimal value
  		to a string before forwarding it on to GetNumberFormat.
  
   Parameters
  		pszFormat
  		   [in] printf style format control string to create the string
  		   representation of the number, as required by GetNumberFormat.
  		pszBuf
  		   [out] Pointer to a buffer to receive the formatted number. 
  		cchBuf
  		   [in] Size of the buffer referenced by pszBuf, in TCHAR's.
  		lpFormat
  		   [in] Pointer to a NUMBERFMT structure used to format the
  		   the number.
  
   Return Value
  	   Returns the number of characters copied to pszBuf if successful,
  		otherwise 0.
  
   Remarks
  		The pszBuf, cchBuf and pNumberFmt parameters are forwarded unchanged 
      to GetNumberFormat, so follow the same rules as that function
 --*/
int 
__cdecl 
GetLocaleFormattedNumber( 
   __in_z __format_string LPCTSTR pszFormat, 
   __out_ecount_opt(cchBuf) LPTSTR lpNumberStr, 
   __in int cchNumber, 
   __in_opt const NUMBERFMT* lpFormat, 
   ...
);

#endif /* __NUMBERFMT_H__ */