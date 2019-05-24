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

inline std::ostream &operator<<( std::ostream &os, const InitList &list )
{
	os << "{ ";
	for ( auto &e : list )
	{
		os << ( e.value.is_some() ? "e " : "[] " );
	}
	os << "}";
	return os;
}

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
  bool,
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

template <typename T>
QualifiedType forward_decl( const std::string &name, const std::string &fullName, bool curr_scope, Json::Value &ast )
{
	static_assert( std::is_base_of<mty::Qualified, T>::value, "" );

	if ( auto sym = curr_scope ? symTable.find_in_scope( fullName ) : symTable.find( fullName ) )
	{
		dbg( "found: ", fullName );
		if ( sym->is_type() ) return sym->as_type();
		INTERNAL_ERROR();
	}
	else
	{
		dbg( "not found: ", fullName );
		auto ty = QualifiedType( std::make_shared<T>( name ) );
		symTable.insert_if( fullName, ty, ast );
		return ty;
	}
}

inline void fix_forward_decl( const std::string &fullName, const QualifiedType &type )
{
	Json::Value ast;
	symTable.insert( fullName, type, ast );
}
