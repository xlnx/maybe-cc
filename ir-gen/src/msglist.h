#pragma once

#include <cstring>
#include <cassert>
#include <utility>
#include <new>
#include <string>
#include <vector>
#include <jsoncpp/json.h>

namespace ffi
{
#define MSG_LIST_MIN_CAP 16

struct Msg final
{
	std::size_t pos[ 4 ] = {};
	char *msg;

public:
	Msg( const Json::Value &cxx_pos,
		 const std::string &cxx_msg ) :
	  msg( (char *)malloc( sizeof( char ) * cxx_msg.length() + 1 ) )
	{
		memcpy( msg, cxx_msg.c_str(), cxx_msg.length() + 1 );
		assert( cxx_pos.size() == 4 );
		for ( auto i = 0; i < 4; i++ )
		{
			pos[ i ] = cxx_pos[ i ].asUInt64();
		}
	}
	~Msg()
	{
		free( msg );
	}

	Msg( const Msg & ) = delete;
	Msg( Msg && ) = delete;
	Msg &operator=( const Msg & ) = delete;
	Msg &operator=( Msg && ) = delete;
};

struct MsgList final
{
	Msg *msgs;
	std::size_t len;
	std::size_t cap;

public:
	MsgList() :
	  len( 0 ),
	  cap( MSG_LIST_MIN_CAP ),
	  msgs( (Msg *)malloc( sizeof( Msg ) * MSG_LIST_MIN_CAP ) )
	{
	}
	~MsgList()
	{
		for ( auto i = 0; i < len; ++i )
		{
			msgs[ i ].~Msg();
		}
		free( msgs );
	}

	MsgList( MsgList && ) = delete;
	MsgList( const MsgList & ) = delete;
	MsgList &operator=( const MsgList & ) = delete;
	MsgList &operator=( MsgList && ) = delete;

public:
	template <typename... Args>
	void add_msg( Args &&... args )
	{
		if ( !( len < cap ) )
		{
			cap <<= 1;
			auto new_msgs = (Msg *)malloc( sizeof( Msg ) * cap );
			memcpy( new_msgs, msgs, sizeof( Msg ) * len );
			free( msgs );
			msgs = new_msgs;
		}
		new ( msgs + len++ ) Msg( std::forward<Args>( args )... );
	}
};

}  // namespace ffi