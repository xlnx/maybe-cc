#include "declaration.h"

static DeclarationSpecifiers handle_decl( Json::Value &node, bool has_def )
{
	auto &children = node[ "children" ];

	DeclarationSpecifiers declspec;

	// Collect all attribute and types.
	for ( int i = 0; i < children.size(); ++i )
	{
		auto &child = children[ i ];
		if ( !child.isObject() )
		{
			declspec.add_attribute( child[ 1 ].asCString(), child );
		}
	}

	bool curr_scope = !declspec.has_attribute( STORAGE_SPECIFIER ) && !has_def;

	for ( int i = 0; i < children.size(); ++i )
	{
		auto &child = children[ i ];
		if ( child.isObject() )
		{
			declspec.add_type( get<QualifiedType>( codegen( child, curr_scope ) ), child );
		}
	}

	return declspec;
}

static QualifiedDecl handle_function_array( Json::Value &node, QualifiedTypeBuilder *const &builder, int an )
{
	auto &children = node[ "children" ];

	auto la = *children[ an ][ 1 ].asCString();
	if ( la == '[' )
	{
		auto elem_ty = builder->get_type();
		if ( !elem_ty->is_complete() )
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "array declared with incomplete element type `", builder->build(), "`" ),
			  node );
			HALT();
		}
		if ( !elem_ty->is_valid_element_type() )
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "array declared with invalid element type `", builder->build(), "`" ),
			  node );
			HALT();
		}

		if ( children.size() - an == 3 )
		{  // w [ X ]
			auto len_val = get<QualifiedValue>( codegen( children[ an + 1 ] ) );

			auto len = dyn_cast_or_null<Constant>( len_val.value( children[ an + 1 ] ).get() );
			if ( !len )
			{
				infoList->add_msg( MSG_TYPE_ERROR, fmt( "array length must be constant" ), children[ an + 1 ] );
				auto &type = TypeView::getLongTy( false );
				len = Constant::getIntegerValue(
				  type->type, APInt( type->as<mty::Integer>()->bits, 1, false ) );
			}
			std::size_t len_i = 1;
			if ( !len->getType()->isIntegerTy() )
			{
				infoList->add_msg(
				  MSG_TYPE_ERROR,
				  fmt( "size of array has non-integer type `", len_val.get_type(), "`" ),
				  children[ an + 1 ] );
			}
			else
			{
				auto ci = dyn_cast_or_null<ConstantInt>( len );
				if ( ci->isNegative() )
				{
					infoList->add_msg(
					  MSG_TYPE_ERROR,
					  fmt( "array has a negative size" ),
					  children[ an + 1 ] );
				}
				else
				{
					len_i = ci->getZExtValue();
				}
			}

			builder->add_level( std::make_shared<mty::Array>( elem_ty->type, len_i ) );
		}
		else
		{  // w [ ]
			builder->add_level( std::make_shared<mty::Array>( elem_ty->type ) );
		}

		if ( an != 0 )
		{
			return get<QualifiedDecl>( codegen( children[ an - 1 ], builder ) );
		}
		else
		{
			return QualifiedDecl( builder->build() );
		}
	}
	else if ( la == '(' )
	{
		std::vector<QualifiedDecl> args;
		bool is_va_args = false;
		for ( int j = 2; j < children.size() - 1; ++j )
		{
			auto &child = children[ j ];
			if ( child.isObject() )
			{
				auto decl = get<QualifiedDecl>( codegen( child ) );
				args.emplace_back( decl );
			}
			else if ( child[ 1 ].asString() == "..." )
			{
				is_va_args = true;
			}
		}
		if ( args.size() != 1 || !args[ 0 ].type.is<mty::Void>() )
		{
			int errs = 0, idx = 0;
			for ( int j = 2; j < children.size() - 1; ++j )
			{
				auto &child = children[ j ];
				if ( child.isObject() )
				{
					auto &arg = args[ idx++ ];

					if ( !arg.type->is_valid_parameter_type() )
					{
						if ( !arg.type->is_complete() )
						{
							infoList->add_msg(
							  MSG_TYPE_ERROR,
							  fmt( "variable of incomplete type `", arg.type, "` cannot be used as function parameter" ),
							  child );
							errs++;
						}
						else
						{
							infoList->add_msg(
							  MSG_TYPE_ERROR,
							  fmt( "variable of type `", arg.type, "` cannot be used as function parameter" ),
							  child );
							errs++;
						}
					}
					else if ( arg.type->is<mty::Function>() )
					{
						arg.type = DeclarationSpecifiers()
									 .add_type( arg.type, child )
									 .into_type_builder( child )
									 .add_level( std::make_shared<mty::Pointer>( arg.type->type ) )
									 .build();
					}
					else if ( arg.type->is<mty::Array>() && !arg.type->is_complete() )
					{
						auto builder = DeclarationSpecifiers()
										 .add_type( TypeView(
													  std::make_shared<QualifiedType>( arg.type ) )
													  .next()
													  .into_type(),
													child )
										 .into_type_builder( child );
						arg.type = builder
									 .add_level( std::make_shared<mty::Pointer>( builder.get_type()->type ) )
									 .build();
					}
				}
			}
			if ( errs ) HALT();
		}
		else
		{
			args.clear();
		}
		builder->add_level( std::make_shared<mty::Function>(
		  builder->get_type()->type, args, is_va_args ) );

		if ( an != 0 )
		{
			return get<QualifiedDecl>( codegen( children[ an - 1 ], builder ) );
		}
		else
		{
			return QualifiedDecl( builder->build() );
		}
	}
	else
	{
		INTERNAL_ERROR();
	}
}

