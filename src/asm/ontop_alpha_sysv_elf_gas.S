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

.file "ontop_alpha_sysv_elf_gas.S"
.text
.align 4
.globl ontop_fcontext
.hidden ontop_fcontext
.type ontop_fcontext, @function
.ent ontop_fcontext
ontop_fcontext:
    .frame $30, 0, $26, 0
    .prologue 0

    # $16: hidden transfer_t pointer
    # $17: fcontext_t to
    # $18: void * vp
    # $19: transfer_t (*fn)(transfer_t)

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

    # restore gp; the resumed context's return address becomes the return
    # address of the ontop-function, so that when it returns it lands where
    # the resumed context suspended
    ldq   $29,  0x78($30)
    ldq   $26,  0x80($30)

    # load the hidden transfer_t pointer of the resumed context; the
    # ontop-function returns its transfer_t through it
    ldq   $2,   0x90($30)

    # skip the resumption PC
    # release the context-data
    lda   $30, 0xa0($30)

    # transfer_t{ $1, $18 } is passed by value in $17/$18; $18 already
    # holds vp
    bis   $31, $2,  $16
    bis   $31, $1,  $17

    # jump to the ontop-function; $27 is its procedure value.  The
    # ontop-function returns to the resumed context rather than to our
    # caller, so the ret hint pops the entry the bsr that called us
    # pushed, which would otherwise be orphaned.
    bis   $31, $19, $27
    ret   $31, ($27), 0
.end ontop_fcontext
.size ontop_fcontext, .-ontop_fcontext

/* Mark that we don't need executable stack. */
.section .note.GNU-stack,"",@progbits
