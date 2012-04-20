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
 *  | R13 | R14 | R15 | R16 | R17 | R18 | R19 | R20 | R21 | R22 |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  10 |  11 |  12 |  13 |  14 |  15 |  16 |  17 |  18 |  19 |  *
 *  -------------------------------------------------------------  *
 *  |  40 |  44 |  48 |  52 |  56 |  60 |  64 |  68 |  72 |  76 |  *
 *  -------------------------------------------------------------  *
 *  | R23 | R24 | R25 | R26 | R27 | R28 | R29 | R30 | R31 |  SP |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  20 |  21 |  22 |  23 |                                   |  *
 *  -------------------------------------------------------------  *
 *  |  80 |  84 |  88 |  92 |                                   |  *
 *  -------------------------------------------------------------  *
 *  |  CR |  LR |  PC |     |                                   |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  24 |  25 |                                               |  *
 *  -------------------------------------------------------------  *
 *  |  96 | 100 |                                               |  *
 *  -------------------------------------------------------------  *
 *  |sbase|slimt|                                               |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  26 |                                                     |  *
 *  -------------------------------------------------------------  *
 *  | 104 |                                                     |  *
 *  -------------------------------------------------------------  *
 *  |flink|                                                     |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  28 |  29 |  30 |  31 |  32 |  33 |  34 |  35 |  36 |  37 |  *
 *  -------------------------------------------------------------  *
 *  | 108 | 112 | 116 | 120 | 124 | 128 | 132 | 136 | 140 | 144 |  *
 *  -------------------------------------------------------------  *
 *  |    F14    |    F15    |    F16    |    F17    |    F18    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  38 |  39 |  40 |  41 |  42 |  43 |  44 |  45 |  46 |  47 |  *
 *  -------------------------------------------------------------  *
 *  | 148 | 152 | 156 | 160 | 164 | 168 | 172 | 176 | 180 | 184 |  *
 *  -------------------------------------------------------------  *
 *  |    F19    |    F20    |    F21    |    F22    |    F23    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  48 |  49 |  50 |  51 |  52 |  53 |  54 |  55 |  56 |  57 |  *
 *  -------------------------------------------------------------  *
 *  | 188 | 192 | 196 | 200 | 204 | 208 | 212 | 216 | 220 | 224 |  *
 *  -------------------------------------------------------------  *
 *  |    F24    |    F25    |    F26    |    F27    |    F28    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  58 |  59 |  60 |  61 |  62 |  63 |  64 |  65 |           |  *
 *  -------------------------------------------------------------  *
 *  | 228 | 232 | 236 | 240 | 244 | 248 | 252 | 256 |           |  *
 *  -------------------------------------------------------------  *
 *  |    F29    |    F30    |    F31    |   fpscr   |           |  *
 *  -------------------------------------------------------------  *
 *                                                                 *
 * *****************************************************************/

