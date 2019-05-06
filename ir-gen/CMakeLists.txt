project(ir-gen)

set(CMAKE_CXX_FLAGS_DEBUG "-g -pg")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(
	./include
	./jsoncpp/include
	./variant-lite
)

file(GLOB SOURCE 
	./src/*.cc
	./jsoncpp/*.cc
)

add_library(ir-gen ${SOURCE})
