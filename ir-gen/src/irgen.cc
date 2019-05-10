#include "common.h"
#include "global.h"
#include "symbolMap.h"
#include "qualifiedType.h"
#include "declarationSpecifier.h"

struct VoidType
{
};

using AstType = variant<
  Value *,
  int,
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

template <typename I, typename O>
std::function<AstType( Json::Value &, const ArgsType & )>
  pack_fn( const std::function<O( Json::Value &, const I & )> &fn )
{
	return [=]( Json::Value &node, const ArgsType &arg ) -> AstType {
		const I *in;
		try
		{
			in = &get<I>( arg );
		}
		catch ( ... )
		{
			INTERNAL_ERROR( "param assertion failed at ", node );
		}
		return AstType( fn( node, *in ) );
	};
}

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

DeclarationSpecifiers handle_decl( Json::Value &node )
{
	auto &children = node[ "children" ];

	DeclarationSpecifiers declspec;

	// Collect all attribute and types.
	for ( int i = 0; i < children.size(); ++i )
	{
		auto &child = children[ i ];
		if ( child.isObject() )
		{
			declspec.add_type( get<QualifiedType>( codegen( child ) ), child );
		}
		else
		{
			declspec.add_attribute( child[ 1 ].asString(), child );
		}
	}

	return declspec;
}

