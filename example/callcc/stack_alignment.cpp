#include <cstdlib>
#include <cstring>
#include <array>
#include <iostream>
#include <boost/context/continuation.hpp>

void stack_overflow(int n)
{
  // Silence warnings about infinite recursion
  if (n == 0xdeadbeef) return;
  // Allocate more than 4k bytes on the stack.
  std::array<char, 8192> blob;
  // Repeat...
  stack_overflow(n + 1);

  // Prevent blob from being optimized away
  std::memcpy(blob.data(), blob.data(), n);
  std::cout << blob.data();
}

int main(int argc, char* argv[])
{
  using namespace boost::context;

  // Calling stack_overflow outside of Boost context results in a regular stack
  // overflow exception.
  // stack_overflow(0);

  callcc([](continuation && sink) {
    // Calling stack_overflow inside results in a memory access violation caused
    // by the compiler generated _chkstk function.
    stack_overflow(0);
    return std::move( sink);
  });
  return EXIT_SUCCESS;
}
