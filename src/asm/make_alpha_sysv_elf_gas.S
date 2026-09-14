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
 *  |    20   |    21   |         |         |         |  *
 *  ---------------------------------------------------  *
 *  |   0xa0  |   0xa8  |         |         |         |  *
 *  ---------------------------------------------------  *
 *  |                transfer_t scratch               |  *
 *  ---------------------------------------------------  *
 *                                                       *
 ********************************************************/

.file "make_alpha_sysv_elf_gas.S"
.text
.align 4
.globl make_fcontext
.hidden make_fcontext
.type make_fcontext, @function
.ent make_fcontext
make_fcontext:
    .frame $30, 0, $26, 0
    ldgp  $29, 0($27)
    .prologue 1

    # $16: top address of the context-stack
    # $17: size of the context-stack
    # $18: address of the context-function

    # shift the address in $16 down to a 16 byte boundary
    bic   $16, 0xF, $0

    # reserve space for the context-data and for the transfer_t that
    # jump_fcontext writes through the hidden pointer when it enters the
    # context-function
    lda   $0, -0xc0($0)

    # the context-function is the resumption PC
    stq   $18, 0x88($0)

    # jump_fcontext restores gp before entering the context-function; the
    # context-function computes its own, but leave a valid value here
    stq   $29, 0x78($0)

    # point the hidden pointer at the scratch transfer_t
    lda   $1, 0xa0($0)
    stq   $1, 0x90($0)

    # start the context with cleared callee-saved registers
    stt   $f31, 0x00($0)
    stt   $f31, 0x08($0)
    stt   $f31, 0x10($0)
    stt   $f31, 0x18($0)
    stt   $f31, 0x20($0)
    stt   $f31, 0x28($0)
    stt   $f31, 0x30($0)
    stt   $f31, 0x38($0)
    stq   $31,  0x40($0)
    stq   $31,  0x48($0)
    stq   $31,  0x50($0)
    stq   $31,  0x58($0)
    stq   $31,  0x60($0)
    stq   $31,  0x68($0)
    stq   $31,  0x70($0)

    # save the address of finish as the return address of the
    # context-function; it is entered after the context-function returns
    ldq   $1, finish($29)    !literal
    stq   $1, 0x80($0)

    # return the pointer to the context-data in $0
    ret   $31, ($26), 1

finish:
    # the context-function returned; gp is not live here, so recompute it
    br    $29, 1f
1:  ldgp  $29, 0($29)

    # exit code is zero
    bis   $31, $31, $16

    # exit application
    ldq   $27, _exit($29)    !literal
    jsr   $26, ($27), _exit
.end make_fcontext
.size make_fcontext, .-make_fcontext

/* Mark that we don't need executable stack. */
.section .note.GNU-stack,"",@progbits
