#include "statement.h"

int Statement::reg()
{
	static decltype( handlers ) stmt = {
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
		{ "expression_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];

			  if ( children.size() > 1 )  // expr ;
			  {
				  codegen( children[ 0 ] );
			  }

			  return VoidType();
		  } ) },
		{ "jump_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  auto typeRet = children[ 0 ][ 1 ].asCString();

			  static JumpTable<VoidType( Json::Value & children, Json::Value & ast )> __ = {
				  { "return", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto fn_res_ty = currentFunction->get_type();
					   fn_res_ty.next();
					   if ( children.size() == 3 )
					   {
						   if ( fn_res_ty->is<mty::Void>() )
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "void function `", funcName, "` should not return a value" ),
								 children[ 1 ] );
							   HALT();
						   }
						   auto retValue = get<QualifiedValue>( codegen( children[ 1 ] ) )
											 .value( children[ 1 ] )
											 .cast( fn_res_ty, children[ 1 ] );
						   Builder.CreateRet( retValue.get() );
					   }
					   else if ( children.size() == 2 )
					   {
						   if ( fn_res_ty->is<mty::Void>() )
						   {
							   Builder.CreateRet( nullptr );
						   }
						   else
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "non-void function `", funcName, "` should return a value" ),
								 ast );
							   HALT();
						   }
					   }
					   else
					   {
						   INTERNAL_ERROR();
					   }

					   return VoidType();
				   } },
				  { "goto", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } },
				  { "continue", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } },
				  { "break", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } }
			  };
			  if ( __.find( typeRet ) != __.end() )
			  {
				  return __[ typeRet ]( children, node );
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
	};

	handlers.insert( stmt.begin(), stmt.end() );

	return 0;
}
