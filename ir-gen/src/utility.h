#pragma once

#include <variant.hpp>
#include <iostream>

struct NoneOpt
{
};

template <typename T>
class Option
{
private:
	nonstd::variant<T, NoneOpt> val;

public:
	Option() :
	  val( NoneOpt{} )
	{
	}
	Option( const T &val ) :
	  val( val )
	{
	}
	Option( const Option & ) = default;
	Option( Option && ) = default;
	Option &operator=( const Option & ) = default;
	Option &operator=( Option && ) = default;

public:
	Option &operator=( const T &val )
	{
		this->val = val;
		return *this;
	}
	Option &clear()
	{
		this->val = NoneOpt{};
		return *this;
	}
	bool is_some() const
	{
		return val.index() == 0;
	}
	bool is_none() const
	{
		return val.index() == 1;
	}
	const T &unwrap() const
	{
		return nonstd::get<T>( val );
	}
	T &unwrap()
	{
		return nonstd::get<T>( val );
	}

	friend std::ostream &operator<<( std::ostream &os, const Option &opt )
	{
		if ( opt.is_none() )
			os << "None";
		else
			os << opt.unwrap();
		return os;
	}
};

struct StackTrace
{
private:
	bool old_flag;

public:
	StackTrace( bool flag = true )
	{
		extern bool stack_trace;
		old_flag = stack_trace;
		stack_trace = flag;
	}
	~StackTrace()
	{
		extern bool stack_trace;
		stack_trace = old_flag;
	}

	StackTrace( StackTrace && ) = delete;
	StackTrace( const StackTrace & ) = delete;
	StackTrace &operator=( StackTrace && ) = delete;
	StackTrace &operator=( const StackTrace & ) = delete;
};

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
