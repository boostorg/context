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
 *  -------------------i------------------------------------------  *
 *  |   0x18  |   0x1c  |                                        |  *
 *  --------------------------------------------------------------  *
 *  | fc_mxcsr|fc_x87_cw|                                        |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    8    |    9    |                                        |  *
 *  --------------------------------------------------------------  *
 *  |  0x20   |   0x24  |                                        |  *
 *  --------------------------------------------------------------  *
 *  |  sbase  | slimit  |                                        |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    10   |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |   0x28  |                                                  |  *
 *  --------------------------------------------------------------  *
 *  | fc_link |                                                  |  *
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

    stmxcsr  0x18(%ecx)              /* save SSE2 control and status word */
    fnstcw   0x1c(%ecx)              /* save x87 control word */

    leal    0x4(%esp),  %eax        /* exclude the return address */
    movl    %eax,       0x10(%ecx)  /* save as stack pointer */
    movl    (%esp),     %eax        /* load return address */
    movl    %eax,       0x14(%ecx)  /* save return address */


    movl    0x8(%esp),   %ecx       /* load address of the second fcontext_t arg */
    movl    (%ecx),      %edi       /* restore EDI */
    movl    0x4(%ecx),   %esi       /* restore ESI */
    movl    0x8(%ecx),   %ebx       /* restore EBX */
    movl    0xc(%ecx),   %ebp       /* restore EBP */

    ldmxcsr  0x18(%ecx)             /* restore SSE2 control and status word */
    fldcw    0x1c(%ecx)             /* restore x87 control word */

    movl    0xc(%esp),   %eax       /* use third arg as return value after jump */

    movl    0x10(%ecx),  %esp       /* restore ESP */
    movl    0x14(%ecx),  %ecx       /* fetch the address to return to */

    jmp     *%ecx                   /* indirect jump to context */
.size jump_fcontext,.-jump_fcontext

.text
.globl make_fcontext
.align 2
.type make_fcontext,@function
make_fcontext:
    movl   0x4(%esp),      %eax         /* load address of the fcontext_t */
    movl   %eax,           (%eax)       /* save the address of current context */
    movl   0x8(%esp),      %ecx         /* load the address of the function supposed to run */
    movl   %ecx,           0x14(%eax)   /* save the address of the function supposed to run */
    movl   0x20(%eax),     %edx         /* load the stack base */

    pushl  %eax                         /* save pointer to fcontext_t */
    pushl  %ebx                         /* save EBX */
    pushl  %edx                         /* stack pointer as arg for align_stack */
    call   1f
1:  popl   %ebx                                    /* address of label 1 */
    addl   $_GLOBAL_OFFSET_TABLE_+[.-1b], %ebx     /* compute address of GOT and store it in EBX */
    call   align_stack@PLT     /* align stack */
    movl   %eax,           %edx         /* begin of aligned stack */
    popl   %eax                         /* remove arg for align_stack */
    popl   %ebx                         /* restore EBX */
    popl   %eax                         /* restore pointer to fcontext_t */

    leal   -0x14(%edx),    %edx         /* reserve space for the last frame on stack, (ESP + 4) % 16 == 0 */
    movl   %edx,           0x10(%eax)   /* save the address as stack pointer */
    movl   0x28(%eax),     %ecx         /* load the address of the next context */
    movl   %ecx,           0x4(%eax)    /* save the address of the next context */
    movl   0xc(%esp),      %ecx         /* load the address of the void pointer */
    movl   %ecx,           0x4(%edx)    /* save the address of the void pointer */
    stmxcsr  0x18(%eax)                 /* save SSE2 control and status word */
    fnstcw   0x1c(%eax)                 /* save x87 control word */
    call    2f
2:  popl    %ecx                        /* address of label 1 */
    addl    $link_fcontext-2b, %ecx     /* compute address of label link_fcontext */
    movl    %ecx, (%edx)                /* store address of link_fcontext as return address on stack */

    xorl   %eax,            %eax        /* set EAX to zero */
    ret

link_fcontext:
    leal   -0xc(%esp),    %esp

    call    3f
3:  popl    %ebx                                    /* address of label 3 */
    addl    $_GLOBAL_OFFSET_TABLE_+[.-3b], %ebx     /* compute address of GOT and store it in EBX */

    testl  %esi,           %esi         /* test if a next context was given */
    je     4f                           /* jump to finish */

    pushl  %esi                         /* push the address of the next context on the stack */
    pushl  %edi                         /* push the address of the current context to stack */
    call   start_fcontext@PLT     /* jump to next context */

4:
    movl    %eax,           %eax
    pushl   %eax                        /* exit code is zero */
    call   _exit@PLT                    /* exit application */
    hlt
.size make_fcontext,.-make_fcontext
