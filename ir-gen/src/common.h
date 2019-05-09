#pragma once

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>

#include <variant.hpp>

#include "msglist.h"
#include "symbolMap.h"

using namespace ffi;
using namespace llvm;
using namespace nonstd;

extern LLVMContext TheContext;
extern IRBuilder<> Builder;
extern std::unique_ptr<Module> TheModule;
extern std::map<std::string, Value *> NamedValues;
extern Function *currentFunction;
extern BasicBlock *currentBB;
extern ffi::MsgList *infoList;
extern symbolTable symTable;
extern bool stackTrace;

#define UNIMPLEMENTED( ... )                                                                              \
	do                                                                                                    \
	{                                                                                                     \
		throw std::logic_error( fmt( "UNIMPLEMENTED:", __FILE__, ":", __LINE__, ":0 ", ##__VA_ARGS__ ) ); \
	} while ( 0 )

#define INTERNAL_ERROR( ... )                                                                              \
	do                                                                                                     \
	{                                                                                                      \
		throw std::logic_error( fmt( "INTERNAL ERROR:", __FILE__, ":", __LINE__, ":0 ", ##__VA_ARGS__ ) ); \
	} while ( 0 )

#define TODO( ... )                                                                                           \
	do                                                                                                        \
	{                                                                                                         \
		infoList->add_msg( MSG_TYPE_WARNING, fmt( "TODO:", __FILE__, ":", __LINE__, ":0 ", ##__VA_ARGS__ ) ); \
	} while ( 0 )

#define HALT()      \
	do              \
	{               \
		throw( 0 ); \
	} while ( 0 )

#define ASSERT_FN( T, ... )                                            \
	( []( Json::Value &node, const ArgsType &arg ) -> AstType {        \
		auto ___ = ( [&] -> AstType { __VA_ARGS__ } )();               \
		try                                                            \
		{                                                              \
			auto &____ = get<T>( ___ );                                \
		}                                                              \
		catch ( ... )                                                  \
		{                                                              \
			INTERNAL_ERROR( "function return type assertion failed" ); \
		}                                                              \
		return ___;                                                    \
	} )

inline void fmt_helper( std::ostringstream &os )
{
}

template <typename T, typename... Args>
void fmt_helper( std::ostringstream &os, T &&x, Args &&... args )
{
	os << std::forward<T>( x );  // << " ";
	fmt_helper( os, std::forward<Args>( args )... );
}

template <typename... Args>
std::string fmt( Args &&... args )
{
	std::ostringstream os;
	fmt_helper( os, std::forward<Args>( args )... );
	return os.str();
}

template <typename... Args>
void dbg( Args &&... args )
{
	std::cerr << fmt( std::forward<Args>( args )... ) << std::endl;
}
