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
 *  |  0  |  4  |   8 |  12 |  16 |  20 |  24 |  28 |  32 |  36 |  *
 *  -------------------------------------------------------------  *
 *  |    R13    |    R14    |    R15    |    R16    |    R17    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  10 |  11 |  12 |  13 |  14 |  15 |  16 |  17 |  18 |  19 |  *
 *  -------------------------------------------------------------  *
 *  |  40 |  44 |  48 |  52 |  56 |  60 |  64 |  68 |  72 |  76 |  *
 *  -------------------------------------------------------------  *
 *  |    R18    |    R19    |    R20    |    R21    |    R22    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  20 |  21 |  22 |  23 |  24 |  25 |  26 |  27 |  28 |  29 |  *
 *  -------------------------------------------------------------  *
 *  |  80 |  84 |  88 |  92 |  96 | 100 | 104 | 108 | 112 | 116 |  *
 *  -------------------------------------------------------------  *
 *  |    R23    |    R24    |    R25    |    R26    |    R27    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  30 |  31 |  32 |  33 |  34 |  35 |  36 |  37 |  38 |  39 |  *
 *  -------------------------------------------------------------  *
 *  | 120 | 124 | 128 | 132 | 136 | 140 | 144 | 148 | 152 | 156 |  *
 *  -------------------------------------------------------------  *
 *  |    R28    |    R29    |    R30    |    R31    |     SP    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  40 |  41 |  42 |  43 |  44 |  45 |                       |  *
 *  -------------------------------------------------------------  *
 *  | 160 | 164 | 168 | 172 | 176 | 180 |                       |  *
 *  -------------------------------------------------------------  *
 *  |    CR     |    LR     |     PC    |                       |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  48 |  49 |  50 |  51 |  52 |  53 |  54 |  55 |  56 |  57 |  *
 *  -------------------------------------------------------------  *
 *  | 192 | 196 | 200 | 204 | 208 | 212 | 216 | 220 | 224 | 228 |  *
 *  -------------------------------------------------------------  *
 *  |    F14    |    F15    |    F16    |    F17    |    F18    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  58 |  59 |  60 |  61 |  62 |  63 |  64 |  65 |  66 |  67 |  *
 *  -------------------------------------------------------------  *
 *  | 232 | 236 | 240 | 244 | 248 | 252 | 256 | 260 | 264 | 268 |  *
 *  -------------------------------------------------------------  *
 *  |    F19    |    F20    |    F21    |    F22    |    F23    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  68 |  69 |  70 |  71 |  72 |  73 |  74 |  75 |  76 |  77 |  *
 *  -------------------------------------------------------------  *
 *  | 272 | 276 | 280 | 284 | 288 | 292 | 296 | 300 | 304 | 308 |  *
 *  -------------------------------------------------------------  *
 *  |    F24    |    F25    |    F26    |    F27    |    F28    |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  78 |  79 |  80 |  81 |  82 |  83 |  84 |  85 |           |  *
 *  -------------------------------------------------------------  *
 *  | 312 | 316 | 320 | 324 | 328 | 332 | 336 | 340 |           |  *
 *  -------------------------------------------------------------  *
 *  |    F29    |    F30    |    F31    |   fpscr   |           |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  86 |  87 |  88 |  89 |                                   |  *
 *  -------------------------------------------------------------  *
 *  | 344 | 348 | 352 | 356 |                                   |  *
 *  -------------------------------------------------------------  *
 *  |   sbase   |   slimt   |                                   |  *
 *  -------------------------------------------------------------  *
 *  -------------------------------------------------------------  *
 *  |  90 |  91 |                                               |  *
 *  -------------------------------------------------------------  *
 *  | 360 | 364 |                                               |  *
 *  -------------------------------------------------------------  *
 *  |   flink   |                                               |  *
 *  -------------------------------------------------------------  *
 *                                                                 *
 * *****************************************************************/

