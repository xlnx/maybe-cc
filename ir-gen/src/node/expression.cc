#include "expression.h"

static QualifiedValue size_of_type( const TypeView &type, Json::Value &ast )
{
	auto &value_type = TypeView::getLongTy( false );
	if ( !type->is_complete() )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "invalid application of `sizeof` to incomplete type `", type, "`" ), ast );
		HALT();
	}
	auto bytes = type->type->isVoidTy() ? 1 : TheDataLayout->getTypeAllocSize( type->type );
	return QualifiedValue(
	  value_type,
	  Constant::getIntegerValue(
		value_type->type, APInt( value_type->as<mty::Integer>()->bits, uint64_t( bytes ), false ) ) );
}

static QualifiedValue pos( QualifiedValue &val, Json::Value &ast )
{
	if ( !val.is_rvalue() ) INTERNAL_ERROR();
	auto &type = val.get_type();
	if ( !type->is<mty::Arithmetic>() )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "invalid argument type `", type, "` to unary expression" ),
		  ast );
		HALT();
	}
	return val;
}

static inline std::string value_mark( const QualifiedValue &val )
{
	return val.is_rvalue() ? "R" : "L";
}

static QualifiedValue neg( QualifiedValue &val, Json::Value &ast )
{
	if ( !val.is_rvalue() ) INTERNAL_ERROR();
	auto &type = val.get_type();
	if ( !type->is<mty::Arithmetic>() )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "invalid argument type `", type, "` to unary expression" ),
		  ast );
		HALT();
	}
	if ( type->is<mty::Integer>() )
	{
		return QualifiedValue( type, Builder.CreateNeg( val.get() ) );
	}
	else
	{
		return QualifiedValue( type, Builder.CreateFNeg( val.get() ) );
	}
}

static QualifiedValue flip( QualifiedValue &val, Json::Value &ast )
{
	if ( !val.is_rvalue() ) INTERNAL_ERROR();
	auto &type = val.get_type();
	if ( !type->is<mty::Integer>() )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "invalid argument type `", type, "` to unary expression" ),
		  ast );
		HALT();
	}
	return QualifiedValue( type, Builder.CreateNot( val.get() ) );
}

static QualifiedValue logical_not( QualifiedValue &val, Json::Value &ast )
{
	if ( !val.is_rvalue() ) INTERNAL_ERROR();
	auto &type = val.get_type();
	if ( !type->is<mty::Arithmetic>() && !type->is<mty::Derefable>() )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "invalid argument type `", type, "` to unary expression" ),
		  ast );
		HALT();
	}
	return QualifiedValue(
	  std::make_shared<QualifiedType>(
		TypeView::getBoolTy().into_type() ),
	  Builder.CreateNot(
		val
		  .cast( TypeView::getBoolTy(), ast )
		  .get() ) );
}

static QualifiedValue add( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast )
{
	if ( lhs.get_type()->is<mty::Derefable>() && rhs.get_type()->is<mty::Integer>() )
	{
		return lhs.value( ast[ "children" ][ 0 ] ).offset( rhs.get(), ast[ "children" ][ 0 ] );
	}
	else if ( lhs.get_type()->is<mty::Integer>() && rhs.get_type()->is<mty::Derefable>() )
	{
		return rhs.value( ast[ "children" ][ 2 ] ).offset( lhs.get(), ast[ "children" ][ 2 ] );
	}
	else
	{
		QualifiedValue::cast_binary_expr( lhs.value( ast[ "children" ][ 0 ] ), rhs, ast );
		auto &type = lhs.get_type();
		if ( auto itype = type->as<mty::Integer>() )
		{
			return QualifiedValue( type, Builder.CreateAdd( lhs.get(), rhs.get() ) );
		}
		else
		{
			return QualifiedValue( type, Builder.CreateFAdd( lhs.get(), rhs.get() ) );
		}
	}
}

