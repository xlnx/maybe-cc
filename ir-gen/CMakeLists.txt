project(ir-gen)

include_directories(
	./include
	./jsoncpp/include
	./variant-lite
)

file(GLOB SOURCE 
	./src/**/*.cc
	./src/*.cc
	./jsoncpp/**/*.cc
	./jsoncpp/*.cc
)

add_library(ir-gen ${SOURCE})
