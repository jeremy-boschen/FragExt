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

/* TaskDlg.h
 *    CTaskDlg class and helpers
 *       The CTaskDlg class is for the main dialog of FragMgx,
 *       which is stored as a DIALOGEX resource
 *
 * Copyright (C) 2004-2010 Jeremy Boschen
 */

#pragma once

// a single list common code thunk

  
/*
 * Packing must be the size of the smallest instruction
 */
#if 0
#pragma pack(push, 1)
struct __CallThunkX86
{
   /*
      mov dword ptr [esp+04h], this
      jmp WndProc
    */
   ULONG _MovEsp04h;
   ULONG _This;
   UCHAR _Jmp;
   DWORD _JmpRelativeAddress;

   BOOLEAN
   Initialize(
      __in DWORD_PTR WndProc,
      __in PVOID This
   )
   {
      _MovEsp04h = 0;
      return 0;
   }

   PVOID
   GetThunkAddress(
   )
   {
      return ( this );
   }
};
#pragma pack(pop)
#endif //0

#pragma pack(push, 2)
struct __CallThunkX64
{
    /*
      mov   r11, this
      mov   r10, TargetWndProc
      mov   rax, offset X64WndProcProxy
      jmp   rax
     */
    USHORT  _MovR11;
    ULONG64 _This;
    USHORT  _MovR10;
    ULONG   _TargetWndProc;
    USHORT  _MovRax;
    ULONG64 _X64WndProcProxy;
    USHORT  _JmpRax;
};
#pragma pack(pop)

/*
 * Global thunk list for the application
 */
SLIST_HEADER ThunkList = {0};

typedef struct _THUNK_ENTRY
{
   SLIST_ENTRY Entry;
}THUNK_ENTRY, *PTHUNK_ENTRY;

DWORD
APIENTRY
InitializeThunkPool(
)
{

}

void
APIENTRY
UninitializeThunkPool(
)
{
}

PVOID
APIENTRY
AllocateThunkPoolBlock(
)
{
}




/*++

   Single Instance Window/Dialog Thunks

   These may only be used for windows/dialogs that only ever have 1 instance at
   any point during the lifetime of an application

 --*/

template < class T >
class 
CWindowThunkT
{
protected:
   CWindowThunkT( ) throw()
   {
      CWindowThunkT<T>::_pT = static_cast<T*>(this);
   }

   static
   LRESULT
   CALLBACK 
   WindowProc(
      __in HWND hWnd,
      __in UINT uMsg,
      __in WPARAM wParam,
      __in LPARAM lParam
   )
   {      
      _ASSERTE(NUL != CWindowThunkT<T>::_pT);
      lRet = CWindowThunkT<T>::_pT->HandleWindowMessage(hWnd,
                                                        uMsg,
                                                        wParam,
                                                        lParam);

      return ( lRet );
   }
   
private:
   static T* _pT;
};

template < class T >
class
CDialogThunkT
{
protected:
   CDialogThunkT( ) throw()
   {
      CDialogThunkT<T>::_pT = static_cast<T*>(this);
   }

   static
   INT_PTR
   CALLBACK
   DialogProc(
      __in HWND hDlg,
      __in UINT uMsg,
      __in WPARAM wParam,
      __in LPARAM lParam
   )
   {
      INT_PTR iRet;
      LRESULT lRet;

      _ASSERTE(NULL != CDialogThunkT<T>::_pT);

     //have to pass
      lRet = CDialogThunkT<T>::_pT->HandleWindowMessage(hDlg,
                                                        uMsg,
                                                        wParam,
                                                        lParam);

      switch ( uMsg ) {
         case WM_COMPAREITEM:
            __fallthrough;
         case WM_VKEYTOITEM:
            __fallthrough;
         case WM_CHARTOITEM:
            __fallthrough;
         case WM_INITDIALOG:
            __fallthrough;
         case WM_QUERYDRAGICON:
            __fallthrough;
         case WM_CTLCOLORMSGBOX:
            __fallthrough;
         case WM_CTLCOLOREDIT:
            __fallthrough;
         case WM_CTLCOLORLISTBOX:
            __fallthrough;
         case WM_CTLCOLORBTN:
            __fallthrough;
         case WM_CTLCOLORDLG:
            __fallthrough;
         case WM_CTLCOLORSCROLLBAR:
            __fallthrough;
         case WM_CTLCOLORSTATIC: 
            iRet = (INT_PTR)lRet;
            break;
         default:
            SetWindowLongPtr(hDlg,
                             DWLP_MSGRESULT,
                             lRet);
            break;
      }

      return ( iRet )

private:
   static _T* _pT;
};


#define DECLARE_DLG_CLASSNAME( DlgClassName ) \
   static LPCWSTR GetWndClassName() throw() \
   { \
      return ( DlgClassName ); \
   }

template < class T >
class
CCustomDialogImplT : public CDialogThunkT<T>
{
protected:
   CCustomDialogImplT( ) throw() : CDialogThunkT<T>()
   {
   }
};
   
class 
CTaskDialog : public CCustomDialogImplT<CTaskDialog>
{
public:
};
