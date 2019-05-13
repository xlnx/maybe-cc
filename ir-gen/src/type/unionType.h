#pragma once

#include "predef.h"
#include "type.h"

namespace mty
{
struct Union : Structural
{
	static constexpr auto self_type = TypeName::StructType;

	std::map<std::string, QualifiedType> comps;
	Option<std::string> name;

	Union() :
	  Structural( StructType::create( TheContext ) )
	{
		type_name = self_type;
	}

	Union( const std::string &name ) :
	  Structural( StructType::create( TheContext, "union." + name ) )
	{
		type_name = self_type;
		this->name = name;
	}

	void setBody( const std::vector<QualifiedDecl> &comps, Json::Value &ast )
	{
		if ( !static_cast<llvm::StructType *>( this->type )->isOpaque() )
		{
			infoList->add_msg( MSG_TYPE_ERROR, fmt( "`union ", name.unwrap(), "` redefined" ), ast );
			HALT();
		}
		static_cast<llvm::StructType *>( this->type )->setBody( map_comp( comps ) );
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
		os << "union";
		if ( this->name.is_some() ) os << " " << this->name.unwrap();
		if ( !static_cast<llvm::StructType *>( this->type )->isOpaque() )
		{
			auto indent = decl_indent;
			decl_indent += "  ";
			os << " {\n";
			for ( auto &arg : this->comps )
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
		return std::make_shared<Union>( *this );
	}

public:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &ref = static_cast<Union const &>( other );
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