std::map<std::string, std::function<AstType( Json::Value &, ArgsType const & )>> handlers = {
	{ "function_definition", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
		  Json::Value &children = node[ "children" ];
		  symTable.push();
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

		  if ( !type->isFunctionTy() )
		  {
			  infoList->add_msg( MSG_TYPE_ERROR, "expected a function defination", children[ 0 ] );
			  HALT();
		  }

		  auto fn_type = type.template as<FunctionType>();

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
	{ "compound_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
		  auto &children = node[ "children" ];
		  symTable.push();
		  // Ignore the { and }
		  for ( int i = 1; i < children.size() - 1; i++ )
		  {
			  codegen( children[ i ] );
		  }
		  dbg( symTable );
		  symTable.pop();
		  return VoidType();
	  } ) },
	{ "declaration_list", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
		  auto &children = node[ "children" ];
		  for ( int i = 0; i < children.size(); i++ )
		  {
			  codegen( children[ i ] );
		  }
		  return VoidType();
	  } ) },
	/* VERIFIED -> void */
	{ "declaration", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
		  auto &children = node[ "children" ];

		  auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );

		  for ( int i = 1; i < children.size(); ++i )
		  {
			  auto &child = children[ i ];
			  if ( child.isObject() )
			  {
				  auto builder = declspec.into_type_builder( children[ 0 ] );
				  auto &child = children[ i ];

				  {
					  auto &children = child[ "children" ];
					  // name can never be empty because "declarator" != empty
					  auto decl = get<QualifiedDecl>( codegen( children[ 0 ], &builder ) );
					  auto type = decl.type;
					  auto name = decl.name.unwrap();

					  if ( declspec.has_attribute( SC_TYPEDEF ) )  // deal with typedef
					  {
						  if ( children.size() > 1 )  // decl with init
						  {
							  infoList->add_msg( MSG_TYPE_ERROR, "only variables can be initialized", child );
							  HALT();
						  }
						  else
						  {
							  symTable.insert( name, type );
						  }
					  }
					  else
					  {
						  TODO( "storage specifiers not handled" );

						  // deal with decl
						  auto alloc = Builder.CreateAlloca( type );
						  alloc->setName( name );
						  symTable.insert( name, alloc );

						  if ( children.size() > 1 )  // decl with init
						  {
							  auto init = get<Value *>( codegen( children[ 2 ], &builder ) );
							  Builder.CreateStore( alloc, init );
						  }
						  else
						  {
							  TODO( "deal with empty initializer list" );
						  }
					  }
				  }
			  }
		  }

		  return VoidType();
	  } ) },
	/* VERIFIED -> DeclarationSpecifiers */
	{ "declaration_specifiers_i", pack_fn<VoidType, DeclarationSpecifiers>( []( Json::Value &node, VoidType const & ) -> DeclarationSpecifiers {
		  return handle_decl( node );
	  } ) },
	/* UNIMPLEMENTED -> QualifiedType */
	{ "struct_or_union_specifier", pack_fn<VoidType, QualifiedType>( []( Json::Value &node, VoidType const & ) -> QualifiedType {
		  auto &children = node[ "children" ];

		  auto struct_or_union = children[ 0 ][ 1 ].asString();
		  if ( struct_or_union == "struct" )
		  {
			  if ( children.size() < 3 )
			  {
				  UNIMPLEMENTED( "pre declaration" );
			  }
			  else
			  {
				  Json::Value *struct_decl;
				  auto la = children[ 1 ][ 0 ].asString();
				  auto has_id = la == "IDENTIFIER";
				  auto struct_ty = get<QualifiedType>( codegen( children[ has_id ? 3 : 2 ] ) );
				  if ( has_id )
				  {  // struct A without typedefs is named "struct.A"
					  auto name = "struct." + children[ 1 ][ 1 ].asString();
					  struct_ty.as<StructType>()->setName( name );

					  symTable.insert( name, struct_ty );
				  }
				  return struct_ty;
			  }
		  }
		  else
		  {
			  UNIMPLEMENTED( "union" );
		  }
	  } ) },
	{ "struct_declaration_list_i", pack_fn<VoidType, QualifiedType>( []( Json::Value &node, VoidType const & ) -> QualifiedType {
		  auto &roots = node[ "children" ];

		  std::vector<QualifiedDecl> decls;

		  for ( int i = 0; i < roots.size(); ++i )
		  {
			  auto &node = roots[ i ];
			  auto &children = node[ "children" ];

			  auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );

			  for ( int i = 1; i < children.size(); ++i )
			  {
				  auto &child = children[ i ];
				  if ( child.isObject() )
				  {
					  auto builder = declspec.into_type_builder( children[ 0 ] );
					  auto decl = get<QualifiedDecl>( codegen( children[ i ], &builder ) );
					  decls.emplace_back( std::move( decl ) );
				  }
			  }
		  }

		  auto struct_ty = QualifiedType( std::make_shared<QualifiedStruct>( decls ) );

		  return struct_ty;
	  } ) },
	/* VERIFIED -> DeclarationSpecifiers */
	{ "specifier_qualifier_list_i", pack_fn<VoidType, DeclarationSpecifiers>( []( Json::Value &node, VoidType const & ) -> DeclarationSpecifiers {
		  return handle_decl( node );
	  } ) },
	{ "userdefined_type_name", pack_fn<VoidType, QualifiedType>( []( Json::Value &node, VoidType const & ) -> QualifiedType {
		  auto name = node[ "children" ][ 0 ][ 1 ].asString();
		  if ( auto sym = symTable.find( name ) )
		  {
			  if ( sym->is_type() )
			  {
				  return sym->as_type();
			  }
			  else
			  {
				  infoList->add_msg( MSG_TYPE_ERROR, fmt( "`", name, "` does not name a type" ) );
				  HALT();
			  }
		  }
		  else
		  {
			  INTERNAL_ERROR( "symbol `", name, "` not found in current scope" );
		  }
	  } ) },
	/* UNIMPLEMENTED -> QualifiedDecl */
	{ "struct_declarator", pack_fn<QualifiedTypeBuilder *, QualifiedDecl>( []( Json::Value &node, QualifiedTypeBuilder *const &builder ) -> QualifiedDecl {
		  auto &children = node[ "children" ];
		  if ( children.size() > 1 )
		  {
			  UNIMPLEMENTED( "bit field" );
		  }
		  else
		  {
			  return get<QualifiedDecl>( codegen( children[ 0 ], builder ) );
		  }
	  } ) },
	/* UNIMPLMENTED -> QualifiedDecl */
	{ "direct_declarator", pack_fn<QualifiedTypeBuilder *, QualifiedDecl>( []( Json::Value &node, QualifiedTypeBuilder *const &builder ) -> QualifiedDecl {
		  // with arg = QualifiedTypeBuilder *
		  auto &children = node[ "children" ];

		  // direct_declarator -> direct_declarator ...
		  if ( children[ 0 ].isObject() )
		  {
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
				  std::vector<QualifiedDecl> args;
				  for ( int j = 2; j < children.size() - 1; ++j )
				  {
					  auto &child = children[ j ];
					  if ( child.isObject() )
					  {
						  auto decl = get<QualifiedDecl>( codegen( child ) );
						  symTable.insert( decl.name.unwrap(), decl.type );
						  args.emplace_back( decl );
					  }
				  }
				  builder->add_level( std::make_shared<QualifiedFunction>(
					builder->get_type(), args ) );

				  return get<QualifiedDecl>( codegen( children[ 0 ], builder ) );
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }

			  INTERNAL_ERROR();
		  }
		  else if ( children[ 0 ].isArray() )
		  {
			  auto tok = children[ 0 ][ 1 ].asString();
			  if ( tok == "(" )
			  {
				  return get<QualifiedDecl>( codegen( children[ 1 ], builder ) );
			  }
			  else
			  {
				  return QualifiedDecl( builder->build(), tok );
			  }
		  }
		  else
		  {
			  INTERNAL_ERROR();
		  }

		  INTERNAL_ERROR();
	  } ) },
	/* VERIFIED -> QualifiedDecl */
	{ "parameter_declaration", pack_fn<VoidType, QualifiedDecl>( []( Json::Value &node, VoidType const & ) -> QualifiedDecl {
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
			  auto decl = get<QualifiedDecl>( codegen( child, &builder ) );
			  //  dbg( "in parameter_declaration: ", res );
			  return decl;
		  }
	  } ) },
	/* VERIFIED -> QualifiedDecl */
	{ "declarator", pack_fn<QualifiedTypeBuilder *, QualifiedDecl>( []( Json::Value &node, QualifiedTypeBuilder *const &builder ) -> QualifiedDecl {
		  // with arg = QualifiedTypeBuilder *
		  auto &children = node[ "children" ];
		  codegen( children[ 0 ], builder );
		  return get<QualifiedDecl>( codegen( children[ 1 ], builder ) );
	  } ) },
	/* VERIFIED -> void */
	{ "pointer", pack_fn<QualifiedTypeBuilder *, VoidType>( []( Json::Value &node, QualifiedTypeBuilder *const &builder ) -> VoidType {
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
					  codegen( children[ 2 ], builder );
				  }
			  }
			  else
			  {
				  codegen( children[ 1 ], builder );
			  }
		  }

		  auto base = builder->get_type();
		  builder->add_level( std::make_shared<QualifiedPointer>(
			base,
			( type_qualifier & TQ_CONST ) != 0,
			( type_qualifier & TQ_VOLATILE ) != 0 ) );

		  return VoidType();
	  } ) },
	/* VERIFIED -> int */
	{ "type_qualifier_list_i", pack_fn<VoidType, int>( []( Json::Value &node, VoidType const & ) -> int {
		  auto &children = node[ "children" ];

		  int type_qualifier = 0;

		  for ( int i = 0; i < children.size(); ++i )
		  {
			  auto child = children[ i ][ "children" ][ 0 ][ 1 ].asString();
			  if ( child == "const" ) type_qualifier |= TQ_CONST;
			  if ( child == "volatile" ) type_qualifier |= TQ_VOLATILE;
		  }

		  return type_qualifier;
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
	std::string type = node[ "type" ].asString();
	type = type.substr( 0, 4 ) == "Expr" ? "Expr" : type;

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
		UNIMPLEMENTED( node.toStyledString() );
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