.text
.globl start_fcontext
.align 2
.type start_fcontext,@function
start_fcontext:
    stw     %r13,  0(%r3)       # save R13
    stw     %r14,  4(%r3)       # save R14
    stw     %r15,  8(%r3)       # save R15
    stw     %r16,  12(%r3)      # save R16
    stw     %r17,  16(%r3)      # save R17
    stw     %r18,  20(%r3)      # save R18
    stw     %r19,  24(%r3)      # save R19
    stw     %r20,  28(%r3)      # save R20
    stw     %r21,  32(%r3)      # save R21
    stw     %r22,  36(%r3)      # save R22
    stw     %r23,  40(%r3)      # save R23
    stw     %r24,  44(%r3)      # save R24
    stw     %r25,  48(%r3)      # save R25
    stw     %r26,  52(%r3)      # save R26
    stw     %r27,  56(%r3)      # save R27
    stw     %r28,  60(%r3)      # save R28
    stw     %r29,  64(%r3)      # save R29
    stw     %r30,  68(%r3)      # save R30
    stw     %r31,  72(%r3)      # save R31
    stw     %r1,   76(%r3)      # save SP

    mfcr    %r0                 # load CR
    stw     %r0,   80(%r3)      # save CR
    mflr    %r0                 # load LR
    stw     %r0,   84(%r3)      # save LR
    stw     %r0,   88(%r3)      # save LR as PC

    stfd    %f14,  108(%r3)     # save F14
    stfd    %f15,  116(%r3)     # save F15
    stfd    %f16,  124(%r3)     # save F16
    stfd    %f17,  132(%r3)     # save F17
    stfd    %f18,  140(%r3)     # save F18
    stfd    %f19,  148(%r3)     # save F19
    stfd    %f20,  156(%r3)     # save F20
    stfd    %f21,  164(%r3)     # save F21
    stfd    %f22,  172(%r3)     # save F22
    stfd    %f23,  180(%r3)     # save F23
    stfd    %f24,  188(%r3)     # save F24
    stfd    %f25,  196(%r3)     # save F25
    stfd    %f26,  204(%r3)     # save F26
    stfd    %f27,  212(%r3)     # save F27
    stfd    %f28,  220(%r3)     # save F28
    stfd    %f29,  228(%r3)     # save F29
    stfd    %f30,  236(%r3)     # save F30
    stfd    %f31,  244(%r3)     # save F31
    mffs    %f0                 # load FPSCR
    stfd    %f0,   252(%r3)     # save FPSCR


    lwz     %r13,  0(%r4)       # restore R13
    lwz     %r14,  4(%r4)       # restore R14
    lwz     %r15,  8(%r4)       # restore R15
    lwz     %r16,  12(%r4)      # restore R16
    lwz     %r17,  16(%r4)      # restore R17
    lwz     %r18,  20(%r4)      # restore R18
    lwz     %r19,  24(%r4)      # restore R19
    lwz     %r20,  28(%r4)      # restore R20
    lwz     %r21,  32(%r4)      # restore R21
    lwz     %r22,  36(%r4)      # restore R22
    lwz     %r23,  40(%r4)      # restore R23
    lwz     %r24,  44(%r4)      # restore R24
    lwz     %r25,  48(%r4)      # restore R25
    lwz     %r26,  52(%r4)      # restore R26
    lwz     %r27,  56(%r4)      # restore R27
    lwz     %r28,  60(%r4)      # restore R28
    lwz     %r29,  64(%r4)      # restore R29
    lwz     %r30,  68(%r4)      # restore R30
    lwz     %r31,  72(%r4)      # restore R31
    lwz     %r1,   76(%r4)      # restore SP

    lwz     %r0,   80(%r4)      # load CR
    mtcr    %r0                 # restore CR
    lwz     %r0,   84(%r4)      # load LR
    mtlr    %r0                 # restore LR

    lfd     %f14,  108(%r4)     # restore F14
    lfd     %f15,  116(%r4)     # restore F15
    lfd     %f16,  124(%r4)     # restore F16
    lfd     %f17,  132(%r4)     # restore F17
    lfd     %f18,  140(%r4)     # restore F18
    lfd     %f19,  148(%r4)     # restore F19
    lfd     %f20,  156(%r4)     # restore F20
    lfd     %f21,  164(%r4)     # restore F21
    lfd     %f22,  172(%r4)     # restore F22
    lfd     %f23,  180(%r4)     # restore F23
    lfd     %f24,  188(%r4)     # restore F24
    lfd     %f25,  196(%r4)     # restore F25
    lfd     %f26,  204(%r4)     # restore F26
    lfd     %f27,  212(%r4)     # restore F27
    lfd     %f28,  220(%r4)     # restore F28
    lfd     %f29,  228(%r4)     # restore F29
    lfd     %f30,  236(%r4)     # restore F30
    lfd     %f31,  244(%r4)     # restore F31
    lfd     %f0,   252(%r4)     # load FPSCR
    mtfsf   0xff,  %f0          # restore FPSCR

    lwz     %r0,   88(%r4)      # load PC
    mr.     %r3,   %r15         # load void pointer as argument
    mtctr   %r0                 # restore CTR

    bctr                        # start to context
.size start_fcontext, .-start_fcontext

