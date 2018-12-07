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
 
/* GuidDb.cpp
 *    CGuidDatabase and helper implementations
 */

#include "Stdafx.h"

#include "FragLibp.h"
#include "GuidDbp.h"

#include <GenSort.h>

/*
 * Loaded during CRT initialization
 */
CLibGuidDatabase g_LibGuidDatabase;

/*++
   Implementations
 --*/
CGuidDatabase::CGuidDatabase( 
)
{
   ResetData();
}

CGuidDatabase::~CGuidDatabase( 
)
{
   Clear();
}

DWORD
CGuidDatabase::LoadArchiveFile(
   __in_z LPCTSTR FilePath
)
{
   DWORD dwRet;

   dwRet = AppendFile(FilePath);

   if ( _RecordCount > 0 )
   {
      SortRecords();
   }

   /* Success / Failure */
   return ( dwRet );
}

void
CGuidDatabase::Clear(
)
{
   if ( _MemoryBase )
   {
      /*
       * Free the database from the working set, then release it
       */
      VirtualUnlock(_MemoryBase,
                    _MemorySize);

      VirtualFree(_MemoryBase,
                  0,
                  MEM_RELEASE);

      ResetData();
   }
}

DWORD
CGuidDatabase::GetNameOfGuidW(
   __in LPCGUID Guid,
   __out_ecount(GuidNameLength) LPWSTR GuidName,
   __in size_t GuidNameLength,
   __in_z_opt LPCWSTR GuidNamePrefixList
)
{
   DWORD dwRet;
   
   char  chGuidName[CCH_NAME_MAXIMUM+1];
   char  chGuidNamePrefix[CCH_NAME_MAXIMUM+1];

   if ( GuidNamePrefixList )
   {
      if ( 0 == WideCharToMultiByte(CP_UTF8,
                                    0,
                                    GuidNamePrefixList,
                                    -1,
                                    chGuidNamePrefix,
                                    sizeof(chGuidNamePrefix),
                                    NULL,
                                    NULL) )
      {
         dwRet = GetLastError();
         /* Failure */
         return ( dwRet );
      }

      GuidNamePrefixList = reinterpret_cast<LPCWSTR>(&(chGuidNamePrefix[0]));
   }

   dwRet = GetNameOfGuidA(Guid,
                          chGuidName,
                          sizeof(chGuidName),
                          reinterpret_cast<LPCSTR>(GuidNamePrefixList));

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   if ( 0 == MultiByteToWideChar(CP_UTF8,
                                 0,
                                 chGuidName,
                                 -1,
                                 GuidName,
                                 static_cast<int>(GuidNameLength)) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   /* Success */
   return ( NO_ERROR );
}

DWORD
CGuidDatabase::GetNameOfGuidA(
   __in LPCGUID Guid,
   __out_ecount(GuidNameLength) LPSTR GuidName,
   __in size_t GuidNameLength,
   __in_z_opt LPCSTR GuidNamePrefixList
)
{
   LPCGUID  pGuid;
   LPCGUID  pGuidTail;
   LPCSTR   pszGuidName;

   if ( !_MemoryBase )
   {
      return ( ERROR_NOT_READY );
   }

   pGuid     = &(_GuidTable[0]);
   pGuidTail = &(_GuidTable[_RecordCount]);

   if ( !GuidNamePrefixList )
   {
      GuidNamePrefixList = "";
   }

   while ( pGuid < pGuidTail )
   {
      if ( InlineIsEqualGUID(*Guid,
                             *pGuid) )
      {
         pszGuidName = _NameBlock + _NameTable[pGuid - _GuidTable];

         if ( pszGuidName && IsNameInPrefixList(GuidNamePrefixList,
                                                pszGuidName) )
         {
            if ( SUCCEEDED(StringCbCopyA(GuidName,
                                         GuidNameLength / sizeof(char),
                                         pszGuidName)) )                             
            {
               return ( NO_ERROR );
            }

            return ( ERROR_INSUFFICIENT_BUFFER );
         }
      }

      pGuid++;
   }

   return ( ERROR_NOT_FOUND );
}

DWORD
CGuidDatabase::GetGuidOfNameW(
   __in_z LPCWSTR GuidName,
   __deref_out GUID* Guid
)
{
   DWORD dwRet;
   
   char  chGuidName[CCH_NAME_MAXIMUM+1];

   if ( 0 == WideCharToMultiByte(CP_UTF8,
                                 0,
                                 GuidName,
                                 -1,
                                 chGuidName,
                                 sizeof(chGuidName),
                                 NULL,
                                 NULL) )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   return ( GetGuidOfNameA(chGuidName,
                           Guid) );
}

DWORD
CGuidDatabase::GetGuidOfNameA(
   __in_z LPCSTR GuidName,
   __deref_out GUID* Guid
)
{
   DWORD* pName;
   DWORD* pNameTail;

   if ( !_MemoryBase )
   {
       return ( ERROR_NOT_READY );
   }

   pName     = &(_NameTable[0]);
   pNameTail = &(_NameTable[_RecordCount]);

   while ( pName < pNameTail )
   {
      if ( 0 == strcmp(GuidName,
                       &(_NameBlock[(*pName)])) )
      {
         (*Guid) = _GuidTable[(pName - &(_NameTable[0])) / sizeof(ULONG)];
         /* Success */
         return ( NO_ERROR );
      }

      pName++;
   }

   return ( ERROR_NOT_FOUND );
}

void
CGuidDatabase::Dump(
   __in DWORD Flags
)
{   
   char     chBuf[512];
   ULONG    idx;
   LPCGUID  pGuid;
   LPCSTR   pszName;
   
   if ( FlagOn(Flags, GDF_DUMP_STATISTICS) )
   {
      StringCchPrintfA(chBuf,
                       _countof(chBuf),
                       "GUID database information\n"
                       "\tName bucket length    - %d\n"
                       "\tAllocation alignment  - %d\n"
                       "\tAllocation size       - %d\n"
                       "\tActual size           - %d\n"
                       "\tUnused bytes          - %d\n"
                       "\tReallocations         - %d\n"
                       "\tGuid table            - 0x%p\n"
                       "\tName table            - 0x%p\n"
                       "\tName block            - 0x%p\n"
                       "\tName block unused     - %d\n"
                       "\tRecord count          - %d\n"
                       "\tMax record count      - %d\n",
                       CCH_NAME_AVERAGE,
                       CB_ALLOCATION_ALIGNMENT,
                       _MemorySize,
                       ((sizeof(GUID) + sizeof(ULONG)) * _RecordCount) + _NextNameOffset,
                       _MemorySize - (((sizeof(GUID) + sizeof(ULONG)) * _RecordCount) + _NextNameOffset),
                       _ReallocationCount - 1,
                       _GuidTable,
                       _NameTable,
                       _NameBlock,
                       (reinterpret_cast<LPSTR>(_MemoryBase) + _MemorySize) - (_NameBlock + _NextNameOffset),
                       _RecordCount,
                       _MaxRecordCount);

      OutputDebugStringA(chBuf);
   }

   if ( FlagOn(Flags, GDF_DUMP_RECORDS) )
   {
      OutputDebugStringA("GUID database records\n");

      for ( idx = 0; idx < _RecordCount; idx++ )
      {
         pGuid   = &(_GuidTable[idx]);
         pszName = &(_NameBlock[_NameTable[idx]]);

         StringCchPrintfA(chBuf,
                          _countof(chBuf),
                          "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X} - %s\n",
                          pGuid->Data1,
                          pGuid->Data2,
                          pGuid->Data3,
                          pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
                          pGuid->Data4[4], pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7],
                          pszName);

         OutputDebugStringA(chBuf);
      }
   }
}

