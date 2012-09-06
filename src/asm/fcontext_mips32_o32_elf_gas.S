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
 *  |  0  |  4  |  8  |  12 |  16 |  20 |  24 |  28 |  32 |  36 |  *
 *  -------------------------------------------------------------  *
 *  |  S0 |  S1 |  S2 |  S3 |  S4 |  S5 |  S6 |  S7 |  FP |  SP |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  10 |  11 |                                               |  *
 *  -------------------------------------------------------------  *
 *  |  40 |  44 |                                               |  *
 *  -------------------------------------------------------------  *
 *  |  RA |  PC |                                               |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  12 |  13 |                                               |  *
 *  -------------------------------------------------------------  *
 *  |  48 |  52 |                                               |  *
 *  -------------------------------------------------------------  *
 *  |  sp | size|                                               |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  14 |  15 |  16 |  17 |  18 |  19 |                       |  *
 *  -------------------------------------------------------------  *
 *  |  56 |  64 |  72 |  80 |  88 |  96 |                       |  *
 *  -------------------------------------------------------------  *
 *  | F20 | F22 | F24 | F26 | F28 | F30 |                       |  *
 *  -------------------------------------------------------------  *
 *                                                                 *
 * *****************************************************************/

.text
.globl jump_fcontext
.align 2
.type jump_fcontext,@function
.ent jump_fcontext
jump_fcontext:
    sw      $s0, ($a0)      # save S0
    sw      $s1, 4($a0)     # save S1
    sw      $s2, 8($a0)     # save S2
    sw      $s3, 12($a0)    # save S3
    sw      $s4, 16($a0)    # save S4
    sw      $s5, 20($a0)    # save S5
    sw      $s6, 24($a0)    # save S6
    sw      $s7, 28($a0)    # save S7
    sw      $fp, 32($a0)    # save FP
    sw      $sp, 36($a0)    # save SP
    sw      $ra, 40($a0)    # save RA
    sw      $ra, 44($a0)    # save RA as PC

#if defined(__mips_hard_float)
    beqz    $a3,  1f        # test if fpu env should be preserved 
    s.d     $f20, 56($a0)   # save F20
    s.d     $f22, 64($a0)   # save F22
    s.d     $f24, 72($a0)   # save F24
    s.d     $f26, 80($a0)   # save F26
    s.d     $f28, 88($a0)   # save F28
    s.d     $f30, 96($a0)   # save F30

    l.d     $f20, 56($a1)   # restore F20
    l.d     $f22, 64($a1)   # restore F22
    l.d     $f24, 72($a1)   # restore F24
    l.d     $f26, 80($a1)   # restore F26
    l.d     $f28, 88($a1)   # restore F28
    l.d     $f30, 96($a1)   # restore F30
1:
#endif

    lw      $s0, ($a1)      # restore S0
    lw      $s1, 4($a1)     # restore S1
    lw      $s2, 8($a1)     # restore S2
    lw      $s3, 12($a1)    # restore S3
    lw      $s4, 16($a1)    # restore S4
    lw      $s5, 20($a1)    # restore S5
    lw      $s6, 24($a1)    # restore S6
    lw      $s7, 28($a1)    # restore S7
    lw      $fp, 32($a1)    # restore FP
    lw      $sp, 36($a1)    # restore SP
    lw      $ra, 40($a1)    # restore RA

    move    $v0, $a2        # use third arg as return value after jump
    move    $a0, $a2        # use third arg as first arg in context function

    lw      $t9, 44($a1)    # load PC
    jr      $t9             # jump to context
.end jump_fcontext
.size jump_fcontext, .-jump_fcontext

.text
.globl make_fcontext
.align 2
.type make_fcontext,@function
.ent make_fcontext
make_fcontext:
#ifdef __PIC__
.set    noreorder
.cpload $t9
.set    reorder
#endif
    addiu   $sp, $sp, -48       # allocate stack space (contains shadow space for subroutines)
    sw      $ra, 44($sp)        # save return address
    sw      $fp, 40($sp)        # save frame pointer

    sw      $a2, 48($sp)        # save 3. arg of make_fcontnext, pointer to context function
    sw      $a1, 52($sp)        # save 2. arg of make_fcontext, context stack size
    sw      $a0, 56($sp)        # save 1. arg of make_fcontext, pointer to context stack (base)
    addiu   $a0, $a0, -104      # reserve space for fcontext_t at top of context stack
    lw      $t9, %call16(align_stack)($gp) # compute adddress of align_stack via global pointer
    jalr    $t9                 # call align_stack, V0 contains address at 16 byte boundary after return
                                # == pointer to fcontext_t and address of context stack
    lw      $a0, 56($sp)        # restore pointer to context stack (base)
    lw      $a1, 52($sp)        # restore context stack size
    lw      $a2, 48($sp)        # restore pointer to context function

    sw      $a0, 48($v0)        # save address of context stack (base) in fcontext_t 
    sw      $a1, 52($v0)        # save context stack size in fcontext_t 
    sw      $a2, 44($v0)        # save address of context function in fcontext_t 
    sw      $gp, ($v0)          # save global pointer in fcontext_t, S0 will contain address of global pointer

    addiu   $t0, $v0, -48       # reserve 48 bytes (includes 4 byte shadow space), T0 % 16 == 0
    sw      $t0, 36($v0)        # save address in T0 as stack pointer for context function, SP % 16 == 0

    la      $t9, finish         # compute abs address of label finish
    sw      $t9, 40($v0)        # save address of finish as return address for context function
                                # entered after context function returns

    lw      $fp, 40($sp)        # restore frame pointer
    lw      $ra, 44($sp)        # restore return address
    addiu   $sp, $sp, 48        # deallocate stack space

    jr      $ra

finish:
    # SP points to same adddress as SP on entry of context function
    addiu   $sp, $sp, -32       # allocate stack space (contains shadow space for subroutines)
    sw      $ra, 28($sp)        # save return address

    move    $gp, $s0                  # restore GP (global pointer)
    move    $a0, $zero                # exit code is zero
    lw      $t9, %call16(_exit)($gp)  # address of exit
    jalr    $t9                       # exit application
.end make_fcontext
.size make_fcontext, .-make_fcontext
