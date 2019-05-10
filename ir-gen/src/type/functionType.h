#pragma once

#include "qualified.h"
#include "builder.h"

struct QualifiedFunction : Qualified
{
	std::vector<QualifiedDecl> args;

	QualifiedFunction( Type *result_type, const std::vector<QualifiedDecl> &args ) :
	  Qualified( FunctionType::get( result_type, map_type( args ), false ) ),
	  args( args )
	{
	}

	void print( std::ostream &os ) const override
	{
		Qualified::print( os );
		os << "Fn (";
		for ( auto arg : this->args )
		{
			os << arg << ", ";
		}
		os << "): ";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<QualifiedFunction>( *this );
	}

private:
	static std::vector<Type *> map_type( const std::vector<QualifiedDecl> &args )
	{
		std::vector<Type *> new_args;
		for ( auto arg : args ) new_args.push_back( arg.type );
		return new_args;
	}
};