void
CGuidDatabase::ResetData(
)
{
   _MemoryBase        = NULL;
   _GuidTable         = NULL;
   _NameTable         = NULL;
   _NameBlock         = NULL;
   _MemorySize        = 0;
   _RecordCount       = 0;
   _MaxRecordCount    = 0;
   _NextNameOffset    = 0;
   _ReallocationCount = 0;
}

DWORD
CGuidDatabase::EnsureStorage(
   ULONG MaxRecordCount
)
{
   SIZE_T   cbActual;
   SIZE_T   cbRequired;
   SIZE_T   cbAllocate;

   LPVOID   pMemoryBase;

   GUID*    pGuidTable;
   ULONG*   pNameTable;
   LPSTR    pNameBlock;

   /*
      In memory layout of the database 

      _GuidTable ->  {  GUID 1  }
                     {  GUID 2  }
                      ..........
                     {  GUID N  } N == _RecordCount
                     ............
                     {  GUID X  } X == _MaxRecordCount
                     ------------
      _NameTable ->  { Offset 1 } _NameBlock + (Offset 1)
                     { Offset 2 } _NameBlock + (Offset 2)
                      ..........
                     { Offset N } _NameBlock + (Offset N)
                     ............
                     { Offset X }
                     ------------
      _NameBlock ->    Name 1\0
                       Name 2\0
                       ........
                       Name N\0
                       ........
                       Name X\0
    */

   /*
    * We don't support shrinking the database
    */
   MaxRecordCount = max(MaxRecordCount, _RecordCount);

   if ( !(MaxRecordCount > _MaxRecordCount) )
   {
      /*
       * No need to redo the allocation as we have enough room for the number of requested
       * records. We can end up here when a previous call increased the record count to fill
       * up extra memory allocated to meet the allocation granularity
       */
      return ( NO_ERROR );
   }

   /*
    * Calculate the amount of memory required to hold the GUID table, the name offset table
    * and a name block with a name of average length for the maximum number of records 
    * specified by the caller and the current actual size of recorded data. Then align the
    * required size to an allocation boundary so we can cut down on how many times we need
    * this function to be called.
    *
    * Currently the allocation alignment is a factor of the allocation granularity of the
    * VirtualAlloc() function
    */
   
   /*
    * Current size of initialized records
    */
   cbActual = ((sizeof(GUID) + sizeof(ULONG)) * _RecordCount) + _NextNameOffset;

   /*
    * Size required to hold current and new records
    */
   cbRequired = ((sizeof(GUID) + sizeof(ULONG) + static_cast<ULONG>(CCH_NAME_AVERAGE)) * (MaxRecordCount - _RecordCount)) + cbActual;
   
   /*
    * Required size aligned to an allocation boundary
    */
   cbAllocate = AlignUp(cbRequired, static_cast<SIZE_T>(CB_ALLOCATION_ALIGNMENT));

   /*
    * Increment our allocations tracker. This is just helpful for fine tuning the constants
    * used to calculate the sizes
    */
   _ReallocationCount += 1;

   /*
    * Try to recalculate the maximum record count to use extra memory we might be allocating
    * to reach the alignment. We'll leave some extra space at the end for a name that's longer
    * than the average length, but not more than the maximum. Note that CCH_NAME_MAXIMUM does
    * not include the null termintator and so we must add it to all our calculations since we
    * are calcuating storage space, not actual data lengths
    */
   if ( (cbAllocate - static_cast<SIZE_T>(CCH_NAME_MAXIMUM + 1)) > cbRequired )
   {
      /* 
       * Add as many records as we can fit into the space being allocated beyond
       * what we already determined is required for the number of records that
       * the caller requested
       */
      MaxRecordCount += static_cast<ULONG>((cbAllocate - static_cast<SIZE_T>(CCH_NAME_MAXIMUM + 1) - cbRequired) / (sizeof(GUID) + sizeof(ULONG) + static_cast<size_t>(CCH_NAME_AVERAGE + 1)));
   }
   
   /*
    * Allocate the new block of memory
    */
   pMemoryBase = VirtualAlloc(NULL,
                              cbAllocate,
                              MEM_COMMIT,
                              PAGE_READWRITE);

   if ( !pMemoryBase )
   {
      /* Failure */
      return ( ERROR_NOT_ENOUGH_MEMORY );
   }
   
   ZeroMemory(pMemoryBase,
              cbAllocate);

   /*
    * Setup our table offsets into the new memory block
    */
   pGuidTable = reinterpret_cast<GUID*>(pMemoryBase);
   pNameTable = reinterpret_cast<ULONG*>(&(pGuidTable[MaxRecordCount]));
   pNameBlock = reinterpret_cast<LPSTR>(&(pNameTable[MaxRecordCount]));

   /*
    * Copy the existing records, if any, into their new positions in the new block
    */
   if ( _MemoryBase )
   {
      CopyMemory(&(pGuidTable[0]),
                 &(_GuidTable[0]),
                 sizeof(GUID) * _RecordCount);

      CopyMemory(&(pNameTable[0]),
                 &(_NameTable[0]),
                 sizeof(ULONG) * _RecordCount);

      CopyMemory(&(pNameBlock[0]),
                 &(_NameBlock[0]),
                 _NextNameOffset);

      /*
       * We're done with the old data now, so free it
       */
      VirtualFree(_MemoryBase,
                  0,
                  0);
   }

   /*
    * Store the new DB data. Note that _RecordCount and _NextNameOffset
    * don't change
    */
   _MemoryBase     = pMemoryBase;
   _GuidTable      = pGuidTable;
   _NameTable      = pNameTable;
   _NameBlock      = pNameBlock;
   _MemorySize     = cbAllocate;
   _MaxRecordCount = MaxRecordCount;

   /* Success */
   return ( NO_ERROR );
}

