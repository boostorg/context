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

.file  "make_s390x_sysv_elf_gas.S"
.text
.align  4 # According to the sample code in the ELF ABI docs
.global make_fcontext
.type 	 make_fcontext, @function

make_fcontext:

		# make_fcontext takes in 3 arguments
		# arg1 --> The address where the context needs to be made
		# arg2 --> The size of the context
		# arg3 --> The address of the context function

		# According to the ELF ABI, the register R2 holds the first arg.
		# R2 also acts as the register which holds return value
		# Register R3 holds the second, R4 the third so on.

		# Shift the address in R2 to a lower 8 byte boundary

		# This is done because according to the ELF ABI Doc, the stack needs
		# to be 8 byte aligned.
		# In order to do so, we need to make sure that the address is divisible
		# by 8. We can check this, by checking if the the last 3 bits of the
		# address is zero or not. If not AND it with `-8`. 

		# Here we AND the lower 16 bits of the memory address present in the 
		# R2 with the bits 1111 1111 1111 1000 which when converted into
		# decimal is 65528
		nill    2,65528

		# Reserve space for context-data on context-stack.
		# This is done by shifting the SP/address by 112 bytes.
		lay 2,-120(2)

		# third arg of make_fcontext() == address of the context-function
		# Store the address as a PC to jump in, whenever we call the 
		# make_fcontext.
		stg 4,112(2)

		# Save the address of finish as return-address for context-function
		# This will be entered after context-function return
		# The address of finish will be saved in Link register, this register
		# specifies where we need to jump after the function executes
		# completely.
		larl 1,finish
		stg  1,64(2)

		# Return pointer to context data
		# R14 acts as the link register
		# R2 holds the address of the context stack. When we return from the
		# make_fcontext, R2 is passed back.
		br 14 

	finish:

		# In finish tasks, you load the exit code and exit the make_fcontext
		# This is called when the context-function is entirely executed

		lghi 2,0
		brasl 14,_exit

.size   make_fcontext,.-make_fcontext
# Mark that we don't need executable stack.
.section .note.GNU-stack,"",%progbits
