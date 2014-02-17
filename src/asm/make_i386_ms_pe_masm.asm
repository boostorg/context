
;           Copyright Oliver Kowalke 2009.
;  Distributed under the Boost Software License, Version 1.0.
;     (See accompanying file LICENSE_1_0.txt or copy at
;           http://www.boost.org/LICENSE_1_0.txt)

;  -------------------------------------------------------------
;  |    0    |    1    |    2    |    3    |    4    |    5    |
;  -------------------------------------------------------------
;  |    0h   |   04h   |   08h   |   0ch   |   010h  |   014h  |
;  -------------------------------------------------------------
;  | fc_mxcsr|fc_x87_cw|fc_deallo|fc_strage|fc_execpt|  limit  |
;  -------------------------------------------------------------
;  -------------------------------------------------------------
;  |    6    |    7    |    8    |    9    |   10    |   11    |
;  -------------------------------------------------------------
;  |   018h  |   01ch  |   020h  |  024h   |  028h   |  02ch   |
;  -------------------------------------------------------------
;  |   EDI   |   ESI   |   EBX   |   EBP   |   ESP   |   EIP   |
;  -------------------------------------------------------------

.386
.XMM
.model flat, c
_exit PROTO, value:SDWORD
.code

make_fcontext PROC EXPORT
    ; first arg of make_fcontext() == top of context-stack
    mov  eax, [esp+04h]

    ; shift address in EAX to lower 16 byte boundary
    and  eax, -16

    ; reserve space for context-data on context-stack
    ; size for fc_mxcsr .. EIP + return-address for context-function
    lea  eax, [eax-034h]

    ; second arg of make_fcontext() == size of context-stack
    mov  edx,         [esp+08h]     ; load 2. arg of make_fcontext, context stack size
    neg  edx                        ; negate stack size for LEA instruction (== substraction)
    lea  ecx,         [ecx+edx]     ; compute bottom address of context stack (limit)
    mov  [eax+020h],  ecx           ; save address of context stack (limit) in fcontext_t
    mov  [eax+034h],  ecx           ; save address of context stack limit as 'dealloction stack'
    ; third arg of make_fcontext() == address of context-function
    mov  ecx, [esp+0ch]
    mov  [eax+02ch], ecx

    stmxcsr [eax+02ch]              ; save MMX control word
    fnstcw  [eax+030h]              ; save x87 control word

    lea  edx,         [eax-024h]    ; reserve space for last frame and seh on context stack, (ESP - 0x4) % 16 == 0
    mov  [eax+010h],  edx           ; save address in EDX as stack pointer for context function

    mov  ecx,         finish        ; abs address of finish
    mov  [edx],       ecx           ; save address of finish as return address for context function
                                    ; entered after context function returns

    ; traverse current seh chain to get the last exception handler installed by Windows
    ; note that on Windows Server 2008 and 2008 R2, SEHOP is activated by default
    ; the exception handler chain is tested for the presence of ntdll.dll!FinalExceptionHandler
    ; at its end by RaiseException all seh andlers are disregarded if not present and the
    ; program is aborted
    assume  fs:nothing
    mov     ecx,      fs:[018h]     ; load NT_TIB into ECX
    assume  fs:error

walk:
    mov  edx,         [ecx]         ; load 'next' member of current SEH into EDX
    inc  edx                        ; test if 'next' of current SEH is last (== 0xffffffff)
    jz   found
    dec  edx
    xchg edx,         ecx           ; exchange content; ECX contains address of next SEH
    jmp  walk                       ; inspect next SEH

found:
    mov  ecx,         [ecx+04h]     ; load 'handler' member of SEH == address of last SEH handler installed by Windows
    mov  edx,         [eax+010h]    ; load address of stack pointer for context function
    mov  [edx+018h],  ecx           ; save address in ECX as SEH handler for context
    mov  ecx,         0ffffffffh    ; set ECX to -1
    mov  [edx+014h],  ecx           ; save ECX as next SEH item
    lea  ecx,         [edx+014h]    ; load address of next SEH item
    mov  [eax+024h],  ecx           ; save next SEH

    ret

finish:
    ; ESP points to same address as ESP on entry of context function + 0x4
    xor   eax,        eax
    mov   [esp],      eax           ; exit code is zero
    call  _exit                     ; exit application
    hlt
make_fcontext ENDP
END
