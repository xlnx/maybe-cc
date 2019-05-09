#pragma once

#include "utility.h"

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
		extern MsgList *infoList;                                                                             \
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
