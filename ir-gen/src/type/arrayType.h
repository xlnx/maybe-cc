#pragma once

#include "predef.h"

namespace mty
{
struct Array : Address
{
	static constexpr auto self_type = TypeName::ArrayType;

	Option<std::size_t> len;

	Array( Type *element_type, std::size_t len ) :
	  Address( llvm::ArrayType::get( element_type, len ) ),
	  len( len )
	{
		type_name = self_type;
	}

	Array( Type *element_type ) :
	  Address( element_type )
	{
	}

	Array &set_length( std::size_t len )
	{
		if ( this->len.is_some() ) INTERNAL_ERROR();
		this->len = len;
		this->type = ArrayType::get( this->type, len );
		return *this;
	}

	virtual bool is_valid_parameter_type() const
	{
		return true;
	}

	bool is_allocable() const override
	{
		return true;
	}

	bool is_complete() const override
	{
		return this->len.is_some();
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( st.size() != ++id )
		{
			if ( st[ id ]->is<mty::Pointer>() ) os << "(";
			st[ id ]->print( os, st, id );
			if ( st[ id ]->is<mty::Pointer>() ) os << ")";
		}
		os << "[";
		if ( len.is_some() ) os << len;
		os << "]";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Array>( *this );
	}

protected:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &ref = static_cast<const Array &>( other );
		return ref.len.is_none() && len.is_none() ||
			   ref.len.is_some() && len.is_some() &&
				 ref.len.unwrap() == len.unwrap();
	}

public:
	Value *deref( TypeView &view, Value *val, Json::Value &ast ) const override
	{
		view.next();
		return Builder.CreateConstGEP2_64( val, 0, 0 );
	}

	Value *offset( const TypeView &view, Value *val, Value *off, Json::Value &ast ) const override
	{
		static const auto zero = ConstantInt::get( TheContext, APInt( 64, 0, true ) );
		static Value *indices[ 2 ] = { zero, nullptr };

		indices[ 1 ] = off;
		return Builder.CreateInBoundsGEP( val, indices );
	}
};

}  // namespace mty
