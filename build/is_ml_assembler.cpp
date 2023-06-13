//          Copyright Nikita Kniazev 2023.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// This should compile only when ml.exe assembler will be used

#if !( defined(_WIN32) && defined(_M_IX86) && !defined(__GNUC__) )
# error "Not Windows + x86 32bit (excluding GCC, including Clang-cl)"
#endif