DWORD
CGuidDatabase::AppendFile(
   __in_z LPCTSTR ArchivePath
)
{
   DWORD    dwRet;
   HANDLE   hFile;
   HANDLE   hMapping;
   LPCVOID  pMapView;

   /* Initialize locals */
   dwRet    = NO_ERROR;
   hFile    = INVALID_HANDLE_VALUE;
   hMapping = NULL;
   pMapView = NULL;

   /*
    * Load the file and map it into memory. This is a lot easier than reading
    * the file in chunks or all at once
    */
   hFile = CreateFile(ArchivePath,
                      GENERIC_READ,
                      FILE_SHARE_READ,
                      NULL,
                      OPEN_EXISTING,
                      0,
                      NULL);

   if ( INVALID_HANDLE_VALUE == hFile )
   {
      dwRet = GetLastError();
      /* Failure */
      return ( dwRet );
   }

   __try
   {      
      hMapping = CreateFileMapping(hFile,
                                   NULL,
                                   PAGE_READONLY,
                                   0,
                                   0,
                                   NULL);

      if ( !hMapping )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      pMapView = MapViewOfFile(hMapping,
                               FILE_MAP_READ,
                               0,
                               0,
                               0);

      if ( !pMapView )
      {
         dwRet = GetLastError();
         /* Failure */
         __leave;
      }

      /*
       * The file is mapped and we have a view, so process it
       */
      dwRet = ImportArchive(pMapView);
   }
   __finally
   {
      if ( pMapView )
      {
         UnmapViewOfFile(pMapView);
      }

      if ( hMapping )
      {
         CloseHandle(hMapping);
      }

      if ( INVALID_HANDLE_VALUE != hFile )
      {
         CloseHandle(hFile);
      }
   }

   /* Success / Failure */
   return ( dwRet );
}

