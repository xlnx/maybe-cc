#pragma once

#include "predef.h"

namespace mty
{
struct Integer : Arithmetic
{
	static constexpr auto self_type = TypeName::IntegerType;

	unsigned bits;
	bool is_signed;

	Integer( unsigned bits, bool is_signed, bool is_const = false, bool is_volatile = false ) :
	  Arithmetic( Type::getIntNTy( TheContext, bits ), is_const, is_volatile ),
	  bits( bits ),
	  is_signed( is_signed )
	{
		type_name = self_type;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		if ( !is_signed ) os << "unsigned ";
		switch ( bits )
		{
		case 8: os << "char"; break;
		case 16: os << "short"; break;
		case 32: os << "int"; break;
		case 64: os << "long"; break;
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
		return std::make_shared<Integer>( *this );
	}

protected:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &ref = static_cast<const Integer &>( other );
		return ref.bits == bits && ref.is_signed == is_signed;
	}
};

}  // namespace mty
