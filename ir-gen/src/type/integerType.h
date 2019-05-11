#pragma once

#include "predef.h"

namespace mty
{
struct Integer : Arithmetic
{
	unsigned bits;
	bool is_signed;

	Integer( unsigned bits, bool is_signed, bool is_const = false, bool is_volatile = false ) :
	  Arithmetic( Type::getIntNTy( TheContext, bits ), is_const, is_volatile ),
	  bits( bits ),
	  is_signed( is_signed )
	{
	}

	void print( std::ostream &os ) const override
	{
		Arithmetic::print( os );
		if ( is_signed )
			os << "i" << bits;
		else
			os << "u" << bits;
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Integer>( *this );
	}
};

}  // namespace mty
