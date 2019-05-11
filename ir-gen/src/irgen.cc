#include "common.h"
#include "global.h"
#include "node/def.h"

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

JumpTable<NodeHandler> handlers = {
	{ "function_definition", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
		  Json::Value &children = node[ "children" ];

		  auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );

		  if ( declspec.has_attribute( SC_TYPEDEF ) )
		  {
			  infoList->add_msg( MSG_TYPE_ERROR, "function definition declared `typedef`", children[ 0 ] );
			  HALT();
		  }
		  if ( declspec.has_attribute( SC_REGISTER ) || declspec.has_attribute( SC_AUTO ) )
		  {
			  infoList->add_msg( MSG_TYPE_ERROR, "illegal storage class on function", children[ 0 ] );
			  HALT();
		  }

		  auto builder = declspec.into_type_builder( children[ 0 ] );

		  auto decl = get<QualifiedDecl>( codegen( children[ 1 ], &builder ) );
		  auto type = decl.type;
		  auto name = decl.name.unwrap();

		  //  dbg( "in function_definition: ", decl );

		  if ( !type.is<mty::Function>() )
		  {
			  infoList->add_msg( MSG_TYPE_ERROR, "expected a function defination", children[ 0 ] );
			  HALT();
		  }

		  auto fn_type = type.as<FunctionType>();

		  auto fn = TheModule->getFunction( name );

		  if ( !fn )
		  {
			  if ( !( fn = Function::Create( fn_type, Function::ExternalLinkage, name, TheModule.get() ) ) )
			  {
				  INTERNAL_ERROR();
			  }
		  }

		  symTable.insert( name, QualifiedValue( std::make_shared<QualifiedType>( type ), fn, false ) );

		  currentFunction = fn;
		  BasicBlock *BB = BasicBlock::Create( TheContext, "entry", fn );
		  currentBB = BB;
		  Builder.SetInsertPoint( BB );

		  symTable.push();

		  //To codegen block
		  auto &basicBlock = children[ 2 ][ "children" ];
		  for ( int i = 1; i < basicBlock.size() - 1; i++ )
		  {
			  codegen( basicBlock[ i ] );
		  }

		  verifyFunction( *fn );

		  dbg( symTable );
		  symTable.pop();
		  return VoidType{};
	  } ) },

	{ "initializer", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 auto &children = node[ "children" ];
		 if ( children[ 0 ][ "type" ].isNull() )
		 {
			 auto type = children[ 0 ][ 0 ].asString();
			 if ( type == "CONSTANT" )
			 {
				 //return children[ 0 ][ 1 ].asString();
			 }
			 else if ( type == "IDENTIFIER" )
			 {
			 }
			 UNIMPLEMENTED( "initializer" );
		 }
		 else
		 {
			 UNIMPLEMENTED( "initializer" );
		 }
	 } }
};

static auto volatile __decl = Declaration::reg();
static auto volatile __stmt = Statement::reg();
static auto volatile __expr = Expression::reg();

std::string indent( int num )
{
	std::string res;
	for ( int i = 0; i < num; ++i )
	{
		res += "  ";
	}
	return res;
}

AstType codegen( Json::Value &node, const ArgsType &arg )
{
	auto type = node[ "type" ].asCString();

	static int ind = 0;

	if ( stack_trace )
	{
		dbg( indent( ind++ ), "+ ", type );
	}

	if ( handlers.find( type ) != handlers.end() )
	{
		auto res = handlers[ type ]( node, arg );
		if ( stack_trace )
		{
			dbg( indent( --ind ), "- ", type );
		}
		return res;
	}
	else
	{
		UNIMPLEMENTED( node );
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
	catch ( int )
	{
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