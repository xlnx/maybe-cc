#include "declaration.h"

static DeclarationSpecifiers handle_decl( Json::Value &node )
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
			declspec.add_attribute( child[ 1 ].asCString(), child );
		}
	}

	return declspec;
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
		/* VERIFIED -> void */
		{ "declaration", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];

			  auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ] ) );

			  int child_cnt = 0;

			  for ( int i = 1; i < children.size(); ++i )
			  {
				  auto &child = children[ i ];
				  if ( child.isObject() )
				  {
					  child_cnt++;

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
							  symTable.insert( name, QualifiedValue( std::make_shared<QualifiedType>( type ),
																	 alloc,
																	 !type.is<mty::Address>() ) );

							  if ( children.size() > 1 )  // decl with init
							  {
								  auto init = get<QualifiedValue>( codegen( children[ 2 ], &builder ) );
								  Builder.CreateStore( alloc, init.value( children[ 2 ] ).get() );
							  }
						  }
					  }
				  }
			  }

			  if ( child_cnt == 0 )
			  {
				  auto &type = declspec.get_type();
				  if ( type.is_none() || !type.unwrap().is<mty::Struct>() )
				  {
					  infoList->add_msg( MSG_TYPE_WARNING, "declaration does not declare anything", node );
				  }
			  }

			  return VoidType();
		  } ) },
		/* VERIFIED -> DeclarationSpecifiers */
		{ "declaration_specifiers_i", pack_fn<VoidType, DeclarationSpecifiers>( []( Json::Value &node, VoidType const & ) -> DeclarationSpecifiers {
			  return handle_decl( node );
		  } ) },
		{ "declaration_specifiers_p", pack_fn<VoidType, DeclarationSpecifiers>( []( Json::Value &node, VoidType const & ) -> DeclarationSpecifiers {
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

			  auto struct_ty = QualifiedType( std::make_shared<mty::Struct>( decls ) );

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
					  {  // w [ X ]
						  builder->add_level( std::make_shared<mty::Array>( builder->get_type(), 10 ) );
						  TODO( "ARRAY LENGTH NOT IMPLEMENTED" );
						  return get<QualifiedDecl>( codegen( children[ 0 ], builder ) );
					  }
					  else
					  {  // w [ ]
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
							  args.emplace_back( decl );
						  }
					  }
					  builder->add_level( std::make_shared<mty::Function>(
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

			  auto la = children[ an ][ 1 ].asString();
			  if ( la == "[" )
			  {
				  if ( children.size() - an == 3 )
				  {  // w [ X ]
					  builder->add_level( std::make_shared<mty::Array>( builder->get_type(), 10 ) );
					  TODO( "ARRAY LENGTH NOT IMPLEMENTED" );
					  if ( an != 0 )
					  {
						  return get<QualifiedDecl>( codegen( children[ 0 ], builder ) );
					  }
					  else
					  {
						  return QualifiedDecl( builder->build() );
					  }
				  }
				  else
				  {  // w [ ]
					  UNIMPLEMENTED();
				  }
			  }
			  else if ( la == "(" )
			  {
				  std::vector<QualifiedDecl> args;
				  for ( int j = an + 1; j < children.size() - 1; ++j )
				  {
					  auto &child = children[ j ];
					  if ( child.isObject() )
					  {
						  auto decl = get<QualifiedDecl>( codegen( child ) );
						  args.emplace_back( decl );
					  }
				  }
				  builder->add_level( std::make_shared<mty::Function>(
					builder->get_type(), args ) );

				  if ( an != 0 )
				  {
					  return get<QualifiedDecl>( codegen( children[ 0 ], builder ) );
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

			  INTERNAL_ERROR();
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
