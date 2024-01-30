;
; This module implements all assembler code
;
.686p
.model flat, c
.MMX
.XMM

; Declare the external functions and global variables
EXTERN g_pfnSys_Error: PTR VOID
EXTERN MH_SysError: PROC

MH_SysErrorWrapper proto
.SAFESEH MH_SysErrorWrapper

.CODE

MH_SysErrorWrapper PROC

    ; Check if g_pfnSys_Error is NULL
    mov eax, dword ptr [g_pfnSys_Error] ; Load the value of g_pfnSysError
    test eax, eax           ; Test if eax is zero (NULL)
    jz fallback             ; If zero, jump to fallback code

    ; Call the function pointed to by g_pfnSysError
    ; Since we're using __cdecl, the arguments are already on the stack
    jmp eax                ; Call the function pointer

fallback:
    ; Call the default function MH_SysError
    ; The arguments are already on the stack, so just call the function
    jmp MH_SysError

MH_SysErrorWrapper ENDP

END
