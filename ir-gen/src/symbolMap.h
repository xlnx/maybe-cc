#pragma once

#include "llvm/IR/Value.h"
#include "qualifiedType.h"
#include <deque>

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

	int getLevel()
	{
		return symbolStack.size() - 1;
	}

private:
	std::deque<symbolMap> symbolStack;
};