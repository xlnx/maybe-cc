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
#include <iostream>
#include <map>

#include <variant.hpp>

#include "irgen.h"
#include "util.h"

using namespace ffi;
using namespace llvm;
using namespace nonstd;

static LLVMContext TheContext;
static IRBuilder<> Builder( TheContext );
static std::unique_ptr<Module> TheModule;
// static std::map<std::string, Value *> NamedValues;

struct VoidType
{
};

using AstType = variant<Value *, Type *, std::string, VoidType>;

AstType codegen( Json::Value &node );
std::map<std::string, std::function<Value *( Value *, Value * )>> binaryOps = {
	{ "=", []( Value *lhs, Value *rhs ) { Builder.CreateStore( rhs, lhs ); return lhs; } },
	// { "+=", []( Value *, Value * ) { return nullptr } },
	// { "-=", []( Value *, Value * ) { return nullptr } },
	// { "*=", []( Value *, Value * ) { return nullptr } },
	// { "/=", []( Value *, Value * ) { return nullptr } },
	// { "%=", []( Value *, Value * ) { return nullptr } },
	// { "<<=", []( Value *, Value * ) { return nullptr } },
	// { "&=", []( Value *, Value * ) { return nullptr } },
	// { "^=", []( Value *, Value * ) { return nullptr } },
	// { "|=", []( Value *, Value * ) { return nullptr } },
	// { "=", []( Value *, Value * ) { return nullptr } },
	// { "=", []( Value *, Value * ) { return nullptr } },
	{ "||", []( Value *lhs, Value *rhs ) { return Builder.CreateOr( lhs, rhs ); } },
	{ "&&", []( Value *lhs, Value *rhs ) { return Builder.CreateAnd( lhs, rhs ); } },
	{ "|", []( Value *lhs, Value *rhs ) { return Builder.CreateOr( lhs, rhs ); } },
	{ "^", []( Value *lhs, Value *rhs ) { return Builder.CreateXor( lhs, rhs ); } },
	{ "&", []( Value *lhs, Value *rhs ) { return Builder.CreateAnd( lhs, rhs ); } },
	{ "==", []( Value *lhs, Value *rhs ) { return Builder.CreateFCmpOEQ( lhs, rhs ); } },
	{ "!=", []( Value *lhs, Value *rhs ) { return Builder.CreateFCmpONE( lhs, rhs ); } },
	{ "<", []( Value *lhs, Value *rhs ) { return Builder.CreateFCmpOLT( lhs, rhs ); } },
	{ ">", []( Value *lhs, Value *rhs ) { return Builder.CreateFCmpOGT( lhs, rhs ); } },
	{ "<=", []( Value *lhs, Value *rhs ) { return Builder.CreateFCmpOLE( lhs, rhs ); } },
	{ ">=", []( Value *lhs, Value *rhs ) { return Builder.CreateFCmpOGE( lhs, rhs ); } },
	{ "<<", []( Value *lhs, Value *rhs ) { return Builder.CreateShl( lhs, rhs ); } },
	{ ">>", []( Value *lhs, Value *rhs ) { return Builder.CreateAShr( lhs, rhs ); } },
	{ "+", []( Value *lhs, Value *rhs ) { return Builder.CreateFAdd( lhs, rhs ); } },
	{ "-", []( Value *lhs, Value *rhs ) { return Builder.CreateFSub( lhs, rhs ); } },
	{ "*", []( Value *lhs, Value *rhs ) { return Builder.CreateFMul( lhs, rhs ); } },
	{ "/", []( Value *lhs, Value *rhs ) { return Builder.CreateFDiv( lhs, rhs ); } },
	{ "%", []( Value *lhs, Value *rhs ) { return Builder.CreateFRem( lhs, rhs ); } },
};
std::map<std::string, std::function<AstType( Json::Value & )>> handlers = {
	{ "Function", []( Json::Value &node ) -> AstType {
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
			 return static_cast<Value *>( nullptr );
		 }

		 BasicBlock *BB = BasicBlock::Create( TheContext, "entry", TheFunction );
		 Builder.SetInsertPoint( BB );

		 auto &Body = children[ 5 ];
		 codegen( Body );
		 verifyFunction( *TheFunction );
		 return TheFunction;
		 //  if ( auto RetVal = get<Value *>( codegen( Body ) ) )
		 //  {
		 // 	 Builder.CreateRet( RetVal );
		 //

		 // 	 return TheFunction;
		 //  }

		 //  TheFunction->eraseFromParent();
		 //  return static_cast<Value *>( nullptr );
	 } },
	{ "Stmt", []( Json::Value &node ) -> AstType {
		 Json::Value &children = node[ "children" ];
		 codegen( children[ 0 ] );

		 return VoidType();
	 } },
	{ "Expr", []( Json::Value &node ) -> AstType {
		 Json::Value &children = node[ "children" ];

		 if ( children.size() == 1 )
		 {
			 float ret = 1.f;

			 if ( children[ 0 ][ 0 ] == "Number" )
			 {
				 std::stringstream ss( children[ 0 ][ 1 ].asString() );
				 ss >> ret;
			 }
			 return ConstantFP::get( TheContext, APFloat( ret ) );  // TO DO variable
		 }
		 else if ( children.size() == 2 )
		 {
			 std::string operation;
			 Value *right;

			 if ( children[ 0 ][ "type" ].isNull() )
			 {
				 operation = children[ 0 ][ 1 ].asString();
				 right = get<Value *>( codegen( children[ 1 ] ) );
			 }
			 else
			 {
				 operation = children[ 1 ][ 1 ].asString();
				 right = get<Value *>( codegen( children[ 0 ] ) );
			 }

			 std::map<std::string, std::function<Value *( Value * )>> unaryOps = {
				 { "++", []( Value *rhs ) { return Builder.CreateFAdd( ConstantFP::get( TheContext, APFloat( 1.f ) ), rhs ); } },
				 { "--", []( Value *rhs ) { return Builder.CreateFAdd( ConstantFP::get( TheContext, APFloat( -1.f ) ), rhs ); } },
				 { "+", []( Value *rhs ) { return rhs; } },
				 { "-", []( Value *rhs ) { return Builder.CreateFNeg( rhs ); } },
				 { "~", []( Value *rhs ) { return Builder.CreateNot( rhs ); } },
				 { "!", []( Value *rhs ) { return Builder.CreateNot( rhs ); } }
			 };

			 if ( unaryOps.find( operation ) == unaryOps.end() )
			 {
				 UNIMPLEMENTED( "operator ", operation, "\nat node: ", node.toStyledString() );
			 }
			 else
			 {
				 return unaryOps[ operation ]( right );
			 }
		 }
		 else if ( children.size() == 3 )
		 {
			 std::string operation = children[ 1 ][ 1 ].asString();
			 auto lhs = get<Value *>( codegen( children[ 0 ] ) );
			 auto rhs = get<Value *>( codegen( children[ 2 ] ) );

			 if ( binaryOps.find( operation ) == binaryOps.end() )
			 {
				 UNIMPLEMENTED( "operator ", operation, "\nat node: ", node.toStyledString() );
			 }

			 return binaryOps[ operation ]( lhs, rhs );
		 }
		 else if ( children.size() == 4 )
		 {
			 UNIMPLEMENTED( node.toStyledString() );
		 }
		 else if ( children.size() == 5 )
		 {
			 return static_cast<Value *>( nullptr );
		 }
		 else
		 {
			 UNIMPLEMENTED( node.toStyledString() );
		 }
	 } },
	{ "Block", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];

		 for ( int i = 1; i < children.size() - 1; i++ )
		 {
			 codegen( children[ i ] );
		 }

		 return VoidType();
	 } }
};

AstType codegen( Json::Value &node )
{
	std::string type = node[ "type" ].asString();
	type = type.substr( 0, 4 ) == "Expr" ? "Expr" : type;
	if ( handlers.find( type ) != handlers.end() )
	{
		return handlers[ type ]( node );
	}
	else
	{
		//  UNIMPLEMENTED( node.toStyledString() );
		std::cout << "undefined: " << type << std::endl;
		return static_cast<Value *>( nullptr );
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
	TheModule->print( errs(), nullptr );
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
	catch ( std::exception &e )
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