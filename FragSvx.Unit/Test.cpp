

extern "C" {
   int __stdcall WndProcMember(void* This, void* hWnd, unsigned int uMsg, unsigned __int64 wParam, unsigned __int64 lParam );
};

extern "C"
int
__stdcall
WndProcProxy(
   void* hWnd,
   unsigned int uMsg,
   unsigned __int64 wParam,
   unsigned __int64 lParam
)
{
   return WndProcMember((void*)0x11223344,
                        hWnd,
                        uMsg,
                        wParam,
                        lParam);
}
