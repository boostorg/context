//          Copyright Nikita Kniazev 2023.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/predef/architecture.h>
#include <boost/predef/other/wordsize.h>
#include <boost/predef/os.h>

// Predef bug https://github.com/boostorg/predef/issues/127
#if defined(BOOST_ARCH_MIPS_AVAILABLE) && !defined(__mips64)
#  undef BOOST_ARCH_WORD_BITS
#  define BOOST_ARCH_WORD_BITS 32
#endif

#if defined(BOOST_CONTEXT_ARCH)
// Defined by user
#elif defined(BOOST_ARCH_X86_64_AVAILABLE)
# define BOOST_CONTEXT_ARCH x86_64
#elif defined(BOOST_ARCH_X86_32_AVAILABLE)
# undef i386
# define BOOST_CONTEXT_ARCH i386
#elif defined(BOOST_ARCH_ARM_AVAILABLE) && BOOST_ARCH_WORD_BITS == 64
# define BOOST_CONTEXT_ARCH arm64
#elif defined(BOOST_ARCH_ARM_AVAILABLE) && BOOST_ARCH_WORD_BITS == 32
# define BOOST_CONTEXT_ARCH arm
#elif defined(BOOST_ARCH_MIPS_AVAILABLE) && BOOST_ARCH_WORD_BITS == 64
# define BOOST_CONTEXT_ARCH mips64
#elif defined(BOOST_ARCH_MIPS_AVAILABLE) && BOOST_ARCH_WORD_BITS == 32
# define BOOST_CONTEXT_ARCH mips32
#elif defined(BOOST_ARCH_PPC_64_AVAILABLE)
# define BOOST_CONTEXT_ARCH ppc64
#elif defined(BOOST_ARCH_PPC_AVAILABLE)
# define BOOST_CONTEXT_ARCH ppc32
#elif defined(BOOST_ARCH_RISCV_AVAILABLE)
# define BOOST_CONTEXT_ARCH riscv64
#elif defined(BOOST_ARCH_SYS390_AVAILABLE)
# define BOOST_CONTEXT_ARCH s390x
#elif defined(BOOST_ARCH_LOONGARCH_AVAILABLE)
# define BOOST_CONTEXT_ARCH loongarch64
#else
# error "Unsupported architecture"
#endif

#if defined(BOOST_CONTEXT_ABI)
// Defined by user
#elif defined(BOOST_ARCH_ARM_AVAILABLE)
# define BOOST_CONTEXT_ABI aapcs
#elif defined(BOOST_ARCH_MIPS_AVAILABLE) && BOOST_ARCH_WORD_BITS == 64
# define BOOST_CONTEXT_ABI n64
#elif defined(BOOST_ARCH_MIPS_AVAILABLE) && BOOST_ARCH_WORD_BITS == 32
# define BOOST_CONTEXT_ABI o32
#elif defined(BOOST_OS_WINDOWS_AVAILABLE)
# define BOOST_CONTEXT_ABI ms
#else
# define BOOST_CONTEXT_ABI sysv
#endif

#if defined(BOOST_CONTEXT_BINFMT)
// Defined by user
#elif defined(BOOST_OS_WINDOWS_AVAILABLE)
# define BOOST_CONTEXT_BINFMT pe
#elif defined(BOOST_OS_AIX_AVAILABLE)
# define BOOST_CONTEXT_BINFMT xcoff
#elif defined(BOOST_OS_MACOS_AVAILABLE)
# define BOOST_CONTEXT_BINFMT macho
#else
# define BOOST_CONTEXT_BINFMT elf
#endif

#if defined(BOOST_CONTEXT_ASSEMBLER)
// Defined by user
#elif defined(__GNUC__)
# define BOOST_CONTEXT_ASSEMBLER gas.S
#elif defined(BOOST_ARCH_ARM_AVAILABLE)
# define BOOST_CONTEXT_ASSEMBLER armasm.asm
#elif defined(BOOST_OS_WINDOWS_AVAILABLE)
# define BOOST_CONTEXT_ASSEMBLER masm.asm
#else
# define BOOST_CONTEXT_ASSEMBLER gas.S
#endif

#include <boost/config/helper_macros.hpp>

#define BOOST_CONTEXT_ASM_INCLUDE(name) \
  BOOST_STRINGIZE(BOOST_JOIN(BOOST_JOIN(BOOST_JOIN(BOOST_JOIN(BOOST_JOIN(BOOST_JOIN(BOOST_JOIN(BOOST_JOIN( \
    name, _), BOOST_CONTEXT_ARCH), _), BOOST_CONTEXT_ABI), _), BOOST_CONTEXT_BINFMT), _), BOOST_CONTEXT_ASSEMBLER))

