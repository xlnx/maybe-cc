project(ir-gen)

if (MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/DEBUG")
	# set(CMAKE_CXX_FLAGS_RELEASE )
else()
	set(CMAKE_CXX_FLAGS_DEBUG "-g -pg")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

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