DWORD
CGuidDatabase::ImportArchive(
   __in LPCVOID Archive
)
{
   DWORD                      dwRet;

   PCLIB_MEMBER_TABLE         pMemberTable;
   PCLIB_SYMBOL_TABLE         pSymbolTable;
   PCLIB_MEMBER_TABLE_ENTRY   pTableEntry;

   ULONG                      cRecordsRequired;

   ULONG                      idx;
   WORD                       jdx;
   
   PCIMAGE_SYMBOL             pImageSymbol;               
   PCIMAGE_SYMBOL             pImageSymbolTail;
   ULONG                      cSectionSymbols;
   
   LPCGUID                    pGuid;
   LPCSTR                     pszGuidName;
   size_t                     cchGuidName;
   
   /*
    * Make sure it's an archive
    */
   if ( 0 != strncmp(reinterpret_cast<LPCSTR>(Archive),
                     IMAGE_ARCHIVE_START, 
                     IMAGE_ARCHIVE_START_SIZE) )
   {
      dwRet = ERROR_FILE_CORRUPT;
      /* Failure */
      return ( dwRet );
   }

   pMemberTable = GetLibMemberTable(Archive);
   pSymbolTable = GetLibSymbolTable(Archive);

   /*
    * Calculate how many records we need, and check for overflow
    */
   cRecordsRequired = _RecordCount + pSymbolTable->NumberOfSymbols;
   if ( cRecordsRequired < _RecordCount )
   {
      /* Failure */
      return ( ERROR_DATABASE_FULL );
   }

   dwRet = EnsureStorage(cRecordsRequired);
   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      return ( dwRet );
   }

   for ( idx = 0; idx < pMemberTable->NumberOfMembers; idx++ )
   {
      pTableEntry = GetLibMemberEntry(Archive, 
                                      pMemberTable, 
                                      idx);

      if ( !pTableEntry )
      {
         continue;
      }

      /*
       * Is this a new style IMPORT_OBJECT_HEADER?
       */
      if ( IsMemberEntryImportObject(pTableEntry) )
      {
         continue;
      }

      /*
       * The symbol table is used by all sections, so the tail doesn't change and
       * we can cache it here instead of retrieving it in each iteration below
       */
      pImageSymbolTail = GetLibMemberEntrySymbolTail(pTableEntry);

      for ( jdx = 0; jdx < pTableEntry->FileHeader.NumberOfSections; jdx++ )
      {            
         if ( !IsPossibleGuidSection(&(pTableEntry->SectionHeaders[jdx])) )
         {
            continue;
         }

         /*
          * Walk the symbol table for this section, and output each GUID if possible. Note
          * that section numbers are 1 based
          */
         pImageSymbol = GetSectionSymbolList(pTableEntry, 
                                             jdx + 1);

         /*
          * Get the number of symbols for this section. Note that this count doesn't include 
          * symbols that are for the section name
          */
         cSectionSymbols = GetSectionSymbolCount(pImageSymbol,
                                                 pImageSymbolTail);

         /*
          * It's possible that the section data size is a factor of sizeof(GUID) but the data
          * is not actually GUIDs. If they are GUIDs however, there should be a symbol for each
          * GUID in the section
          */
         if ( cSectionSymbols != (pTableEntry->SectionHeaders[jdx].SizeOfRawData / sizeof(GUID)) )
         {
            continue;
         }

         /*
          * Ok this looks like a GUID section, so extract all the GUIDs. We can't rely on the
          * section symbol count for this loop because it doesn't include symbols for the section
          * name and we're going to hit that one in our iteration, if it's present
          */
         while ( pImageSymbol < pImageSymbolTail )
         {
            /*
             * If we've passed all the symbols for this section, bust out and move on
             */
            if ( pImageSymbol->SectionNumber > (jdx + 1) )
            {
               break;
            }

            if ( !IsSymbolSectionName(pImageSymbol) && IsSymbolValueOffset(pImageSymbol) )
            {
               pszGuidName = GetImageSymbolStringInfo(pTableEntry,
                                                      pImageSymbol,
                                                      &cchGuidName);

               if ( pszGuidName )
               {
                  pGuid = reinterpret_cast<LPCGUID>(reinterpret_cast<LPCSTR>(&(pTableEntry->FileHeader)) + (pTableEntry->SectionHeaders[jdx].PointerToRawData + pImageSymbol->Value));

                  /* 
                   * This is a possible GUID, so we'll add it to the DB 
                   */
                  AddGuidToDatabase(pGuid,
                                    pszGuidName,
                                    cchGuidName);
               }
            }

            pImageSymbol = GetNextImageSymbol(pImageSymbol);
         }
      }
   }

   dwRet = NO_ERROR;
         
   /* Success / Failure */
   return ( dwRet );
}

