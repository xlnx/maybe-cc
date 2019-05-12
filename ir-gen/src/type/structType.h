#pragma once

#include "predef.h"
#include "type.h"

namespace mty
{
struct Struct : Qualified
{
	static constexpr auto self_type = TypeName::StructType;

	struct Defination
	{
		std::map<std::string, QualifiedType> comps;
	};

	std::shared_ptr<Defination> def;
	Option<std::string> name;

	Struct( const std::vector<QualifiedDecl> &comps ) :
	  Qualified( StructType::create( TheContext, map_comp( comps ) ) ),
	  def( std::make_shared<Defination>() )
	{
		type_name = self_type;
		for ( auto &comp : comps )
		{
			if ( comp.name.is_some() )
			{
				this->def->comps.emplace( comp.name.unwrap(), comp.type );
			}
			else
			{
				infoList->add_msg( MSG_TYPE_WARNING, "declaration does not declare anything" );
			}
		}
	}

	Struct &setName( std::string const &new_name )
	{
		this->name = new_name;
		return *this;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		os << "struct";
		if ( this->name.is_some() ) os << " " << this->name.unwrap();
		if ( this->def )
		{
			auto indent = decl_indent;
			decl_indent += "  ";
			os << " {\n";
			for ( auto &arg : this->def->comps )
			{
				os << decl_indent << arg.first << " : " << arg.second << ";\n";
			}
			decl_indent = indent;
			os << decl_indent << "}";
		}
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
		auto &ref = static_cast<Struct const &>( other );
		return this->name.is_some() && ref.name.is_some() &&
			   this->name.unwrap() == ref.name.unwrap();
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
