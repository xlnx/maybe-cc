project(ir-gen)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

include_directories(
	./include
	./jsoncpp/include
)

file(GLOB SOURCE 
	./src/*.cc
	./jsoncpp/*.cc
)

add_library(ir-gen ${SOURCE})
