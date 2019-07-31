/*******************************************************
*                                                     *
*  -------------------------------------------------  *
*  |  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  *
*  -------------------------------------------------  *
*  |     0     |     8     |    16     |     24    |  *
*  -------------------------------------------------  *
*  |    R6     |    R7     |    R8     |    R9     |  *
*  -------------------------------------------------  *
*  -------------------------------------------------  *
*  |  8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |  *
*  -------------------------------------------------  *
*  |     32    |    40     |     48    |     56    |  *
*  -------------------------------------------------  *
*  |    R10    |    R11    |     R12   |     R13   |  *
*  -------------------------------------------------  *
*  -------------------------------------------------  *
*  |  16 |  17 |  18 |  19 |  20 |  21 |  22 |  23 |  *
*  -------------------------------------------------  *
*  |     64    |    72     |     80    |     88    |  *
*  -------------------------------------------------  *
*  |   R14/LR  |    R15    |     F1    |     F3    |  *
*  -------------------------------------------------  *
*  -------------------------------------------------  *
*  |  24 |  25 |  26 |  27 |  28 | 29  |           |  *
*  -------------------------------------------------  *
*  |     96    |    104    |    112    |    120    |  *
*  -------------------------------------------------  *
*  |    F5     |    F7     |     PC    |           |  *
*  -------------------------------------------------  *
* *****************************************************/

.file  "jump_s390x_sysv_elf_gas.S"
.text
.align  4 # According to the sample code in the ELF ABI docs
.global jump_fcontext
.type   jump_fcontext, @function

jump_fcontext:
    
    # Reserved the space for stack to store the data of current context
    # before we jump to the new context.
    lay 15,-120(15)

    # save the registers to the stack
    stg 6,  0(15)       # save R6     
    stg 7,  8(15)       # save R7     
    stg 8,  16(15)      # save R8
    stg 9,  24(15)      # save R9
    stg 10, 32(15)      # save R10
    stg 11, 40(15)      # save R11
    stg 12, 48(15)      # save R12
    stg 13, 56(15)      # save R13
    stg 14, 64(15)      # save R14
    stg 15, 72(15)      # save R15

    # save the floating point registers
    # Load the FPR into R0 then save it to the stack
    # Load F1 into R0
    lgdr 0,1 
    stg  0,80(15)       # save F1

    # Load F3 into R0
    lgdr 0,3
    stg  0,88(15)       # save F3

    # Load F5 into R0
    lgdr 0,5
    stg  0,96(15)       # save F5

    # Load F7 into R0
    lgdr 0,7
    stg  0,104(15)      # save F7

    # Save LR as PC
    stg 14,112(15)

    # Store the SP pointing to the old context-data into R0
    lgr 0,15

    # Get the SP pointing to the new context-data
    # Note: Since the return type of the jump_fcontext is struct whose
    # size is more than 8. The compiler automatically passes the 
    # address of the transfer_t where the data needs to store into R2.

    # Hence the first param passed to the jump_fcontext which represent
    # the fctx we want to switch to is present in R3
    # R2 --> Address of the return transfer_t struct
    # R3 --> Context we want to switch to
    # R4 --> Data
    lgr 15,3

    # Load the registers with the data present in context-data of the
    # context we are going to switch to
    lg 6,  0(15)       # restore R6     
    lg 7,  8(15)       # restore R7     
    lg 8,  16(15)      # restore R8
    lg 9,  24(15)      # restore R9
    lg 10, 32(15)      # restore R10
    lg 11, 40(15)      # restore R11
    lg 12, 48(15)      # restore R12
    lg 13, 56(15)      # restore R13
    lg 14, 64(15)      # restore R14
    lg 15, 72(15)      # restore R15

    # Restore Floating point registers
    lg   1,80(15)
    ldgr 1,1            # restore F1

    lg   1,88(15)
    ldgr 1,3            # restore F3

    lg   1,96(15)
    ldgr 1,5            # restore F5

    lg   1,104(15)
    ldgr 1,7            # restore F7

    # Load PC
    lg  1,112(15)

    # Adjust the stack
    lay 15, 120(15)

    # R2 --> Address where the return transfer_t is stored
    # R0 --> FCTX
    # R4 --> DATA

    # Store the elements to return transfer_t
    lg 0, 0(2)
    lg 4, 8(2)

    # Note: The address in R2 points to the place where the return
    # transfer_t is stored. Since context_function take transfer_t
    # as first parameter. And R2 is the register which holds the
    # first parameter value.

    #jump to context
    br 1

.size   jump_fcontext,.-jump_fcontext
# Mark that we don't need executable stack.
.section .note.GNU-stack,"",%progbits



