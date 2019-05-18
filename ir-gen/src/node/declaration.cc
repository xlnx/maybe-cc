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

static void traverse( const InitItem &item, const std::function<void( const InitItem & )> &handle )
{
	if ( item.value.is_some() ) handle( item );
	for ( auto &node : item.childs ) traverse( node, handle );
}

static Constant *make_constant_object( TypeView elem, InitList &init,
									   std::size_t &curr );

static Constant *make_constant_array( TypeView array_ty, InitList &init,
									  std::size_t &curr );

static Constant *make_constant_struct( const mty::Struct *struct_ty, InitList &init,
									   std::size_t &curr );

static Constant *make_constant_union( const mty::Union *union_ty, InitList &init,
									  std::size_t &curr );

Constant *make_constant_value( const TypeView &view, QualifiedValue &value, Json::Value &ast )
{
	if ( dyn_cast_or_null<Constant>( value.get() ) )
	{
		return static_cast<Constant *>( value.cast( view, ast ).get() );
	}
	else
	{  // this elem is not constant
		return ConstantAggregateZero::get( view->type );
	}
}

static Constant *make_constant_object( TypeView elem, InitList &init,
									   std::size_t &curr )
{
	if ( auto arr_ty = elem->as<mty::Array>() )
	{
		if ( init[ curr ].value.is_none() )
		{  // int a[][2] = { {0, 1}, {2, 3} };
			auto &desc = init[ curr ].childs;
			std::size_t curr_ch = 0;
			auto cc = make_constant_array( elem, desc, curr_ch );

			if ( desc.size() > curr_ch )
			{  // int a[][2] = { {0, 1, 2, 3} ... };
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "excess elements in array initializer" ),
				  desc[ curr_ch ].ast );
			}
			++curr;
			return cc;
		}
		else
		{  // int a[][2] = { 0, 1, 2, 3 };
			return make_constant_array( elem, init, curr );
		}
	}
	else if ( auto struct_ty = elem->as<mty::Struct>() )
	{  // struct A a[] = {...};
		if ( init[ curr ].value.is_none() )
		{  // struct A a[] = {{...} ...};
			auto &desc = init[ curr ].childs;
			std::size_t curr_ch = 0;
			auto cc = make_constant_struct( struct_ty, desc, curr_ch );
			if ( desc.size() > curr_ch )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "excess elements in struct initializer" ),
				  desc[ curr_ch ].ast );
			}
			++curr;
			return cc;
		}
		else
		{
			return make_constant_struct( struct_ty, init, curr );
		}
	}
	else if ( auto union_ty = elem->as<mty::Union>() )
	{
		if ( init[ curr ].value.is_none() )
		{  // struct A a[] = {{...} ...};
			auto &desc = init[ curr ].childs;
			std::size_t curr_ch = 0;
			auto cc = make_constant_union( union_ty, desc, curr_ch );
			if ( desc.size() > curr_ch )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "excess elements in union initializer" ),
				  desc[ curr_ch ].ast );
			}
			++curr;
			return cc;
		}
		else
		{
			return make_constant_union( union_ty, init, curr );
		}
	}
	else
	{
		auto *elem_ptr = &init[ curr ];
		if ( init[ curr ].value.is_none() )
		{
			auto &desc = init[ curr ].childs;
			if ( desc.size() == 0 )
			{
				infoList->add_msg(
				  MSG_TYPE_ERROR,
				  fmt( "scalar initializer cannot be empty" ),
				  init[ curr ].ast );
				HALT();
			}
			elem_ptr = &desc[ 0 ];
			while ( elem_ptr->value.is_none() )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "too many braces around scalar initializer" ),
				  elem_ptr->ast );
				if ( elem_ptr->childs.size() == 0 )
				{
					infoList->add_msg(
					  MSG_TYPE_ERROR,
					  fmt( "scalar initializer cannot be empty" ),
					  elem_ptr->ast );
					HALT();
				}
				elem_ptr = &elem_ptr->childs[ 0 ];
			}
			if ( desc.size() > 1 )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "excess elements in scalar initializer" ),
				  desc[ 1 ].ast );
			}
		}
		++curr;
		if ( elem_ptr )
		{
			return make_constant_value( elem, elem_ptr->value.unwrap(), elem_ptr->ast );
		}
		else
		{
			return ConstantAggregateZero::get( elem->type );
		}
	}
}

