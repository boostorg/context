#!/bin/sh

g++ -x assembler-with-cpp -fPIC -O3 -finline-functions -Wno-inline -Wall -march=native -DBOOST_ALL_NO_LIB=1 -DBOOST_CONTEXT_DYN_LINK=1 -DNDEBUG -I../../.. -c -o fcontext_x86_64_sysv_elf_gas.o ../src/asm/fcontext_x86_64_sysv_elf_gas.S 

g++ -ftemplate-depth-128 -fPIC -O3 -finline-functions -Wno-inline -Wall -march=native -DBOOST_ALL_NO_LIB=1 -DBOOST_CONTEXT_DYN_LINK=1 -DNDEBUG -I../../.. -c -o fcontext.o ../src/fcontext.cpp 

g++ -ftemplate-depth-128 -fPIC -O3 -finline-functions -Wno-inline -Wall -march=native -DBOOST_ALL_NO_LIB=1 -DBOOST_CONTEXT_DYN_LINK=1 -DNDEBUG -I../../.. -c -o stack_allocator_posix.o ../src/stack_allocator_posix.cpp

g++ -ftemplate-depth-128 -fPIC -O3 -finline-functions -Wno-inline -Wall -march=native -DBOOST_ALL_NO_LIB=1 -DBOOST_CONTEXT_DYN_LINK=1 -DNDEBUG -I../../.. -c -o stack_utils_posix.o ../src/stack_utils_posix.cpp

gcc -shared -Wl,-soname,libboost_my_lib.so -o libboost_my_lib.so fcontext.o stack_allocator_posix.o stack_utils_posix.o fcontext_x86_64_sysv_elf_gas.o -lc -lstdc++

gcc -g ./transfer.cpp -I../../../ -L./ -Wl,-rpath=./ -lboost_my_lib -lrt 
