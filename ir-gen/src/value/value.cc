#include "value.h"

void QualifiedValue::cast_binary_expr( QualifiedValue &self, QualifiedValue &other, Json::Value &node, bool allow_float )
{
	if ( self.is_lvalue || other.is_lvalue )
	{
		INTERNAL_ERROR();
	}

	auto lhs = allow_float ? self.type->as<mty::Arithmetic>() : self.type->as<mty::Integer>();
	auto rhs = allow_float ? other.type->as<mty::Arithmetic>() : other.type->as<mty::Integer>();

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
					self.val = Builder.CreateIntCast( self.val, rhs->type, false );
					self.type = other.type;
				}
				else if ( !lsign && rsign )
				{
					other.val = Builder.CreateIntCast( other.val, lhs->type, false );
					other.type = self.type;
				}
			}
			else if ( lbits < rbits )  // l < r; l -> r
			{
				self.val = Builder.CreateIntCast( self.val, rhs->type, rsign );
				self.type = other.type;
			}
			else
			{
				other.val = Builder.CreateIntCast( other.val, lhs->type, rsign );
				other.type = self.type;
			}
		}
		else
		{
			auto flhs = lhs->as<mty::FloatingPoint>();
			auto frhs = rhs->as<mty::FloatingPoint>();

			if ( ilhs )
			{
				self.val = ilhs->is_signed ? Builder.CreateSIToFP( self.val, rhs->type ) : Builder.CreateUIToFP( self.val, rhs->type );
				self.type = other.type;
			}
			else if ( irhs )
			{
				other.val = irhs->is_signed ? Builder.CreateSIToFP( other.val, lhs->type ) : Builder.CreateUIToFP( other.val, lhs->type );
				other.type = self.type;
			}
			else  // FP FP
			{
				int lbits = flhs->bits;
				int rbits = frhs->bits;

				if ( lbits < rbits )  // l -> r
				{
					self.val = Builder.CreateFPCast( self.val, rhs->type );
					self.type = other.type;
				}
				else  // r -> l;
				{
					other.val = Builder.CreateFPCast( other.val, lhs->type );
					other.type = self.type;
				}
			}
		}
	}
	else
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "invalid operands to binary expression (`", self.type, "` and `", other.type, "`)" ),
		  node );
	}
}

QualifiedValue &QualifiedValue::cast( const TypeView &dst, Json::Value &node )
{
	if ( this->is_lvalue )
	{
		INTERNAL_ERROR();
	}

	if ( dst->is<mty::Arithmetic>() )
	{
		if ( auto dty = dst->as<mty::Integer>() )
		{
			if ( auto ty = this->type->as<mty::Integer>() )
			{
				if ( !dst.is_same_discard_qualifiers( this->type ) )
				{
					this->type = dst;
					this->val = Builder.CreateIntCast( this->val, dst->type, ty->is_signed );
				}
			}
			else
			{
				this->type = dst;
				if ( dty->is_signed )
				{
					this->val = Builder.CreateFPToSI( this->val, dst->type );
				}
				else
				{
					this->val = Builder.CreateFPToUI( this->val, dst->type );
				}
			}
		}
		else  // dty = fp
		{
			if ( auto ty = this->type->as<mty::Integer>() )
			{
				this->type = dst;
				if ( ty->is_signed )
				{
					this->val = Builder.CreateSIToFP( this->val, dst->type );
				}
				else
				{
					this->val = Builder.CreateUIToFP( this->val, dst->type );
				}
			}
			else
			{
				if ( !dst.is_same_discard_qualifiers( this->type ) )
				{
					this->type = dst;
					this->val = Builder.CreateFPCast( this->val, dst->type );
				}
			}
		}
	}
	else if ( dst->is<mty::Struct>() )
	{
		UNIMPLEMENTED();
	}
	else if ( dst->is<mty::Pointer>() )
	{
		if ( this->type->is<mty::Derefable>() )
		{
			auto dest = dst;
			auto type = this->type;

			if ( !type->is<mty::Function>() ) type.next();
			dest.next();

			if ( !dest->is<mty::Void>() && !type->is<mty::Void>() )
			{
				if ( !dest.is_same_discard_qualifiers( type ) )
				{
					infoList->add_msg(
					  MSG_TYPE_WARNING,
					  fmt( "incompatible pointer types assigning to `", dst, "` from `", this->type, "`" ),
					  node );
				}
				else if ( !dest.is_qualifiers_compatible( type ) )
				{
					infoList->add_msg(
					  MSG_TYPE_WARNING,
					  fmt( "assigning to `", dst, "` from `", this->type, "` discards qualifiers" ),
					  node );
				}
			}
			else
			{
				if ( !( int( dest->is_const ) >= int( type->is_const ) &&
						int( dest->is_volatile ) >= int( type->is_volatile ) ) )
				{
					infoList->add_msg(
					  MSG_TYPE_WARNING,
					  fmt( "assigning to `", dst, "` from `", this->type, "` discards qualifiers" ),
					  node );
				}
				if ( !dest->is<mty::Void>() )
				{
					if ( !dest.is_same_discard_qualifiers( TypeView::getCharTy( true ) ) )
					{
						this->val = Builder.CreatePointerCast( this->val, dst->type );
					}
				}
				else if ( !type->is<mty::Void>() )
				{
					if ( !type.is_same_discard_qualifiers( TypeView::getCharTy( true ) ) )
					{
						this->val = Builder.CreatePointerCast( this->val, dst->type );
					}
				}
			}
		}
		else if ( this->type->is<mty::Integer>() )
		{
			infoList->add_msg(
			  MSG_TYPE_WARNING,
			  fmt( "incompatible integer to pointer conversion assigning to `", dst, "` from `", this->type, "`" ),
			  node );
			this->val = Builder.CreateIntToPtr( this->val, dst->type );
		}
		else
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "assigning to `", dst, "` from incompatible type `", this->type, "`" ),
			  node );
			HALT();
		}
	}
	else if ( dst->is<mty::Void>() )
	{
		this->type = dst;
		this->val = nullptr;
	}
	else if ( dst->is<mty::Function>() )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "used type `", dst, "` where arithmetic or pointer type is required" ),
		  node );
		HALT();
	}
	else
	{
		INTERNAL_ERROR();
	}
	return *this;
}
