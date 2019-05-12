#pragma once

#include "predef.h"

namespace mty
{
struct FloatingPoint : Arithmetic
{
	static constexpr auto self_type = TypeName::FloatingPointType;

	unsigned bits;

	FloatingPoint( unsigned bits, bool is_const = false, bool is_volatile = false ) :
	  Arithmetic( get_float_type( bits ), is_const, is_volatile ),
	  bits( bits )
	{
		type_name = self_type;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		switch ( bits )
		{
		case 32: os << "float"; break;
		case 64: os << "double"; break;
		case 128: os << "long double"; break;
		default: INTERNAL_ERROR();
		}
		if ( st.size() != ++id )
		{
			os << " ";
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<FloatingPoint>( *this );
	}

protected:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &ref = static_cast<const FloatingPoint &>( other );
		return ref.bits == bits;
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
