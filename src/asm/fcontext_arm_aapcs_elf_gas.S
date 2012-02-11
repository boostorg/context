/*
            Copyright Oliver Kowalke 2009.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/

/*******************************************************************
 *                                                                 *
 *  -------------------------------------------------------------  *
 *  |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  *
 *  -------------------------------------------------------------  *
 *  | 0x0 | 0x4 | 0x8 | 0xc | 0x10| 0x14| 0x18| 0x1c| 0x20| 0x24|  *
 *  -------------------------------------------------------------  *
 *  |  v1 |  v2 |  v3 |  v4 |  v5 |  v6 |  v7 |  v8 |  sp |  lr |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  10 |                                                     |  *
 *  -------------------------------------------------------------  *
 *  | 0x28|                                                     |  *
 *  -------------------------------------------------------------  *
 *  |  pc |                                                     |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  11 | 12 |  13 |  14 |  15 |  16 |  17 |  18 |  19 |  20  |  *
 *  -------------------------------------------------------------  *
 *  | 0x2c|0x30| 0x34| 0x38| 0x3c| 0x40| 0x44| 0x48| 0x4c| 0x50 |  *
 *  -------------------------------------------------------------  *
 *  | s16 | s17 | s18 | s19 | s20 | s21 | s22 | s23 | s24 | s25 |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  21 |  22 |  23 |  24 |  25 |  26 |                       |  *
 *  -------------------------------------------------------------  *
 *  | 0x54| 0x58| 0x5c| 0x60| 0x64| 0x68|                       |  *
 *  -------------------------------------------------------------  *
 *  | s26 | s27 | s28 | s29 | s30 | s31 |                       |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  27 |  28 |                                               |  *
 *  -------------------------------------------------------------  *
 *  | 0x6c| 0x70|                                               |  *
 *  -------------------------------------------------------------  *
 *  |sbase|slimit|                                              |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  29 |                                                     |  *
 *  -------------------------------------------------------------  *
 *  | 0x74|                                                     |  *
 *  -------------------------------------------------------------  *
 *  |fclnk|                                                     |  *
 *  -------------------------------------------------------------  *
 *                                                                 *
 * *****************************************************************/

.text
.globl boost_fcontext_start
.align 2
.type boost_fcontext_start,%function
boost_fcontext_start:
    stmia   a1, {v1-v8,sp-lr}       @ save V1-V8,SP-LR
    str     lr, [a1,#40]            @ save LR as PC
#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
    ldr     a4, [a1,#44]
    stmia   a4, {s16-s31}           @ save S16-S31
#endif

    ldr     a1, [a2,#8]             @ restore A1 as first arg
#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
    ldr     a4, [a2,#44]
    ldmia   a4, {s16-s31}           @ restore S16-S31
#endif
	
    ldmia   a2, {v1-v8,sp-pc}       @ restore v1-V8,SP-PC
.size boost_fcontext_start,.-boost_fcontext_start

.text
.globl boost_fcontext_jump
.align 2
.type boost_fcontext_jump,%function
boost_fcontext_jump:
    stmia   a1, {v1-v8,sp-lr}       @ save V1-V8,SP-LR
    str     lr, [a1,#40]            @ save LR as PC
#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
    ldr     a4, [a1,#44]
    stmia   a4, {s16-s31}           @ save S16-S31
#endif

	mov		a1, a3                  @ use thrid arg as return value after jump
#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
    ldr     a4, [a2,#44]
    ldmia   a4, {s16-s31}           @ restore S16-S31
#endif

    ldmia   a2, {v1-v8,sp-pc}       @ restore v1-V8,SP-PC
.size boost_fcontext_jump,.-boost_fcontext_jump

.text
.globl boost_fcontext_make
.align 2
.type boost_fcontext_make,%function
boost_fcontext_make:
    str     a1, [a1,#0]         @ save the address of the current context
    str     a2, [a1,#40]        @ save address of the function supposed to be run
    str     a3, [a1,#8]         @ save void pointer
    ldr     a2, [a1,#108]       @ load the stack base

	push    {a1,lr}				@ save pointer to fcontext_t
	mov     a1, a2				@ stack pointer as arg for boost_fcontext_align
#ifdef __PIC__
	bl		boost_fcontext_align@PLT @ align stack
#else
	bl		boost_fcontext_align     @ align stack
#endif
	mov		a2, a1				@ begin of aligned stack 
	pop     {a1,lr}				@ restore pointer to boost_fcontext_t

    str     a2, [a1,#32]        @ save the stack base
    ldr     a2, [a1,#116]       @ load the address of the next context
    str     a2, [a1,#4]         @ save the address of the next context
    adr     a2, link_fcontext   @ load address of link_fcontext
    str     a2, [a1,#36]        @ save address of link_fcontext
    mov     a1, #0
    bx      lr

link_fcontext:
    mov     a1, v1              @ load the adddress of the current context
    mov     a2, v2              @ load the adddress of the next context
    teq     a2, #0              @ test if the address of the next context is a null pointer

#ifdef __PIC__
    bne     boost_fcontext_start@PLT   @ install next context

    mov     a1, #0
    bl      _exit@PLT                  @ exit application
#else
    bne     boost_fcontext_start       @ install next context

    mov     a1, #0
    bl      _exit                      @ exit application
#endif
.size boost_fcontext_make,.-boost_fcontext_make