.text
.globl jump_fcontext
.align 2
.type jump_fcontext,@function
jump_fcontext:
    stw     %r13,  0(%r3)       # save R13
    stw     %r14,  4(%r3)       # save R14
    stw     %r15,  8(%r3)       # save R15
    stw     %r16,  12(%r3)      # save R16
    stw     %r17,  16(%r3)      # save R17
    stw     %r18,  20(%r3)      # save R18
    stw     %r19,  24(%r3)      # save R19
    stw     %r20,  28(%r3)      # save R20
    stw     %r21,  32(%r3)      # save R21
    stw     %r22,  36(%r3)      # save R22
    stw     %r23,  40(%r3)      # save R23
    stw     %r24,  44(%r3)      # save R24
    stw     %r25,  48(%r3)      # save R25
    stw     %r26,  52(%r3)      # save R26
    stw     %r27,  56(%r3)      # save R27
    stw     %r28,  60(%r3)      # save R28
    stw     %r29,  64(%r3)      # save R29
    stw     %r30,  68(%r3)      # save R30
    stw     %r31,  72(%r3)      # save R31
    stw     %r1,   76(%r3)      # save SP

    mfcr    %r0                 # load CR
    stw     %r0,   80(%r3)      # save CR
    mflr    %r0                 # load LR
    stw     %r0,   84(%r3)      # save LR
    stw     %r0,   88(%r3)      # save LR as PC

    stfd    %f14,  108(%r3)     # save F14
    stfd    %f15,  116(%r3)     # save F15
    stfd    %f16,  124(%r3)     # save F16
    stfd    %f17,  132(%r3)     # save F17
    stfd    %f18,  140(%r3)     # save F18
    stfd    %f19,  148(%r3)     # save F19
    stfd    %f20,  156(%r3)     # save F20
    stfd    %f21,  164(%r3)     # save F21
    stfd    %f22,  172(%r3)     # save F22
    stfd    %f23,  180(%r3)     # save F23
    stfd    %f24,  188(%r3)     # save F24
    stfd    %f25,  196(%r3)     # save F25
    stfd    %f26,  204(%r3)     # save F26
    stfd    %f27,  212(%r3)     # save F27
    stfd    %f28,  220(%r3)     # save F28
    stfd    %f29,  228(%r3)     # save F29
    stfd    %f30,  236(%r3)     # save F30
    stfd    %f31,  244(%r3)     # save F31
    mffs    %f0                 # load FPSCR
    stfd    %f0,   252(%r3)     # save FPSCR


    lwz     %r13,  0(%r4)       # restore R13
    lwz     %r14,  4(%r4)       # restore R14
    lwz     %r15,  8(%r4)       # restore R15
    lwz     %r16,  12(%r4)      # restore R16
    lwz     %r17,  16(%r4)      # restore R17
    lwz     %r18,  20(%r4)      # restore R18
    lwz     %r19,  24(%r4)      # restore R19
    lwz     %r20,  28(%r4)      # restore R20
    lwz     %r21,  32(%r4)      # restore R21
    lwz     %r22,  36(%r4)      # restore R22
    lwz     %r23,  40(%r4)      # restore R23
    lwz     %r24,  44(%r4)      # restore R24
    lwz     %r25,  48(%r4)      # restore R25
    lwz     %r26,  52(%r4)      # restore R26
    lwz     %r27,  56(%r4)      # restore R27
    lwz     %r28,  60(%r4)      # restore R28
    lwz     %r29,  64(%r4)      # restore R29
    lwz     %r30,  68(%r4)      # restore R30
    lwz     %r31,  72(%r4)      # restore R31
    lwz     %r1,   76(%r4)      # restore SP

    lwz     %r0,   80(%r4)      # load CR
    mtcr    %r0                 # restore CR
    lwz     %r0,   84(%r4)      # load LR
    mtlr    %r0                 # restore LR

    lfd     %f14,  108(%r4)     # restore F14
    lfd     %f15,  116(%r4)     # restore F15
    lfd     %f16,  124(%r4)     # restore F16
    lfd     %f17,  132(%r4)     # restore F17
    lfd     %f18,  140(%r4)     # restore F18
    lfd     %f19,  148(%r4)     # restore F19
    lfd     %f20,  156(%r4)     # restore F20
    lfd     %f21,  164(%r4)     # restore F21
    lfd     %f22,  172(%r4)     # restore F22
    lfd     %f23,  180(%r4)     # restore F23
    lfd     %f24,  188(%r4)     # restore F24
    lfd     %f25,  196(%r4)     # restore F25
    lfd     %f26,  204(%r4)     # restore F26
    lfd     %f27,  212(%r4)     # restore F27
    lfd     %f28,  220(%r4)     # restore F28
    lfd     %f29,  228(%r4)     # restore F29
    lfd     %f30,  236(%r4)     # restore F30
    lfd     %f31,  244(%r4)     # restore F31
    lfd     %f0,   252(%r4)     # load FPSCR
    mtfsf   0xff,  %f0          # restore FPSCR

    mr.     %r3,   %r5          # use third arg as return value after jump

    lwz     %r0,   88(%r4)      # load PC
    mtctr   %r0                 # restore CTR

    bctr                        # jump to context
.size jump_fcontext, .-jump_fcontext

.text
.globl make_fcontext
.align 2
.type make_fcontext,@function
make_fcontext:
    stw     %r3, 0(%r3)         # save the current context
    stw     %r4, 88(%r3)        # save the address of the function supposed to to be run
    stw     %r5, 8(%r3)         # save void pointer
    lwz     %r0, 96(%r3)        # load the stack base

    li      %r4, 28
    subf    %r1, %r4, %r1       # reserve space on stack
    stw     %r3, 24(%r1)        # store pointer to fcontext_t on stack
    mflr    %r4                 # load LR
    stw     %r4, 20(%r1)        # store LR on stack
    mr.     %r3, %r0            # context stack as arg to align_stack
    bl      align_stack@plt     # call align_stack
    mr.     %r0, %r3            # load result into R0
    lwz     %r4, 20(%r1)        # pop LR from stack
    mtlr    %r4                 # restore LR
    lwz     %r3, 24(%r1)        # pop pointer to fcontext_t from stack
    addi    %r1, %r1, 28        # release space on stack

    li      %r4, 32
    subf    %r0, %r4, %r0       # 32 bytes on stack for parameter area(== 8 registers)
    stw     %r0, 76(%r3)        # save the stack base
    lwz     %r0, 104(%r3)       # load address of next user context
    stw     %r0, 4(%r3)         # save the next context
    mflr    %r0                 # load LR
    bl      1f                  # jump to label 1
1:
    mflr    %r4                             # load LR
    addi    %r4, %r4, link_fcontext - 1b    # calulate absolute address of link_fcontext
    mtlr    %r0                             # restore LR
    stw     %r4, 84(%r3)        # save address of link_fcontext

    li      %r3,  0             # set return value to zero
    blr

link_fcontext:
    mr.     %r3, %r13
    mr.     %r4, %r14
    beq     2f                  # test if a next context was given

    bl      start_fcontext@plt   # install next context

2:
    li      %r3,  0             # set return value to zero
    bl      _exit@plt           # exit application
.size make_fcontext, .-make_fcontext
