#pragma once

#include "predef.h"

namespace mty
{
struct Pointer : Derefable
{
	static constexpr auto self_type = TypeName::PointerType;

	Pointer( Type *base_type, bool is_const = false, bool is_volatile = false ) :
	  Derefable( PointerType::getUnqual( base_type ), is_const, is_volatile )
	{
		type_name = self_type;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const && is_volatile )
			os << "*const volatile";
		else if ( is_const )
			os << "*const";
		else if ( is_volatile )
			os << "*volatile";
		else
			os << "*";
		if ( st.size() != ++id )
		{
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Pointer>( *this );
	}

protected:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		return true;
	}

public:
	Value *deref( TypeView &view, Value *val, Json::Value &ast ) const override
	{
		view.next();
		return val;
	}

	Value *offset( TypeView &view, Value *val, Value *off, Json::Value &ast ) const override
	{
		view.next();
		return Builder.CreateGEP( val, off );
	}
};

}  // namespace mty
