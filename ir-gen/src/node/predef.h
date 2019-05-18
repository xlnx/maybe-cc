#pragma once

#include "../common.h"
#include "../global.h"
#include "../type/def.h"
#include "../value/def.h"

struct VoidType
{
};

struct InitItem
{
	Json::Value ast;
	std::vector<InitItem> childs;
	Option<QualifiedValue> value;
};

using InitList = std::vector<InitItem>;

using AstType = variant<
  int,
  QualifiedValue,
  QualifiedType,
  DeclarationSpecifiers,
  QualifiedDecl,
  QualifiedTypeBuilder *,
  Option<QualifiedValue>,
  InitItem,
  VoidType>;

using ArgsType = variant<
  QualifiedTypeBuilder *,
  TypeView,
  VoidType>;

template <typename I, typename O>
std::function<AstType( Json::Value &, const ArgsType & )>
  pack_fn( const std::function<O( Json::Value &, const I & )> &fn )
{
	return [=]( Json::Value &node, const ArgsType &arg ) -> AstType {
		const I *in;
		try
		{
			in = &get<I>( arg );
		}
		catch ( ... )
		{
			INTERNAL_ERROR( "param assertion failed at ", node );
		}
		return AstType( fn( node, *in ) );
	};
}

using NodeHandler = AstType( Json::Value &, ArgsType const & );

extern AstType codegen( Json::Value &node, const ArgsType &arg = VoidType() );

extern JumpTable<NodeHandler> handlers;
