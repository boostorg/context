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
 *  |  0  |  4  |  8  |  12 |  16 |  20 |  24 |  28 |  *
 *  -------------------------------------------------  *
 *  |    F20    |    F22    |    F24    |    F26    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |  *
 *  -------------------------------------------------  *
 *  |  32 |  36 |  40 |  44 |  48 |  52 |  56 |  60 |  *
 *  -------------------------------------------------  *
 *  |    F28    |    F30    |     S0    |     S1    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  16 |  17 |  18 |  19 |  20 |  21 |  22 |  23 |  *
 *  -------------------------------------------------  *
 *  |  64 |  68 |  72 |  76 |  80 |  84 |  88 |  92 |  *
 *  -------------------------------------------------  *
 *  |     S2    |     S3    |     S4    |     S5    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  24 |  25 |  26 |  27 |  28 |  29 |  30 |  31 |  *
 *  -------------------------------------------------  *
 *  |  96 | 100 | 104 | 108 | 112 | 116 | 120 | 124 |  *
 *  -------------------------------------------------  *
 *  |     S6    |     S7    |     FP    |     GP    |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  32 |  33 |  34 |  35 |  36 |  37 |  38 |  39 |  *
 *  -------------------------------------------------  *
 *  | 128 | 132 | 136 | 140 | 144 | 148 | 152 | 156 |  *
 *  -------------------------------------------------  *
 *  |   hiddn   |     RA    |     PC    |    FCTX   |  *
 *  -------------------------------------------------  *
 *  -------------------------------------------------  *
 *  |  40 |  41 |  42 |  43 |  44 |  45 |  46 |  47 |  *
 *  -------------------------------------------------  *
 *  | 160 | 164 | 168 | 172 | 176 | 180 | 184 | 188 |  *
 *  -------------------------------------------------  *
 *  |    DATA   |                                   |  *
 *  -------------------------------------------------  *
 *                                                     *
 * *****************************************************/

.text
.globl jump_fcontext
.align 2
.type jump_fcontext,@function
.ent jump_fcontext
jump_fcontext:
    # reserve space on stack
    addiu $sp, $sp, -152

    s.d  $s0, 48($sp)  # save S0
    s.d  $s1, 56($sp)  # save S1
    s.d  $s2, 64($sp)  # save S2
    s.d  $s3, 72($sp)  # save S3
    s.d  $s4, 80($sp)  # save S4
    s.d  $s5, 88($sp)  # save S5
    s.d  $s6, 96($sp)  # save S6
    s.d  $s7, 104($sp)  # save S7
    s.d  $fp, 112($sp)  # save FP
    s.d  $gp, 120($sp)  # save GP
    s.d  $a0, 128($sp)  # save hidden, address of returned transfer_t
    s.d  $ra, 136($sp)  # save RA
    s.d  $ra, 144($sp)  # save RA as PC

#if defined(__mips_hard_float)
    s.d  $f20, ($sp)  # save F20
    s.d  $f22, 8($sp)  # save F22
    s.d  $f24, 16($sp)  # save F24
    s.d  $f26, 24($sp)  # save F26
    s.d  $f28, 32($sp)  # save F28
    s.d  $f30, 40($sp)  # save F30
#endif

    # store SP (pointing to context-data) in A0
    move  $a0, $sp

    # restore SP (pointing to context-data) from A1
    move  $sp, $a1

#if defined(__mips_hard_float)
    l.d  $f20, ($sp)  # restore F20
    l.d  $f22, 8($sp)  # restore F22
    l.d  $f24, 16($sp)  # restore F24
    l.d  $f26, 24($sp)  # restore F26
    l.d  $f28, 32($sp)  # restore F28
    l.d  $f30, 40($sp)  # restore F30
#endif

    l.d  $s0, 48($sp)  # restore S0
    l.d  $s1, 56($sp)  # restore S1
    l.d  $s2, 64($sp)  # restore S2
    l.d  $s3, 72($sp)  # restore S3
    l.d  $s4, 80($sp)  # restore S4
    l.d  $s5, 88($sp)  # restore S5
    l.d  $s6, 96($sp)  # restore S6
    l.d  $s7, 104($sp)  # restore S7
    l.d  $fp, 112($sp)  # restore FP
    l.d  $gp, 120($sp)  # restore GP
    l.d  $v0, 128($sp)  # restore hidden, address of returned transfer_t
    l.d  $ra, 136($sp)  # restore RA

    # load PC
    l.d  $t9, 144($sp)

    # adjust stack
    addiu $sp, $sp, 152
    
    # return transfer_t from jump
    s.d  $a0, ($v0)  # fctx of transfer_t
    s.d  $a2, 8($v0) # data of transfer_t
    # pass transfer_t as first arg in context function
    # A0 == fctx, A1 == data
    move  $a1, $a2 

    # jump to context
    jr  $t9
.end jump_fcontext
.size jump_fcontext, .-jump_fcontext

/* Mark that we don't need executable stack.  */
.section .note.GNU-stack,"",%progbits
