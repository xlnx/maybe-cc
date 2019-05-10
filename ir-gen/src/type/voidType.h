#pragma once

#include "qualified.h"

struct QualifiedVoid : Qualified
{
	QualifiedVoid( bool is_const = false, bool is_volatile = false ) :
	  Qualified( Type::getVoidTy( TheContext ), is_const, is_volatile )
	{
	}

	void print( std::ostream &os ) const override
	{
		Qualified::print( os );
		os << "Void";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<QualifiedVoid>( *this );
	}
};
