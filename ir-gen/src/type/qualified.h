#pragma once

#include "../common.h"

extern LLVMContext TheContext;
extern MsgList *infoList;

struct Qualified
{
	Type *type;
	bool is_const;
	bool is_volatile;

	Qualified( Type *type, bool is_const = false, bool is_volatile = false ) :
	  type( type ),
	  is_const( is_const ),
	  is_volatile( is_volatile )
	{
	}

	virtual ~Qualified() = default;

	virtual void print( std::ostream &os ) const
	{
		if ( is_const ) os << "c";
		if ( is_volatile ) os << "v";
	}

	virtual std::shared_ptr<Qualified> clone() const = 0;
};
