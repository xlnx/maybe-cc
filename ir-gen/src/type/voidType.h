#pragma once

#include "predef.h"

namespace mty
{
struct Void : Qualified
{
	Void( bool is_const = false, bool is_volatile = false ) :
	  Qualified( Type::getVoidTy( TheContext ), is_const, is_volatile )
	{
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		os << "void";
		if ( st.size() != ++id )
		{
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Void>( *this );
	}
};

}  // namespace mty
