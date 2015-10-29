/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/*******************************************************
 *                                                     *
 *  -------------------------------------------------  *
 *  |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  *
 *  -------------------------------------------------  *
 *  | 0x0 | 0x4 | 0x8 | 0xc | 0x10| 0x14| 0x18| 0x1c|  *
 *  -------------------------------------------------  *
 *  |  v1 |  v2 |  v3 |  v4 |  v5 |  v6 |  v7 |  v8 |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |  *
 *  -------------------------------------------------  *
 *  | 0x20| 0x24| 0x28| 0x2c| 0x30| 0x34| 0x38| 0x3c|  *
 *  -------------------------------------------------  *
 *  |  lr |  pc |                                   |  *
 *  -------------------------------------------------  *
 *                                                     *
 *******************************************************/

.text
.globl jump_fcontext
.align 2
.type jump_fcontext,%function
jump_fcontext:
    @ save LR as PC
    push {lr}
    @ save V1-V8,LR
    push {v1-v8,lr}

    @ store RSP (pointing to context-data) in A1
    str  sp, [a1]

    @ restore RSP (pointing to context-data) from A2
    mov  sp, a2

    @ use third arg as return value after jump
    @ and as first arg in context function
    mov  a1, a3
    
    @ restore v1-V8,LR,PC
    pop {v1-v8,lr,pc}
.size jump_fcontext,.-jump_fcontext

@ Mark that we don't need executable stack.
.section .note.GNU-stack,"",%progbits
