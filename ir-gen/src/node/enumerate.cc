#include "enumerate.h"

int Enumerate::reg()
{
	static decltype( handlers ) __ = {
		{ "enum_specifier", pack_fn<bool, QualifiedType>( []( Json::Value &node, const bool &curr_scope ) -> QualifiedType {
			  auto &children = node[ "children" ];
			  auto name = children[ 0 ][ 1 ].asString();
			  int an = name == "{" ? 2 : 1;
			  if ( children.size() < 3 )
			  {
				  auto ty = QualifiedType( std::make_shared<mty::Enum>() );
				  symTable.insert( name, ty, node );
				  return ty;
			  }
			  else
			  {
				  std::vector<EnumItem> enums;
				  auto &list = children[ an + 1 ];

				  for ( int i = an + 1; i < children.size() - 1; ++i )
				  {
					  if ( list[ i ].isObject() )
					  {
						  enums.emplace_back( get<EnumItem>( codegen( list[ i ] ) ) );
					  }
				  }

				  //   if ( symTable.findThisLevel )
			  }
		  } ) },
		{ "enumerator", pack_fn<VoidType, EnumItem>( []( Json::Value &node, const VoidType & ) -> EnumItem {
			  //   return EnumItem();
		  } ) }
	};

	for ( auto &_ : __ )
	{
		handlers.emplace( _.first, _.second );
	}

	return 0;
}