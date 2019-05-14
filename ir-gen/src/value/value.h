#pragma once

#include "predef.h"

class QualifiedValue
{
private:
	TypeView type;
	Value *val;
	bool is_lvalue;

public:
	QualifiedValue( const TypeView &type, Value *val, bool is_lvalue = false ) :
	  type( type ),
	  val( val ),
	  is_lvalue( is_lvalue )
	{
	}

	QualifiedValue( const std::shared_ptr<QualifiedType> &type,
					AllocaInst *alloc, bool is_lvalue = false ) :
	  type( type ),
	  val( alloc ),
	  is_lvalue( is_lvalue )
	{
	}

	Value *get() const
	{
		return val;
	}
	const TypeView &get_type() const
	{
		return type;
	}
	bool is_rvalue() const
	{
		return !is_lvalue;
	}

	QualifiedValue &value( Json::Value &ast )  // cast to rvalue
	{
		if ( is_lvalue )
		{
			val = Builder.CreateLoad( val );
			is_lvalue = false;
		}
		return *this;
	}
	QualifiedValue &deref( Json::Value &ast )  // cast to lvalue?
	{
		if ( !is_lvalue )
		{
			if ( auto derefable = type->as<mty::Derefable>() )
			{
				val = derefable->deref( type, val, ast );
				is_lvalue = !type->is<mty::Address>();
			}
			else
			{
				infoList->add_msg( MSG_TYPE_ERROR, "lvalue required for dereference", ast );
				HALT();
			}
		}
		return *this;
	}
	QualifiedValue &offset( Value *off, Json::Value &ast )  // cast to lvalue?
	{
		if ( !is_lvalue )
		{
			if ( auto derefable = type->as<mty::Derefable>() )
			{
				val = derefable->offset( type, val, off, ast );
				is_lvalue = !type->is<mty::Address>();
			}
			else
			{
				infoList->add_msg( MSG_TYPE_ERROR, "lvalue required for dereference", ast );
				HALT();
			}
		}
		return *this;
	}
	QualifiedValue &call( std::vector<QualifiedValue> &args, Json::Value &ast )
	{
		if ( is_lvalue )
		{
			INTERNAL_ERROR();
		}

		auto &children = ast[ "children" ];
		while ( type->is<mty::Pointer>() )
		{
			deref( children[ 0 ] ).value( children[ 0 ] );
		}
		if ( auto fn = type->as<mty::Function>() )
		{
			if ( args.size() != fn->args.size() )
			{
				infoList->add_msg(
				  MSG_TYPE_ERROR,
				  fmt( "too ", args.size() > fn->args.size() ? "many" : "few",
					   " arguments to function call, exprected ", fn->args.size(),
					   ", have ", args.size() ),
				  ast );
				HALT();
			}
			std::vector<Value *> args_val;
			for ( auto i = 0; i != args.size(); ++i )
			{
				args_val.emplace_back(
				  args[ i ].cast(
							 TypeView( std::make_shared<QualifiedType>( fn->args[ i ].type ) ),
							 children[ i + 2 ] )
					.get() );
			}
			this->type.next();
			this->val = Builder.CreateCall( this->val, args_val );
		}
		else
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "called object type `", type,
				   "` is not a function or function pointer" ),
			  children[ 0 ] );
			HALT();
		}
		return *this;
	}

public:
	static void cast_binary_expr( QualifiedValue &self, QualifiedValue &other, Json::Value &node, bool allow_float = true );
	QualifiedValue &cast( const TypeView &dst, Json::Value &node );
};
