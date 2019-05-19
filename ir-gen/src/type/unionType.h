#pragma once

#include "predef.h"
#include "type.h"

extern std::unique_ptr<DataLayout> TheDataLayout;

namespace mty
{
struct Union : Structural
{
	static constexpr auto self_type = TypeName::StructType;

	Option<QualifiedDecl> first_comp;
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

	void set_body( const std::vector<QualifiedDecl> &comps, Json::Value &ast )
	{
		if ( !static_cast<llvm::StructType *>( this->type )->isOpaque() )
		{
			infoList->add_msg( MSG_TYPE_ERROR, fmt( "`union ", name.unwrap(), "` redefined" ), ast );
			HALT();
		}

		if ( comps.size() > 0 )
		{
			first_comp = comps[ 0 ];
		}

		static_cast<llvm::StructType *>( this->type )->setBody( map_comp( comps ) );

		for ( auto &comp : comps )
		{
			this->comps.emplace( comp.name.unwrap(), comp.type );
		}
	}

	const QualifiedType &get_member( const std::string &member, Json::Value &ast ) const
	{
		if ( this->comps.find( member ) != this->comps.end() )
		{
			return this->comps.find( member )->second;
		}
		else
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "union has no member named `", member, "`" ),
			  ast );
			HALT();
		}
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		os << "union";
		if ( this->name.is_some() ) os << " " << this->name.unwrap();
		// if ( !static_cast<llvm::StructType *>( this->type )->isOpaque() )
		// {
		// 	auto indent = decl_indent;
		// 	decl_indent += "  ";
		// 	os << " {\n";
		// 	for ( auto &arg : this->comps )
		// 	{
		// 		os << decl_indent << arg.first << " : " << arg.second << ";\n";
		// 	}
		// 	decl_indent = indent;
		// 	os << decl_indent << "}";
		// }
		if ( st.size() != ++id )
		{
			os << " ";
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
		return this->type == ref.type;
	}

private:
	static std::vector<Type *> map_comp( const std::vector<QualifiedDecl> &comps )
	{
		std::vector<Type *> new_args;
		unsigned max_bytes_size = 0;
		Type *long_ty = nullptr;

		for ( auto comp : comps )
		{
			if ( auto bytes_size = TheDataLayout->getTypeAllocSize( comp.type->type ) )
			{
				if ( bytes_size > max_bytes_size )
				{
					max_bytes_size = bytes_size;
					long_ty = comp.type->type;
				}
			}
		}

		if ( long_ty )
		{
			new_args.emplace_back( long_ty );
		}

		return new_args;
	}
};

}  // namespace mty