static Constant *make_constant_array( TypeView array_ty, InitList &init, std::size_t &curr )
{
	std::vector<Constant *> elems;

	auto arr = array_ty->as<mty::Array>();
	if ( !arr ) INTERNAL_ERROR();

	auto is_fixed_size = arr->len.is_some();

	auto &elem_ty = array_ty.next();

	if ( is_fixed_size )
	{
		for ( int i = 0; i < arr->len.unwrap(); ++i )
		{
			if ( curr < init.size() )
			{
				elems.emplace_back( make_constant_object( elem_ty, init, curr ) );
			}
			else
			{
				elems.emplace_back( ConstantAggregateZero::get( elem_ty->type ) );
			}
		}
	}
	else
	{
		while ( curr < init.size() )
		{
			elems.emplace_back( make_constant_object( elem_ty, init, curr ) );
		}
	}
	return ConstantArray::get( static_cast<ArrayType *>( arr->type ), elems );
}

static Constant *make_constant_struct( const mty::Struct *struct_ty, InitList &init,
									   std::size_t &curr )
{
	std::vector<Constant *> elems;
	auto &sel_comps = struct_ty->sel_comps;

	for ( auto &comp : sel_comps )
	{
		if ( curr < init.size() )
		{
			elems.emplace_back( make_constant_object(
			  TypeView( std::make_shared<QualifiedType>( comp.type ) ),
			  init, curr ) );
		}
		else
		{
			elems.emplace_back( ConstantAggregateZero::get( comp.type->type ) );
		}
	}

	return ConstantStruct::get(
	  static_cast<StructType *>( struct_ty->type ), elems );
}

static Constant *make_constant_union( const mty::Union *union_ty, InitList &init,
									  std::size_t &curr )
{
	std::vector<Constant *> elems;
	auto &comp = union_ty->first_comp;

	if ( comp.is_some() )
	{
		if ( curr < init.size() )
		{
			elems.emplace_back( make_constant_object(
			  TypeView( std::make_shared<QualifiedType>( comp.unwrap() ) ),
			  init, curr ) );
		}
		else
		{
			elems.emplace_back( ConstantAggregateZero::get( comp.unwrap()->type ) );
		}
	}

	return ConstantStruct::get(
	  static_cast<StructType *>( union_ty->type ), elems );
}

static Constant *make_constant_init( const QualifiedType &type, InitItem &init )
{
	auto view = TypeView( std::make_shared<QualifiedType>( type ) );
	bool is_constant = true;
	Json::Value ast;

	traverse( init, [&]( const InitItem &item ) {
		if ( is_constant )
		{
			is_constant = dyn_cast_or_null<Constant>( item.value.unwrap().get() );
			if ( !is_constant )
			{
				ast = item.ast;
			}
		}
	} );

	if ( !is_constant )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "initializer element is not a compile-time constant" ),
		  ast );
		HALT();
	}

	// all elements are constant
	if ( init.value.is_none() )
	{
		std::size_t curr = 0;
		InitList fake = { init };

		auto cc = make_constant_object(
		  TypeView( std::make_shared<QualifiedType>( type ) ),
		  fake, curr );

		return cc;
	}
	else
	{
		return make_constant_value(
		  TypeView( std::make_shared<QualifiedType>( type ) ),
		  init.value.unwrap(),
		  init.ast );
	}
}