void
CGuidDatabase::AddGuidToDatabase(
   __in LPCGUID Guid,
   __in_z LPCSTR GuidName,
   __in SIZE_T GuidNameLength
)
{
   SIZE_T cbRemaining;
   ULONG  cRecordsRequired;

   /*
    * Names cannot be greater than 255 characters in length. This is also the maximum length
    * of an identifier in MIDL which is where most GUIDs come from.
    */
   if ( GuidNameLength > static_cast<SIZE_T>(CCH_NAME_MAXIMUM) )
   {
      _ASSERTE(GuidNameLength < CCH_NAME_MAXIMUM);
      return;
   }

   cbRemaining      = static_cast<SIZE_T>((reinterpret_cast<LPSTR>(_MemoryBase) + _MemorySize) - (_NameBlock + _NextNameOffset));
   cRecordsRequired = _MaxRecordCount;

   /* 
    * If we've hit the maximum record count, allocate some more space
    */
   if ( !(_RecordCount < _MaxRecordCount) )
   {
      cRecordsRequired = _MaxRecordCount + static_cast<ULONG>(CC_RECORD_GROW);
   }

   /*
    * If there's not enough room to hold the GuidName, allocate some more space
    */      
   if ( (GuidNameLength + 1) > cbRemaining )
   {
      /*
       * We've run out of room in the name block, so allocate the number of records
       * necessary to contain a GuidName of the specified length. Each record contains 
       * space for an average name length, so we have to compute how many average sized 
       * names we need for this record. The GuidName length could be less than the average
       * name length too if previous entries overran their average space, in which case 
       * we fall back to the average name length. Also note that we add a byte for the 
       * null terminator.
       */
      cRecordsRequired = _MaxRecordCount + ((max(static_cast<ULONG>(CCH_NAME_AVERAGE + 1), static_cast<ULONG>(GuidNameLength)) + 1) / static_cast<ULONG>(CCH_NAME_AVERAGE + 1));
   }

   /*
    * We may need more space to hold this record, and if we do allocate it now
    */
   if ( cRecordsRequired != _MaxRecordCount )
   {
      if ( NO_ERROR != EnsureStorage(cRecordsRequired) )
      {
         _ASSERTE(FALSE);
         /* Failure */
         return;
      }
   
      /*
       * Recalculate how much remaining room there is for the GuidName with the newly
       * allocated buffer. The only way this will fail to be less than we need is if
       * we screwed up the calculation above
       */
      cbRemaining = static_cast<SIZE_T>((reinterpret_cast<LPSTR>(_MemoryBase) + _MemorySize) - (_NameBlock + _NextNameOffset));
      _ASSERTE((GuidNameLength + 1) <= cbRemaining);
   }

   /*
    * Store the GUID
    */
   _GuidTable[_RecordCount] = (*Guid);

   /*
    * Store the name offset
    */
   _NameTable[_RecordCount] = _NextNameOffset;

   /*
    * Copy the name
    */
   StringCchCopyNA(_NameBlock + _NextNameOffset,
                   cbRemaining,
                   GuidName,
                   GuidNameLength);

   /*
    * Update the record count
    */
   _RecordCount += 1;

   /*
    * Calculate the offset for the start of the next record's GuidName
    */
   _ASSERTE((_NextNameOffset + (static_cast<ULONG>(GuidNameLength) + 1)) > _NextNameOffset);
   _NextNameOffset += (static_cast<ULONG>(GuidNameLength) + 1);
}

int 
CGuidDatabase::CompareRecordProxy(
   __in void* Context,
   __in ULONG A,
   __in ULONG B
)
{
   return ( reinterpret_cast<CGuidDatabase*>(Context)->CompareRecord(A, 
                                                                     B) );
}

int
CGuidDatabase::CompareRecord(
   ULONG A,
   ULONG B
)
{
   LPCGUID  GuidA;
   LPCGUID  GuidB;
   int      idx;
         
   if ( A == B )
   {
      return ( 0 );
   }

   GuidA = &(_GuidTable[A]);
   GuidB = &(_GuidTable[B]);

   if ( GuidA->Data1 != GuidB->Data1 )
   {
      return ( GuidA->Data1 < GuidB->Data1 ? -1 : 1 );
   }

   if ( GuidA->Data2 != GuidB->Data2 )
   {
      return ( GuidA->Data2 < GuidB->Data2 ? -1 : 1 );
   }

   if ( GuidA->Data3 != GuidB->Data3 )
   {
      return ( GuidA->Data3 < GuidB->Data3 ? -1 : 1 );
   }

   for ( idx = 0; idx < sizeof(GuidA->Data4); idx++ )
   {
      if ( GuidA->Data4[idx] < GuidB->Data4[idx] )
      {
         return ( -1 );
      }

      if ( GuidA->Data4[idx] > GuidB->Data4[idx] )
      {
         return ( 1 );
      }
   }

#ifdef GUIDDB_SORTCLOCALE
   return ( _stricoll(_NameBlock + _NameTable[A], 
                      _NameBlock + _NameTable[B]) );
#else /* GUIDDB_SORTCLOCALE */
   return ( strcmp(_NameBlock + _NameTable[A], 
                   _NameBlock + _NameTable[B]) );
#endif /* GUIDDB_SORTCLOCALE */
}

void
CGuidDatabase::SortRecords(
)
{
   GenericSortT<ULONG>(_RecordCount,
                       &CGuidDatabase::CompareRecordProxy,
                       &CGuidDatabase::ExchangeRecordProxy,
                       this);
}

void
CGuidDatabase::ExchangeRecordProxy(
   void* Context,
   __in ULONG A,
   __in ULONG B
)
{
   reinterpret_cast<CGuidDatabase*>(Context)->ExchangeRecord(A,
                                                             B);
}

void 
CGuidDatabase::ExchangeRecord( 
   __in ULONG A,
   __in ULONG B
)
{
   GUID   GuidTemp;
   ULONG  NameTemp;

   GuidTemp      = _GuidTable[A];
   _GuidTable[A] = _GuidTable[B];
   _GuidTable[B] = GuidTemp;
   
   NameTemp      = _NameTable[A];
   _NameTable[A] = _NameTable[B];
   _NameTable[B] = NameTemp;
}