.section ".text"
.align 2
.globl boost_fcontext_start
.section ".opd","aw"
.align 3
boost_fcontext_start:
.quad .boost_fcontext_start,.TOC.@tocbase,0
.previous
.size boost_fcontext_start,24
.type .boost_fcontext_start,@function
.globl .boost_fcontext_start
.boost_fcontext_start:
    std     %r13,  0(%r3)       # save R13
    std     %r14,  8(%r3)       # save R14
    std     %r15,  16(%r3)      # save R15
    std     %r16,  24(%r3)      # save R16
    std     %r17,  32(%r3)      # save R17
    std     %r18,  40(%r3)      # save R18
    std     %r19,  48(%r3)      # save R19
    std     %r20,  56(%r3)      # save R20
    std     %r21,  64(%r3)      # save R21
    std     %r22,  72(%r3)      # save R22
    std     %r23,  80(%r3)      # save R23
    std     %r24,  88(%r3)      # save R24
    std     %r25,  96(%r3)      # save R25
    std     %r26,  104(%r3)     # save R26
    std     %r27,  112(%r3)     # save R27
    std     %r28,  120(%r3)     # save R28
    std     %r29,  128(%r3)     # save R29
    std     %r30,  136(%r3)     # save R30
    std     %r31,  144(%r3)     # save R31
    std     %r1,   152(%r3)     # save SP

    mfcr    %r0                 # load CR
    std     %r0,   160(%r3)     # save CR
    mflr    %r0                 # load LR
    std     %r0,   168(%r3)     # save LR
    std     %r0,   176(%r3)     # save LR as PC

    stfd    %f14,  192(%r3)     # save F14
    stfd    %f15,  200(%r3)     # save F15
    stfd    %f16,  208(%r3)     # save F16
    stfd    %f17,  216(%r3)     # save F17
    stfd    %f18,  224(%r3)     # save F18
    stfd    %f19,  232(%r3)     # save F19
    stfd    %f20,  240(%r3)     # save F20
    stfd    %f21,  248(%r3)     # save F21
    stfd    %f22,  256(%r3)     # save F22
    stfd    %f23,  264(%r3)     # save F23
    stfd    %f24,  272(%r3)     # save F24
    stfd    %f25,  280(%r3)     # save F25
    stfd    %f26,  288(%r3)     # save F26
    stfd    %f27,  296(%r3)     # save F27
    stfd    %f28,  304(%r3)     # save F28
    stfd    %f29,  312(%r3)     # save F29
    stfd    %f30,  320(%r3)     # save F30
    stfd    %f31,  328(%r3)     # save F31
    mffs    %f0                 # load FPSCR
    stfd    %f0,   336(%r3)     # save FPSCR


    ld      %r13,  0(%r4)       # restore R13
    ld      %r14,  8(%r4)       # restore R14
    ld      %r15,  16(%r4)      # restore R15
    ld      %r16,  24(%r4)      # restore R16
    ld      %r17,  32(%r4)      # restore R17
    ld      %r18,  40(%r4)      # restore R18
    ld      %r19,  48(%r4)      # restore R19
    ld      %r20,  56(%r4)      # restore R20
    ld      %r21,  64(%r4)      # restore R21
    ld      %r22,  72(%r4)      # restore R22
    ld      %r23,  80(%r4)      # restore R23
    ld      %r24,  88(%r4)      # restore R24
    ld      %r25,  96(%r4)      # restore R25
    ld      %r26,  104(%r4)     # restore R26
    ld      %r27,  112(%r4)     # restore R27
    ld      %r28,  120(%r4)     # restore R28
    ld      %r29,  128(%r4)     # restore R29
    ld      %r30,  136(%r4)     # restore r30
    ld      %r31,  144(%r4)     # restore r31
    ld      %r1,   152(%r4)     # restore SP

    ld      %r0,   160(%r4)     # load CR
    mtcr    %r0                 # restore CR
    ld      %r0,   168(%r4)     # load LR
    mtlr    %r0                 # restore LR

    lfd     %f14,  192(%r4)     # restore F14
    lfd     %f15,  200(%r4)     # restore F15
    lfd     %f16,  208(%r4)     # restore F16
    lfd     %f17,  216(%r4)     # restore F17
    lfd     %f18,  224(%r4)     # restore F18
    lfd     %f19,  232(%r4)     # restore F19
    lfd     %f20,  240(%r4)     # restore F20
    lfd     %f21,  248(%r4)     # restore F21
    lfd     %f22,  256(%r4)     # restore F22
    lfd     %f23,  264(%r4)     # restore F23
    lfd     %f24,  272(%r4)     # restore F24
    lfd     %f25,  280(%r4)     # restore F25
    lfd     %f26,  288(%r4)     # restore F26
    lfd     %f27,  296(%r4)     # restore F27
    lfd     %f28,  304(%r4)     # restore F28
    lfd     %f29,  312(%r4)     # restore F29
    lfd     %f30,  320(%r4)     # restore F30
    lfd     %f31,  328(%r4)     # restore F31
    lfd     %f0,   336(%r4)     # load FPSCR
    mtfsf   0xff,  %f0          # restore FPSCR

    ld      %r0,   176(%r4)     # load PC
    mr.     %r3,   %r15         # load void pointer as argument
    mtctr   %r0                 # restore CTR

    bctr                        # start to context
