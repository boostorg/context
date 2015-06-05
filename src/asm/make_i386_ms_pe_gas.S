/*
            Copyright Oliver Kowalke 2009.
            Copyright Thomas Sailer 2013.
   Distributed under the Boost Software License, Version 1.0.
      (See accompanying file LICENSE_1_0.txt or copy at
            http://www.boost.org/LICENSE_1_0.txt)
*/

/********************************************************************
 *                                                                  *
 *  --------------------------------------------------------------  *
 *  |    0    |    1    |    2    |    3    |    4     |    5    |  *
 *  --------------------------------------------------------------  *
 *  |    0h   |   04h   |   08h   |   0ch   |   010h   |   014h  |  *
 *  --------------------------------------------------------------  *
 *  |   EDI   |   ESI   |   EBX   |   EBP   |   ESP    |   EIP   |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    6    |    7    |    8    |                              |  *
 *  --------------------------------------------------------------  *
 *  |   018h  |   01ch  |   020h  |                              |  *
 *  --------------------------------------------------------------  *
 *  |    sp   |   size  |  limit  |                              |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |    9    |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |  024h   |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |fc_execpt|                                                  |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |   10    |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |  028h   |                                                  |  *
 *  --------------------------------------------------------------  *
 *  |fc_strage|                                                  |  *
 *  --------------------------------------------------------------  *
 *  --------------------------------------------------------------  *
 *  |   11    |    12   |                                        |  *
 *  --------------------------------------------------------------  *
 *  |  02ch   |   030h  |                                        |  *
 *  --------------------------------------------------------------  *
 *  | fc_mxcsr|fc_x87_cw|                                        |  *
 *  --------------------------------------------------------------  *
 *                                                                  *
 * *****************************************************************/

.file	"make_i386_ms_pe_gas.S"
.text
.p2align 4,,15
.globl	_make_fcontext
.def	_make_fcontext;	.scl	2;	.type	32;	.endef
_make_fcontext:
    movl    0x04(%esp), %eax        /* load 1. arg of make_fcontext, pointer to context stack (base) */
    leal    -0x34(%eax),%eax        /* reserve space for fcontext_t at top of context stack */

    /* shift address in EAX to lower 16 byte boundary */
    /* == pointer to fcontext_t and address of context stack */
    andl    $-16,       %eax

    movl    0x04(%esp), %ecx        /* load 1. arg of make_fcontext, pointer to context stack (base) */
    movl    %ecx,       0x18(%eax)  /* save address of context stack (base) in fcontext_t */
    movl    0x08(%esp), %edx        /* load 2. arg of make_fcontext, context stack size */
    movl    %edx,       0x1c(%eax)  /* save context stack size in fcontext_t */
    negl    %edx                    /* negate stack size for LEA instruction (== substraction) */
    leal    (%ecx,%edx),%ecx        /* compute bottom address of context stack (limit) */
    movl    %ecx,       0x20(%eax)  /* save address of context stack (limit) in fcontext_t */
    movl    0x0c(%esp), %ecx        /* load 3. arg of make_fcontext, pointer to context function */
    movl    %ecx,       0x14(%eax)  /* save address of context function in fcontext_t */

    stmxcsr 0x02c(%eax)             /* save MMX control word */
    fnstcw  0x030(%eax)             /* save x87 control word */

    leal    -0x1c(%eax),%edx        /* reserve space for last frame and seh on context stack, (ESP - 0x4) % 16 == 0 */
    movl    %edx,       0x10(%eax)  /* save address in EDX as stack pointer for context function */

    movl    $finish,    %ecx        /* abs address of finish */
    movl    %ecx,       (%edx)      /* save address of finish as return address for context function */
                                    /* entered after context function returns */

    /* traverse current seh chain to get the last exception handler installed by Windows */
    /* note that on Windows Server 2008 and 2008 R2, SEHOP is activated by default */
    /* the exception handler chain is tested for the presence of ntdll.dll!FinalExceptionHandler */
    /* at its end by RaiseException all seh andlers are disregarded if not present and the */
    /* program is aborted */
    movl    %fs:(0x18), %ecx        /* load NT_TIB into ECX */

walk:
    movl    (%ecx),     %edx        /* load 'next' member of current SEH into EDX */
    incl    %edx                    /* test if 'next' of current SEH is last (== 0xffffffff) */
    jz      found
    decl    %edx
    xchgl    %ecx,      %edx        /* exchange content; ECX contains address of next SEH */
    jmp     walk                    /* inspect next SEH */

found:
    movl    0x04(%ecx), %ecx        /* load 'handler' member of SEH == address of last SEH handler installed by Windows */
    movl    0x10(%eax), %edx        /* load address of stack pointer for context function */
    movl    %ecx,       0x18(%edx)  /* save address in ECX as SEH handler for context */
    movl    $0xffffffff,%ecx        /* set ECX to -1 */
    movl    %ecx,       0x14(%edx)  /* save ECX as next SEH item */
    leal    0x14(%edx), %ecx        /* load address of next SEH item */
    movl    %ecx,       0x24(%eax)  /* save next SEH */

    ret

finish:
    /* ESP points to same address as ESP on entry of context function + 0x4 */
    xorl    %eax,       %eax
    movl    %eax,       (%esp)      /* exit code is zero */
    call    __exit                  /* exit application */
    hlt

.def	__exit;	.scl	2;	.type	32;	.endef  /* standard C library function */
