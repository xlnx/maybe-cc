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
				infoList->add_msg( MSG_TYPE_ERROR, "not derefable", ast );
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
				infoList->add_msg( MSG_TYPE_ERROR, "not derefable", ast );
				HALT();
			}
		}
		return *this;
	}

public:
	QualifiedValue &cast_binary_expr( QualifiedValue &other, Json::Value &node )
	{
		if ( is_lvalue || other.is_lvalue )
		{
			INTERNAL_ERROR();
		}

		auto lhs = type->as<mty::Arithmetic>();
		auto rhs = other.type->as<mty::Arithmetic>();

		if ( lhs && rhs )
		{
			auto ilhs = lhs->as<mty::Integer>();
			auto irhs = rhs->as<mty::Integer>();

			if ( ilhs && irhs )
			{
				auto lsign = ilhs->is_signed;
				auto rsign = irhs->is_signed;

				auto lbits = ilhs->bits;
				auto rbits = irhs->bits;

				if ( lbits == rbits )
				{
					if ( lsign && !rsign )  // l < r; l -> r
					{
						this->val = Builder.CreateIntCast( this->val, rhs->type, false );
					}
					else if ( !lsign && rsign )
					{
						other.val = Builder.CreateIntCast( other.val, lhs->type, false );
					}
				}
				else if ( lbits < rbits )  // l < r; l -> r
				{
					this->val = Builder.CreateIntCast( this->val, rhs->type, rsign );
				}
				else
				{
				}
			}
			else
			{
				UNIMPLEMENTED();
			}
			// if ( auto ilhs = lhs->as<mty::Integer> )
			// {
			// }
			UNIMPLEMENTED( "cast" );
		}
		else
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "invalid operands to binary expression (`", type, "` and `", other.type, "`)" ),
			  node );
		}
		return *this;
	}
};
