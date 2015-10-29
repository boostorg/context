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
 *  | sjlj|  v1 |  v2 |  v3 |  v4 |  v5 |  v6 |  v7 |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |  *
 *  -------------------------------------------------  *
 *  | 0x20| 0x24| 0x28| 0x2c| 0x30| 0x34| 0x38| 0x3c|  *
 *  -------------------------------------------------  *
 *  |  v8 |  lr |  pc |                             |  *
 *  -------------------------------------------------  *
 *                                                     *
 *******************************************************/

.text
.globl _jump_fcontext
.align 2
_jump_fcontext:
    @ save LR as PC
    push {lr}
    @ save V1-V8,LR
    push {v1-v8,lr}

    @ locate TLS to save/restore SjLj handler
    mrc  p15, 0, v2, c13, c0, #3
    bic  v2, v2, #3

    @ load TLS[__PTK_LIBC_DYLD_Unwind_SjLj_Key]
    ldr  v1, [v2, #8]
    @ save SjLj handler
    push  {v1}

    @ store RSP (pointing to context-data) in A1
    str  sp, [a1]

    @ restore RSP (pointing to context-data) from A2
    mov  sp, a2

    @ r#estore SjLj handler
    pop  {v1}
    @ store SjLj handler in TLS
    str  v1, [v2, #8]

    @ use third arg as return value after jump
    @ and as first arg in context function
    mov  a1, a3
    
    @ restore v1-V8,LR,PC
    pop {v1-v8,lr,pc}