BOOLEAN
CGuidDatabase::IsNameInPrefixList(
   __in_z LPCSTR PrefixList,
   __in_z LPCSTR Name
)
{
   do
   {
      if ( ('\0' == (*PrefixList)) || (';' == (*PrefixList)) )
      {
         return ( TRUE );
      }

      if ( '\0' == (*Name) )
      {
         return ( FALSE );
      }

      if ( ';' == (*PrefixList) )
      {
         PrefixList++;
      }
   }
   while ( (*PrefixList++) == (*Name++) );

   return ( FALSE );
}

ULONG
CGuidDatabase::GetArchiveMemberSize(
   PCIMAGE_ARCHIVE_MEMBER_HEADER ArchiveMember
)
{
   char chSize[sizeof(ArchiveMember->Size) + 1];

   ZeroMemory(chSize,
              sizeof(chSize));

   strncpy(chSize,
           reinterpret_cast<const char*>(&(ArchiveMember->Size[0])),
           sizeof(ArchiveMember->Size));

   return ( static_cast<ULONG>(atoi(chSize)) );
}

CGuidDatabase::PCLIB_MEMBER_TABLE
CGuidDatabase::GetLibMemberTable(
   __in LPCVOID FileBase
)
{
   ULONG cbArchiveMember;
   ULONG cbMemberOffset;
   
   cbArchiveMember = GetArchiveMemberSize(reinterpret_cast<PCIMAGE_ARCHIVE_MEMBER_HEADER>(reinterpret_cast<LPCSTR>(FileBase) + IMAGE_ARCHIVE_START_SIZE));
   cbMemberOffset  = IMAGE_ARCHIVE_START_SIZE + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR + cbArchiveMember;

   return ( reinterpret_cast<PCLIB_MEMBER_TABLE>(reinterpret_cast<LPCSTR>(FileBase) + AlignUp(cbMemberOffset, 2)) );
}

CGuidDatabase::PCLIB_MEMBER_TABLE_ENTRY
CGuidDatabase::GetLibMemberEntry(
   __in LPCVOID FileBase,
   __in PCLIB_MEMBER_TABLE MemberTable,
   __in ULONG Index
)
{
   /*
    * Some LIB files have elements with a value of 0 in the second linker member table. I have no
    * idea why but without an offset there is nothing to reference
    */
   if ( MemberTable->MemberOffset[Index] > 0 )
   {
      return ( reinterpret_cast<PCLIB_MEMBER_TABLE_ENTRY>(reinterpret_cast<LPCSTR>(FileBase) + MemberTable->MemberOffset[Index]) );
   }

   return ( NULL );
}

BOOLEAN
CGuidDatabase::IsMemberEntryImportObject(
   __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
)
{
   return ( (IMAGE_FILE_MACHINE_UNKNOWN == TableEntry->ObjectHeader.Sig1) && (IMPORT_OBJECT_HDR_SIG2 == TableEntry->ObjectHeader.Sig2) );
}

CGuidDatabase::PCLIB_SYMBOL_TABLE
CGuidDatabase::GetLibSymbolTable(
   __in LPCVOID FileBase
)
{
   PCLIB_MEMBER_TABLE MemberTable;

   MemberTable = GetLibMemberTable(FileBase);

   return ( reinterpret_cast<PCLIB_SYMBOL_TABLE>(&(MemberTable->MemberOffset[MemberTable->NumberOfMembers])) );
}

CGuidDatabase::PCIMAGE_SYMBOL
CGuidDatabase::GetLibMemberEntrySymbolTable(
   __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
)
{
   if ( TableEntry->FileHeader.PointerToSymbolTable )
   {
      return ( reinterpret_cast<PCIMAGE_SYMBOL>(reinterpret_cast<LPCSTR>(&(TableEntry->FileHeader)) + TableEntry->FileHeader.PointerToSymbolTable) );
   }

   return ( NULL );
}
   
CGuidDatabase::PCIMAGE_SYMBOL
CGuidDatabase::GetNextImageSymbol(
   PCIMAGE_SYMBOL ImageSymbol
)
{
   /* Pointer math.. */
   return ( ImageSymbol + 1 + ImageSymbol->NumberOfAuxSymbols );
}

CGuidDatabase::PCIMAGE_SYMBOL
CGuidDatabase::GetLibMemberEntrySymbolTail(
   __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
)
{
   PCIMAGE_SYMBOL ImageSymbol;

   ImageSymbol = GetLibMemberEntrySymbolTable(TableEntry);
   if ( ImageSymbol )
   {
      ImageSymbol = &(ImageSymbol[TableEntry->FileHeader.NumberOfSymbols]);
   }

   return ( ImageSymbol );
}

CGuidDatabase::PCIMAGE_AUX_SYMBOL
CGuidDatabase::GetImageAuxSymbol(
   __in PCIMAGE_SYMBOL ImageSymbol,
   __in BYTE Index
)
{
   if ( Index >= ImageSymbol->NumberOfAuxSymbols )
   {
      return ( NULL );
   }

   return ( &((reinterpret_cast<PCIMAGE_AUX_SYMBOL>(ImageSymbol + 1))[Index]) );
}

