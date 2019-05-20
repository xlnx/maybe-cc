#include "initializer.h"

static void traverse( const InitItem &item, const std::function<void( const InitItem & )> &handle )
{
	if ( item.value.is_some() ) handle( item );
	for ( auto &node : item.childs ) traverse( node, handle );
}

static Constant *make_constant_object( TypeView elem, InitList &init,
									   std::size_t &curr, uint64_t *array_len = nullptr );

static Constant *make_constant_array( TypeView array_ty, InitList &init,
									  std::size_t &curr, uint64_t *array_len = nullptr );

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
									   std::size_t &curr, uint64_t *array_len )
{
	if ( auto arr_ty = elem->as<mty::Array>() )
	{
		if ( init[ curr ].value.is_none() )
		{  // int a[][2] = { {0, 1}, {2, 3} };
			auto &desc = init[ curr ].childs;
			std::size_t curr_ch = 0;
			auto cc = make_constant_array( elem, desc, curr_ch, array_len );

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
			return make_constant_array( elem, init, curr, array_len );
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
		return make_constant_value( elem, elem_ptr->value.unwrap(), elem_ptr->ast );
	}
}

static Constant *make_constant_array( TypeView array_ty, InitList &init, std::size_t &curr, uint64_t *array_len )
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
		if ( array_len )
		{
			*array_len = elems.size();
		}
		else
		{
			INTERNAL_ERROR();
		}
	}

	auto arr_val = ConstantArray::get(
	  is_fixed_size ? static_cast<ArrayType *>( arr->type )
					: ArrayType::get( arr->type, *array_len ),
	  elems );

	return arr_val;
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
			  TypeView( std::make_shared<QualifiedType>( comp.unwrap().type ) ),
			  init, curr ) );
		}
		else
		{
			elems.emplace_back( ConstantAggregateZero::get( comp.unwrap().type->type ) );
		}
	}

	return ConstantStruct::get(
	  static_cast<StructType *>( union_ty->type ), elems );
}

static Constant *make_constant_init( const QualifiedType &type, InitItem &init, uint64_t &array_len )
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
		  fake, curr, &array_len );

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

static Constant *make_local_constant_init( const QualifiedType &type, InitItem &init, uint64_t &array_len )
{
	auto view = TypeView( std::make_shared<QualifiedType>( type ) );

	// all elements are constant
	if ( init.value.is_none() )
	{
		std::size_t curr = 0;
		InitList fake = { init };

		return make_constant_object( view, fake, curr, &array_len );
	}
	else
	{
		return make_constant_value( view, init.value.unwrap(), init.ast );
	}
}

//

static void make_local_object( QualifiedValue val, InitList &init,
							   std::size_t &curr );

static void make_local_array( QualifiedValue &val, InitList &init,
							  std::size_t &curr );

static void make_local_struct( QualifiedValue &agg, InitList &init,
							   std::size_t &curr );

static void make_local_union( QualifiedValue &agg, InitList &init,
							  std::size_t &curr );

void make_local_value( QualifiedValue alloc, QualifiedValue value, Json::Value &ast )
{
	if ( !dyn_cast_or_null<Constant>( value.get() ) )
	{
		alloc.store( value, ast, ast, true );
	}
}

static bool is_all_constant( const InitList &init )
{
	bool is_constant = true;

	for ( auto &item : init )
	{
		traverse( item, [&]( const InitItem &item ) {
			if ( is_constant )
			{
				is_constant = dyn_cast_or_null<Constant>( item.value.unwrap().get() );
			}
		} );
	}

	return is_constant;
}

static void make_local_object( QualifiedValue elem, InitList &init,
							   std::size_t &curr )
{
	if ( is_all_constant( init ) ) return;

	if ( auto arr_ty = elem.get_type()->as<mty::Array>() )
	{
		if ( init[ curr ].value.is_none() )
		{  // int a[][2] = { {0, 1}, {2, 3} };
			auto &desc = init[ curr ].childs;
			std::size_t curr_ch = 0;
			make_local_array( elem, desc, curr_ch );

			if ( desc.size() > curr_ch )
			{  // int a[][2] = { {0, 1, 2, 3} ... };
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "excess elements in array initializer" ),
				  desc[ curr_ch ].ast );
			}
			++curr;
		}
		else
		{  // int a[][2] = { 0, 1, 2, 3 };
			make_local_array( elem, init, curr );
		}
	}
	else if ( auto struct_ty = elem.get_type()->as<mty::Struct>() )
	{  // struct A a[] = {...};
		if ( init[ curr ].value.is_none() )
		{  // struct A a[] = {{...} ...};
			auto &desc = init[ curr ].childs;
			std::size_t curr_ch = 0;
			make_local_struct( elem, desc, curr_ch );
			if ( desc.size() > curr_ch )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "excess elements in struct initializer" ),
				  desc[ curr_ch ].ast );
			}
			++curr;
		}
		else
		{
			make_local_struct( elem, init, curr );
		}
	}
	else if ( auto union_ty = elem.get_type()->as<mty::Union>() )
	{
		if ( init[ curr ].value.is_none() )
		{  // struct A a[] = {{...} ...};
			auto &desc = init[ curr ].childs;
			std::size_t curr_ch = 0;
			make_local_union( elem, desc, curr_ch );
			if ( desc.size() > curr_ch )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "excess elements in union initializer" ),
				  desc[ curr_ch ].ast );
			}
			++curr;
		}
		else
		{
			make_local_union( elem, init, curr );
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
		make_local_value( elem, elem_ptr->value.unwrap(), elem_ptr->ast );
	}
}

