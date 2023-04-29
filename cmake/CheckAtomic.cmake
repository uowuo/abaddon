#[[
From https://github.com/pothosware/SoapyRTLSDR/blob/master/CheckAtomic.cmake
This file is licensed as MIT:
The MIT License (MIT)

Copyright (c) 2015 Charles J. Cliffe

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
]]

# - Try to find if atomics need -latomic linking
# Once done this will define
#  HAVE_CXX_ATOMICS_WITHOUT_LIB - Wether atomic types work without -latomic
#  HAVE_CXX_ATOMICS64_WITHOUT_LIB - Wether 64 bit atomic types work without -latomic

INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckLibraryExists)

# Sometimes linking against libatomic is required for atomic ops, if
# the platform doesn't support lock-free atomics.

function(check_working_cxx_atomics varname)
set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -std=c++11")
CHECK_CXX_SOURCE_COMPILES("
#include <atomic>
std::atomic<int> x;
int main() {
return std::atomic_is_lock_free(&x);
}
" ${varname})
set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics)

function(check_working_cxx_atomics64 varname)
set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(CMAKE_REQUIRED_FLAGS "-std=c++11 ${CMAKE_REQUIRED_FLAGS}")
CHECK_CXX_SOURCE_COMPILES("
#include <atomic>
#include <cstdint>
std::atomic<uint64_t> x (0);
int main() {
uint64_t i = x.load(std::memory_order_relaxed);
return std::atomic_is_lock_free(&x);
}
" ${varname})
set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})
endfunction(check_working_cxx_atomics64)

# Check for atomic operations.
if(MSVC)
  # This isn't necessary on MSVC.
  set(HAVE_CXX_ATOMICS_WITHOUT_LIB True)
else()
  # First check if atomics work without the library.
  check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITHOUT_LIB)
endif()

# If not, check if the library exists, and atomics work with it.
if(NOT HAVE_CXX_ATOMICS_WITHOUT_LIB)
  check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
  if(NOT HAVE_LIBATOMIC)
    message(STATUS "Host compiler appears to require libatomic, but cannot locate it.")
  endif()
  list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
  check_working_cxx_atomics(HAVE_CXX_ATOMICS_WITH_LIB)
  if (NOT HAVE_CXX_ATOMICS_WITH_LIB)
    message(FATAL_ERROR "Host compiler must support std::atomic!")
  endif()
endif()

# Check for 64 bit atomic operations.
if(MSVC)
  set(HAVE_CXX_ATOMICS64_WITHOUT_LIB True)
else()
  check_working_cxx_atomics64(HAVE_CXX_ATOMICS64_WITHOUT_LIB)
endif()

# If not, check if the library exists, and atomics work with it.
if(NOT HAVE_CXX_ATOMICS64_WITHOUT_LIB)
  check_library_exists(atomic __atomic_load_8 "" HAVE_LIBATOMIC64)
  if(NOT HAVE_LIBATOMIC64)
    message(STATUS "Host compiler appears to require libatomic, but cannot locate it.")
  endif()
  list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
  check_working_cxx_atomics64(HAVE_CXX_ATOMICS64_WITH_LIB)
  if (NOT HAVE_CXX_ATOMICS64_WITH_LIB)
    message(FATAL_ERROR "Host compiler must support std::atomic!")
  endif()
endif()

