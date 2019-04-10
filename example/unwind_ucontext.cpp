
//          Copyright Oliver Kowalke 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <ucontext.h>
#include <unwind.h>

struct context_t {
    ucontext_t  ucf;
    ucontext_t  ucm;
};

struct X {
	int	i;
	X( int i_) : i{ i_ } { std::printf("X(%d)\n", i); }
	~X() { std::printf("~X(%d)\n", i); }
};

_Unwind_Reason_Code stop_fn( int version,
						_Unwind_Action actions,
						_Unwind_Exception_Class exc_class,
						_Unwind_Exception *exc,
						_Unwind_Context *context,
						void * param) {
    context_t * ctx = ( context_t *) param;
	if ( actions & _UA_END_OF_STACK) {
		std::printf("end of stack, jump back\n");
		_Unwind_DeleteException( exc);
		if ( -1 == ::swapcontext( & ctx->ucf, & ctx->ucm) ) {
			std::abort();
		}
	}
	return _URC_NO_REASON;
}

void foo( int i, context_t * ctx) {
	if ( 0 < i) {
		X x{ i };
		foo( --i, ctx);
	} else {
        _Unwind_Exception * exc = ( _Unwind_Exception *) std::malloc( sizeof( _Unwind_Exception) );
        std::memset( exc, 0, sizeof( * exc) );
        _Unwind_ForcedUnwind( exc, stop_fn, ctx);
    	//ctx::detail::unwind( stop_fn, ctx);
	}
	std::printf("return from foo()\n");
}

int main( int argc, char * argv[]) {
    context_t ctx;
	std::vector< char > buffer(16384, '\0');
	void * fn_stack = & buffer[0];
	std::printf("buffer : %p\n", fn_stack);
	fn_stack = ( char * )( ( ( ( ( uintptr_t) fn_stack) - 16) >> 4) << 4);
	std::printf("aligned : %p\n", fn_stack);
	if ( -1 == ::getcontext( & ctx.ucf) ) {
		std::abort();
	}	
	ctx.ucf.uc_stack.ss_sp = fn_stack;
	ctx.ucf.uc_stack.ss_size = buffer.size();
	::makecontext( & ctx.ucf, (void(*)())( foo), 2, 5, & ctx);
	if ( -1 == ::swapcontext( & ctx.ucm, & ctx.ucf) ) {
		std::abort();
	}
    std::printf("main: done\n");
    return EXIT_SUCCESS;
}
