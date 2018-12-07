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
 
/* GuidDb.h
 *    In memory GUID database
 */

#pragma once

#include <GuidDb.h>

/*++
   Private Declarations
 --*/
#define GDF_DUMP_STATISTICS   0x00000001U
#define GDF_DUMP_RECORDS      0x00000002U
#define GDF_DUMP_ALL          (GDF_DUMP_STATISTICS|GDF_DUMP_RECORDS)

/*++
   CGuidDatabase
      Creates an in memory database of GUID/Guid-Name pairs for debugging
      lookups of QI/CreateInstance calls.

      Currently this only supports loading data from archive (LIB) files

   - This class is NOT thread safe!

 --*/
class CGuidDatabase
{
public:
   CGuidDatabase( 
   ) throw();

   ~CGuidDatabase( 
   ) throw();

   DWORD
   LoadArchiveFile(
      __in_z LPCTSTR FilePath
   );

   void
   Clear(
   );

   DWORD
   GetNameOfGuidW(
      __in LPCGUID Guid,
      __out_ecount(GuidNameLength) LPWSTR GuidName,
      __in size_t GuidNameLength,
      __in_z_opt LPCWSTR GuidNamePrefixList
   );

   DWORD
   GetNameOfGuidA(
      __in LPCGUID Guid,
      __out_ecount(GuidNameLength) LPSTR GuidName,
      __in size_t GuidNameLength,
      __in_z_opt LPCSTR GuidNamePrefixList
   );

   DWORD
   GetGuidOfNameW(
      __in_z LPCWSTR GuidName,
      __deref_out GUID* Guid
   );

   DWORD
   GetGuidOfNameA(
      __in_z LPCSTR GuidName,
      __deref_out GUID* Guid
   );

   void
   Dump(
      __in DWORD Flags
   );

   /*++
      These are specialized definitions of types defined in the Microsoft Portable 
      Executable and Common Object File Format Specification, suited for this class

      See the following for further reference:
      http://www.microsoft.com/whdc/system/platform/firmware/PECOFF.mspx
    --*/
#pragma pack(push, 1)
   typedef struct _LIB_MEMBER_TABLE
   {
      IMAGE_ARCHIVE_MEMBER_HEADER ArchiveMemberHeader;
      ULONG                       NumberOfMembers;
      ULONG                       MemberOffset[ANYSIZE_ARRAY];
   }LIB_MEMBER_TABLE, *PLIB_MEMBER_TABLE;
   typedef const struct _LIB_MEMBER_TABLE* PCLIB_MEMBER_TABLE;

   typedef struct _LIB_SYMBOL_TABLE
   {
      ULONG  NumberOfSymbols;
      USHORT MemberOffsetIndex[ANYSIZE_ARRAY];
   }LIB_SYMBOL_TABLE, *PLIB_SYMBOL_TABLE;
   typedef const struct _LIB_SYMBOL_TABLE* PCLIB_SYMBOL_TABLE;

   typedef struct _IMAGE_SYMBOL_STRING_TABLE
   {
      ULONG SizeOfTable;
   }IMAGE_SYMBOL_STRING_TABLE, *PIMAGE_SYMBOL_STRING_TABLE;
   typedef const struct _IMAGE_SYMBOL_STRING_TABLE* PCIMAGE_SYMBOL_STRING_TABLE;

   typedef struct _LIB_MEMBER_TABLE_ENTRY
   {
      IMAGE_ARCHIVE_MEMBER_HEADER ArchiveMemberHeader;
      union
      {
         struct
         {
            IMAGE_FILE_HEADER    FileHeader;
            IMAGE_SECTION_HEADER SectionHeaders[ANYSIZE_ARRAY];
         };
         IMPORT_OBJECT_HEADER    ObjectHeader;
      };
   }LIB_MEMBER_TABLE_ENTRY, *PLIB_MEMBER_TABLE_ENTRY;
   typedef const struct _LIB_MEMBER_TABLE_ENTRY* PCLIB_MEMBER_TABLE_ENTRY;   
#pragma pack(pop)

   typedef const struct _IMAGE_ARCHIVE_MEMBER_HEADER* PCIMAGE_ARCHIVE_MEMBER_HEADER;
   typedef const struct _IMAGE_SYMBOL* PCIMAGE_SYMBOL;
   typedef const union _IMAGE_AUX_SYMBOL* PCIMAGE_AUX_SYMBOL;
   typedef const struct _IMAGE_SECTION_HEADER* PCIMAGE_SECTION_HEADER;

   enum
   {
      CC_RECORD_GROW          = 128,
      CCH_NAME_AVERAGE        = 12,
      CCH_NAME_MAXIMUM        = 255,
      CB_ALLOCATION_ALIGNMENT = 1 * (64 * 1024)
   };

