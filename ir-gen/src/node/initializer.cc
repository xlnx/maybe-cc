#include "initializer.h"

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
		  } ) }
	};

	handlers.insert( expr.begin(), expr.end() );

	return 0;
}