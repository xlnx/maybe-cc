#pragma once

#include "predef.h"
#include "type.h"

namespace mty
{
struct Struct : Qualified
{
	static constexpr auto type = TypeName::StructType;

	std::map<std::string, QualifiedType> comps;

	Struct( const std::vector<QualifiedDecl> &comps ) :
	  Qualified( StructType::create( TheContext, map_comp( comps ) ) )
	{
		type_name = type;
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

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		os << "struct";
		if ( st.size() != ++id )
		{
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Struct>( *this );
	}

public:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		UNIMPLEMENTED();
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
