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

	bool is_array_type() const override { return true; }

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Array>( *this );
	}

public:
	Value *deref( TypeView &view, Value *val, Json::Value &ast ) const override
	{
		view.next();
		return Builder.CreateConstGEP1_32( val, 0 );
	}

	Value *offset( TypeView &view, Value *val, Value *off, Json::Value &ast ) const override
	{
		view.next();
		return Builder.CreateInBoundsGEP( val, off );
	}
};

}  // namespace mty