static Constant *make_local_constant_init( const QualifiedType &type, InitItem &init )
{
	auto view = TypeView( std::make_shared<QualifiedType>( type ) );

	// all elements are constant
	if ( init.value.is_none() )
	{
		std::size_t curr = 0;
		InitList fake = { init };

		return make_constant_object( view, fake, curr );
	}
	else
	{
		return make_constant_value( view, init.value.unwrap(), init.ast );
	}
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
						  auto &type = decl.type;
						  auto &name = decl.name.unwrap();

						  auto make_type_len = [&]( uint64_t len ) {
							  auto builder = DeclarationSpecifiers()
											   .add_type(
												 TypeView( std::make_shared<QualifiedType>( type ) )
												   .next()
												   .into_type(),
												 children[ 0 ] )
											   .into_type_builder( children[ 0 ] );
							  type = builder
									   .add_level( std::make_shared<mty::Array>( builder.get_type()->type, len ) )
									   .build();
						  };

						  if ( declspec.has_attribute( SC_TYPEDEF ) )  // deal with typedef
						  {
							  if ( children.size() > 1 )  // decl with init
							  {
								  infoList->add_msg( MSG_TYPE_ERROR, "only variables can be initialized", child );
								  HALT();
							  }
							  else
							  {
								  symTable.insert( name, type, children[ 0 ] );
							  }
						  }
						  else
						  {
							  if ( !type.as<mty::Qualified>()->is_allocable() )
							  {
								  infoList->add_msg(
									MSG_TYPE_ERROR,
									fmt( "variable has incomplete type `", type, "`" ),
									children[ 0 ] );
								  HALT();
							  }
							  else if ( type.is<mty::Function>() )
							  {  // function declaration
								  if ( children.size() > 1 )
								  {
									  infoList->add_msg(
										MSG_TYPE_ERROR,
										fmt( "only variables can be initialized" ),
										child );
								  }
								  else if ( declspec.has_attribute( SC_STATIC ) )
								  {
									  if ( symTable.getLevel() > 0 )
									  {
										  infoList->add_msg(
											MSG_TYPE_ERROR,
											fmt( "function declared in block scope cannot have `static` storage class" ),
											child );
										  HALT();
									  }
								  }
								  auto fn = mty::Function::get( name );
								  if ( fn.is_none() )
								  {
									  auto ty = std::make_shared<QualifiedType>( type );
									  auto fn_val = std::make_pair(
										ty,
										// std::make_shared<QualifiedType>( type ),
										Function::Create(
										  static_cast<FunctionType *>( type.get()->type ),
										  GlobalValue::ExternalLinkage, name, TheModule.get() ) );
									  mty::Function::declare( fn_val, name );
									  symTable.insert( name, QualifiedValue( ty, fn_val.second ), children[ 0 ] );
								  }
								  else
								  {
									  auto &fn_val = *fn.unwrap();
									  if ( !fn_val.first->is_same_without_cv( type ) )
									  {
										  infoList->add_msg(
											MSG_TYPE_ERROR,
											fmt( "conflict declaration of function `", name, "`" ),
											child );
										  HALT();
									  }
									  symTable.insert( name, QualifiedValue( fn_val.first, fn_val.second ), children[ 0 ] );
								  }
							  }
							  else
							  {  // variable declaration

								  //   Value *init = nullptr;  // decl with init
								  Option<InitItem> init;

								  if ( children.size() > 1 )
								  {
									  if ( declspec.has_attribute( SC_EXTERN ) )
									  {
										  infoList->add_msg(
											MSG_TYPE_ERROR,
											fmt( "external variable must not have an initializer" ),
											children[ 2 ] );
										  HALT();
									  }

									  init = get<InitItem>( codegen( children[ 2 ] ) );

									  if ( type->is<mty::Array>() && init.unwrap().value.is_some() )
									  {
										  infoList->add_msg(
											MSG_TYPE_ERROR,
											fmt( "array initializer must be an initializer list" ),
											children[ 2 ] );
										  HALT();
									  }
								  }

								  // deal with decl
								  Value *alloc = nullptr;
								  if ( declspec.has_attribute( SC_EXTERN ) ||
									   declspec.has_attribute( SC_STATIC ) || symTable.getLevel() == 0 )
								  {
									  Constant *cc = nullptr;

									  if ( init.is_some() )
									  {
										  cc = make_constant_init( type, init.unwrap() );
									  }

									  if ( !type->is_complete() )
									  {
										  uint64_t len = 0;
										  if ( !declspec.has_attribute( SC_EXTERN ) )
										  {
											  if ( init.is_none() )
											  {
												  infoList->add_msg(
													MSG_TYPE_ERROR,
													fmt( "definition of variable with array type needs an explicit size or an initializer" ),
													node );
												  HALT();
											  }
											  if ( auto ll = dyn_cast_or_null<ArrayType>( cc->getType() ) )
											  {
												  len = ll->getNumElements();
											  }
											  else
											  {
												  INTERNAL_ERROR();
											  }
										  }
										  make_type_len( len );
									  }

									  auto linkage = GlobalVariable::CommonLinkage;
									  if ( declspec.has_attribute( SC_EXTERN ) )
									  {
										  linkage = GlobalVariable::ExternalLinkage;
									  }
									  else
									  {
										  if ( declspec.has_attribute( SC_STATIC ) )
										  {
											  linkage = GlobalVariable::InternalLinkage;
										  }
										  else if ( cc )
										  {
											  linkage = GlobalVariable::ExternalWeakLinkage;
										  }
										  if ( !cc ) cc = ConstantAggregateZero::get( type->type );
									  }

									  alloc = new GlobalVariable( *TheModule, type->type, false, linkage, cc );
								  }
								  else
								  {  // stack allocated.
									  if ( !type->is_complete() )
									  {
										  if ( init.is_none() )
										  {
											  infoList->add_msg(
												MSG_TYPE_ERROR,
												fmt( "definition of variable with array type needs an explicit size or an initializer" ),
												children[ 0 ] );
											  HALT();
										  }
									  }
									  Constant *cc = nullptr;
									  if ( init.is_some() )
									  {
										  cc = make_local_constant_init( type, init.unwrap() );
										  if ( !type->is_complete() )
										  {
											  if ( auto ll = dyn_cast_or_null<ArrayType>( cc->getType() ) )
											  {
												  make_type_len( ll->getNumElements() );
											  }
											  else
											  {
												  INTERNAL_ERROR();
											  }
										  }
									  }
									  alloc = Builder.CreateAlloca( type->type );
									  //   if ( cc )
									  //   {
									  // 	  Builder.CreateMemCpy(
									  // 		alloc,
									  // 		alloc->getPointerAlignment(),
									  // 		cc, cc->getPointerAlignment(),
									  // 		cc->getType()->getPrimitiveSizeInBits() );
									  //   }
								  }
								  alloc->setName( name );
								  symTable.insert(
									name,
									QualifiedValue(
									  std::make_shared<QualifiedType>( type ), alloc, !type.is<mty::Address>() ),
									children[ 0 ] );
							  }
						  }
					  }
				  }
			  }

			  if ( child_cnt == 0 )
			  {
				  auto &type = declspec.get_type();
				  if ( type.is_none() || !type.unwrap().is<mty::Structural>() )
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
		{ "struct_or_union_specifier", pack_fn<VoidType, QualifiedType>( []( Json::Value &node, VoidType const & ) -> QualifiedType {
			  auto &children = node[ "children" ];

			  auto struct_or_union = *children[ 0 ][ 1 ].asCString();
			  if ( struct_or_union == 's' )
			  {
				  if ( children.size() < 3 )
				  {
					  auto name = children[ 1 ][ 1 ].asString();
					  auto fullName = "struct." + name;

					  if ( auto sym = symTable.find( fullName ) )
					  {
						  if ( sym->is_type() ) return sym->as_type();
						  INTERNAL_ERROR();
					  }
					  else
					  {
						  return QualifiedType( std::make_shared<mty::Struct>( name ) );
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

					  if ( auto sym = symTable.find( fullName ) )
					  {
						  if ( sym->is_type() ) return sym->as_type();
						  INTERNAL_ERROR();
					  }
					  else
					  {
						  return QualifiedType( std::make_shared<mty::Union>( name ) );
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
			  return handle_decl( node );
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