static QualifiedValue sub( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast )
{
	if ( lhs.get_type()->is<mty::Derefable>() && rhs.get_type()->is<mty::Integer>() )
	{
		return lhs.value( ast[ "children" ][ 0 ] ).offset( neg( rhs, ast ).get(), ast[ "children" ][ 0 ] );
	}
	else if ( lhs.get_type()->is<mty::Derefable>() && rhs.get_type()->is<mty::Derefable>() )
	{
		lhs.ensure_is_ptr_if_deref();
		rhs.ensure_is_ptr_if_deref();

		if ( lhs.get_type().is_same_discard_qualifiers( rhs.get_type() ) )
		{
			auto diff = Builder.CreatePtrDiff(
			  lhs.value( ast[ "children" ][ 0 ] ).get(),
			  rhs.value( ast[ "children" ][ 2 ] ).get() );
			return QualifiedValue( TypeView::getLongTy( true ), diff );
		}
		else
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "`", lhs.get_type(), "` and `", rhs.get_type(), "` are not pointer to compatible types" ),
			  ast );
			HALT();
		}
	}
	else
	{
		QualifiedValue::cast_binary_expr( lhs.value( ast[ "children" ][ 0 ] ), rhs, ast );
		auto &type = lhs.get_type();
		if ( auto itype = type->as<mty::Integer>() )
		{
			return QualifiedValue( type, Builder.CreateSub( lhs.get(), rhs.get() ) );
		}
		else
		{
			return QualifiedValue( type, Builder.CreateFSub( lhs.get(), rhs.get() ) );
		}
	}
}

