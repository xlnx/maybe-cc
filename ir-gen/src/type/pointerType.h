#pragma once

#include "qualified.h"

struct QualifiedPointer : Qualified
{
	QualifiedPointer( Type *base_type, bool is_const = false, bool is_volatile = false ) :
	  Qualified( PointerType::getUnqual( base_type ), is_const, is_volatile )
	{
	}

	void print( std::ostream &os ) const override { os << "Ptr: "; }

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<QualifiedPointer>( *this );
	}
};