   /*
    * Address and size of memory region allocated for database records
    */
   LPVOID   _MemoryBase;
   SIZE_T   _MemorySize;
   
   /*
    * Database table pointers
    */
   GUID*    _GuidTable;
   ULONG*   _NameTable;
   LPSTR    _NameBlock;
   ULONG    _NextNameOffset;

   /*
    * Record keeping, debugging info
    */
   ULONG    _RecordCount;
   ULONG    _MaxRecordCount;
   ULONG    _ReallocationCount;

   void
   ResetData(
   );

   DWORD
   EnsureStorage(
      ULONG MaxRecordCount
   );

   DWORD
   AppendFile(
      __in_z LPCTSTR ArchivePath
   );

   DWORD
   ImportArchive(
      __in LPCVOID Archive
   );

   void
   AddGuidToDatabase(
      __in LPCGUID Guid,
      __in_z LPCSTR GuidName,
      __in SIZE_T GuidNameLength
   );

   static
   int 
   CompareRecordProxy(
      __in void* Context,
      __in ULONG A,
      __in ULONG B
   );

   int
   CompareRecord(
      ULONG A,
      ULONG B
   );

   void
   SortRecords(
   );

   static
   void
   ExchangeRecordProxy(
      void* Context,
      __in ULONG A,
      __in ULONG B
   );

   void 
   ExchangeRecord( 
      __in ULONG A,
      __in ULONG B
   );

   BOOLEAN
   IsNameInPrefixList(
      __in_z LPCSTR PrefixList,
      __in_z LPCSTR Name
   );
   
   ULONG
   GetArchiveMemberSize(
      PCIMAGE_ARCHIVE_MEMBER_HEADER ArchiveMember
   );

   PCLIB_MEMBER_TABLE
   GetLibMemberTable(
      __in LPCVOID FileBase
   );

   PCLIB_MEMBER_TABLE_ENTRY
   GetLibMemberEntry(
      __in LPCVOID FileBase,
      __in PCLIB_MEMBER_TABLE MemberTable,
      __in ULONG Index
   );
   
   BOOLEAN
   IsMemberEntryImportObject(
      __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
   );

   PCLIB_SYMBOL_TABLE
   GetLibSymbolTable(
      __in LPCVOID FileBase
   );

   PCIMAGE_SYMBOL
   GetLibMemberEntrySymbolTable(
      __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
   );
      
   PCIMAGE_SYMBOL
   GetNextImageSymbol(
      PCIMAGE_SYMBOL ImageSymbol
   );

   PCIMAGE_SYMBOL
   GetLibMemberEntrySymbolTail(
      __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
   );

   PCIMAGE_AUX_SYMBOL
   GetImageAuxSymbol(
      __in PCIMAGE_SYMBOL ImageSymbol,
      __in BYTE Index
   );

   BOOLEAN
   IsSymbolSectionName(
      PCIMAGE_SYMBOL ImageSymbol
   );

   BOOLEAN
   IsSymbolValueOffset(
      PCIMAGE_SYMBOL ImageSymbol
   );

   BOOLEAN
   IsPossibleGuidSection(
      PCIMAGE_SECTION_HEADER SectionHeader
   );

   PCIMAGE_SYMBOL
   GetSectionSymbolList(
      __in PCLIB_MEMBER_TABLE_ENTRY TableEntry,
      __in WORD SectionNumber
   );

   ULONG
   GetSectionSymbolCount(
      __in PCIMAGE_SYMBOL ImageSymbolList,
      __in PCIMAGE_SYMBOL ImageSymbolTail
   );

   PCIMAGE_SYMBOL_STRING_TABLE
   GetImageSymbolStringInfoTable(
      __in PCLIB_MEMBER_TABLE_ENTRY TableEntry
   );

   LPCSTR
   GetImageSymbolStringInfo(
      __in PCLIB_MEMBER_TABLE_ENTRY TableEntry,
      __in PCIMAGE_SYMBOL ImageSymbol,
      __deref_out size_t* StringLength
   );
};

/*++
   CLibGuidDatabase
      To use this, define GUIDDB_LIB_PATH as the full path to the lib
      directory. Class will look for UUID.LIB and OLE32.LIB

      C_DEFINES=$(C_DEFINES) /DGUIDDB_LIB_PATH="$(SDK_LIB_PATH:\=\\)"
 --*/
class CLibGuidDatabase : public CGuidDatabase
{
public:
   CLibGuidDatabase( 
   ) throw();
};

extern CLibGuidDatabase g_LibGuidDatabase;
