#pragma once

#include <sstream>
#include <iostream>

#define UNIMPLEMENTED( ... )                                                                              \
	do                                                                                                    \
	{                                                                                                     \
		throw std::logic_error( fmt( "UNIMPLEMENTED:", __FILE__, ":", __LINE__, ":0 ", ##__VA_ARGS__ ) ); \
	} while ( 0 )

static void fmt_helper( std::ostringstream &os )
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