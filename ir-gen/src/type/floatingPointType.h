#pragma once

#include "predef.h"

namespace mty
{
struct FloatingPoint : Qualified
{
	unsigned bits;

	FloatingPoint( unsigned bits, bool is_const = false, bool is_volatile = false ) :
	  Qualified( Type::getFloatPtrTy( TheContext, bits ), is_const, is_volatile ),
	  bits( bits )
	{
	}

	void print( std::ostream &os ) const override
	{
		Qualified::print( os );
		os << "f" << bits;
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<FloatingPoint>( *this );
	}

private:
	static Type *get_float_type( unsigned bits )
	{
		switch ( bits )
		{
		case 16: return Type::getHalfTy( TheContext );
		case 32: return Type::getFloatTy( TheContext );
		case 64: return Type::getDoubleTy( TheContext );
		case 128: return Type::getFP128Ty( TheContext );
		default:
		{
			infoList->add_msg( MSG_TYPE_ERROR, "invalid floating point type: f", bits );
			HALT();
		}
		}
	}
};

}  // namespace mty
