#pragma once

#include "../common.h"
#include "../type/def.h"

extern LLVMContext TheContext;
extern MsgList *infoList;

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
			if ( auto derefable = dynamic_cast<const mty::Derefable *>( type.get() ) )
			{
				val = derefable->deref( type, val, ast );
				is_lvalue = !dynamic_cast<const mty::Address *>( type.get() );
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
			if ( auto derefable = dynamic_cast<const mty::Derefable *>( type.get() ) )
			{
				val = derefable->offset( type, val, off, ast );
				is_lvalue = !dynamic_cast<const mty::Address *>( type.get() );
			}
			else
			{
				infoList->add_msg( MSG_TYPE_ERROR, "not derefable", ast );
				HALT();
			}
		}
		return *this;
	}
};