static void make_local_array( QualifiedValue &array, InitList &init, std::size_t &curr )
{
	auto arr = array.get_type()->as<mty::Array>();

	Json::Value ast;

	for ( uint64_t i = 0; i < arr->len.unwrap(); ++i )
	{
		auto elem = array;
		elem.offset(
		  ConstantInt::get(
			TypeView::getLongLongTy( false )->type,
			APInt( 64, i, false ) ),
		  ast );
		if ( curr < init.size() )
		{
			make_local_object( elem, init, curr );
		}
	}
}

static void make_local_struct( QualifiedValue &agg, InitList &init,
							   std::size_t &curr )
{
	auto struct_ty = agg.get_type()->as<mty::Struct>();
	auto &sel_comps = struct_ty->sel_comps;

	Json::Value ast;

	for ( auto &comp : sel_comps )
	{
		auto elem = agg;
		elem.get_member( comp.name.unwrap(), ast );

		if ( curr < init.size() )
		{
			make_local_object( elem, init, curr );
		}
	}
}

static void make_local_union( QualifiedValue &agg, InitList &init,
							  std::size_t &curr )
{
	auto union_ty = agg.get_type()->as<mty::Union>();
	auto &comp = union_ty->first_comp;

	Json::Value ast;

	if ( comp.is_some() )
	{
		auto elem = agg;
		elem.get_member( comp.unwrap().name.unwrap(), ast );

		if ( curr < init.size() )
		{
			make_local_object( elem, init, curr );
		}
	}
}

static void make_local_init( QualifiedValue val, InitItem &init )
{
	if ( init.value.is_none() )
	{
		std::size_t curr = 0;
		InitList fake = { init };

		make_local_object( val, fake, curr );
	}
	else
	{
		make_local_value( val, init.value.unwrap(), init.ast );
	}
}

int Initializer::reg()
{
	static decltype( handlers ) expr = {
		{ "initializer", pack_fn<VoidType, InitItem>( []( Json::Value &node, const VoidType & ) -> InitItem {
			  auto &children = node[ "children" ];
			  InitItem item;
			  item.ast = node;
			  if ( children.size() > 1 )
			  {
				  for ( int i = 1; i < children.size() - 1; ++i )
				  {
					  if ( children[ i ].isObject() )
					  {
						  item.childs.emplace_back( get<InitItem>( codegen( children[ i ] ) ) );
					  }
				  }
			  }
			  else
			  {
				  item.value = get<QualifiedValue>( codegen( children[ 0 ] ) ).value( children[ 0 ] );
			  }

			  return item;
		  } ) },
		{ "declaration", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];

			  int child_cnt = 0;

			  for ( int i = 1; i < children.size(); ++i )
			  {
				  auto &child = children[ i ];
				  if ( child.isObject() )
				  {
					  child_cnt++;
				  }
			  }

			  auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ], child_cnt != 0 ) );

			  if ( child_cnt == 0 )
			  {
				  auto &type = declspec.get_type();
				  if ( type.is_none() ||
					   !( type.unwrap().is<mty::Structural>() || type.unwrap().is<mty::Enum>() ) )
				  {
					  infoList->add_msg(
						MSG_TYPE_WARNING,
						"declaration does not declare anything", node );
				  }
			  }

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
									  uint64_t array_len;

									  if ( init.is_some() )
									  {
										  cc = make_constant_init( type, init.unwrap(), array_len );
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
											  len = array_len;
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
										  uint64_t len;
										  cc = make_local_constant_init( type, init.unwrap(), len );
										  if ( !type->is_complete() )
										  {
											  make_type_len( len );
										  }
									  }
									  alloc = Builder.CreateAlloca( type->type );
									  if ( cc )
									  {
										  auto glob = new GlobalVariable( *TheModule, type->type, false, GlobalValue::InternalLinkage, cc );
										  auto size = TheDataLayout->getTypeAllocSize( type->type );
										  Builder.CreateMemCpy( alloc, 1, glob, 1, size );
										  auto ival = QualifiedValue(
											std::make_shared<QualifiedType>( type ), alloc, !type.is<mty::Address>() );
										  make_local_init( ival, init.unwrap() );
									  }
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

			  return VoidType();
		  } ) },
	};

	handlers.insert( expr.begin(), expr.end() );

	return 0;
}