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
 *  |   EDI   |   ESI   |   EBX   |   EBP   |   EIP    |   EXIT  |  *
 *  --------------------------------------------------------------  *
 *                                                                  *
 ********************************************************************/

.text
.globl _jump_fcontext
.align 2
_jump_fcontext:
    pushl  %ebp  /* save EBP */
    pushl  %ebx  /* save EBX */
    pushl  %esi  /* save ESI */
    pushl  %edi  /* save EDI */

    /* first arg of jump_fcontext() == context jumping from */
    movl  0x14(%esp), %eax

    /* store ESP (pointing to context-data) in EAX */
    movl  %esp, (%eax)

    /* second arg of jump_fcontext() == context jumping to */
    movl  0x18(%esp), %edx

    /* third arg of jump_fcontext() == value to be returned after jump */
    movl  0x1c(%esp), %eax

    /* restore ESP (pointing to context-data) from EDX */
    movl  %edx, %esp

    popl  %edi  /* restore EDI */
    popl  %esi  /* restore ESI */
    popl  %ebx  /* restore EBX */
    popl  %ebp  /* restore EBP */

    /* restore return-address */
    popl  %edx

    /* use value in EAX as return-value after jump */
    /* use value in EAX as first arg in context function */
    movl  %eax, 0x4(%esp)

    /* indirect jump to context */
    jmp  *%edx
