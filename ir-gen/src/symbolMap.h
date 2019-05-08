#pragma once

#include "llvm/IR/Value.h"
#include <deque>

struct symbol
{
	llvm::Value *member;
	int level;
};

class symbolMap
{
public:
	symbolMap();
	~symbolMap();

	symbol *find( const std::string &str ) const;

	void insert( const std::string &str, int level, llvm::Value *llvmValue );

private:
	std::map<std::string, symbol *> symMap;
};

symbolMap::symbolMap()
{
}

symbolMap::~symbolMap()
{
	for ( auto member : symMap )
	{
		symbol *target = member.second;
		delete target;
	}
}

symbol *symbolMap::find( const std::string &str ) const
{
	auto target = symMap.find( str );
	if ( target != symMap.end() )
	{
		return ( *target ).second;
	}
	return nullptr;
}

void symbolMap::insert( const std::string &str, int level, llvm::Value *llvmValue )
{
	symbol *target = new symbol();
	target->level = level;
	target->member = llvmValue;
	symMap[ str ] = target;
}

class symbolTable
{
public:
	symbolTable();
	~symbolTable();

	symbol *find( const std::string &str ) const;

	void insert( const std::string &str, llvm::Value *llvmValue );

	void push();
	void pop();

	int getLevel();

private:
	std::deque<symbolMap> symbolStack;
};

symbolTable::symbolTable()
{
	symbolStack.push_back( symbolMap() );
}

symbol *symbolTable::find( const std::string &str ) const
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

void symbolTable::insert( const std::string &str, llvm::Value *symbol )
{
	symbolMap &smap = symbolStack.back();
	smap.insert( str, getLevel(), symbol );
}

void symbolTable::push()
{
	symbolStack.push_back( symbolMap() );
}

void symbolTable::pop()
{
	symbolStack.pop_back();
}

int symbolTable::getLevel()
{
	return symbolStack.size() - 1;
}
