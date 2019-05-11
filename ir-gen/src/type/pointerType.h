#pragma once

#include "predef.h"

namespace mty
{
struct Pointer : Derefable
{
	Pointer( Type *base_type, bool is_const = false, bool is_volatile = false ) :
	  Derefable( PointerType::getUnqual( base_type ), is_const, is_volatile )
	{
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		os << " *";
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		if ( st.size() != ++id )
		{
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Pointer>( *this );
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
