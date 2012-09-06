/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/********************************************************************
 *                                                                  *
 *  --------------------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4     |    5    |  *
 *  --------------------------------------------------------------  *
 *  |   0x0   |   0x4   |   0x8   |   0xc   |   0x10   |   0x14  |  *
 *  --------------------------------------------------------------  *
 *  |   EDI   |   ESI   |   EBX   |   EBP   |   ESP    |   EIP   |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    6    |    7    |                                        |  *
 *  --------------------------------------------------------------  *
 *  |   0x18  |   0x1c  |                                        |  *
 *  --------------------------------------------------------------  *
 *  |    sp   |  size   |                                        |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    8    |    9    |                                        |  *
 *  --------------------------------------------------------------  *
 *  |   0x20  |   0x24  |                                        |  *
 *  --------------------------------------------------------------  *
 *  | fc_mxcsr|fc_x87_cw|                                        |  *
 *  --------------------------------------------------------------  *
 *                                                                  *
 * *****************************************************************/

.text
.globl jump_fcontext
.align 2
.type jump_fcontext,@function
jump_fcontext:
    movl    0x4(%esp), %ecx         /* load address of the first fcontext_t arg */
    movl    %edi,       (%ecx)      /* save EDI */
    movl    %esi,       0x4(%ecx)   /* save ESI */
    movl    %ebx,       0x8(%ecx)   /* save EBX */
    movl    %ebp,       0xc(%ecx)   /* save EBP */

    leal    0x4(%esp),  %eax        /* exclude the return address */
    movl    %eax,       0x10(%ecx)  /* save as stack pointer */
    movl    (%esp),     %eax        /* load return address */
    movl    %eax,       0x14(%ecx)  /* save return address */

    movl    0x8(%esp),   %edx       /* load address of the second fcontext_t arg */
    movl    (%edx),      %edi       /* restore EDI */
    movl    0x4(%edx),   %esi       /* restore ESI */
    movl    0x8(%edx),   %ebx       /* restore EBX */
    movl    0xc(%edx),   %ebp       /* restore EBP */

    movl    0x10(%esp),  %eax       /* check if fpu enve preserving was requested */
    test    %eax,        %eax
    je      1f

    stmxcsr  0x20(%ecx)             /* save MMX control and status word */
    fnstcw   0x24(%ecx)             /* save x87 control word */
    ldmxcsr  0x20(%edx)             /* restore MMX control and status word */
    fldcw    0x24(%edx)             /* restore x87 control word */
1:
    movl    0xc(%esp),   %eax       /* use third arg as return value after jump */

    movl    0x10(%edx),  %esp       /* restore ESP */
    movl    %eax,        0x4(%esp)  /* use third arg as first arg in context function */
    movl    0x14(%edx),  %edx       /* fetch the address to return to */

    jmp     *%edx                   /* indirect jump to context */
.size jump_fcontext,.-jump_fcontext

.text
.globl make_fcontext
.align 2
.type make_fcontext,@function
make_fcontext:
    pushl  %ebp                         /* save previous frame pointer; get the stack 16 byte aligned */
    movl   %esp,           %ebp         /* set EBP to ESP */
    subl   $0x10,          %esp         /* allocate stack space */

    movl   %ebx,           0x4(%esp)    /* save EBX */
    movl   0x8(%ebp),      %eax         /* load 1. arg of make_fcontext, pointer to context stack (base) */
    leal   -0x28(%eax),    %eax         /* reserve space for fcontext_t at top of context stack */
    movl   %eax,           (%esp)       /* address in EAX becomes 1. arg of align_stack */

    call   1f
1:  popl   %ebx                                    /* address of label 1 */
    addl   $_GLOBAL_OFFSET_TABLE_+[.-1b], %ebx     /* compute address of GOT and store it in EBX */
    call   align_stack@PLT                         /* call align_stack, EAX contains address at 16 byte boundary after return */
                                                   /* == pointer to fcontext_t and address of context stack */
    movl   0x4(%esp),      %ebx                    /* restore EBX */

    movl   0x8(%ebp),      %edx         /* load 1. arg of make_fcontext, pointer to context stack (base) */
    movl   %edx,           0x18(%eax)   /* save address of context stack (base) in fcontext_t */
    movl   0xc(%ebp),      %edx         /* load 2. arg of make_fcontext, context stack size */
    movl   %edx,           0x1c(%eax)   /* save stack size in fcontext_t */
    movl   0x10(%ebp),     %edx         /* load 3. arg of make_fcontext, pointer to context function */
    movl   %edx,           0x14(%eax)   /* save address of context function in fcontext_t */

    stmxcsr  0x20(%eax)                 /* save MMX control and status word */
    fnstcw   0x24(%eax)                 /* save x87 control word */

    leal   -0xc(%eax),     %edx         /* reserve space for the last frame on context stack; (ESP - 0x4) % 16 == 0 */
    movl   %edx,           0x10(%eax)   /* save address in EDX as stack pointer for context function */

    call   2f
2:  popl   %ecx                        /* address of label 2 */
    addl   $finish-2b, %ecx            /* compute abs address of label finish */
    movl   %ecx, (%edx)                /* save address of finish as return address for context functions */
                                       /* entered after context function returns */

    addl   $0x10,          %esp        /* deallocate stack space */
    pop    %ebp

    ret

finish:
    /* ESP points to same address as ESP on entry of context function + 0x4 */
    call    3f
3:  popl    %ebx                                    /* address of label 3 */
    addl    $_GLOBAL_OFFSET_TABLE_+[.-3b], %ebx     /* compute address of GOT and store it in EBX */

    xorl    %eax,  %eax
    movl    %eax,  (%esp)               /* exit code is zero */
    call   _exit@PLT                    /* exit application */
    hlt
.size make_fcontext,.-make_fcontext
