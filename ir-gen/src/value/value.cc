#include "value.h"

void QualifiedValue::cast_binary_expr( QualifiedValue &self, QualifiedValue &other, Json::Value &node )
{
	if ( self.is_lvalue || other.is_lvalue )
	{
		INTERNAL_ERROR();
	}

	auto lhs = self.type->as<mty::Arithmetic>();
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
				other.val = ilhs->is_signed ? Builder.CreateSIToFP( other.val, lhs->type ) : Builder.CreateUIToFP( other.val, lhs->type );
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