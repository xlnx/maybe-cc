cmake_minimum_required(VERSION 2.8)
project(ir-gen CXX)

set(CMAKE_CXX_STANDARD 11)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(
	${CMAKE_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/jsoncpp/include
)

file(GLOB SOURCE 
	${CMAKE_SOURCE_DIR}/src/*.cc
	${CMAKE_SOURCE_DIR}/jsoncpp/*.cc
)

add_library(ir-gen STATIC ${SOURCE})

install(TARGETS ir-gen DESTINATION .)
