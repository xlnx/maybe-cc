#pragma once

#include "predef.h"
#include "type.h"

namespace mty
{
struct Function : Address
{
	std::vector<QualifiedDecl> args;

	Function( Type *result_type, const std::vector<QualifiedDecl> &args ) :
	  Address( FunctionType::get( result_type, map_type( args ), false ) ),
	  args( args )
	{
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
		for ( auto &arg : args )
		{
			os << arg.type << ", ";
		}
		os << ")";
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Function>( *this );
	}

public:
	Value *deref( TypeView &view, Value *val, Json::Value &ast ) const override
	{
		return val;
	}

	Value *offset( TypeView &view, Value *val, Value *off, Json::Value &ast ) const override
	{
		infoList->add_msg( MSG_TYPE_ERROR, "subscribing a function is not allowed", ast );
		HALT();
	}

private:
	static std::vector<Type *> map_type( const std::vector<QualifiedDecl> &args )
	{
		std::vector<Type *> new_args;
		for ( auto arg : args ) new_args.push_back( arg.type );
		return new_args;
	}
};

}  // namespace mty