BOOLEAN
CGuidDatabase::IsSymbolSectionName(
   PCIMAGE_SYMBOL ImageSymbol
)
{
   return ( (IMAGE_SYM_CLASS_STATIC == ImageSymbol->StorageClass) && (0 == ImageSymbol->Value) );
}

BOOLEAN
CGuidDatabase::IsSymbolValueOffset(
   PCIMAGE_SYMBOL ImageSymbol
)
{
   return ( (IMAGE_SYM_CLASS_EXTERNAL == ImageSymbol->StorageClass) && (IMAGE_SYM_UNDEFINED != ImageSymbol->SectionNumber) );
}

BOOLEAN
CGuidDatabase::IsPossibleGuidSection(
   PCIMAGE_SECTION_HEADER SectionHeader
)
{
   if ( 0 == (SectionHeader->SizeOfRawData % sizeof(GUID)) )
   {
      if ( FlagOn(SectionHeader->Characteristics,
                  IMAGE_SCN_CNT_INITIALIZED_DATA|IMAGE_SCN_LNK_COMDAT|IMAGE_SCN_MEM_READ) )
      {
         if ( 0 == strncmp((const char*)&(SectionHeader->Name[0]),
                           ".rdata",
                           IMAGE_SIZEOF_SHORT_NAME) )
         {
            return ( TRUE );
         }
      }
   }

   return ( FALSE );
}

CGuidDatabase::PCIMAGE_SYMBOL
CGuidDatabase::GetSectionSymbolList(
   __in PCLIB_MEMBER_TABLE_ENTRY TableEntry,
   __in WORD SectionNumber
)
{
   PCIMAGE_SYMBOL ImageSymbol;
   PCIMAGE_SYMBOL ImageSymbolTail;

   ImageSymbol = GetLibMemberEntrySymbolTable(TableEntry);
   if ( ImageSymbol )
   {
      ImageSymbolTail = &(ImageSymbol[TableEntry->FileHeader.NumberOfSymbols]);

      while ( ImageSymbol < ImageSymbolTail )
      {
         if ( ImageSymbol->SectionNumber == SectionNumber )
         {
            return ( ImageSymbol );
         }

         ImageSymbol = GetNextImageSymbol(ImageSymbol);
      }
   }

   return ( NULL );
}

ULONG
CGuidDatabase::GetSectionSymbolCount(
   __in PCIMAGE_SYMBOL ImageSymbolList,
   __in PCIMAGE_SYMBOL ImageSymbolTail
)
{
   ULONG cSymbols;
   SHORT iSectionNumber;

   cSymbols       = 0;
   iSectionNumber = ImageSymbolList->SectionNumber;

   while ( ImageSymbolList < ImageSymbolTail )
   {
      if ( ImageSymbolList->SectionNumber > iSectionNumber )
      {
         break;
      }

      if ( !IsSymbolSectionName(ImageSymbolList) )
      {
         cSymbols += 1;
      }

      ImageSymbolList = GetNextImageSymbol(ImageSymbolList);
   }

   return ( cSymbols );
}

CGuidDatabase::PCIMAGE_SYMBOL_STRING_TABLE
CGuidDatabase::GetImageSymbolStringInfoTable(
   __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
)
{
   PCIMAGE_SYMBOL ImageSymbol;
   ImageSymbol = GetLibMemberEntrySymbolTable(TableEntry);

   if ( ImageSymbol )
   {
      return ( reinterpret_cast<PCIMAGE_SYMBOL_STRING_TABLE>(&(ImageSymbol[TableEntry->FileHeader.NumberOfSymbols])) );
   }

   return ( NULL );
}

LPCSTR
CGuidDatabase::GetImageSymbolStringInfo(
   __in PCLIB_MEMBER_TABLE_ENTRY TableEntry,
   __in PCIMAGE_SYMBOL ImageSymbol,
   __deref_out size_t* StringLength
)
{
   PCIMAGE_SYMBOL_STRING_TABLE StringTable;
   LPCSTR                      pszString;

   if ( ImageSymbol->N.Name.Short )
   {
      pszString = reinterpret_cast<LPCSTR>(&(ImageSymbol->N.ShortName[0]));

      /*
       * There is a chance that the symbol name is exactly 8 bytes long, in which
       * case it won't be null-terminated so we need to set the length and return
       * here instead of falling through to the code path below which assumes the
       * string is null-terminated, if it is exactly 8 bytes long
       */
      if ( '\0' != ImageSymbol->N.ShortName[sizeof(ImageSymbol->N.ShortName) - 1] )
      {
         (*StringLength) = sizeof(ImageSymbol->N.ShortName);

         return ( pszString );
      }
   }
   else
   {
      StringTable = GetImageSymbolStringInfoTable(TableEntry);
      pszString   = reinterpret_cast<LPCSTR>(reinterpret_cast<LPCSTR>(StringTable) + ImageSymbol->N.Name.Long);
   }

   (*StringLength) = strlen(pszString);

   return ( pszString );
}

