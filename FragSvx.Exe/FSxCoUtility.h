__inline
HRESULT
FSxManagerQueryService(
   __in REFGUID guidService,
   __in REFIID riid,
   __deref_out void** ppService
)
{
   HRESULT            hr;
   IServiceProvider*  pService;
#ifndef _WIN64
   OSVERSIONINFO      VersionInfo;
   DWORD              dwClsContext;

   ZeroMemory(&VersionInfo,
              sizeof(VersionInfo));

   VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);

   GetVersionEx(&VersionInfo);

   dwClsContext = CLSCTX_LOCAL_SERVER|((VersionInfo.dwMajorVersion > 5) || ((VersionInfo.dwMajorVersion == 5) && (VersionInfo.dwMinorVersion >= 1)) ? CLSCTX_DISABLE_AAA : 0);
#endif /* _WIN64 */

   hr = CoCreateInstance(__uuidof(FSxServiceManager),
                         NULL,
#ifdef _WIN64
                         CLSCTX_LOCAL_SERVER|CLSCTX_DISABLE_AAA|CLSCTX_ACTIVATE_64_BIT_SERVER,
#else /* _WIN64 */
                         dwClsContext,
#endif /* _WIN64 */
                         __uuidof(IServiceProvider),
                         (void**)&pService);

   if ( SUCCEEDED(hr) )
   {
      hr = pService->QueryService(guidService,
                                  riid,
                                  ppService);

      pService->Release();
   }

   /* Success / Failure */
   return ( hr );
}