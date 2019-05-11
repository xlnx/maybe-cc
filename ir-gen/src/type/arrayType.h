#pragma once

#include "predef.h"

namespace mty
{
struct Array : Address
{
	std::size_t len;

	Array( Type *element_type, std::size_t len ) :
	  Address( ArrayType::get( element_type, len ) ),
	  len( len )
	{
	}

	void print( std::ostream &os ) const override
	{
		Address::print( os );
		os << "[" << len << "]: ";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Array>( *this );
	}

public:
	Value *deref( TypeView &view, Value *val, Json::Value &ast ) const override
	{
		view.next();
		return Builder.CreateConstGEP2_64( val, 0, 0 );
	}

	Value *offset( TypeView &view, Value *val, Value *off, Json::Value &ast ) const override
	{
		static const auto zero = ConstantInt::get( TheContext, APInt( 64, 0, true ) );
		static Value *indices[ 2 ] = { zero, nullptr };

		view.next();
		indices[ 1 ] = off;
		return Builder.CreateInBoundsGEP( val, indices );
	}
};

}  // namespace mty