static QualifiedValue handle_binary_expr( const char *op, QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &node )
{
	auto &children = node[ "children" ];

	lhs.value( children[ 0 ] );
	rhs.value( children[ 2 ] );

	static JumpTable<QualifiedValue( QualifiedValue & lhs, QualifiedValue & rhs, Json::Value & ast )> __ = {
		{ "*", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( type->is<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateMul( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFMul( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "/", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateSDiv( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateUDiv( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFDiv( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "%", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 if ( itype->is_signed )
				 {
					 return QualifiedValue( type, Builder.CreateSRem( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateURem( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFRem( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "+", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 return add( lhs, rhs, ast );
		 } },
		{ "-", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 return sub( lhs, rhs, ast );
		 } },

		{ "<<", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateShl( lhs.get(), rhs.get() ) );
		 } },
		{ ">>", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 auto type = lhs.get_type()->as<mty::Integer>();
			 if ( type->is_signed )
			 {
				 return QualifiedValue( lhs.get_type(), Builder.CreateAShr( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( lhs.get_type(), Builder.CreateLShr( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "<", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 auto &type = TypeView::getBoolTy();
			 if ( !QualifiedValue::cast_binary_ptr( lhs, rhs, ast ) )
			 {
				 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
				 if ( auto itype = lhs.get_type()->as<mty::Integer>() )
				 {
					 if ( itype->is_signed )
					 {
						 return QualifiedValue( type, Builder.CreateICmpSLT( lhs.get(), rhs.get() ) );
					 }
					 else
					 {
						 return QualifiedValue( type, Builder.CreateICmpULT( lhs.get(), rhs.get() ) );
					 }
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateFCmpOLT( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateICmpULT( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ ">", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 auto &type = TypeView::getBoolTy();
			 if ( !QualifiedValue::cast_binary_ptr( lhs, rhs, ast ) )
			 {
				 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
				 if ( auto itype = lhs.get_type()->as<mty::Integer>() )
				 {
					 if ( itype->is_signed )
					 {
						 return QualifiedValue( type, Builder.CreateICmpSGT( lhs.get(), rhs.get() ) );
					 }
					 else
					 {
						 return QualifiedValue( type, Builder.CreateICmpUGT( lhs.get(), rhs.get() ) );
					 }
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateFCmpOGT( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateICmpUGT( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "<=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 auto &type = TypeView::getBoolTy();
			 if ( !QualifiedValue::cast_binary_ptr( lhs, rhs, ast ) )
			 {
				 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
				 if ( auto itype = lhs.get_type()->as<mty::Integer>() )
				 {
					 if ( itype->is_signed )
					 {
						 return QualifiedValue( type, Builder.CreateICmpSLE( lhs.get(), rhs.get() ) );
					 }
					 else
					 {
						 return QualifiedValue( type, Builder.CreateICmpULE( lhs.get(), rhs.get() ) );
					 }
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateFCmpOLE( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateICmpULE( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ ">=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 auto &type = TypeView::getBoolTy();
			 if ( !QualifiedValue::cast_binary_ptr( lhs, rhs, ast ) )
			 {
				 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
				 if ( auto itype = lhs.get_type()->as<mty::Integer>() )
				 {
					 if ( itype->is_signed )
					 {
						 return QualifiedValue( type, Builder.CreateICmpSGE( lhs.get(), rhs.get() ) );
					 }
					 else
					 {
						 return QualifiedValue( type, Builder.CreateICmpUGE( lhs.get(), rhs.get() ) );
					 }
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateFCmpOGE( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateICmpUGE( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "==", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 auto &type = TypeView::getBoolTy();
			 if ( !QualifiedValue::cast_binary_ptr( lhs, rhs, ast, true ) )
			 {
				 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
				 if ( auto itype = lhs.get_type()->as<mty::Integer>() )
				 {
					 return QualifiedValue( type, Builder.CreateICmpEQ( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateFCmpOEQ( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateICmpEQ( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "!=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 auto &type = TypeView::getBoolTy();
			 if ( !QualifiedValue::cast_binary_ptr( lhs, rhs, ast, true ) )
			 {
				 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
				 if ( auto itype = lhs.get_type()->as<mty::Integer>() )
				 {
					 return QualifiedValue( type, Builder.CreateICmpNE( lhs.get(), rhs.get() ) );
				 }
				 else
				 {
					 return QualifiedValue( type, Builder.CreateFCmpONE( lhs.get(), rhs.get() ) );
				 }
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateICmpNE( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "&", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateAnd( lhs.get(), rhs.get() ) );
		 } },
		{ "^", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateXor( lhs.get(), rhs.get() ) );
		 } },
		{ "|", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast, false );
			 return QualifiedValue( lhs.get_type(), Builder.CreateOr( lhs.get(), rhs.get() ) );
		 } }
	};

	if ( __.find( op ) != __.end() )
	{
		auto beg = fmt( sharp( lhs.get_type(), value_mark( lhs ) ), " ",
						sharp( op ), " ",
						sharp( rhs.get_type(), value_mark( rhs ) ) );
		auto res = __[ op ]( lhs, rhs, node );
		dbg( beg, " ==> ",
			 sharp( res.get_type(), value_mark( res ) ) );
		return res;
	}
	else
	{
		INTERNAL_ERROR( op );
	}
}

int Expression::reg()
{
	static decltype( handlers ) expr = {
		{ "expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];
			  codegen( children[ 0 ] );
			  return get<QualifiedValue>( codegen( children[ 2 ] ) );
		  } ) },
		{ "assignment_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];
			  auto lhs = get<QualifiedValue>( codegen( children[ 0 ] ) );
			  auto rhs = get<QualifiedValue>( codegen( children[ 2 ] ) ).value( children[ 2 ] );
			  auto lhs_a = lhs;
			  auto op = children[ 1 ][ 1 ].asString();
			  auto idx = op.find_first_of( '=' );
			  if ( idx ) op[ idx ] = '\0';
			  auto val = idx ? handle_binary_expr( op.c_str(), lhs_a, rhs, node ) : rhs;

			  return lhs.store( val, children[ 0 ], children[ 2 ] );
		  } ) },
		{ "cast_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];
			  auto type = std::make_shared<QualifiedType>( get<QualifiedType>( codegen( children[ 1 ] ) ) );
			  auto val = get<QualifiedValue>( codegen( children[ 3 ] ) ).value( children[ 3 ] );
			  return val.cast( TypeView( type ), children[ 3 ], false );
		  } ) },
		{ "unary_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];

			  if ( children.size() == 2 )
			  {
				  auto key = children[ 0 ][ 1 ].asCString();
				  auto val = get<QualifiedValue>( codegen( children[ 1 ] ) );

				  static JumpTable<QualifiedValue( Json::Value & children, QualifiedValue & val, Json::Value & ast )> __ = {
					  { "*", []( Json::Value &children, QualifiedValue &val, Json::Value &ast ) -> QualifiedValue {
						   return val.value( children[ 1 ] ).deref( ast );
					   } },
					  { "&", []( Json::Value &children, QualifiedValue &val, Json::Value &ast ) -> QualifiedValue {
						   if ( val.is_rvalue() )
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "cannot take the address of an rvalue of type `", val.get_type(), "`" ),
								 ast );
							   HALT();
						   }
						   auto builder = DeclarationSpecifiers()
											.add_type( val.get_type().into_type(), ast )
											.into_type_builder( ast );
						   return QualifiedValue(
							 TypeView(
							   std::make_shared<QualifiedType>(
								 builder
								   .add_level( std::make_shared<mty::Pointer>( builder.get_type()->type ) )
								   .build() ) ),
							 val.get() );
					   } },
					  { "+", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
						   return pos( val.value( children[ 1 ] ), node );
					   } },
					  { "-", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
						   return neg( val.value( children[ 1 ] ), node );
					   } },
					  { "~", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
						   return flip( val.value( children[ 1 ] ), node );
					   } },
					  { "!", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
						   return logical_not( val.value( children[ 1 ] ), node );
					   } },
					  { "++", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
						   auto &int_ty = TypeView::getIntTy( true );
						   auto rhs = QualifiedValue(
							 int_ty,
							 Constant::getIntegerValue( int_ty->type, APInt( 32, 1, true ) ) );
						   auto lhs = val;
						   lhs = add( lhs.value( children[ 1 ] ), rhs, node );
						   return val.store( lhs, children[ 1 ], children[ 1 ] );
					   } },
					  { "--", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
						   auto &int_ty = TypeView::getIntTy( true );
						   auto rhs = QualifiedValue(
							 int_ty,
							 Constant::getIntegerValue( int_ty->type, APInt( 32, 1, true ) ) );
						   auto lhs = val;
						   lhs = sub( lhs.value( children[ 1 ] ), rhs, node );
						   return val.store( lhs, children[ 1 ], children[ 1 ] );
					   } },
					  { "sizeof", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
						   return size_of_type( val.get_type(), children[ 1 ] );
					   } }
				  };

				  if ( __.find( key ) != __.end() )
				  {
					  auto beg = fmt( sharp( key ), " ",
									  sharp( val.get_type(), value_mark( val ) ) );
					  auto res = __[ key ]( children, val, node );
					  dbg( beg, " ==> ",
						   sharp( res.get_type(), value_mark( res ) ) );
					  return res;
				  }
				  else
				  {
					  UNIMPLEMENTED();
				  }
			  }
			  else
			  {
				  // sizeof ( TYPE )
				  auto type = get<QualifiedType>( codegen( children[ 2 ] ) );
				  return size_of_type(
				    TypeView( std::make_shared<QualifiedType>( type ) ),
					  children[ 2 ] );
			  }
		  } ) },
		{ "postfix_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];

			  auto key = children[ 1 ][ 1 ].asCString();
			  auto val = get<QualifiedValue>( codegen( children[ 0 ] ) );

			  static JumpTable<QualifiedValue( Json::Value & children, QualifiedValue & val, Json::Value & )> __ = {
				  { "[", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   auto off = get<QualifiedValue>( codegen( children[ 2 ] ) );
					   return val.value( children[ 0 ] )
						 .offset( off.value( children[ 2 ] ).get(), node )
						 .deref( node );
				   } },
				  { "++", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   auto &int_ty = TypeView::getIntTy( true );
					   auto rhs = QualifiedValue(
						 int_ty,
						 Constant::getIntegerValue( int_ty->type, APInt( 32, 1, true ) ) );
					   auto lhs = val;
					   auto prev = lhs.value( children[ 0 ] );
					   lhs = add( lhs, rhs, node );
					   val.store( lhs, children[ 0 ], children[ 0 ] );
					   return prev;
				   } },
				  { "--", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   auto &int_ty = TypeView::getIntTy( true );
					   auto rhs = QualifiedValue(
						 int_ty,
						 Constant::getIntegerValue( int_ty->type, APInt( 32, 1, true ) ) );
					   auto lhs = val;
					   auto prev = lhs.value( children[ 0 ] );
					   lhs = sub( lhs, rhs, node );
					   val.store( lhs, children[ 0 ], children[ 0 ] );
					   return prev;
				   } },
				  { "->", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   if ( auto struct_ty = val.get_type()->as<mty::Derefable>() )
					   {
						   return val.value( children[ 1 ] )
							 .deref( children[ 1 ] )
							 .get_member( node[ "children" ][ 2 ][ 1 ].asString(), node );
					   }
					   else
					   {
						   infoList->add_msg(
							 MSG_TYPE_ERROR,
							 fmt( "member reference type `", val.get_type(), "` is not a pointer" ),
							 node );
						   HALT();
					   }
				   } },
				  { ".", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   return val.get_member( node[ "children" ][ 2 ][ 1 ].asString(), node );
				   } },
				  { "(", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   std::vector<QualifiedValue> args;
					   for ( auto i = 2; i < children.size() - 1; ++i )
					   {
						   if ( children[ i ].isObject() )
						   {
							   //    dbg( "arg begin ", i );
							   args.emplace_back(
								 get<QualifiedValue>( codegen( children[ i ] ) )
								   .value( children[ i ] ) );
							   //    dbg( "arg end ", i );
						   }
					   }

					   auto view = val.get_type();
					   if ( !view->is<mty::Function>() )
					   {
						   if ( view->is<mty::Pointer>() )
						   {
							   val.value( children[ 0 ] ).deref( children[ 0 ] );
						   }
						   if ( !val.get_type()->is<mty::Function>() )
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "called object type `", view, "` is not a function or function pointer" ),
								 children[ 0 ] );
							   HALT();
						   }
					   }

					   return val.call( args, node );
				   } }
			  };

			  if ( __.find( key ) != __.end() )
			  {
				  auto beg = fmt( sharp( val.get_type(), value_mark( val ) ), " ",
								  sharp( key ) );
				  auto res = __[ key ]( children, val, node );
				  dbg( beg, " ==> ",
					   sharp( res.get_type(), value_mark( res ) ) );
				  return res;
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
		{ "primary_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];
			  if ( children.size() > 1 )
			  {  // ( expr )
				  return get<QualifiedValue>( codegen( children[ 1 ] ) );
			  }
			  else
			  {  // deal with Identifer | Literal
				  auto &child = children[ 0 ];
				  auto type = child.isArray() ? child[ 0 ].asCString() : child["type"].asCString();

				  static JumpTable<QualifiedValue( Json::Value & )> __ = {
																						 { "IDENTIFIER", []( Json::Value &node ) -> QualifiedValue {
						   auto val = node[1].asCString();
						   if ( auto sym = symTable.find( val ) )
						   {
							   if ( sym->is_value() )
							   {
								   return sym->as_value();
							   }
							   else
							   {
								   INTERNAL_ERROR();
							   }
						   }
						   else
						   {
							   infoList->add_msg( MSG_TYPE_ERROR, fmt( "use of undeclared identifier `", val, "`" ), node );
							   HALT();
						   }
					   } },
					  { "INTEGER", []( Json::Value &node ) -> QualifiedValue {
						   auto val = node[1].asCString();
						   auto num = std::strtoll( val, nullptr, 0 );
						   bool is_signed = true;
						   bool is_long = false;

						   while ( *val != 0 )
						   {
							   switch ( *val++ )
							   {
							   case 'u':
							   case 'U': is_signed = false; break;
							   case 'l':
							   case 'L': is_long = true; break;
							   }
						   }

						   auto &type = !is_long ? TypeView::getIntTy( is_signed ) : TypeView::getLongTy( is_signed );
						   return QualifiedValue( type, Constant::getIntegerValue(
														  type->type, APInt( type->as<mty::Integer>()->bits, num, is_signed ) ) );
					   } },
					  { "FLOATING_POINT", []( Json::Value &node ) -> QualifiedValue {
						   auto val = node[1].asCString();
						   auto num = std::strtold( val, nullptr );
						   int is_double = 0;

						   while ( *val != 0 )
						   {
							   switch ( *val++ )
							   {
							   case 'f':
							   case 'F': is_double = -1; break;
							   case 'l':
							   case 'L':
								   is_double = 1;
								   infoList->add_msg( MSG_TYPE_WARNING, "`long double` literal is not currently supported", node );
								   break;
							   }
						   }
						   auto &type = is_double == 0 ? TypeView::getDoubleTy() : is_double == 1 ? TypeView::getLongDoubleTy() : TypeView::getFloatTy();

						   return QualifiedValue( type, ConstantFP::get( type->type, num ) );
					   } },
					  { "CHAR", []( Json::Value &node ) -> QualifiedValue {
						   auto val = node[1].asCString();
						   std::string esc_str = val;
						   esc_str = esc_str.substr( esc_str.find_first_of( '\'' ) + 1, esc_str.length() - 2 );

						   auto wit = esc_str.begin();
						   auto rit = esc_str.begin();
						   if ( *rit == '\\' )
						   {
							   switch ( *++rit )
							   {
							   case 'a': *wit = '\a'; break;
							   case 'b': *wit = '\b'; break;
							   case 'e': *wit = '\e'; break;
							   case 'f': *wit = '\f'; break;
							   case 'n': *wit = '\n'; break;
							   case 'r': *wit = '\r'; break;
							   case 't': *wit = '\t'; break;
							   case 'v': *wit = '\v'; break;
							   case '\\': *wit = '\\'; break;
							   case '\'': *wit = '\''; break;
							   case '"': *wit = '"'; break;
							   case '?': *wit = '\?'; break;
							   case 'u': break;
							   case 'x':
							   {
								   auto beg = ++rit;
								   for ( int i = 0; i < 2; ++i, ++rit )
								   {
									   if ( !( ( *rit <= '9' && *rit >= '0' ) ||
											   ( *rit <= 'F' && *rit >= 'A' ) ||
											   ( *rit <= 'f' && *rit >= 'a' ) ) ||
											( rit == esc_str.end() ) )
									   {
										   break;
									   }
								   }
								   if ( rit <= beg )
								   {
									   infoList->add_msg(
										 MSG_TYPE_ERROR,
										 fmt( "\\x used with no following hex digits" ),
										 node );
									   HALT();
								   }
								   std::string s( beg, rit-- );
								   *wit = std::stoi( s, nullptr, 16 );
								   break;
							   }
							   default:
							   {
								   if ( *rit <= '7' && *rit >= '0' )
								   {
									   auto beg = rit++;
									   for ( int i = 0; i < 2; ++i, ++rit )
									   {
										   auto ch = *rit;
										   if ( !( ch <= '7' && ch >= '0' ) )
										   {
											   break;
										   }
									   }
									   std::string s( beg, rit-- );
									   *wit = std::stoi( s, nullptr, 8 );
								   }
								   else
								   {
									   char s[ 3 ] = { '\\', *rit, 0 };
									   infoList->add_msg(
										 MSG_TYPE_WARNING,
										 fmt( "unknown escape sequence `", s, "`" ),
										 node );
								   }
								   break;
							   }
							   }
						   }
						   else
						   {
							   *wit = *rit;
						   }

						   auto &type = TypeView::getCharTy( true );

						   Json::Value dummy;
						   auto builder = DeclarationSpecifiers()
											.add_type( TypeView::getCharTy( true ).into_type(), dummy )
											.into_type_builder( dummy );
						   return QualifiedValue(
							 type,
							 Constant::getIntegerValue( type->type, APInt( 8, esc_str[ 0 ], true ) ),
							 false );
					   } },
					  { "string_literal_i", []( Json::Value &node ) -> QualifiedValue {
						   auto children = node["children"];
						   std::string esc_str;

						   for (int i = 0; i != children.size(); ++i)
						   {
							   auto &child = children[i];
							   auto sep = child[1].asString();
							   esc_str += sep.substr( sep.find_first_of( '"' ) + 1, sep.length() - 2 );
						   }

						   auto wit = esc_str.begin();
						   for ( auto rit = esc_str.begin(); rit != esc_str.end(); ++rit, ++wit )
						   {
							   if ( *rit == '\\' )
							   {
								   switch ( *++rit )
								   {
								   case 'a': *wit = '\a'; break;
								   case 'b': *wit = '\b'; break;
								   case 'e': *wit = '\e'; break;
								   case 'f': *wit = '\f'; break;
								   case 'n': *wit = '\n'; break;
								   case 'r': *wit = '\r'; break;
								   case 't': *wit = '\t'; break;
								   case 'v': *wit = '\v'; break;
								   case '\\': *wit = '\\'; break;
								   case '\'': *wit = '\''; break;
								   case '"': *wit = '"'; break;
								   case '?': *wit = '\?'; break;
								   case 'u': break;
								   case 'x':
								   {
									   auto beg = ++rit;
									   for ( int i = 0; i < 2; ++i, ++rit )
									   {
										   if ( !( ( *rit <= '9' && *rit >= '0' ) ||
												   ( *rit <= 'F' && *rit >= 'A' ) ||
												   ( *rit <= 'f' && *rit >= 'a' ) ) ||
												( rit == esc_str.end() ) )
										   {
											   break;
										   }
									   }
									   if ( rit <= beg )
									   {
										   infoList->add_msg(
											 MSG_TYPE_ERROR,
											 fmt( "\\x used with no following hex digits" ),
											 node );
										   HALT();
									   }
									   std::string s( beg, rit-- );

									   *wit = std::stoi( s, nullptr, 16 );
									   break;
								   }
								   default:
								   {
									   if ( *rit <= '7' && *rit >= '0' )
									   {
										   auto beg = rit++;
										   for ( int i = 0; i < 2; ++i, ++rit )
										   {
											   auto ch = *rit;
											   if ( !( ch <= '7' && ch >= '0' ) )
											   {
												   break;
											   }
										   }
										   std::string s( beg, rit-- );
										   *wit = std::stoi( s, nullptr, 8 );
									   }
									   else
									   {
										   char s[ 3 ] = { '\\', *rit, 0 };
										   infoList->add_msg(
											 MSG_TYPE_WARNING,
											 fmt( "unknown escape sequence `", s, "`" ),
											 node );
									   }
									   break;
								   }
								   }
							   }
							   else
							   {
								   *wit = *rit;
							   }
						   }
						   *wit = '\0';
						   esc_str.resize( strlen( esc_str.c_str() ) );

						   Json::Value dummy;
						   auto builder = DeclarationSpecifiers()
											.add_type( TypeView::getCharTy( true ).into_type(), dummy )
											.into_type_builder( dummy );
						   auto type = builder
										 .add_level( std::make_shared<mty::Array>(
										   builder.get_type()->type, esc_str.length() + 1 ) )
										 .build();
						   return QualifiedValue(
							 std::make_shared<QualifiedType>( type ),
							 Builder.CreateGlobalString( esc_str ),
							 false );
					   } }
				  };

				  if ( __.find( type ) != __.end() )
				  {
					  return __[ type ]( child );
				  }
				  else
				  {
					  INTERNAL_ERROR();
				  }
			  }
		  } ) },

		{ "binary_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];

			  auto op = children[ 1 ][ 1 ].asCString();
			  if ( !strcmp( op, "&&" ) )
			  {
				  auto lhs = get<QualifiedValue>( codegen( children[ 0 ] ) )
							   .value( children[ 0 ] )
							   .cast(
								 TypeView::getBoolTy(),
								 children[ 0 ] );

				  auto fn = static_cast<Function *>(
					currentFunction->get() );
				  auto andEnd = BasicBlock::Create( TheContext, "and.end", fn );
				  auto andNext = BasicBlock::Create( TheContext, "and.next", fn, andEnd );

				  Builder.CreateCondBr( lhs.get(), andNext, andEnd );
				  auto bb0 = Builder.GetInsertBlock();

				  Builder.SetInsertPoint( andNext );
				  auto rhs = get<QualifiedValue>( codegen( children[ 2 ] ) )
							   .value( children[ 2 ] )
							   .cast(
								 TypeView::getBoolTy(),
								 children[ 2 ] );
				  Builder.CreateBr( andEnd );
				  auto bb1 = Builder.GetInsertBlock();

				  Builder.SetInsertPoint( andEnd );
				  auto phi = Builder.CreatePHI( TypeView::getBoolTy()->type, 2, "phi" );
				  phi->addIncoming( lhs.get(), bb0 );
				  phi->addIncoming( rhs.get(), bb1 );

				  return QualifiedValue( TypeView::getBoolTy(), phi );
			  }
			  else if ( !strcmp( op, "||" ) )
			  {
				  auto lhs = get<QualifiedValue>( codegen( children[ 0 ] ) )
							   .value( children[ 0 ] )
							   .cast(
								 TypeView::getBoolTy(),
								 children[ 0 ] );

				  auto fn = static_cast<Function *>(
					currentFunction->get() );
				  auto orEnd = BasicBlock::Create( TheContext, "or.end", fn );
				  auto orNext = BasicBlock::Create( TheContext, "or.next", fn, orEnd );

				  Builder.CreateCondBr( lhs.get(), orEnd, orNext );
				  auto bb0 = Builder.GetInsertBlock();

				  Builder.SetInsertPoint( orNext );
				  auto rhs = get<QualifiedValue>( codegen( children[ 2 ] ) )
							   .value( children[ 2 ] )
							   .cast(
								 TypeView::getBoolTy(),
								 children[ 2 ] );
				  Builder.CreateBr( orEnd );
				  auto bb1 = Builder.GetInsertBlock();

				  Builder.SetInsertPoint( orEnd );
				  auto phi = Builder.CreatePHI( TypeView::getBoolTy()->type, 2, "phi" );
				  phi->addIncoming( lhs.get(), bb0 );
				  phi->addIncoming( rhs.get(), bb1 );

				  return QualifiedValue( TypeView::getBoolTy(), phi );
			  }

			  auto lhs = get<QualifiedValue>( codegen( children[ 0 ] ) );
			  auto rhs = get<QualifiedValue>( codegen( children[ 2 ] ) );
			  return handle_binary_expr( op, lhs, rhs, node );
		  } ) },
		// { "conditional_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
		// 	  TODO( "unimplemented()" );
		// 	  auto &children = node[ "children" ];
		// 	  return get<QualifiedValue>( codegen( children[ 0 ] ) );
		//   } ) }
		{ "conditional_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];

			  auto value = get<QualifiedValue>( codegen( children[ 0 ] ) )
							 .value( children[ 0 ] )
							 .cast(
							   TypeView::getBoolTy(),
							   children[ 0 ] );

			  if ( currentFunction )
			  {
				  auto fn = static_cast<Function *>( currentFunction->get() );

				  auto quesEnd = BasicBlock::Create( TheContext, "ques.end", fn );
				  auto quesSecd = BasicBlock::Create( TheContext, "ques.secd", fn, quesEnd );
				  auto quesFst = BasicBlock::Create( TheContext, "ques.fst", fn, quesSecd );

				  Builder.CreateCondBr( value.get(), quesFst, quesSecd );

				  Builder.SetInsertPoint( quesFst );
				  auto lhs = get<QualifiedValue>( codegen( children[ 2 ] ) )
							   .value( children[ 2 ] )
							   .ensure_is_ptr_if_deref();
				  auto bb0 = Builder.GetInsertBlock();

				  Builder.SetInsertPoint( quesSecd );
				  auto rhs = get<QualifiedValue>( codegen( children[ 4 ] ) )
							   .value( children[ 4 ] )
							   .ensure_is_ptr_if_deref();
				  auto bb1 = Builder.GetInsertBlock();

				  QualifiedValue::cast_ternary_expr( lhs, rhs, node, bb0, bb1 );

				  Builder.SetInsertPoint( bb0 );
				  Builder.CreateBr( quesEnd );
				  Builder.SetInsertPoint( bb1 );
				  Builder.CreateBr( quesEnd );

				  Builder.SetInsertPoint( quesEnd );
				  auto phi = Builder.CreatePHI( lhs.get_type()->type, 2, "phi" );
				  phi->addIncoming( lhs.get(), bb0 );
				  phi->addIncoming( rhs.get(), bb1 );

				  return QualifiedValue( lhs.get_type(), phi );
				  //   return value;
			  }
			  else
			  {
				  if ( auto cc = dyn_cast_or_null<ConstantInt>( value.get() ) )
				  {
					  auto lhs = get<QualifiedValue>( codegen( children[ 2 ] ) )
								   .value( children[ 2 ] )
								   .ensure_is_ptr_if_deref();
					  auto rhs = get<QualifiedValue>( codegen( children[ 4 ] ) )
								   .value( children[ 4 ] )
								   .ensure_is_ptr_if_deref();
					  QualifiedValue::cast_ternary_expr( lhs, rhs, node );

					  return cc->getZExtValue() ? lhs : rhs;
				  }
				  else
				  {
					  infoList->add_msg(
						MSG_TYPE_ERROR,
						fmt( "must be constant" ),
						children[ 0 ] );
					  HALT();
				  }
			  }
		  } ) }

	};

	handlers.insert( expr.begin(), expr.end() );

	return 0;
}
