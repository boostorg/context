/*
            Copyright Oliver Kowalke 2009.
            Copyright Thomas Sailer 2013.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

/********************************************************************
 *                                                                  *
 *  --------------------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4     |    5    |  *
 *  --------------------------------------------------------------  *
 *  |    0h   |   04h   |   08h   |   0ch   |   010h   |   014h  |  *
 *  --------------------------------------------------------------  *
 *  |   EDI   |   ESI   |   EBX   |   EBP   |   ESP    |   EIP   |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    6    |    7    |    8    |                              |  *
 *  --------------------------------------------------------------  *
 *  |   018h  |   01ch  |   020h  |                              |  *
 *  --------------------------------------------------------------  *
 *  |    sp   |   size  |  limit  |                              |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    9    |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |  024h   |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |fc_execpt|                                                  |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |   10    |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |  028h   |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |fc_strage|                                                  |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |   11    |    12   |                                        |  *
 *  --------------------------------------------------------------  *
 *  |  02ch   |   030h  |                                        |  *
 *  --------------------------------------------------------------  *
 *  | fc_mxcsr|fc_x87_cw|                                        |  *
 *  --------------------------------------------------------------  *
 *                                                                  *
 * *****************************************************************/

.file	"jump_i386_ms_pe_gas.S"
.text
.p2align 4,,15
.globl	_jump_fcontext
.def	_jump_fcontext;	.scl	2;	.type	32;	.endef
_jump_fcontext:
    movl    0x04(%esp), %ecx        /* load address of the first fcontext_t arg */
    movl    %edi,       (%ecx)      /* save EDI */
    movl    %esi,       0x04(%ecx)  /* save ESI */
    movl    %ebx,       0x08(%ecx)  /* save EBX */
    movl    %ebp,       0x0c(%ecx)  /* save EBP */

    movl    %fs:(0x18), %edx        /* load NT_TIB */
    movl    (%edx),     %eax        /* load current SEH exception list */
    movl    %eax,       0x24(%ecx)  /* save current exception list */
    movl    0x04(%edx), %eax        /* load current stack base */
    movl    %eax,       0x18(%ecx)  /* save current stack base */
    movl    0x08(%edx), %eax        /* load current stack limit */
    movl    %eax,       0x20(%ecx)  /* save current stack limit */
    movl    0x10(%edx), %eax        /* load fiber local storage */
    movl    %eax,       0x28(%ecx)  /* save fiber local storage */

    leal    0x04(%esp), %eax        /* exclude the return address */
    movl    %eax,       0x10(%ecx)  /* save as stack pointer */
    movl    (%esp),     %eax        /* load return address */
    movl    %eax,       0x14(%ecx)  /* save return address */

    movl    0x08(%esp), %edx        /* load address of the second fcontext_t arg */
    movl    (%edx),     %edi        /* restore EDI */
    movl    0x04(%edx), %esi        /* restore ESI */
    movl    0x08(%edx), %ebx        /* restore EBX */
    movl    0x0c(%edx), %ebp        /* restore EBP */

    movl    0x10(%esp), %eax        /* check if fpu enve preserving was requested */
    testl   %eax,       %eax 
    je      1f

    stmxcsr 0x2c(%ecx)              /* save MMX control word */
    fnstcw  0x30(%ecx)              /* save x87 control word */
    ldmxcsr 0x2c(%edx)              /* restore MMX control word */
    fldcw   0x30(%edx)              /* restore x87 control word */
1:
    movl    %edx,       %ecx        
    movl    %fs:(0x18), %edx        /* load NT_TIB */
    movl    0x24(%ecx), %eax        /* load SEH exception list */
    movl    %eax,       (%edx)      /* restore next SEH item */
    movl    0x18(%ecx), %eax        /* load stack base */
    movl    %eax,       0x04(%edx)  /* restore stack base */
    movl    0x20(%ecx), %eax        /* load stack limit */
    movl    %eax,       0x08(%edx)  /* restore stack limit */
    movl    0x28(%ecx), %eax        /* load fiber local storage */
    movl    %eax,       0x10(%edx)  /* restore fiber local storage */
			            
    movl    0x0c(%esp), %eax        /* use third arg as return value after jump */
			            
    movl    0x10(%ecx), %esp        /* restore ESP */
    movl    %eax,       0x04(%esp)  /* use third arg as first arg in context function */
    movl    0x14(%ecx), %ecx        /* fetch the address to return to */

    jmp     *%ecx                   /* indirect jump to context */
