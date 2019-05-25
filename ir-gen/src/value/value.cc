#include "value.h"

bool QualifiedValue::deref_into_ptr_unwrap( TypeView &view, Value *&val )
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

bool QualifiedValue::cast_binary_ptr( QualifiedValue &self, QualifiedValue &other, Json::Value &node, bool supress_warning )
{
	auto lhs_ty = self.type;
	auto rhs_ty = other.type;

	self.ensure_is_ptr_if_deref();
	other.ensure_is_ptr_if_deref();

	auto lhs = self.type->as<mty::Pointer>();
	auto rhs = other.type->as<mty::Pointer>();

	if ( lhs || rhs )
	{
		if ( lhs && rhs )
		{
			if ( !self.type.is_same_discard_qualifiers( other.type ) )
			{
				if ( !supress_warning )
				{
					infoList->add_msg(
					  MSG_TYPE_WARNING,
					  fmt( "comparison of distinct pointer types (`", self.type,
						   "` and `", other.type,
						   "`)" ),
					  node );
				}

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
					if ( !supress_warning )
					{
						infoList->add_msg(
						  MSG_TYPE_WARNING,
						  fmt( "comparison between pointer and integer (`", self.type,
							   "` and `", other.type, "`)" ),
						  node );
					}
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
					if ( !supress_warning )
					{
						infoList->add_msg(
						  MSG_TYPE_WARNING,
						  fmt( "comparison between pointer and integer (`", self.type,
							   "` and `", other.type, "`)" ),
						  node );
					}
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

void QualifiedValue::cast_binary_expr( QualifiedValue &self, QualifiedValue &other, Json::Value &node, bool allow_float,
									   BasicBlock *lhsbb, BasicBlock *rhsbb )
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
					if ( lhsbb ) Builder.SetInsertPoint( lhsbb );
					self.val = Builder.CreateIntCast( self.val, rhs->type, false );
					self.type = other.type;
				}
				else if ( !lsign && rsign )
				{
					if ( rhsbb ) Builder.SetInsertPoint( rhsbb );
					other.val = Builder.CreateIntCast( other.val, lhs->type, false );
					other.type = self.type;
				}
			}
			else if ( lbits < rbits )  // l < r; l -> r
			{
				if ( lhsbb ) Builder.SetInsertPoint( lhsbb );
				self.val = Builder.CreateIntCast( self.val, rhs->type, rsign );
				self.type = other.type;
			}
			else
			{
				if ( rhsbb ) Builder.SetInsertPoint( rhsbb );
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
				if ( lhsbb ) Builder.SetInsertPoint( lhsbb );
				self.val = ilhs->is_signed ? Builder.CreateSIToFP( self.val, rhs->type ) : Builder.CreateUIToFP( self.val, rhs->type );
				self.type = other.type;
			}
			else if ( irhs )
			{
				if ( rhsbb ) Builder.SetInsertPoint( rhsbb );
				other.val = irhs->is_signed ? Builder.CreateSIToFP( other.val, lhs->type ) : Builder.CreateUIToFP( other.val, lhs->type );
				other.type = self.type;
			}
			else  // FP FP
			{
				int lbits = flhs->bits;
				int rbits = frhs->bits;

				if ( lbits < rbits )  // l -> r
				{
					if ( lhsbb ) Builder.SetInsertPoint( lhsbb );
					self.val = Builder.CreateFPCast( self.val, rhs->type );
					self.type = other.type;
				}
				else  // r -> l;
				{
					if ( rhsbb ) Builder.SetInsertPoint( rhsbb );
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

void QualifiedValue::cast_ternary_expr( QualifiedValue &self, QualifiedValue &other, Json::Value &node,
										BasicBlock *lhsbb, BasicBlock *rhsbb )
{
	if ( self.type->is<mty::Arithmetic>() && other.type->is<mty::Arithmetic>() )
	{
		cast_binary_expr( self, other, node, true, lhsbb, rhsbb );
	}
	else if ( self.type->is<mty::Derefable>() && other.type->is<mty::Integer>() )
	{
		if ( rhsbb ) Builder.SetInsertPoint( rhsbb );
		other.cast( self.type, node );
	}
	else if ( self.type->is<mty::Integer>() && other.type->is<mty::Derefable>() )
	{
		if ( lhsbb ) Builder.SetInsertPoint( lhsbb );
		self.cast( other.type, node );
	}
	else if ( self.type->is<mty::Derefable>() && other.type->is<mty::Derefable>() )
	{
		if ( lhsbb ) Builder.SetInsertPoint( lhsbb );
		self.cast( other.type, node );
	}
	else
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "incompatible operand types (`", self.type, "` and `", other.type, "`)" ),
		  node );
		HALT();
	}
}

QualifiedValue &QualifiedValue::cast( const TypeView &dst, Json::Value &node, bool warn )
{
	// dbg( "CAST ", sharp( type ), " ===> ", sharp( dst ) );

	if ( this->is_lvalue )
	{
		INTERNAL_ERROR();
	}

	this->ensure_is_ptr_if_deref();

	if ( dst->is<mty::Arithmetic>() )
	{
		if ( auto this_arith_ty = this->type->as<mty::Arithmetic>() )
		{
			if ( auto dty = dst->as<mty::Integer>() )
			{
				if ( auto ty = this->type->as<mty::Integer>() )  // int -> int
				{
					if ( !dst.is_same_discard_qualifiers( this->type ) )
					{
						auto int_ty = this->type->type;
						this->type = dst;
						if ( dty->bits == 1 )
						{
							this->val = Builder.CreateICmpNE(
							  this->val,
							  Constant::getIntegerValue( int_ty, APInt( ty->bits, 0, ty->is_signed ) ) );
						}
						else
						{
							this->val = Builder.CreateIntCast( this->val, dst->type, ty->is_signed );
						}
					}
				}
				else  // float -> int
				{
					auto fp_ty = this->type->type;
					this->type = dst;
					if ( dty->bits == 1 )
					{
						this->val = Builder.CreateFCmpONE(
						  this->val,
						  ConstantFP::get( fp_ty, APFloat( 0.0 ) ) );
					}
					else
					{
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
		else if ( this->type->is<mty::Pointer>() && dst->is<mty::Integer>() )
		{
			auto is_bool = dst->as<mty::Integer>()->bits == 1;
			if ( !is_bool && warn )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "incompatible pointer to integer conversion to `", dst, "` from `", this->type, "`" ),
				  node );
			}
			this->type = dst;
			if ( is_bool )
			{
				auto &long_ty = TypeView::getLongTy( false )->type;
				this->val = Builder.CreateICmpNE(
				  Builder.CreatePtrToInt( this->val, long_ty ),
				  Constant::getIntegerValue( long_ty, APInt( 64, 0 ) ) );
			}
			else
			{
				this->val = Builder.CreatePtrToInt( this->val, dst->type );
			}
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
			auto val = dyn_cast_or_null<ConstantInt>( this->val );
			if ( !( val && val->getZExtValue() == 0 ) && warn )
			{
				infoList->add_msg(
				  MSG_TYPE_WARNING,
				  fmt( "incompatible integer to pointer conversion casting to `", dst, "` from `", this->type, "`" ),
				  node );
			}
			this->type = dst;
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
