
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXTS_FLAGS_H
#define BOOST_CONTEXTS_FLAGS_H

namespace boost {
namespace contexts {

enum flag_unwind_t
{
	stack_unwind = 0,
	no_stack_unwind
};

enum flag_return_t
{
	return_to_caller = 0,
	exit_application
};

}}

#endif // BOOST_CONTEXTS_FLAGS_H
