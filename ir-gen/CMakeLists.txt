project(ir-gen)

include_directories(
	./include
	./jsoncpp/include
)

file(GLOB SOURCE 
	./src/*.cc
	./jsoncpp/*.cc
)

add_library(ir-gen ${SOURCE})
