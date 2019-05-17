#pragma once

#include "predef.h"
#include "type.h"

namespace mty
{
struct Function : Address
{
	static constexpr auto self_type = TypeName::FunctionType;

	std::vector<QualifiedDecl> args;
	bool is_va_args = false;

	Function( Type *result_type, const std::vector<QualifiedDecl> &args, bool is_va_args ) :
	  Address( FunctionType::get( result_type, map_type( args ), is_va_args ) ),
	  args( args ),
	  is_va_args( is_va_args )
	{
		type_name = self_type;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( st.size() != ++id )
		{
			if ( st[ id ]->is<mty::Pointer>() ) os << "(";
			st[ id ]->print( os, st, id );
			if ( st[ id ]->is<mty::Pointer>() ) os << ")";
		}
		os << "(";
		for ( int i = 0; i < args.size(); ++i )
		{
			os << ( i == 0 ? "" : ", " ) << args[ i ].type;
		}
		if ( is_va_args )
		{
			if ( args.size() > 0 ) os << ", ";
			os << "...";
		}
		os << ")";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Function>( *this );
	}

	static void declare( const std::pair<std::shared_ptr<QualifiedType>, Value *> &val, const std::string &name )
	{
		decls().emplace( name, val );
	}

	static Option<const std::pair<std::shared_ptr<QualifiedType>, Value *> *> get( const std::string &name )
	{
		if ( decls().find( name ) != decls().end() )
		{
			return &decls()[ name ];
		}
		return Option<const std::pair<std::shared_ptr<QualifiedType>, Value *> *>();
	}

protected:
	static std::map<std::string, std::pair<std::shared_ptr<QualifiedType>, Value *>> &decls()
	{
		static std::map<std::string, std::pair<std::shared_ptr<QualifiedType>, Value *>> __;
		return __;
	}

	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &fn = static_cast<const Function &>( other );
		if ( this->args.size() != fn.args.size() ) return false;
		for ( int i = 0; i != this->args.size(); ++i )
		{
			if ( !this->args[ i ].type.is_same_without_cv( fn.args[ i ].type ) ) return false;
		}
		return true;
	}

public:
	Value *deref( TypeView &view, Value *val, Json::Value &ast ) const override
	{
		return val;
	}

	Value *offset( TypeView &view, Value *val, Value *off, Json::Value &ast ) const override
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "subscript of pointer to function type `", view, "`" ),
		  ast );
		HALT();
	}

private:
	static std::vector<Type *> map_type( const std::vector<QualifiedDecl> &args )
	{
		std::vector<Type *> new_args;
		for ( auto arg : args ) new_args.push_back( arg.type->type );
		return new_args;
	}
};

}  // namespace mty