.size .boost_fcontext_start, .-.boost_fcontext_start

.section ".text"
.align 2
.globl boost_fcontext_jump
.section ".opd","aw"
.align 3
boost_fcontext_jump:
.quad .boost_fcontext_jump,.TOC.@tocbase,0
.previous
.size boost_fcontext_jump,24
.type .boost_fcontext_jump,@function
.globl .boost_fcontext_jump
.boost_fcontext_jump:
    std     %r13,  0(%r3)       # save R13
    std     %r14,  8(%r3)       # save R14
    std     %r15,  16(%r3)      # save R15
    std     %r16,  24(%r3)      # save R16
    std     %r17,  32(%r3)      # save R17
    std     %r18,  40(%r3)      # save R18
    std     %r19,  48(%r3)      # save R19
    std     %r20,  56(%r3)      # save R20
    std     %r21,  64(%r3)      # save R21
    std     %r22,  72(%r3)      # save R22
    std     %r23,  80(%r3)      # save R23
    std     %r24,  88(%r3)      # save R24
    std     %r25,  96(%r3)      # save R25
    std     %r26,  104(%r3)     # save R26
    std     %r27,  112(%r3)     # save R27
    std     %r28,  120(%r3)     # save R28
    std     %r29,  128(%r3)     # save R29
    std     %r30,  136(%r3)     # save R30
    std     %r31,  144(%r3)     # save R31
    std     %r1,   152(%r3)     # save SP

    mfcr    %r0                 # load CR
    std     %r0,   160(%r3)     # save CR
    mflr    %r0                 # load LR
    std     %r0,   168(%r3)     # save LR
    std     %r0,   176(%r3)     # save LR as PC

    stfd    %f14,  192(%r3)     # save F14
    stfd    %f15,  200(%r3)     # save F15
    stfd    %f16,  208(%r3)     # save F16
    stfd    %f17,  216(%r3)     # save F17
    stfd    %f18,  224(%r3)     # save F18
    stfd    %f19,  232(%r3)     # save F19
    stfd    %f20,  240(%r3)     # save F20
    stfd    %f21,  248(%r3)     # save F21
    stfd    %f22,  256(%r3)     # save F22
    stfd    %f23,  264(%r3)     # save F23
    stfd    %f24,  272(%r3)     # save F24
    stfd    %f25,  280(%r3)     # save F25
    stfd    %f26,  288(%r3)     # save F26
    stfd    %f27,  296(%r3)     # save F27
    stfd    %f28,  304(%r3)     # save F28
    stfd    %f29,  312(%r3)     # save F29
    stfd    %f30,  320(%r3)     # save F30
    stfd    %f31,  328(%r3)     # save F31
    mffs    %f0                 # load FPSCR
    stfd    %f0,   336(%r3)     # save FPSCR


    ld      %r13,  0(%r4)       # restore R13
    ld      %r14,  8(%r4)       # restore R14
    ld      %r15,  16(%r4)      # restore R15
    ld      %r16,  24(%r4)      # restore R16
    ld      %r17,  32(%r4)      # restore R17
    ld      %r18,  40(%r4)      # restore R18
    ld      %r19,  48(%r4)      # restore R19
    ld      %r20,  56(%r4)      # restore R20
    ld      %r21,  64(%r4)      # restore R21
    ld      %r22,  72(%r4)      # restore R22
    ld      %r23,  80(%r4)      # restore R23
    ld      %r24,  88(%r4)      # restore R24
    ld      %r25,  96(%r4)      # restore R25
    ld      %r26,  104(%r4)     # restore R26
    ld      %r27,  112(%r4)     # restore R27
    ld      %r28,  120(%r4)     # restore R28
    ld      %r29,  128(%r4)     # restore R29
    ld      %r30,  136(%r4)     # restore r30
    ld      %r31,  144(%r4)     # restore r31
    ld      %r1,   152(%r4)     # restore SP

    ld      %r0,   160(%r4)     # load CR
    mtcr    %r0                 # restore CR
    ld      %r0,   168(%r4)     # load LR
    mtlr    %r0                 # restore LR

    lfd     %f14,  192(%r4)     # restore F14
    lfd     %f15,  200(%r4)     # restore F15
    lfd     %f16,  208(%r4)     # restore F16
    lfd     %f17,  216(%r4)     # restore F17
    lfd     %f18,  224(%r4)     # restore F18
    lfd     %f19,  232(%r4)     # restore F19
    lfd     %f20,  240(%r4)     # restore F20
    lfd     %f21,  248(%r4)     # restore F21
    lfd     %f22,  256(%r4)     # restore F22
    lfd     %f23,  264(%r4)     # restore F23
    lfd     %f24,  272(%r4)     # restore F24
    lfd     %f25,  280(%r4)     # restore F25
    lfd     %f26,  288(%r4)     # restore F26
    lfd     %f27,  296(%r4)     # restore F27
    lfd     %f28,  304(%r4)     # restore F28
    lfd     %f29,  312(%r4)     # restore F29
    lfd     %f30,  320(%r4)     # restore F30
    lfd     %f31,  328(%r4)     # restore F31
    lfd     %f0,   336(%r4)     # load FPSCR
    mtfsf   0xff,  %f0          # restore FPSCR

    mr.     %r3,   %r5          # use third arg as return value after jump

    ld      %r0,   176(%r4)     # load PC
    mtctr   %r0                 # restore CTR

    bctr                        # jump to context