CLibGuidDatabase::CLibGuidDatabase(
)
{
#ifdef GUIDDB_LIB_PATH
   int               idx;
   HANDLE            hFind;
   WIN32_FIND_DATA   FindData;
   WCHAR             chPath[MAX_PATH];      
   LPCWSTR           rgLibFile[] = 
   {
#ifdef GUIDDB_LIB_LOADALL
      L"*.LIB"
#else /* GUIDDB_LIB_LOADALL */
      L"UUID.LIB",
      L"OLE32.LIB"
#endif /* GUIDDB_LIB_LOADALL */
   };

   for ( idx = 0; idx < _countof(rgLibFile); idx++ )
   {
      if ( SUCCEEDED(StringCchPrintfW(chPath,
                                      _countof(chPath),
                                      L"%s\\%s",
                                      _LTEXT(_tostring(GUIDDB_LIB_PATH)),
                                      rgLibFile[idx])) )
      {
         hFind = FindFirstFileW(chPath,
                                &FindData);

         if ( INVALID_HANDLE_VALUE != hFind )
         {
            do
            {
               if ( SUCCEEDED(StringCchPrintfW(chPath,
                                               _countof(chPath),
                                               L"%s\\%s",
                                               _LTEXT(_tostring(GUIDDB_LIB_PATH)),
                                               &(FindData.cFileName[0]))) )
               {
                  LoadArchiveFile(chPath);
               }
            }
            while ( FindNextFileW(hFind,
                                  &FindData) );

            FindClose(hFind);
         }
      }
   }   
#endif /* GUIDDB_LIB_PATH */
}

/*++
   
   PUBLIC

 --*/

DWORD
APIENTRY
LookupNameOfGuidA(
   __in REFGUID riid,
   __out_ecount(cchNameLength) LPSTR Name,
   __in size_t cchNameLength
)
{
   HKEY     hKey;

   CHAR     chKey[60];
   CHAR     chName[GDB_CCH_MAXGUIDNAME + 3];

   DWORD    dwType;
   DWORD    cbData;

   BOOL     bFound;
   size_t   idx;
   LPCSTR   rgSubKey[] =
   {
      "Interface",
      "CLSID"
   };

   StringCchPrintfA(Name,
                    cchNameLength,
                    "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                    riid.Data1,
                    riid.Data2,
                    riid.Data3,
                    riid.Data4[0], riid.Data4[1], riid.Data4[2], riid.Data4[3],
                    riid.Data4[4], riid.Data4[5], riid.Data4[6], riid.Data4[7]);

   chName[0] = ' ';
   chName[1] = '-';
   chName[2] = ' ';
   
   /*
    * Try to find it in the global guid database first if available
    */
   if ( NO_ERROR == g_LibGuidDatabase.GetNameOfGuidA(&riid,
                                                     &(chName[3]),
                                                     _countof(chName) - 3,
                                                     "IID;CLSID;SID;LIBID;GUID;DIID") )
   {
      StringCchCatA(Name,
                    cchNameLength,
                    chName);

      /* Success */
      return ( NO_ERROR );
   }                                                   

   bFound = FALSE;
   
   for ( idx = 0; idx < _countof(rgSubKey); idx++ )
   {
      StringCchPrintfA(chKey,
                       _countof(chKey),
                       "%s\\%s",
                       rgSubKey[idx],
                       Name);

      if ( ERROR_SUCCESS == RegOpenKeyExA(HKEY_CLASSES_ROOT,
                                          chKey,
                                          0,
                                          KEY_READ,
                                          &hKey) )
      {
         dwType = REG_SZ;
         cbData = sizeof(chName) - (4 * sizeof(CHAR));

         if ( ERROR_SUCCESS == RegQueryValueExA(hKey,
                                                NULL,
                                                NULL,
                                                &dwType,
                                                reinterpret_cast<LPBYTE>(&(chName[3])),
                                                &cbData) )
         {
            if ( dwType == REG_SZ )
            {
               bFound = TRUE;
               chName[_countof(chName)-1] = '\0';
            }
         }

         RegCloseKey(hKey);

         if ( bFound )
         {
            break;
         }
      }
   }

   if ( bFound )
   {
      StringCchCatA(Name,
                    cchNameLength,
                    chName);

      /* Success */
      return ( NO_ERROR );
   }

   /* Failure */
   return ( ERROR_NOT_FOUND );
}

DWORD
APIENTRY
LookupNameOfGuidW(
   __in REFGUID riid,
   __out_ecount(cchNameLength) LPWSTR Name,
   __in size_t cchNameLength
)
{
   DWORD    dwRet;
   
   LPSTR    pszName;
   char     chName[GDB_CCH_MAXGUIDNAME + 3];
   size_t   cchName;
   
   if ( cchNameLength > sizeof(chName) )
   {
      cchName = cchNameLength;
      pszName = reinterpret_cast<LPSTR>(malloc(cchName));
   }
   else
   {
      pszName = chName;
      cchName = sizeof(chName);
   }

   ZeroMemory(pszName,
              cchNameLength * sizeof(char));


   dwRet = LookupNameOfGuidA(riid,
                             pszName,
                             cchName);

   if ( NO_ERROR != dwRet )
   {
      /* Failure */
      goto __CLEANUP;
   }

   if ( 0 == MultiByteToWideChar(CP_UTF8,
                                 0,
                                 pszName,
                                 -1,
                                 Name,
                                 static_cast<int>(cchNameLength)) )
   {
      dwRet = GetLastError();
      /* Failure */
      goto __CLEANUP;
   }

__CLEANUP:
   if ( pszName != chName )
   {
      free(pszName);
   }

   /* Success / Failure */
   return ( dwRet );
}
