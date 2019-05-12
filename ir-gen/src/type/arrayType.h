#pragma once

#include "predef.h"

namespace mty
{
struct Array : Address
{
	static constexpr auto self_type = TypeName::ArrayType;

	std::size_t len;

	Array( Type *element_type, std::size_t len ) :
	  Address( ArrayType::get( element_type, len ) ),
	  len( len )
	{
		type_name = self_type;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( st.size() != ++id )
		{
			if ( st[ id ]->is<mty::Pointer>() ) os << "(";
			st[ id ]->print( os, st, id );
			if ( st[ id ]->is<mty::Pointer>() ) os << ")";
		}
		os << "[" << len << "]";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Array>( *this );
	}

protected:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &ref = static_cast<const Array &>( other );
		return ref.len == len;
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
