#include "value.h"

static bool deref_into_ptr_unwrap( TypeView &view, Value *&val )
{
	if ( view->is<mty::Derefable>() )
	{
		if ( !view->is<mty::Function>() )
		{
			if ( auto deref = view->as<mty::Array>() )
			{
				Json::Value ast;
				val = deref->deref( view, val, ast );
			}
			else
			{
				view.next();
			}
		}
		return true;
	}
	return false;
}

bool QualifiedValue::cast_binary_ptr( QualifiedValue &self, QualifiedValue &other, Json::Value &node )
{
	auto lhs_ty = self.type;
	auto rhs_ty = other.type;

	static auto ensure_is_ptr_if_deref = []( QualifiedValue &val ) {
		if ( deref_into_ptr_unwrap( val.type, val.val ) )
		{
			Json::Value ast;
			auto builder = DeclarationSpecifiers()
							 .add_type( val.type.into_type(), ast )
							 .into_type_builder( ast );
			val.type = TypeView( std::make_shared<QualifiedType>(
			  builder
				.add_level( std::make_shared<mty::Pointer>( val.type->type ) )
				.build() ) );
		}
		return val;
	};

	ensure_is_ptr_if_deref( self );
	ensure_is_ptr_if_deref( other );

	auto lhs = self.type->as<mty::Pointer>();
	auto rhs = other.type->as<mty::Pointer>();

	if ( lhs || rhs )
	{
		if ( lhs && rhs )
		{
			if ( !self.type.is_same_discard_qualifiers( other.type ) )
			{
				infoList->add_msg(
				  MSG_TYPE_ERROR,
				  fmt( "comparison of distinct pointer types (`", self.type,
					   "` and `", other.type,
					   "`)" ),
				  node );

				self.type = other.type;
				self.val = Builder.CreatePointerCast( self.val, other.type->type );
			}
		}
		else
		{
			if ( lhs )
			{
				if ( other.type->is<mty::Integer>() )
				{
					other.cast( self.type, node[ "children" ][ 2 ], false );
					infoList->add_msg(
					  MSG_TYPE_WARNING,
					  fmt( "comparison between pointer and integer (`", self.type,
						   "` and `", other.type, "`)" ),
					  node );
				}
				else
				{
					infoList->add_msg(
					  MSG_TYPE_ERROR,
					  fmt( "invalid operands to binary expression (`", lhs_ty,
						   "` and `", rhs_ty, "`)" ),
					  node );
					HALT();
				}
			}
			else
			{
				if ( self.type->is<mty::Integer>() )
				{
					self.cast( other.type, node[ "children" ][ 0 ], false );
					infoList->add_msg(
					  MSG_TYPE_WARNING,
					  fmt( "comparison between pointer and integer (`", self.type,
						   "` and `", other.type, "`)" ),
					  node );
				}
				else
				{
					infoList->add_msg(
					  MSG_TYPE_ERROR,
					  fmt( "invalid operands to binary expression (`", lhs_ty,
						   "` and `", rhs_ty, "`)" ),
					  node );
					HALT();
				}
			}
		}
		return true;
	}

	return false;
}

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
		HALT();
	}
}

QualifiedValue &QualifiedValue::cast( const TypeView &dst, Json::Value &node, bool warn )
{
	dbg( type, " -> ", dst );

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
	else if ( dst->is<mty::Structural>() )
	{
		if ( !dst.is_same_discard_qualifiers( this->type ) )
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "casting to `", dst, "` from incompatible type `", this->type, "`" ),
			  node );
			HALT();
		}
	}
	else if ( dst->is<mty::Pointer>() )
	{
		if ( this->type->is<mty::Derefable>() )
		{
			auto dest = dst;
			auto type = this->type;

			deref_into_ptr_unwrap( type, this->val );
			dest.next();

			if ( !dest->is<mty::Void>() && !type->is<mty::Void>() )
			{
				if ( !dest.is_same_discard_qualifiers( type ) )
				{
					if ( warn )
					{
						infoList->add_msg(
						  MSG_TYPE_WARNING,
						  fmt( "incompatible pointer types casting to `", dst, "` from `", this->type, "`" ),
						  node );
					}
					this->type = dst;
					this->val = Builder.CreatePointerCast( this->val, dst->type );
				}
				else if ( !dest.is_qualifiers_compatible( type ) )
				{
					if ( warn )
					{
						infoList->add_msg(
						  MSG_TYPE_WARNING,
						  fmt( "casting to `", dst, "` from `", this->type, "` discards qualifiers" ),
						  node );
					}
					this->type = dst;
				}
			}
			else
			{
				if ( !( int( dest->is_const ) >= int( type->is_const ) &&
						int( dest->is_volatile ) >= int( type->is_volatile ) ) )
				{
					if ( warn )
					{
						infoList->add_msg(
						  MSG_TYPE_WARNING,
						  fmt( "casting to `", dst, "` from `", this->type, "` discards qualifiers" ),
						  node );
					}
					this->type = dst;
				}
				if ( !dest->is<mty::Void>() )
				{
					if ( !dest.is_same_discard_qualifiers( TypeView::getCharTy( true ) ) )
					{
						this->type = dst;
						this->val = Builder.CreatePointerCast( this->val, dst->type );
					}
				}
				else if ( !type->is<mty::Void>() )
				{
					if ( !type.is_same_discard_qualifiers( TypeView::getCharTy( true ) ) )
					{
						this->type = dst;
						this->val = Builder.CreatePointerCast( this->val, dst->type );
					}
				}
			}
		}
		else if ( this->type->is<mty::Integer>() )
		{
			if ( warn )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "incompatible integer to pointer conversion casting to `", dst, "` from `", this->type, "`" ),
				  node );
			}
			this->val = Builder.CreateIntToPtr( this->val, dst->type );
		}
		else
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "casting to `", dst, "` from incompatible type `", this->type, "`" ),
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
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "invalid cast destination type `", dst, "`" ),
		  node );
		HALT();
	}

	return *this;
}
