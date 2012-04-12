
;           Copyright Oliver Kowalke 2009.
;  Distributed under the Boost Software License, Version 1.0.
;     (See accompanying file LICENSE_1_0.txt or copy at
;           http://www.boost.org/LICENSE_1_0.txt)

;  --------------------------------------------------------------
;  |    0    |    1    |    2    |    3    |    4     |    5    |
;  --------------------------------------------------------------
;  |    0h   |   04h   |   08h   |   0ch   |   010h   |   014h  |
;  --------------------------------------------------------------
;  |   EDI   |   ESI   |   EBX   |   EBP   |   ESP    |   EIP   |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |    6    |    7    |                                        |
;  --------------------------------------------------------------
;  |   018h  |   01ch  |                                        |
;  --------------------------------------------------------------
;  | fc_mxcsr|fc_x87_cw|                                        |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |    8    |    9    |                                        |
;  --------------------------------------------------------------
;  |  020h   |  024h   |                                        |
;  --------------------------------------------------------------
;  | ss_base | ss_limit|                                        |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |   10    |                                                  |
;  --------------------------------------------------------------
;  |  028h   |                                                  |
;  --------------------------------------------------------------
;  | fc_link |                                                  |
;  --------------------------------------------------------------
;  --------------------------------------------------------------
;  |    12   |                                                  |
;  --------------------------------------------------------------
;  |   030h  |                                                  |
;  --------------------------------------------------------------
;  |fc_strge |                                                  |
;  --------------------------------------------------------------

.386
.XMM
.model flat, c
_exit PROTO, value:SDWORD 
align_stack PROTO, vp:DWORD
seh_fcontext PROTO, except:DWORD, frame:DWORD, context:DWORD, dispatch:DWORD
start_fcontext PROTO, from:DWORD, to:DWORD, vp:DWORD
.code

jump_fcontext PROC EXPORT
    mov     ecx,         [esp+04h]  ; load address of the first fcontext_t arg
    mov     [ecx],       edi        ; save EDI
    mov     [ecx+04h],   esi        ; save ESI
    mov     [ecx+08h],   ebx        ; save EBX
    mov     [ecx+0ch],   ebp        ; save EBP

    assume  fs:nothing
    mov     edx,         fs:[018h]  ; load NT_TIB
    assume  fs:error
    mov     eax,         [edx]      ; load current SEH exception list
    mov     [ecx+02ch],  eax        ; save current exception list
    mov     eax,         [edx+04h]  ; load current stack base
    mov     [ecx+020h],  eax        ; save current stack base
    mov     eax,         [edx+08h]  ; load current stack limit
    mov     [ecx+024h],  eax        ; save current stack limit
    mov     eax,         [edx+010h] ; load fiber local storage
    mov     [ecx+030h],  eax        ; save fiber local storage

    stmxcsr [ecx+018h]              ; save SSE2 control word
    fnstcw  [ecx+01ch]              ; save x87 control word

    lea     eax,         [esp+04h]  ; exclude the return address
    mov     [ecx+010h],  eax        ; save as stack pointer
    mov     eax,         [esp]      ; load return address
    mov     [ecx+014h],  eax        ; save return address


    mov     ecx,        [esp+08h]   ; load address of the second fcontext_t arg
    mov     edi,        [ecx]       ; restore EDI
    mov     esi,        [ecx+04h]   ; restore ESI
    mov     ebx,        [ecx+08h]   ; restore EBX
    mov     ebp,        [ecx+0ch]   ; restore EBP

    assume  fs:nothing
    mov     edx,        fs:[018h]   ; load NT_TIB
    assume  fs:error
    mov     eax,        [ecx+02ch]  ; load SEH exception list
    mov     [edx],      eax         ; restore next SEH item
    mov     eax,        [ecx+020h]  ; load stack base
    mov     [edx+04h],  eax         ; restore stack base
    mov     eax,        [ecx+024h]  ; load stack limit
    mov     [edx+08h],  eax         ; restore stack limit
    mov     eax,        [ecx+030h]  ; load fiber local storage
    mov     [edx+010h], eax         ; restore fiber local storage

    ldmxcsr [ecx+018h]              ; restore SSE2 control word
    fldcw   [ecx+01ch]              ; restore x87 control word

    mov     eax,        [esp+0ch]   ; use third arg as return value after jump

    mov     esp,        [ecx+010h]  ; restore ESP
    mov     ecx,        [ecx+014h]  ; fetch the address to return to

    jmp     ecx                     ; indirect jump to context
jump_fcontext ENDP

make_fcontext PROC EXPORT
    mov  eax,         [esp+04h]     ; load address of the fcontext_t arg0
    mov  [eax],       eax           ; save the address of current context
    mov  ecx,         [esp+08h]     ; load the address of the function supposed to run
    mov  [eax+014h],  ecx           ; save the address of the function supposed to run
    mov  edx,         [eax+020h]    ; load the stack base

    push  eax                       ; save pointer to fcontext_t
    push  edx                       ; stack pointer as arg for align_stack
    call  align_stack      ; align stack
    mov   edx,        eax           ; begin of aligned stack
    pop   eax                       ; remove arg for align_stack
    pop   eax                       ; restore pointer to fcontext_t

    lea  edx,         [edx-014h]    ; reserve space for last frame on stack, (ESP + 4) % 16 == 0
    mov  [eax+010h],  edx           ; save the address

    mov  ecx,         seh_fcontext    ; set ECX to exception-handler
    mov  [edx+0ch],   ecx               ; save ECX as SEH handler
    mov  ecx,         0ffffffffh        ; set ECX to -1
    mov  [edx+08h],   ecx               ; save ECX as next SEH item
    lea  ecx,         [edx+08h]         ; load address of next SEH item
    mov  [eax+02ch],  ecx               ; save next SEH

    mov  ecx,         [eax+028h]    ; load the address of the next context
    mov  [eax+04h],   ecx           ; save the address of the next context
    mov  ecx,         [esp+0ch]     ; load the address of the void pointer arg2
    mov  [edx+04h],   ecx           ; save the address of the void pointer onto the context stack
    stmxcsr [eax+018h]              ; save SSE2 control word
    fnstcw  [eax+01ch]              ; save x87 control word
    mov  ecx,         link_fcontext ; load helper code executed after fn() returns
    mov  [edx],       ecx           ; save helper code executed adter fn() returns
    xor  eax,         eax           ; set EAX to zero
    ret
make_fcontext ENDP

link_fcontext PROC
    lea   esp,        [esp-0ch]     ; adjust the stack to proper boundaries
    test  esi,        esi           ; test if a next context was given
    je    finish                    ; jump to finish

    push  esi                       ; push the address of the next context on the stack
    push  edi                       ; push the address of the current context on the stack
    call  start_fcontext      ; install next context

finish:
    xor   eax,        eax           ; set EAX to zero
    push  eax                       ; exit code is zero
    call  _exit                     ; exit application
    hlt
link_fcontext ENDP
END
