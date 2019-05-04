#include <iostream>

#include "irgen.h"
#include "msglist.h"

using namespace ffi;

auto srcJson =
#include "a.txt"
  ;

int main()
{
	MsgList *list = nullptr;

	auto ir = gen_llvm_ir( srcJson, &list );

	std::cout << "ir = \n"
			  << ir << std::endl;

	std::cout << "notes = \n";

	for ( int i = 0; i < list->len; ++i )
	{
		std::cout << "#" << i << ":"
				  << list->msgs[ i ].pos[ 0 ] << ":"
				  << list->msgs[ i ].pos[ 1 ] << ": "
				  << list->msgs[ i ].msg << std::endl;
	}

	free_llvm_ir( list, ir );

	return 0;
}