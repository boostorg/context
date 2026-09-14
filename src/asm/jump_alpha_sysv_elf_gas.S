/*
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
          http://www.boost.org/LICENSE_1_0.txt)
*/
/*********************************************************
 *                                                       *
 *  ---------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4    |  *
 *  ---------------------------------------------------  *
 *  |   0x0   |   0x8   |   0x10  |   0x18  |   0x20  |  *
 *  ---------------------------------------------------  *
 *  |   $f2   |   $f3   |   $f4   |   $f5   |   $f6   |  *
 *  ---------------------------------------------------  *
 *  |    5    |    6    |    7    |    8    |    9    |  *
 *  ---------------------------------------------------  *
 *  |   0x28  |   0x30  |   0x38  |   0x40  |   0x48  |  *
 *  ---------------------------------------------------  *
 *  |   $f7   |   $f8   |   $f9   |    $9   |   $10   |  *
 *  ---------------------------------------------------  *
 *  |    10   |    11   |    12   |    13   |    14   |  *
 *  ---------------------------------------------------  *
 *  |   0x50  |   0x58  |   0x60  |   0x68  |   0x70  |  *
 *  ---------------------------------------------------  *
 *  |   $11   |   $12   |   $13   |   $14   |   $15   |  *
 *  ---------------------------------------------------  *
 *  |    15   |    16   |    17   |    18   |    19   |  *
 *  ---------------------------------------------------  *
 *  |   0x78  |   0x80  |   0x88  |   0x90  |   0x98  |  *
 *  ---------------------------------------------------  *
 *  |   $29   |   $26   |    PC   |  hidden |   pad   |  *
 *  |   (gp)  |   (ra)  |         |   ptr   |         |  *
 *  ---------------------------------------------------  *
 *                                                       *
 ********************************************************/

/* The Alpha calling standard returns a 16 byte struct such as transfer_t
   via a hidden pointer in $16, shifting the declared arguments to $17 and
   $18.  That pointer belongs to the frame that made the call, so it has to
   travel with the context: each context records the pointer its own
   suspending call was given, and whoever resumes it stores through it. */

.file "jump_alpha_sysv_elf_gas.S"
.text
.align 4
.globl jump_fcontext
.hidden jump_fcontext
.type jump_fcontext, @function
.ent jump_fcontext
jump_fcontext:
    .frame $30, 0, $26, 0
    .prologue 0

    # reserve space for context-data on the current stack
    lda   $30, -0xa0($30)

    # save callee-saved floating point registers $f2 - $f9
    stt   $f2,  0x00($30)
    stt   $f3,  0x08($30)
    stt   $f4,  0x10($30)
    stt   $f5,  0x18($30)
    stt   $f6,  0x20($30)
    stt   $f7,  0x28($30)
    stt   $f8,  0x30($30)
    stt   $f9,  0x38($30)

    # save callee-saved integer registers $9 - $15
    stq   $9,   0x40($30)
    stq   $10,  0x48($30)
    stq   $11,  0x50($30)
    stq   $12,  0x58($30)
    stq   $13,  0x60($30)
    stq   $14,  0x68($30)
    stq   $15,  0x70($30)

    # save gp and return address
    stq   $29,  0x78($30)
    stq   $26,  0x80($30)

    # save the return address as the resumption PC
    stq   $26,  0x88($30)

    # save the hidden transfer_t pointer this call was given
    stq   $16,  0x90($30)

    # $1 = fcontext_t of the context we are suspending
    bis   $31, $30, $1

    # switch to the stack of the context we are resuming
    bis   $31, $17, $30

    # restore callee-saved floating point registers $f2 - $f9
    ldt   $f2,  0x00($30)
    ldt   $f3,  0x08($30)
    ldt   $f4,  0x10($30)
    ldt   $f5,  0x18($30)
    ldt   $f6,  0x20($30)
    ldt   $f7,  0x28($30)
    ldt   $f8,  0x30($30)
    ldt   $f9,  0x38($30)

    # restore callee-saved integer registers $9 - $15
    ldq   $9,   0x40($30)
    ldq   $10,  0x48($30)
    ldq   $11,  0x50($30)
    ldq   $12,  0x58($30)
    ldq   $13,  0x60($30)
    ldq   $14,  0x68($30)
    ldq   $15,  0x70($30)

    # restore gp and return address
    ldq   $29,  0x78($30)
    ldq   $26,  0x80($30)

    # load the resumption PC into $27; it doubles as the procedure value
    # for a context-function entered for the first time, which computes
    # its own gp from it
    ldq   $27,  0x88($30)

    # load the hidden transfer_t pointer of the resumed context
    ldq   $2,   0x90($30)

    # release the context-data
    lda   $30, 0xa0($30)

    # return transfer_t{ $1, $18 } through the hidden pointer
    stq   $1,  0x00($2)
    stq   $18, 0x08($2)
    bis   $31, $2, $0

    # and pass it by value, for a context entered via make_fcontext
    bis   $31, $1,  $16
    bis   $31, $18, $17

    # the ret hint pops the return prediction stack entry pushed by the
    # bsr that called us; the resumption PC is where that call appears to
    # return.  A jmp hint would orphan one entry per switch.
    ret   $31, ($27), 0
.end jump_fcontext
.size jump_fcontext, .-jump_fcontext

/* Mark that we don't need executable stack. */
.section .note.GNU-stack,"",@progbits
