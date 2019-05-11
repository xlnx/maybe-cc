#include "expression.h"

static QualifiedValue handle_binary_expr( Json::Value &node, VoidType const &_ )
{
	auto &children = node[ "children" ];
	auto lhs = get<QualifiedValue>( codegen( children[ 0 ], _ ) ).value( children[ 0 ] );
	auto rhs = get<QualifiedValue>( codegen( children[ 2 ], _ ) ).value( children[ 2 ] );
	auto op = children[ 1 ][ 1 ].asCString();

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
			 TODO( "pointer arithmetic not implemented" );
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateAdd( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFAdd( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "-", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 TODO( "pointer arithmetic not implemented" );
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateSub( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFSub( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "<<", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 TODO( "fuck shift" );
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateShl( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 infoList->add_msg( MSG_TYPE_ERROR, fmt( "invalid operands to binary expression (`", "` and `", "`)" ), ast );
				 HALT();
			 }
		 } },
		{ ">>", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
		 } },

		{ "<", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
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
		 } },
		{ ">", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
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
		 } },
		{ "<=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
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
		 } },
		{ ">=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
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
		 } },

		{ "==", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateICmpEQ( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpOEQ( lhs.get(), rhs.get() ) );
			 }
		 } },
		{ "!=", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateICmpNE( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 return QualifiedValue( type, Builder.CreateFCmpONE( lhs.get(), rhs.get() ) );
			 }
		 } },

		{ "&", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
		 } },
		{ "^", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
		 } },
		{ "|", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
			 QualifiedValue::cast_binary_expr( lhs, rhs, ast );
			 auto &type = lhs.get_type();
			 if ( auto itype = type->as<mty::Integer>() )
			 {
				 return QualifiedValue( type, Builder.CreateShl( lhs.get(), rhs.get() ) );
			 }
			 else
			 {
				 infoList->add_msg( MSG_TYPE_ERROR, fmt( "invalid operands to binary expression (`", "` and `", "`)" ), ast );
				 HALT();
			 }
		 } },

		{ "&&", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
		 } },
		{ "||", []( QualifiedValue &lhs, QualifiedValue &rhs, Json::Value &ast ) -> QualifiedValue {
			 UNIMPLEMENTED();
		 } }
	};

	if ( __.find( op ) != __.end() )
	{
		return __[ op ]( lhs, rhs, node );
	}
	else
	{
		UNIMPLEMENTED( op );
	}
}

int Expression::reg()
{
	static decltype( handlers ) expr = {
		{ "assignment_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];
			  auto lhs = get<QualifiedValue>( codegen( children[ 0 ] ) ).deref( children[ 0 ] );
			  auto rhs = get<QualifiedValue>( codegen( children[ 2 ] ) ).value( children[ 2 ] );
			  return QualifiedValue( lhs.get_type(), Builder.CreateStore( rhs.get(), lhs.get() ) );
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
					   } }
				  };

				  if ( __.find( key ) != __.end() )
				  {
					  return __[ key ]( children, val, node );
				  }
				  else
				  {
					  UNIMPLEMENTED();
				  }
			  }
			  else
			  {
				  // sizeof ( TYPE )
				  UNIMPLEMENTED();
			  }
		  } ) },
		{ "postfix_expression", pack_fn<VoidType, QualifiedValue>( []( Json::Value &node, VoidType const & ) -> QualifiedValue {
			  auto &children = node[ "children" ];

			  auto key = children[ 1 ][ 1 ].asCString();
			  auto val = get<QualifiedValue>( codegen( children[ 0 ] ) );

			  static JumpTable<QualifiedValue( Json::Value & children, QualifiedValue & val, Json::Value & )> __ = {
				  { "[", []( Json::Value &children, QualifiedValue &val, Json::Value &node ) -> QualifiedValue {
					   auto off = get<QualifiedValue>( codegen( children[ 2 ] ) );
					   return val.value( children[ 0 ] ).offset( off.value( children[ 2 ] ).get(), node );
				   } }
			  };

			  if ( __.find( key ) != __.end() )
			  {
				  return __[ key ]( children, val, node );
			  }
			  else
			  {
				  UNIMPLEMENTED();
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
				  auto val = child[ 1 ].asCString();
				  auto type = child[ 0 ].asCString();

				  static JumpTable<QualifiedValue( const char *, Json::Value & )> __ = {
					  { "IDENTIFIER", []( const char *val, Json::Value &node ) -> QualifiedValue {
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
					  { "INTEGER", []( const char *val, Json::Value &node ) -> QualifiedValue {
						   auto num = std::atoi( val );
						   TODO( "Literal type not implemented" );
						   TODO( "Literal Hex not implemented" );
						   TODO( "Immediate Type" );
						   return QualifiedValue( TypeView::getIntTy( true ),
												  Constant::getIntegerValue(
													Type::getInt32Ty( TheContext ),
													APInt( 32, num, true ) ) );
					   } }
				  };

				  if ( __.find( type ) != __.end() )
				  {
					  return __[ type ]( val, child );
				  }
				  else
				  {
					  UNIMPLEMENTED();
					  //   INTERNAL_ERROR();
				  }
			  }
		  } ) },

		{ "binary_expression", pack_fn<VoidType, QualifiedValue>( handle_binary_expr ) }
	};

	handlers.insert( expr.begin(), expr.end() );

	return 0;
}
