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
	};

	handlers.insert( stmt.begin(), stmt.end() );

	return 0;
}
