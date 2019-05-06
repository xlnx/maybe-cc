cmake_minimum_required(VERSION 3.0)
project(maybe-cc)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-rs/cmake")

enable_language(Rust)
include(CMakeCargo)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

add_executable(mcc ./null.c)

cargo_build(NAME fe-lib)
add_subdirectory(ir-gen)

if(WIN32)
    target_link_libraries(mcc fe-lib ws2_32 userenv)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(mcc fe-lib resolv)
else()
    target_link_libraries(mcc fe-lib pthread dl)
endif()

target_link_libraries(mcc ir-gen)
target_link_libraries(mcc LLVM-6.0)
