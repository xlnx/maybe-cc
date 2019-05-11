#pragma once

#include "predef.h"
#include "builder.h"

namespace mty
{
struct Struct : Qualified
{
	std::map<std::string, QualifiedType> comps;

	Struct( const std::vector<QualifiedDecl> &comps ) :
	  Qualified( StructType::create( TheContext, map_comp( comps ) ) )
	{
		for ( auto &comp : comps )
		{
			if ( comp.name.is_some() )
			{
				this->comps.emplace( comp.name.unwrap(), comp.type );
			}
			else
			{
				infoList->add_msg( MSG_TYPE_WARNING, "declaration does not declare anything" );
			}
		}
	}

	void print( std::ostream &os ) const override
	{
		Qualified::print( os );
		os << "Struct {";
		for ( auto comp : this->comps )
		{
			os << comp.first << ": " << comp.second << ", ";
		}
		os << "}";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Struct>( *this );
	}

private:
	static std::vector<Type *> map_comp( const std::vector<QualifiedDecl> &comps )
	{
		std::vector<Type *> new_args;
		for ( auto arg : comps ) new_args.push_back( arg.type );
		return new_args;
	}
};

}  // namespace mty
