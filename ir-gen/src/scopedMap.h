#pragma once

#include "common.h"
#include "type/def.h"
#include "value/def.h"

struct Symbol
{
	variant<
	  QualifiedValue,
	  QualifiedType>
	  data;

	Symbol( const QualifiedValue &value ) :
	  data( value )
	{
	}

	Symbol( const QualifiedType &type ) :
	  data( type )
	{
	}

public:
	bool is_type() const
	{
		return this->data.index() == 1;
	}
	bool is_value() const
	{
		return this->data.index() == 0;
	}

	const QualifiedType &as_type() const
	{
		return get<QualifiedType>( data );
	}
	const QualifiedValue &as_value() const
	{
		return get<QualifiedValue>( data );
	}
};

template <typename T>
class ScopedMap
{
public:
	const T *find( const std::string &str ) const
	{
		for ( auto target = scopes.rbegin(); target != scopes.rend(); ++target )
		{
			auto sym = target->find( str );
			if ( sym != target->end() )
			{
				return &sym->second;
			}
		}
		return nullptr;
	}

	const T *find_in_scope( const std::string &str, int scope = -1 ) const
	{
		if ( scope == -1 ) scope = get_scope();
		auto &target = scopes.back();
		auto sym = target.find( str );
		if ( sym != target.end() )
		{
			return &sym->second;
		}
		return nullptr;
	}

	template <typename X>
	void insert( const std::string &str, const X &type, Json::Value &node )
	{
		insert_if( str, type, node, []( const std::string &, const T &, const T &, Json::Value & ) { return true; } );
	}

	template <typename X>
	void insert_if( const std::string &str, const X &type, Json::Value &node,
					const std::function<bool( const std::string &, const T &, const T &, Json::Value & )> &cmp_when =
					  []( const std::string &name, const T &, const T &, Json::Value &node ) {
						  infoList->add_msg(
							MSG_TYPE_ERROR,
							fmt( "redefination of `", name, "` as different kind of symbol" ),
							node );
						  return true;
					  } )
	{
		auto &smap = scopes.back();
		const T val = type;
		if ( auto old = find_in_scope( str ) )
		{
			if ( !cmp_when( str, *old, val, node ) ) return;
		}
		smap.emplace( str, std::move( val ) );
	}

	void push()
	{
		scopes.emplace_back();
	}

	void pop()
	{
		scopes.pop_back();
	}

	int get_scope() const
	{
		return scopes.size() - 1;
	}

	friend std::ostream &operator<<( std::ostream &os, ScopedMap &symTable )
	{
		int level = symTable.get_scope();
		auto &symMap = symTable.scopes.back();
		decl_indent = "  ";
		os << "{\n";
		for ( auto iter = symMap.begin(); iter != symMap.end(); iter++ )
		{
			os << decl_indent << iter->first;
			// if ( iter->second.is_value() )
			// {
			// 	os << " : " << iter->second.as_value().get_type();
			// }
			// else
			// {
			// os << " = " << iter->second;  //.as_type();
			// }
			os << "\n";
		}
		os << "}\n";
		decl_indent = "";
		return os;
	}

private:
	std::deque<std::map<std::string, T>> scopes;
};
