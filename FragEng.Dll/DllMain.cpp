
#include "Stdafx.h"

extern "C" 
BOOL 
WINAPI 
DllMain( 
   __in HINSTANCE /*hInstance*/, 
   __in DWORD dwReason, 
   __in LPVOID /*lpReserved*/ )
{
   switch ( dwReason )
   {
      case DLL_PROCESS_DETACH:
         _CrtDumpMemoryLeaks();
         break;
   }

   return ( TRUE );
}