.size .boost_fcontext_jump, .-.boost_fcontext_jump

.section ".text"
.align 2
.globl boost_fcontext_make
.section ".opd","aw"
.align 3
boost_fcontext_make:
.quad .boost_fcontext_make,.TOC.@tocbase,0
.previous
.size boost_fcontext_make,24
.type .boost_fcontext_make,@function
.globl .boost_fcontext_make
.boost_fcontext_make:
    std     %r3, 0(%r3)         # save the current context
    std     %r4, 176(%r3)       # save the address of the function supposed to be run
    std     %r5, 16(%r3)        # save void pointer
    ld      %r0, 344(%r3)       # load the stack base

    li      %r4, 56
    subf    %r1, %r4, %r1       # reserve space on stack
    stw     %r3, 48(%r1)        # store pointer to fcontext_t on stack
    mflr    %r4                 # load LR
    stw     %r4, 40(%r1)        # store LR on stack
    mr.     %r3, %r0            # context stack as arg to boost_fcontext_algin
    bl      boost_fcontext_align@plt    # call boost_fcontext_align
    mr.     %r0, %r3            # load result into R0
    lwz     %r4, 40(%r1)        # pop LR from stack
    mtlr    %r4                 # restore LR
    lwz     %r3, 48(%r1)        # pop pointer to fcontext_t from stack
    addi    %r1, %r1, 56        # release space on stack

    li      %r4, 64
    subf    %r0, %r4, %r0       # 64 bytes on stack for parameter area (== 8 registers)
    std     %r0, 152(%r3)       # save the stack base
    ld      %r0, 360(%r3)       # load address of next user context
    std     %r0, 8(%r3)         # save the next context
    mflr    %r0                 # load LR
    bl      1f                  # jump to label 1
1:
    mflr    %r4                             # load LR
    addi    %r4, %r4, link_fcontext - 1b    # calulate absolute address of link_fcontext
    mtlr    %r0                 # restore LR
    std     %r4, 168(%r3)       # save address of link_fcontext

    li      %r3,  0             # set return value to zero
    blr

link_fcontext:
    mr.     %r3, %r13
    mr.     %r4, %r14
    beq     2f                  # test if a next context was given

    bl      boost_fcontext_start@plt   # install next context

2:
    li      %r3,  0             # set return value to zero
    bl      _exit@plt           # exit application
.size .boost_fcontext_make, .-.boost_fcontext_make
