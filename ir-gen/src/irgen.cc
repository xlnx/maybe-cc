#include "common.h"

#include "irgen.h"
#include "utility.h"
#include "symbolMap.h"
#include "declarationSpecifier.h"

struct VoidType
{
};

using AstType = variant<
  Value *,
  int,
  std::string,
  Type *,
  QualifiedType,
  DeclarationSpecifiers,
  QualifiedDecl,
  QualifiedTypeBuilder *,
  VoidType>;

using ArgsType = variant<
  Value *,
  QualifiedTypeBuilder *,
  VoidType>;

AstType codegen( Json::Value &node, const ArgsType &arg = VoidType() );

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
std::map<std::string, std::function<AstType( Json::Value &, const ArgsType & )>> handlers = {
	{ "function_definition", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 Json::Value &children = node[ "children" ];

		 auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );
		 auto builder = declspec.into_type_builder( children[ 0 ] );

		 auto name = get<std::string>( codegen( children[ 1 ], &builder ) );
		 auto type = builder.build();

		 auto decl = QualifiedDecl( type, name );

		 //  dbg( "in function_definition: ", decl );

		 if ( !type.get_type()->isFunctionTy() )
		 {
			 infoList->add_msg( MSG_TYPE_ERROR, "expected a function defination", children[ 0 ] );
			 HALT();
		 }

		 auto fn_type = static_cast<FunctionType *>( type.get_type() );

		 auto fn = TheModule->getFunction( name );

		 if ( !fn )
		 {
			 if ( !( fn = Function::Create( fn_type, Function::ExternalLinkage, name, TheModule.get() ) ) )
			 {
				 INTERNAL_ERROR();
			 }
		 }

		 currentFunction = fn;

		 BasicBlock *BB = BasicBlock::Create( TheContext, "entry", fn );
		 currentBB = BB;
		 Builder.SetInsertPoint( BB );

		 codegen( children[ 2 ] );
		 verifyFunction( *fn );

		 return fn;
		 //  if ( auto RetVal = get<Value *>( codegen( Body ) ) )
		 //  {
		 // 	 Builder.CreateRet( RetVal );
		 //

		 // 	 return TheFunction;
		 //  }

		 //  TheFunction->eraseFromParent();
		 //  return static_cast<Value *>( nullptr );
	 } },
	{ "Stmt", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 Json::Value &children = node[ "children" ];
		 codegen( children[ 0 ] );

		 return VoidType();
	 } },
	{ "Expr", []( Json::Value &node, const ArgsType &arg ) -> AstType {
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
	{ "compound_statement", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 auto &children = node[ "children" ];

		 // Ignore the { and }
		 for ( int i = 1; i < children.size() - 1; i++ )
		 {
			 codegen( children[ i ] );
		 }
		 symTable.push();
		 return VoidType();
	 } },
	{ "declaration_list", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 auto &children = node[ "children" ];
		 for ( int i = 0; i < children.size(); i++ )
		 {
			 codegen( children[ i ] );
		 }
		 return VoidType();
	 } },
	/* VERIFIED */
	{ "declaration", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 auto &children = node[ "children" ];

		 auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );

		 for ( int i = 1; i < children.size(); ++i )
		 {
			 auto &child = children[ i ];
			 if ( child.isObject() )
			 {
				 auto builder = declspec.into_type_builder( children[ 0 ] );
				 codegen( children[ i ], &builder );
			 }
		 }

		 return VoidType();
	 } },
	/* VERIFIED */
	{ "declaration_specifiers_i", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 auto &children = node[ "children" ];

		 DeclarationSpecifiers declspec;

		 // Collect all attribute and types.
		 for ( int i = 0; i < children.size(); ++i )
		 {
			 auto &child = children[ i ];
			 auto type = child[ "type" ].asString();
			 if ( type == "type_specifier" )
			 {
				 declspec.add_type( get<Type *>( codegen( child ) ), child );
			 }
			 else
			 {
				 auto &token = child[ "children" ][ 0 ][ 1 ];
				 declspec.add_attribute( token.asString(), token );
			 }
		 }

		 return declspec;
	 } },
	{ "type_specifier", []( Json::Value &node, const ArgsType &arg ) -> AstType {
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
	/* IMPLEMENTING -> QualifiedDecl */
	{ "init_declarator", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 // with arg = QualifiedTypeBuilder *
		 auto &children = node[ "children" ];
		 // name can never be empty because "declarator" != empty
		 auto name = get<std::string>( codegen( children[ 0 ], arg ) );
		 auto builder = get<QualifiedTypeBuilder *>( arg );
		 auto type = builder->build();

		 auto alloc = Builder.CreateAlloca( type.get_type() );
		 alloc->setName( name );
		 symTable.insert( name, alloc );

		 if ( children.size() > 1 )
		 {
			 auto init = get<Value *>( codegen( children[ 2 ], arg ) );
			 Builder.CreateStore( alloc, init );
		 }
		 else
		 {
			 TODO( "deal with empty initializer list" );
		 }

		 auto res = QualifiedDecl( type, name );
		 //  dbg( "in init_declarator: ", res );
		 return res;
	 } },
	{ "direct_declarator", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 // with arg = QualifiedTypeBuilder *
		 auto &children = node[ "children" ];

		 // direct_declarator -> direct_declarator ...
		 if ( children[ 0 ].isObject() )
		 {
			 // calculate return type first!
			 children[ 0 ];
			 auto res = codegen( children[ 0 ], arg );
			 auto la = children[ 1 ][ 1 ].asString();
			 if ( la == "[" )
			 {
				 if ( children.size() == 4 )
				 {
					 UNIMPLEMENTED();
				 }
				 else
				 {
					 UNIMPLEMENTED();
				 }
			 }
			 else if ( la == "(" )
			 {
				 auto builder = get<QualifiedTypeBuilder *>( arg );
				 std::vector<QualifiedDecl> args;
				 for ( int j = 2; j < children.size() - 1; ++j )
				 {
					 auto &child = children[ j ];
					 if ( child.isObject() )
					 {
						 auto decl = get<QualifiedDecl>( codegen( child ) );
						 args.emplace_back( decl );
					 }
				 }
				 builder->add_level( std::make_shared<QualifiedFunction>(
				   builder->get_type(), args ) );

				 return res;
			 }
			 else
			 {
				 INTERNAL_ERROR();
			 }
			 return res;
		 }
		 else if ( children[ 0 ].isArray() )
		 {
			 auto tok = children[ 0 ][ 1 ].asString();
			 if ( tok == "(" )
			 {
				 return codegen( children[ 1 ], arg );
			 }
			 else
			 {
				 return tok;
			 }
		 }
		 else
		 {
			 INTERNAL_ERROR();
		 }

		 INTERNAL_ERROR();
	 } },
	/* VERIFIED -> QualifiedDecl */
	{ "parameter_declaration", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 auto &children = node[ "children" ];

		 auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );
		 auto builder = declspec.into_type_builder( children[ 0 ] );

		 if ( children.size() == 1 )
		 {
			 return QualifiedDecl( builder.build() );
		 }
		 auto &child = children[ 1 ];
		 auto type = child[ "type" ].asString();
		 if ( type == "abstract_declarator" )
		 {
			 UNIMPLEMENTED();
		 }
		 else
		 {
			 auto name = get<std::string>( codegen( child, &builder ) );
			 auto res = QualifiedDecl( builder.build(), name );
			 //  dbg( "in parameter_declaration: ", res );
			 return res;
		 }
	 } },
	/* VERIFIED */
	{ "declarator", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 // with arg = QualifiedTypeBuilder *
		 auto &children = node[ "children" ];
		 auto res = codegen( children[ 1 ], arg );
		 codegen( children[ 0 ], arg );
		 return res;
	 } },
	/* VERIFIED */
	{ "pointer", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 // with arg = QualifiedTypeBuilder *
		 auto &children = node[ "children" ];
		 int type_qualifier = 0;

		 if ( children.size() > 1 )
		 {
			 if ( children[ 1 ][ "type" ].asString() == "type_qualifier_list_i" )
			 {
				 type_qualifier = get<int>( codegen( children[ 1 ] ) );
				 if ( children.size() > 2 )
				 {
					 codegen( children[ 2 ], arg );
				 }
			 }
			 else
			 {
				 codegen( children[ 1 ], arg );
			 }
		 }

		 auto builder = get<QualifiedTypeBuilder *>( arg );
		 auto base = builder->get_type();
		 builder->add_level( std::make_shared<QualifiedPointer>(
		   base,
		   ( type_qualifier & TQ_CONST ) != 0,
		   ( type_qualifier & TQ_VOLATILE ) != 0 ) );

		 return VoidType();
	 } },
	{ "type_qualifier_list_i", []( Json::Value &node, const ArgsType &arg ) -> AstType {
		 auto &children = node[ "children" ];

		 int type_qualifier = 0;

		 for ( int i = 0; i < children.size(); ++i )
		 {
			 auto child = children[ i ][ "children" ][ 0 ][ 1 ].asString();
			 if ( child == "const" ) type_qualifier |= TQ_CONST;
			 if ( child == "volatile" ) type_qualifier |= TQ_VOLATILE;
		 }

		 return type_qualifier;
	 } },
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

AstType codegen( Json::Value &node, const ArgsType &arg )
{
	std::string type = node[ "type" ].asString();
	type = type.substr( 0, 4 ) == "Expr" ? "Expr" : type;

	if ( stackTrace )
	{
		dbg( "+ ", type );
	}

	if ( handlers.find( type ) != handlers.end() )
	{
		auto res = handlers[ type ]( node, arg );
		if ( stackTrace )
		{
			dbg( "- ", type );
		}
		return res;
	}
	else
	{
		//  UNIMPLEMENTED( node.toStyledString() );
		dbg( "U ", type );
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