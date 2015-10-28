/*
            Copyright Oliver Kowalke 2009.
            Copyright Thomas Sailer 2013.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

/****************************************************************************************
 *                                                                                      *
 *  ----------------------------------------------------------------------------------  *
 *  |     0   |     1   |     2    |     3   |     4   |     5   |     6   |     7   |  *
 *  ----------------------------------------------------------------------------------  *
 *  |    0x0  |    0x4  |    0x8   |    0xc  |   0x10  |   0x14  |   0x18  |   0x1c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  |      fbr_strg     |      fc_dealloc    |       limit       |        base       |  *
 *  ----------------------------------------------------------------------------------  *
 *  ----------------------------------------------------------------------------------  *
 *  |     8   |    9    |    10    |    11   |    12   |    13   |    14   |    15   |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0xc20 |  0x24   |   0x28   |   0x2c  |   0x30  |   0x34  |   0x38  |   0x3c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  |        R12        |         R13        |        R14        |        R15        |  *
 *  ----------------------------------------------------------------------------------  *
 *  ----------------------------------------------------------------------------------  *
 *  |    16   |    17   |    18   |    19    |    20   |    21   |    22   |    23   |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0xe40  |   0x44 |   0x48  |   0x4c   |   0x50  |   0x54  |   0x58  |   0x5c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  |        RDI         |       RSI         |        RBX        |        RBP        |  *
 *  ----------------------------------------------------------------------------------  *
 *  ----------------------------------------------------------------------------------  *
 *  |    24   |   25    |    26    |   27    |    28   |    29   |    30   |    31   |  *
 *  ----------------------------------------------------------------------------------  *
 *  |   0x60  |   0x64  |   0x68   |   0x6c  |   0x70  |   0x74  |   0x78  |   0x7c  |  *
 *  ----------------------------------------------------------------------------------  *
 *  |        RIP        |        EXIT        |                                       |  *
 *  ----------------------------------------------------------------------------------  *
 *                                                                                      *
 * *************************************************************************************/

.file	"jump_x86_64_ms_pe_gas.asm"
.text
.p2align 4,,15
.globl	jump_fcontext
.def	jump_fcontext;	.scl	2;	.type	32;	.endef
.seh_proc	jump_fcontext
jump_fcontext:
.seh_endprologue

    pushq  %rbp  /* save RBP */
    pushq  %rbx  /* save RBX */
    pushq  %rsi  /* save RSI */
    pushq  %rdi  /* save RDI */
    pushq  %r15  /* save R15 */
    pushq  %r14  /* save R14 */
    pushq  %r13  /* save R13 */
    pushq  %r12  /* save R12 */

    /* load NT_TIB */
    movq  %gs:(0x30), %r10
    /* save current stack base */
    movq  0x08(%r10), %rax
    pushq  %rax
    /* save current stack limit */
    movq  0x10(%r10), %rax
    pushq  %rax
    /* save current deallocation stack */
    movq  0x1478(%r10), %rax
    pushq  %rax
    /* save fiber local storage */
    movq  0x18(%r10), %rax
    pushq  %rax

    /* store RSP (pointing to context-data) in RCX */
    movq  %rsp, (%rcx)

    /* restore RSP (pointing to context-data) from RDX */
    movq  %rdx, %rsp

    /* load NT_TIB */
    movq  %gs:(0x30), %r10
    /* restore fiber local storage */
    popq  %rax
    movq  %rax, 0x18(%r10)
    /* restore deallocation stack */
    popq  %rax
    movq  %rax, 0x1478(%r10)
    /* restore stack limit */
    popq  %rax
    movq  %rax, 0x10(%r10)
    /* restore stack base */
    popq  %rax
    movq  %rax, 0x8(%r10)

    popq  %r12  /* restore R12 */
    popq  %r13  /* restore R13 */
    popq  %r14  /* restore R14 */
    popq  %r15  /* restore R15 */
    popq  %rdi  /* restore RDI */
    popq  %rsi  /* restore RSI */
    popq  %rbx  /* restore RBX */
    popq  %rbp  /* restore RBP */

    /* restore return-address */
    popq  %r10

    /* use third arg as return-value after jump */
    movq  %r8, %rax
    /* use third arg as first arg in context function */
    movq  %r8, %rcx

    /* indirect jump to context */
    jmp  *%r10
.seh_endproc