static std::vector<QualifiedDecl> get_structural_decl( Json::Value &node )
{
	auto &roots = node[ "children" ];
	std::vector<QualifiedDecl> decls;

	int errs = 0;

	for ( int i = 0; i < roots.size(); ++i )
	{
		auto &node = roots[ i ];
		auto &children = node[ "children" ];

		auto old_size = decls.size();

		auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );

		for ( int i = 1; i < children.size(); ++i )
		{
			auto &child = children[ i ];
			if ( child.isObject() )
			{
				auto builder = declspec.into_type_builder( children[ 0 ] );
				auto decl = get<QualifiedDecl>( codegen( children[ i ], &builder ) );
				auto &type = decl.type;
				if ( !type->is_valid_element_type() )
				{
					infoList->add_msg(
					  MSG_TYPE_ERROR,
					  fmt( "variable of type `", type, "` cannot be used as aggregrate member" ),
					  children[ i ] );
					++errs;
				}
				decls.emplace_back( std::move( decl ) );
			}
		}

		if ( decls.size() == old_size )
		{
			infoList->add_msg(
			  MSG_TYPE_WARNING,
			  "declaration does not declare anything",
			  node );
		}
	}

	if ( errs ) HALT();

	return decls;
}

int Declaration::reg()
{
	static decltype( handlers ) decl = {
		{ "declaration_list", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  for ( int i = 0; i < children.size(); i++ )
			  {
				  codegen( children[ i ] );
			  }
			  return VoidType();
		  } ) },
		/* VERIFIED -> DeclarationSpecifiers */
		{ "declaration_specifiers_i", pack_fn<bool, DeclarationSpecifiers>( []( Json::Value &node, bool const &has_decl ) -> DeclarationSpecifiers {
			  return handle_decl( node, has_decl );
		  } ) },
		{ "declaration_specifiers_p", pack_fn<VoidType, DeclarationSpecifiers>( []( Json::Value &node, VoidType const & ) -> DeclarationSpecifiers {
			  return handle_decl( node, true );
		  } ) },
		{ "struct_or_union_specifier", pack_fn<bool, QualifiedType>( []( Json::Value &node, bool const &curr_scope ) -> QualifiedType {
			  auto &children = node[ "children" ];

			  auto struct_or_union = *children[ 0 ][ 1 ].asCString();
			  if ( struct_or_union == 's' )
			  {
				  if ( children.size() < 3 )
				  {
					  auto name = children[ 1 ][ 1 ].asString();
					  auto fullName = "struct." + name;

					  if ( auto sym = curr_scope ? symTable.findThisLevel( fullName ) : symTable.find( fullName ) )
					  {
						  dbg( "found: ", fullName );
						  if ( sym->is_type() ) return sym->as_type();
						  INTERNAL_ERROR();
					  }
					  else
					  {
						  dbg( "not found: ", fullName );
						  auto ty = QualifiedType( std::make_shared<mty::Struct>( name ) );
						  symTable.insert( fullName, ty, node );
						  return ty;
					  }
				  }
				  else
				  {
					  Json::Value *struct_decl;
					  auto la = children[ 1 ][ 0 ].asString();
					  auto has_id = la == "IDENTIFIER";

					  auto struct_ty = has_id ? std::make_shared<mty::Struct>( children[ 1 ][ 1 ].asString() ) : std::make_shared<mty::Struct>();

					  {
						  auto decls = get_structural_decl( children[ has_id ? 3 : 2 ] );
						  struct_ty->set_body( decls, node );
					  }

					  auto type = QualifiedType( struct_ty );

					  if ( has_id )
					  {  // struct A without typedefs is named "struct.A"
						  auto name = children[ 1 ][ 1 ].asString();
						  auto fullName = "struct." + name;

						  symTable.insert( fullName, type, children[ 1 ] );
					  }

					  return type;
				  }
			  }
			  else if ( struct_or_union == 'u' )
			  {
				  if ( children.size() < 3 )
				  {
					  auto name = children[ 1 ][ 1 ].asString();
					  auto fullName = "union." + name;

					  if ( auto sym = curr_scope ? symTable.findThisLevel( fullName ) : symTable.find( fullName ) )
					  {
						  dbg( "found: ", fullName );
						  if ( sym->is_type() ) return sym->as_type();
						  INTERNAL_ERROR();
					  }
					  else
					  {
						  dbg( "not found: ", fullName );
						  auto ty = QualifiedType( std::make_shared<mty::Union>( name ) );
						  symTable.insert( fullName, ty, node );
						  return ty;
					  }
				  }
				  else
				  {
					  Json::Value *struct_decl;
					  auto la = children[ 1 ][ 0 ].asString();
					  auto has_id = la == "IDENTIFIER";

					  auto union_ty = has_id ? std::make_shared<mty::Union>( children[ 1 ][ 1 ].asString() ) : std::make_shared<mty::Union>();

					  {
						  auto decls = get_structural_decl( children[ has_id ? 3 : 2 ] );
						  union_ty->set_body( decls, node );
					  }

					  auto type = QualifiedType( union_ty );

					  if ( has_id )
					  {  // struct A without typedefs is named "struct.A"
						  auto name = children[ 1 ][ 1 ].asString();
						  auto fullName = "union." + name;

						  symTable.insert( fullName, type, children[ 1 ] );
					  }

					  return type;
				  }
				  //   UNIMPLEMENTED( "union" );
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
		/* VERIFIED -> DeclarationSpecifiers */
		{ "specifier_qualifier_list_i", pack_fn<VoidType, DeclarationSpecifiers>( []( Json::Value &node, VoidType const & ) -> DeclarationSpecifiers {
			  return handle_decl( node, true );
		  } ) },
		{ "type_name", pack_fn<VoidType, QualifiedType>( []( Json::Value &node, VoidType const & ) -> QualifiedType {
			  auto &children = node[ "children" ];
			  auto builder = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) )
							   .into_type_builder( children[ 0 ] );
			  if ( children.size() > 1 )
			  {
				  return get<QualifiedDecl>( codegen( children[ 1 ], &builder ) ).type;
			  }
			  else
			  {
				  return builder.build();
			  }
		  } ) },
		{ "userdefined_type_name", pack_fn<bool, QualifiedType>( []( Json::Value &node, bool const & ) -> QualifiedType {
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
				  return handle_function_array( node, builder, 1 );
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
			  return get<QualifiedDecl>( codegen( child, &builder ) );
		  } ) },
		/* VERIFIED -> QualifiedDecl */
		{ "declarator", pack_fn<QualifiedTypeBuilder *, QualifiedDecl>( []( Json::Value &node, QualifiedTypeBuilder *const &builder ) -> QualifiedDecl {
			  // with arg = QualifiedTypeBuilder *
			  auto &children = node[ "children" ];
			  codegen( children[ 0 ], builder );
			  return get<QualifiedDecl>( codegen( children[ 1 ], builder ) );
		  } ) },
		{ "abstract_declarator", pack_fn<QualifiedTypeBuilder *, QualifiedDecl>( []( Json::Value &node, QualifiedTypeBuilder *const &builder ) -> QualifiedDecl {
			  auto &children = node[ "children" ];
			  auto decl = codegen( children[ 0 ], builder );
			  if ( children.size() > 1 )
			  {
				  return get<QualifiedDecl>( codegen( children[ 1 ], builder ) );
			  }
			  else
			  {
				  if ( children[ 0 ][ "type" ].asString() == "pointer" )
				  {
					  return QualifiedDecl( builder->build() );
				  }
				  else
				  {
					  return get<QualifiedDecl>( decl );
				  }
			  }
		  } ) },
		{ "direct_abstract_declarator", pack_fn<QualifiedTypeBuilder *, QualifiedDecl>( []( Json::Value &node, QualifiedTypeBuilder *const &builder ) -> QualifiedDecl {
			  // with arg = QualifiedTypeBuilder *
			  auto &children = node[ "children" ];

			  // direct_declarator -> direct_declarator ...

			  int an = children[ 0 ].isObject() ? 1 : 0;

			  if ( children.size() >= 3 && children[ 1 ].isObject() &&
				   children[ 1 ][ "type" ].asString() == "abstract_declarator" )
			  {
				  return get<QualifiedDecl>( codegen( children[ 1 ], builder ) );
			  }

			  return handle_function_array( node, builder, an );
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
			  builder->add_level( std::make_shared<mty::Pointer>(
				base->type,
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
				  auto child = children[ i ][ 1 ].asString();
				  if ( child == "const" ) type_qualifier |= TQ_CONST;
				  if ( child == "volatile" ) type_qualifier |= TQ_VOLATILE;
			  }

			  return type_qualifier;
		  } ) },
	};

	handlers.insert( decl.begin(), decl.end() );

	return 0;
}
