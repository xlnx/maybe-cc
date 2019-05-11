#pragma once

#include "common.h"
#include "type/def.h"
#include "value/def.h"

namespace __impl
{
class SymbolMap;
}

struct Symbol
{
	friend class __impl::SymbolMap;

	variant<
	  QualifiedValue,
	  QualifiedType>
	  data;
	int level;

	Symbol( const QualifiedValue &value, int level ) :
	  data( value ),
	  level( level )
	{
	}

	Symbol( const QualifiedType &type, int level ) :
	  data( type ),
	  level( level )
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

class SymbolTable
{
public:
	SymbolTable()
	{
		symbolStack.emplace_back();
	}

	const Symbol *find( const std::string &str ) const
	{
		for ( auto target = symbolStack.rbegin(); target != symbolStack.rend(); ++target )
		{
			auto sym = target->find( str );
			if ( sym != target->end() )
			{
				return &sym->second;
			}
		}
		return nullptr;
	}

	const Symbol *findThisLevel( const std::string &str ) const
	{
		auto &target = symbolStack.back();
		auto sym = target.find( str );
		if ( sym != target.end() )
		{
			return &sym->second;
		}
		return nullptr;
	}

	void insert( const std::string &str, const QualifiedType &type )
	{
		auto &smap = symbolStack.back();
		smap.emplace( str, Symbol( type, getLevel() ) );
	}

	void insert( const std::string &str, const QualifiedValue &value )
	{
		auto &smap = symbolStack.back();
		smap.emplace( str, Symbol( value, getLevel() ) );
	}

	void push()
	{
		symbolStack.emplace_back();
	}

	void pop()
	{
		symbolStack.pop_back();
	}

	int getLevel() const
	{
		return symbolStack.size() - 1;
	}

	friend std::ostream &operator<<( std::ostream &os, SymbolTable &symTable )
	{
		int level = symTable.getLevel();
		auto &symMap = symTable.symbolStack.back();
		os << "{\n";
		for ( auto iter = symMap.begin(); iter != symMap.end(); iter++ )
		{
			os << "\tName is: " << iter->first << ";\n"
			   << "\tContent index is: " << iter->second.data.index() << ";\n";
		}
		os << "}\n";
		return os;
	}

private:
	std::deque<std::map<std::string, Symbol>> symbolStack;
};
