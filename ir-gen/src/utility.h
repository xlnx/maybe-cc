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
    extern bool is_debug_mode;
    if (is_debug_mode)
    {
	    std::cerr << fmt( std::forward<Args>( args )... ) << std::endl;
    }
}

namespace __impl
{
struct str_cmp_op
{
	bool operator()( char const *a, char const *b ) const
	{
		return std::strcmp( a, b ) < 0;
	}
};

}  // namespace __impl

template <typename T>
using JumpTable = std::map<const char *, std::function<T>, __impl::str_cmp_op>;

template <typename T>
using LookupTable = std::map<const char *, T, __impl::str_cmp_op>;

inline bool curr_bb_has_ret()
{
	extern llvm::IRBuilder<> Builder;

	auto bb = Builder.GetInsertBlock();
	if ( !bb->empty() )
	{
		auto inst = &bb->back();
		if ( llvm::dyn_cast_or_null<llvm::ReturnInst>( inst ) ||
			 ( [=] {
				 auto br_inst = llvm::dyn_cast_or_null<llvm::BranchInst>( inst );
				 return br_inst && !br_inst->isConditional();
			 } )() )
		{
			return true;
		}
	}
	return false;
}

inline bool secure_exec(const std::function<void()> &wrapped)
{
    using namespace ffi;
    extern MsgList *infoList;
	try
	{
		wrapped();
		return true;
	}
	catch ( std::exception &e )
	{
		infoList->add_msg( MSG_TYPE_ERROR,
						   std::string( "internal error: ir-gen crashed with exception: " ) + e.what() );
		return false;
	}
	catch ( int )
	{
		return false;
	}
	catch ( ... )
	{
		infoList->add_msg( MSG_TYPE_ERROR,
						   std::string( "internal error: ir-gen crashed with unknown error." ) );
		return false;
	}
}
