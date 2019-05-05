#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <map>
#include <iostream>

#include "irgen.h"

using namespace ffi;
using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder( TheContext );
static std::unique_ptr<Module> TheModule;
// static std::map<std::string, Value *> NamedValues;

Value *codegen( Json::Value &node );

std::map<std::string, std::function<Value *( Json::Value & )>> handlers = {
	{ "Function", []( Json::Value &node ) -> Value * {
		 Json::Value &children = node[ "children" ];
		 auto name = children[ 1 ][ 1 ].asString();
		 Function *TheFunction = TheModule->getFunction( name );

		 if ( !TheFunction )
		 {
			 std::vector<Type *> Args;
			 FunctionType *FT = FunctionType::get( Type::getDoubleTy( TheContext ), Args, false );
			 TheFunction = Function::Create( FT, Function::ExternalLinkage, name, TheModule.get() );
		 }

		 if ( !TheFunction )
		 {
			 return nullptr;
		 }

		 BasicBlock *BB = BasicBlock::Create( TheContext, "entry", TheFunction );
		 Builder.SetInsertPoint( BB );

		 auto &Body = children[ 5 ];

		 if ( Value *RetVal = codegen( Body ) )
		 {
			 Builder.CreateRet( RetVal );
			 verifyFunction( *TheFunction );

			 return TheFunction;
		 }
	 } }
};

Value *codegen( Json::Value &node )
{
	std::string type = node[ "type" ].asString();
	if ( handlers.find( type ) != handlers.end() )
	{
		handlers[ type ]( node );
	}
	else
	{
		std::cout << "undefined: " << type << std::endl;
		return nullptr;
	}
}

char *gen_llvm_ir_cxx( const char *ast_json, MsgList &list )
{
	Json::Reader reader;
	Json::Value root;

	TheModule = make_unique<Module>( "asd", TheContext );

	if ( !reader.parse( ast_json, root ) ) return nullptr;

	for ( auto i = 0; i < root.size(); ++i )
	{
		codegen( root[ i ] );
	}

	for ( int i = 0; i < 2; i++ )
	{
		list.add_msg( MSG_TYPE_WARNING, "To DO" );
	}

	std::string cxx_ir;
	raw_string_ostream str_stream( cxx_ir );
	TheModule->print( str_stream, nullptr );
	auto ir = new char[ cxx_ir.length() + 1 ];
	memcpy( ir, cxx_ir.c_str(), cxx_ir.length() + 1 );

	return ir;
}

extern "C" {
char *gen_llvm_ir( const char *ast_json, MsgList **msg )
{
	*msg = new MsgList();
	try
	{
		return gen_llvm_ir_cxx( ast_json, **msg );
	}
	catch ( std::exception e )
	{
		( *msg )->add_msg( MSG_TYPE_ERROR,
						   std::string( "internal error: ir-gen crashed with exception: " ) + e.what() );
		return nullptr;
	}
	catch ( ... )
	{
		( *msg )->add_msg( MSG_TYPE_ERROR,
						   std::string( "internal error: ir-gen crashed with unknown error." ) );
		return nullptr;
	}
}

void free_llvm_ir( MsgList *msg, char *ir )
{
	delete msg;
	delete ir;
}
}