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

#include "irgen.h"

using namespace ffi;
using namespace llvm;

static LLVMContext TheContext;
static IRBuilder<> Builder( TheContext );
static std::unique_ptr<Module> TheModule;
// static std::map<std::string, Value *> NamedValues;

Value *codegen( Json::Value &node )
{
	std::string type = node[ "type" ].asString();
	Json::Value &children = node[ "children" ];
	if ( type == "S" )
	{
		for ( int i = 0; i < children.size(); i++ )
		{
			codegen( children[ i ] );
		}

		return nullptr;
	}
	else if ( type == "Function" )
	{
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
	}
	else if ( type == "Type" )
	{
		return nullptr;
	}
	else if ( type == "Block" )
	{
		return nullptr;
	}
	else if ( type == "Stmts" )
	{
		return nullptr;
	}
}

extern "C" {
char *gen_llvm_ir( const char *ast_json, MsgList **msg )
{
	Json::Reader reader;
	Json::Value root;

	*msg = new MsgList();

	TheModule = make_unique<Module>( "asd", TheContext );

	if ( !reader.parse( ast_json, root ) ) return nullptr;

	codegen( root );

	for ( int i = 0; i < 2; i++ )
	{
		( *msg )->add_msg( root[ "pos" ], "To DO" );
	}

	std::string cxx_ir;
	raw_string_ostream str_stream( cxx_ir );
	TheModule->print( str_stream, nullptr );
	auto ir = new char[ cxx_ir.length() + 1 ];
	memcpy( ir, cxx_ir.c_str(), cxx_ir.length() + 1 );

	return ir;
}

void free_llvm_ir( MsgList *msg, char *ir )
{
	delete msg;
	delete ir;
}
}