
;           Copyright Oliver Kowalke 2009.
;  Distributed under the Boost Software License, Version 1.0.
;     (See accompanying file LICENSE_1_0.txt or copy at
;           http://www.boost.org/LICENSE_1_0.txt)

;  ----------------------------------------------------------------------------------
;  |    0    |    1    |    2    |    3    |    4     |    5    |    6    |    7    |
;  ----------------------------------------------------------------------------------
;  |   0x0   |   0x4   |   0x8   |   0xc   |   0x10   |   0x14  |   0x18  |   0x1c  |
;  ----------------------------------------------------------------------------------
;  |        R12        |         R13       |         R14        |        R15        |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    8    |    9    |   10    |   11    |    12    |    13   |    14   |    15   |
;  ----------------------------------------------------------------------------------
;  |   0x20  |   0x24  |   0x28  |  0x2c   |   0x30   |   0x34  |   0x38  |   0x3c  |
;  ----------------------------------------------------------------------------------
;  |        RDI        |        RSI        |         RBX        |        RBP        |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    16   |    17   |   18   |    19    |                                        |
;  ----------------------------------------------------------------------------------
;  |   0x40  |   0x44  |  0x48  |   0x4c   |                                        |
;  ----------------------------------------------------------------------------------
;  |        RSP        |       RIP         |                                        |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    20   |    21   |                                                            |
;  ----------------------------------------------------------------------------------
;  |   0x50  |   0x54  |                                                            |
;  ----------------------------------------------------------------------------------
;  | fc_mxcsr|fc_x87_cw|                                                            |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    22    |   23   |    24    |    25   |                                       |
;  ----------------------------------------------------------------------------------
;  |   0x58   |  0x5c  |   0x60   |   0x64  |                                       |
;  ----------------------------------------------------------------------------------
;  |       sbase       |       slimit       |                                       |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    26   |    27   |                                                            |
;  ----------------------------------------------------------------------------------
;  |   0x68  |   0x6c  |                                                            |
;  ----------------------------------------------------------------------------------
;  |       fc_link     |                                                            |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    28   |    29   |                                                            |
;  ----------------------------------------------------------------------------------
;  |   0x70  |   0x74  |                                                            |
;  ----------------------------------------------------------------------------------
;  |      fbr_strg     |                                                            |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    30   |    31   |                                                            |
;  ----------------------------------------------------------------------------------
;  |   0x78  |   0x7c  |                                                            |
;  ----------------------------------------------------------------------------------
;  |       fc_fp       |                                                            |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    32    |   33   |   34    |   35    |   36     |   37    |   38    |   39    |
;  ----------------------------------------------------------------------------------
;  |   0x80   |  0x84  |  0x88   |  0x8c   |   0x90   |   0x94  |   0x98  |   0x9c  |
;  ----------------------------------------------------------------------------------
;  |                  XMM6                 |                   XMM7                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    40    |   41   |   42    |   43    |    44    |   45    |   46    |   47    |
;  ----------------------------------------------------------------------------------
;  |  0x100   | 0x104  |  0x108  |  0x10c  |   0x110  |  0x114  |  0x118  |  0x11c  |
;  ----------------------------------------------------------------------------------
;  |                  XMM8                 |                   XMM9                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    48    |   49   |   50    |   51    |    52    |   53    |   54    |   55    |
;  ----------------------------------------------------------------------------------
;  |  0x120   | 0x124  |  0x128  |  0x12c  |   0x130  |  0x134  |  0x138  |  0x13c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM10                 |                  XMM11                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    56    |   57   |   58    |   59    |    60    |   61    |   62    |   63    |
;  ----------------------------------------------------------------------------------
;  |  0x140   | 0x144  |  0x148  |  0x14c  |   0x150  |  0x154  |  0x158  |  0x15c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM12                 |                  XMM13                 |
;  ----------------------------------------------------------------------------------
;  ----------------------------------------------------------------------------------
;  |    64    |   65   |   66    |   67    |    68    |   69    |   70    |   71    |
;  ----------------------------------------------------------------------------------
;  |  0x160   | 0x164  |  0x168  |  0x16c  |   0x170  |  0x174  |  0x178  |  0x17c  |
;  ----------------------------------------------------------------------------------
;  |                 XMM14                 |                  XMM15                 |
;  ----------------------------------------------------------------------------------

EXTERN  _exit:PROC            ; standard C library function
EXTERN  align_stack:PROC      ; stack alignment
EXTERN  seh_fcontext:PROC     ; exception handler
EXTERN  start_fcontext:PROC   ; start fcontext
.code

jump_fcontext PROC EXPORT FRAME:seh_fcontext
    .endprolog

    mov     [rcx],       r12        ; save R12
    mov     [rcx+08h],   r13        ; save R13
    mov     [rcx+010h],  r14        ; save R14
    mov     [rcx+018h],  r15        ; save R15
    mov     [rcx+020h],  rdi        ; save RDI
    mov     [rcx+028h],  rsi        ; save RSI
    mov     [rcx+030h],  rbx        ; save RBX
    mov     [rcx+038h],  rbp        ; save RBP

    mov     r9,          gs:[030h]  ; load NT_TIB
    mov     rax,         [r9+08h]   ; load current stack base
    mov     [rcx+058h],  rax        ; save current stack base
    mov     rax,         [r9+010h]  ; load current stack limit
    mov     [rcx+060h],  rax        ; save current stack limit
    mov     rax,         [r9+018h]  ; load fiber local storage
    mov     [rcx+070h],  rax        ; save fiber local storage

    lea     rax,         [rsp+08h]  ; exclude the return address
    mov     [rcx+040h],  rax        ; save as stack pointer
    mov     rax,         [rsp]      ; load return address
    mov     [rcx+048h],  rax        ; save return address


    mov     r12,        [rdx]       ; restore R12
    mov     r13,        [rdx+08h]   ; restore R13
    mov     r14,        [rdx+010h]  ; restore R14
    mov     r15,        [rdx+018h]  ; restore R15
    mov     rdi,        [rdx+020h]  ; restore RDI
    mov     rsi,        [rdx+028h]  ; restore RSI
    mov     rbx,        [rdx+030h]  ; restore RBX
    mov     rbp,        [rdx+038h]  ; restore RBP

;    stmxcsr [rcx+050h]              ; save SSE2 control and status word
;    fnstcw  [rcx+054h]              ; save x87 control word

    movaps  [rcx+080h],  xmm6
    movaps  [rcx+090h],  xmm7
    movaps  [rcx+0100h], xmm8
    movaps  [rcx+0110h], xmm9
    movaps  [rcx+0120h], xmm10
    movaps  [rcx+0130h], xmm11
    movaps  [rcx+0140h], xmm12
    movaps  [rcx+0150h], xmm13
    movaps  [rcx+0160h], xmm14
    movaps  [rcx+0170h], xmm15

;    ldmxcsr  [rdx+050h]             ; restore SSE2 control and status word
;    fldcw    [rdx+054h]             ; restore x87 control word

    movaps  xmm6,  [rdx+080h]
    movaps  xmm7,  [rdx+090h]
    movaps  xmm8,  [rdx+0100h]
    movaps  xmm9,  [rdx+0110h]
    movaps  xmm10, [rdx+0120h]
    movaps  xmm11, [rdx+0130h]
    movaps  xmm12, [rdx+0140h]
    movaps  xmm13, [rdx+0150h]
    movaps  xmm14, [rdx+0160h]
    movaps  xmm15, [rdx+0170h]

    mov     r9,         gs:[030h]   ; load NT_TIB
    mov     rax,        [rdx+058h]  ; load stack base
    mov     [r9+08h],   rax         ; restore stack base
    mov     rax,        [rdx+060h]  ; load stack limit
    mov     [r9+010h],  rax         ; restore stack limit
    mov     rax,        [rdx+070h]  ; load fiber local storage
    mov     [r9+018h],  rax         ; restore fiber local storage

    mov     rsp,        [rdx+040h]  ; restore RSP
    mov     r9,         [rdx+048h]  ; fetch the address to returned to
    mov     rcx,        r14         ; restore RCX as first argument for called context

    mov     rax,        r8          ; use third arg as return value after jump

    jmp     r9                      ; indirect jump to caller
jump_fcontext ENDP

make_fcontext PROC EXPORT FRAME ; generate function table entry in .pdata and unwind information in    E
    .endprolog                        ; .xdata for a function's structured exception handling unwind behavior

    mov  [rcx],      rcx         ; store the address of current context
    mov  [rcx+048h], rdx         ; save the address of the function supposed to run
    mov  [rcx+010h], r8          ; save the the void pointer
    mov  rdx,        [rcx+058h]  ; load the address where the context stack beginns

    push  rcx                    ; save pointer to fcontext_t
    sub   rsp,       028h        ; reserve shadow space for boost_fcontext_algin
    mov   rcx,       rdx         ; stack pointer as arg for align_stack
    mov   [rsp+8],   rcx
    call  align_stack   ; align stack
    mov   rdx,       rax         ; begin of aligned stack
    add   rsp,       028h
    pop   rcx                    ; restore pointer to fcontext_t

    lea  rdx,        [rdx-028h]  ; reserve 32byte shadow space + return address on stack, (RSP + 8) % 16 == 0
    mov  [rcx+040h], rdx         ; save the address where the context stack beginns

    mov  rax,       [rcx+068h]   ; load the address of the next context
    mov  [rcx+08h], rax          ; save the address of next context
    stmxcsr [rcx+050h]           ; save SSE2 control and status word
    fnstcw  [rcx+054h]           ; save x87 control word

    lea  rax,       link_fcontext   ; helper code executed after fn() returns
    mov  [rdx],     rax          ; store address off the helper function as return address

    xor  rax,       rax          ; set RAX to zero
    ret
make_fcontext ENDP

link_fcontext PROC FRAME   ; generate function table entry in .pdata and unwind information in
    .endprolog                   ; .xdata for a function's structured exception handling unwind behavior

    sub   rsp,      028h         ; reserve shadow space for boost_fcontext_algin
    test  r13,      r13          ; test if a next context was given
    je    finish                 ; jump to finish

    mov   rcx,      r12          ; first argument eq. address of current context
    mov   rdx,      r13          ; second argumnet eq. address of next context
    mov   [rsp+010h], rdx
    mov   [rsp+08h],  rcx
    call  start_fcontext   ; install next context

finish:
    xor   rcx,        rcx
    mov   [rsp+08h],  rcx
    call  _exit                  ; exit application
    hlt
link_fcontext ENDP
END
