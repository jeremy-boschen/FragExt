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
 
/* GenSort.h
 *    Generic sort routines
 */

#pragma once

/*++   
   GenericSortT
      A generic sorting function that sorts user-defined data-sets using
      shared array indexes, delegating compare and exchange operations
      to caller supplied functions

   Template parameters
      IndexType
         - Storage type of the array index, eg. size_t, int, etc

   Parameters
      Count
         - Count in elements of the array being sorted
      CompareRoutine
         - Address of a function to compare two elements. The routine
         should return 0 if the indexes represent equal elements, -1 
         if index A represents an element less than index B, and 1 if
         index A represents an element greater than index B.
      ExchangeRoutine
         - Address of a routine to swap two elements represented by the
         given indexes
      Context
         - Caller supplied context passed to compare/exchange routines
      
   Return value
      None

   Remarks
      This function uses a combination of a comb and gnome sort to achieve
      acceptable performance with low complexity for small to medium sized
      data sets.
      
      Indexes are used, rather than accesssing data directly so callers 
      aren't restricted to sorting single dimension arrays. For instance, 
      a caller can use a custom compare/exchange routine to sort multiple 
      tables at once based on a common indexing scheme.
 --*/
template < class IndexType >
inline
void
GenericSortT(
   __in typename IndexType Count,
   __in int (*CompareRoutine)( void* Context, IndexType A, IndexType B ),
   __in void (*ExchangeRoutine)( void* Context, IndexType A, IndexType B ),
   __in_opt void* Context
)
{
   IndexType Gap;
   IndexType Idx;
   IndexType Jdx;

   /*
    * Start off using a comb sort while the gap size is greater than 10, then 
    * break out and finish up using a gnome sort
    */
   Gap = Count;

   while ( (Gap = static_cast<IndexType>(static_cast<float>(Gap) / 1.279604943109628F)) > 11 )
   {
      Idx = 0;

      while ( (Idx + Gap) < Count )
      {
         if ( CompareRoutine(Context,
                             Idx,
                             Idx + Gap) > 0 )
         {
            ExchangeRoutine(Context,
                            Idx,
                            Idx + Gap);
         }

         Idx++;
      }
   }

   /*
    * The data is nearly sorted at this point, so finish it off using a gnome sort
    */
   Idx = 1;
   Jdx = 2;

   while ( Idx < Count )
   {
      if ( CompareRoutine(Context,
                          Idx - 1,
                          Idx) <= 0 )
      {
         Idx  = Jdx;
         Jdx += 1;
      }
      else
      {
         ExchangeRoutine(Context,
                         Idx - 1,
                         Idx);

         Idx -= 1;

         if ( 0 == Idx )
         {
            Idx  = Jdx;
            Jdx += 1;
         }
      }      
   }
}
