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
#include "symbolMap.h"

using namespace ffi;
using namespace llvm;
using namespace nonstd;

static LLVMContext TheContext;
static IRBuilder<> Builder( TheContext );
static std::unique_ptr<Module> TheModule;
static std::map<std::string, Value *> NamedValues;
static Function *currentFunction;
static BasicBlock *currentBB;
static MsgList *infoList;

struct VoidType
{
};

using AstType = variant<Value *, Type *, std::string, VoidType>;

AstType codegen( Json::Value &node );
std::map<std::string, std::function<Value *( Value *, Value * )>> binaryOps = {
	{ "=", []( Value *lhs, Value *rhs ) { Builder.CreateStore( rhs, lhs ); return lhs; } },
	{ "+=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateFAdd( lhs, rhs ), lhs ); } },
	{ "-=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateFSub( lhs, rhs ), lhs ); } },
	{ "*=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateFMul( lhs, rhs ), lhs ); } },
	{ "/=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateFDiv( lhs, rhs ), lhs ); } },
	{ "%=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateFRem( lhs, rhs ), lhs ); } },
	{ "<<=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateShl( lhs, rhs ), lhs ); } },
	{ "&=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateAnd( lhs, rhs ), lhs ); } },
	{ "^=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateXor( lhs, rhs ), lhs ); } },
	{ "|=", []( Value *lhs, Value *rhs ) { return Builder.CreateStore( Builder.CreateOr( lhs, rhs ), lhs ); } },
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
	{ "function_definition", []( Json::Value &node ) -> AstType {
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

		 currentFunction = TheFunction;

		 BasicBlock *BB = BasicBlock::Create( TheContext, "entry", TheFunction );
		 currentBB = BB;
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
			 int ret = 1;
			 if ( children[ 0 ][ 0 ].asString() == "Number" )
			 {
				 std::stringstream ss( children[ 0 ][ 1 ].asString() );
				 ss >> ret;
			 }
			 return ConstantInt::get( TheContext, APInt( 32, ret, true ) );  // TO DO variable
																			 //  else if ( children[ 0 ][ 0 ].asString() == "Id" )
																			 //  {
																			 // 	 std::string variableName = children[ 0 ][ 1 ].asString();
																			 // 	 return ConstantArray::get( TheContext, variableName.c_str() );
																			 //  }
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
			 auto lhs = get<Value *>( codegen( children[ 0 ] ) );
			 auto mid = get<Value *>( codegen( children[ 2 ] ) );

			 UNIMPLEMENTED( node.toStyledString() );
		 }
		 else if ( children.size() == 5 )
		 {
			 UNIMPLEMENTED( node.toStyledString() );
		 }
		 else
		 {
			 UNIMPLEMENTED( node.toStyledString() );
		 }
	 } },
	{ "compound_statement", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];

		 // Ignore the { and }
		 for ( int i = 1; i < children.size() - 1; i++ )
		 {
			 codegen( children[ i ] );
		 }

		 return VoidType();
	 } },
	{ "declaration_list", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];
		 for ( int i = 0; i < children.size(); i++ )
		 {
			 codegen( children[ i ] );
		 }
		 return VoidType();
	 } },
	{ "declaration", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];
		 // Ignore the ;
		 Type *type = get<Type *>( codegen( children[ 0 ] ) );

		 UNIMPLEMENTED( "declaration to do" );
		 return VoidType();
	 } },
	{ "declaration_specifiers_i", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];

		 if ( children.size() != 1 )
		 {
			 infoList->add_msg( MSG_TYPE_ERROR, "muti type specifiers" );
		 }
		 Type *retType = get<Type *>( codegen( children[ 0 ] ) );
		 return retType;
		 //  Builder.SetInsertPoint( currentBB );
		 //  auto allocated = Builder.CreateAlloca( retType );
		 //  allocated->setName( children[ 1 ][ 1 ].asString() );
		 //  auto lv = new AllocaInst( retType, 0, children[ 1 ][ 1 ].asString(), currentBB );
	 } },
	{ "type_specifier", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];
		 auto variableType = children[ 0 ][ 1 ].asString();
		 std::map<std::string, std::function<Type *()>> typeTable = {
			 { "int", []() { return Type::getInt32Ty( TheContext ); } }
		 };

		 if ( typeTable.find( variableType ) == typeTable.end() )
		 {
			 UNIMPLEMENTED( "unimplemented type" );
		 }
		 return typeTable[ variableType ]();
	 } },
	{ "init_declarator_list_i", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];

		 for ( int i = 0; i < children.size(); i++ )
		 {
			 if ( !children[ i ][ "type" ].isNull() )
			 {
				 codegen( children[ i ] );
			 }
		 }
		 UNIMPLEMENTED();
		 //return VoidType();
	 } },
	{ "init_declarator", []( Json::Value &node ) -> AstType {
		 UNIMPLEMENTED();
	 } },
	{ "initializer", []( Json::Value &node ) -> AstType {
		 auto &children = node[ "children" ];
		 if ( children[ 0 ][ "type" ].isNull() )
		 {
			 auto type = children[ 0 ][ 0 ].asString();
			 if ( type == "CONSTANT" )
			 {
				 return children[ 0 ][ 1 ].asString();
			 }
			 else if ( type == "IDENTIFIER" )
			 {
			 }
		 }
		 else
		 {
			 UNIMPLEMENTED( "initializer" );
		 }
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

	infoList = &list;

	TheModule = make_unique<Module>( "asd", TheContext );

	if ( !reader.parse( ast_json, root ) ) return nullptr;

	for ( auto i = 0; i < root.size(); ++i )
	{
		codegen( root[ i ] );
	}

	for ( int i = 0; i < 2; i++ )
	{
		infoList->add_msg( MSG_TYPE_WARNING, "To DO" );
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