#pragma once

#include "common.h"

struct NoneOpt
{
};

template <typename T>
class Option
{
private:
	variant<T, NoneOpt> val;

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
		return get<T>( val );
	}
	T &unwrap()
	{
		return get<T>( val );
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
	StackTrace( bool flag = true ) :
	  old_flag( stackTrace )
	{
		stackTrace = flag;
	}
	~StackTrace()
	{
		stackTrace = old_flag;
	}

	StackTrace( StackTrace && ) = delete;
	StackTrace( const StackTrace & ) = delete;
	StackTrace &operator=( StackTrace && ) = delete;
	StackTrace &operator=( const StackTrace & ) = delete;
};
