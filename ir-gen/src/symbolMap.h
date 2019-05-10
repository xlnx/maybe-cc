#pragma once

#include "common.h"
#include "qualifiedType.h"

struct symbol
{
	variant<llvm::Value *, QualifiedType> member;
	int level;
};

class symbolMap
{
public:
	symbolMap()
	{
	}

	~symbolMap()
	{
		for ( auto member : symMap )
		{
			symbol *target = member.second;
			delete target;
		}
	}

	symbol *find( const std::string &str ) const
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
		symbol *target = new symbol();
		target->level = level;
		target->member = llvmValue;
		symMap[ str ] = target;
	}

	void insert( const std::string &str, int level, QualifiedType type )
	{
		symbol *target = new symbol();
		target->level = level;
		target->member = type;
		symMap[ str ] = target;
	}

	std::map<std::string, symbol *> &getMap()
	{
		return symMap;
	}

private:
	std::map<std::string, symbol *> symMap;
};

class symbolTable
{
public:
	symbolTable()
	{
		symbolStack.push_back( symbolMap() );
	}
	~symbolTable()
	{
	}

	symbol *find( const std::string &str ) const
	{
		for ( auto target = symbolStack.rbegin(); target != symbolStack.rend(); target++ )
		{
			const symbolMap &symmap = *target;
			symbol *sym = symmap.find( str );
			if ( sym != nullptr )
			{
				return sym;
			}
		}
		return nullptr;
	}

	void insert( const std::string &str, QualifiedType type )
	{
		symbolMap &smap = symbolStack.back();
		smap.insert( str, getLevel(), type );
	}

	void insert( const std::string &str, llvm::Value *llvmValue )
	{
		symbolMap &smap = symbolStack.back();
		smap.insert( str, getLevel(), llvmValue );
	}

	void push()
	{
		symbolStack.push_back( symbolMap() );
	}
	void pop()
	{
		symbolStack.pop_back();
	}

	int getLevel() const
	{
		return symbolStack.size() - 1;
	}

	std::map<std::string, symbol *> &getContent( int level )
	{
		return symbolStack[ level ].getMap();
	}

	friend std::ostream &operator<<( std::ostream &os, symbolTable &symTable )
	{
		dbg( symTable.getLevel() );
		for ( int i = 0; i <= symTable.getLevel(); i++ )
		{
			auto symMap = symTable.getContent( i );
			for ( auto iter = symMap.begin(); iter != symMap.end(); iter++ )
			{
				os << "Name is: " << iter->first << "; Content index is: " << iter->second->member.index() << "\n";
			}
		}
		return os;
	}

private:
	std::deque<symbolMap> symbolStack;
};