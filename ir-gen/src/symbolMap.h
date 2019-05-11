#pragma once

#include "common.h"
#include "type/def.h"

struct Symbol
{
	variant<llvm::Value *, QualifiedType> member;
	int level;

public:
	bool is_type() const
	{
		return this->member.index() == 1;
	}
	bool is_value() const
	{
		return this->member.index() == 0;
	}

	const QualifiedType &as_type() const
	{
		return get<QualifiedType>( member );
	}
	llvm::Value *as_value() const
	{
		return get<llvm::Value *>( member );
	}
};

namespace __impl
{
class SymbolMap
{
public:
	SymbolMap()
	{
	}

	~SymbolMap()
	{
		for ( auto member : symMap )
		{
			Symbol *target = member.second;
			delete target;
		}
	}

	Symbol *find( const std::string &str ) const
	{
		auto target = symMap.find( str );
		if ( target != symMap.end() )
		{
			return ( *target ).second;
		}
		return nullptr;
	}

	void insert( const std::string &str, int level, llvm::Value *llvmValue )
	{
		Symbol *target = new Symbol();
		target->level = level;
		target->member = llvmValue;
		symMap[ str ] = target;
	}

	void insert( const std::string &str, int level, const QualifiedType &type )
	{
		Symbol *target = new Symbol();
		target->level = level;
		target->member = type;
		symMap[ str ] = target;
	}

	std::map<std::string, Symbol *> &getMap()
	{
		return symMap;
	}

private:
	std::map<std::string, Symbol *> symMap;
};

}  // namespace __impl

class SymbolTable
{
	using SymbolMap = __impl::SymbolMap;

public:
	SymbolTable()
	{
		symbolStack.push_back( SymbolMap() );
	}
	~SymbolTable()
	{
	}

	Symbol *find( const std::string &str ) const
	{
		for ( auto target = symbolStack.rbegin(); target != symbolStack.rend(); target++ )
		{
			const SymbolMap &symmap = *target;
			Symbol *sym = symmap.find( str );
			if ( sym != nullptr )
			{
				return sym;
			}
		}
		return nullptr;
	}

	Symbol *findThisLevel( const std::string &str ) const
	{
		const SymbolMap &smap = symbolStack.back();
		Symbol *sym = smap.find( str );
		if ( sym != nullptr )
		{
			return sym;
		}
		return nullptr;
	}

	void insert( const std::string &str, const QualifiedType &type )
	{
		SymbolMap &smap = symbolStack.back();
		smap.insert( str, getLevel(), type );
	}

	void insert( const std::string &str, llvm::Value *llvmValue )
	{
		SymbolMap &smap = symbolStack.back();
		smap.insert( str, getLevel(), llvmValue );
	}

	void push()
	{
		symbolStack.push_back( SymbolMap() );
	}
	void pop()
	{
		symbolStack.pop_back();
	}

	int getLevel() const
	{
		return symbolStack.size() - 1;
	}

	std::map<std::string, Symbol *> &getContent( int level )
	{
		return symbolStack[ level ].getMap();
	}

	friend std::ostream &operator<<( std::ostream &os, SymbolTable &symTable )
	{
		int level = symTable.getLevel();
		auto symMap = symTable.getContent( level );
		os << "{\n";
		for ( auto iter = symMap.begin(); iter != symMap.end(); iter++ )
		{
			os << "\tName is: " << iter->first << ";\n"
			   << "\tContent index is: " << iter->second->member.index() << ";\n";
		}
		os << "}\n";
		return os;
	}

private:
	std::deque<SymbolMap> symbolStack;
};
