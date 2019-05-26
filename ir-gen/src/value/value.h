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
	QualifiedValue &store( QualifiedValue &val, Json::Value &lhs, Json::Value &rhs, bool ignore_const = false )
	{
		if ( !is_lvalue )
		{
			infoList->add_msg( MSG_TYPE_ERROR,
							   fmt( "expression is not assignable" ),
							   lhs );
			HALT();
		}
		if ( val.is_lvalue )
		{
			INTERNAL_ERROR();
		}

		if ( type->is<mty::Address>() || type->is<mty::Void>() )
		{
			infoList->add_msg( MSG_TYPE_ERROR,
							   fmt( "object of type `", type, "` is not assignable" ),
							   lhs );
			HALT();
		}
		if ( !ignore_const && type->is_const )
		{
			infoList->add_msg( MSG_TYPE_ERROR,
							   fmt( "cannot assign to const-qualified type `", type, "`" ),
							   lhs );
		}

		this->deref( lhs );

		Builder.CreateStore( val.value( rhs ).cast( type, rhs ).get(), this->val );

		return *this;
	}
	QualifiedValue &get_member( std::string const &member, Json::Value &ast )
	{
		if ( !is_lvalue )
		{
			UNIMPLEMENTED( "function returning a struct is not implemented yet" );
		}

		auto &children = ast[ "children" ];
		if ( !type->is<mty::Structural>() )
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "member reference base type `", type, "` is not a structure or union" ),
			  children[ 0 ] );
			HALT();
		}
		if ( auto struct_obj = type->as<mty::Struct>() )
		{
			auto &mem = struct_obj->get_member( member, children[ 2 ] );
			static const auto zero = ConstantInt::get( TheContext, APInt( 64, 0, true ) );
			static Value *idx[ 2 ] = { zero, nullptr };
			idx[ 1 ] = mem.second;
			auto builder = DeclarationSpecifiers()
							 .add_type( mem.first, ast );
			if ( type->is_const ) builder.add_attribute( "const", ast );
			if ( type->is_volatile ) builder.add_attribute( "volatile", ast );

			this->type = TypeView( std::make_shared<QualifiedType>(
			  builder
				.into_type_builder( ast )
				.build() ) );
			this->val = Builder.CreateGEP( this->val, idx );
		}
		else if ( auto union_obj = type->as<mty::Union>() )
		{
			auto &mem = union_obj->get_member( member, children[ 2 ] );
			auto builder = DeclarationSpecifiers()
							 .add_type( mem, ast );
			if ( this->get_type()->is_const ) builder.add_attribute( "const", ast );
			if ( this->get_type()->is_volatile ) builder.add_attribute( "volatile", ast );

			this->type = TypeView( std::make_shared<QualifiedType>(
			  builder
				.into_type_builder( ast )
				.build() ) );

			this->val = Builder.CreateBitCast( this->get(), PointerType::getUnqual( mem->type ) );
		}
		else
		{
			INTERNAL_ERROR();
		}

		return *this;
	}
	QualifiedValue &value( Json::Value &ast )  // cast to rvalue
	{
		if ( this->type->is<mty::Void>() )
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "unable to evalutate expression of type `void`" ),
			  ast );
			HALT();
		}
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
		if ( is_lvalue )
		{
			INTERNAL_ERROR();
		}

		if ( auto derefable = type->as<mty::Derefable>() )
		{
			val = derefable->offset( type, val, off, ast );
			if ( derefable->is<mty::Array>() )
			{
				type.next();
				Json::Value ast;
				type = TypeView(
				  std::make_shared<QualifiedType>(
					DeclarationSpecifiers()
					  .add_type( type.into_type(), ast )
					  .into_type_builder( ast )
					  .add_level( std::make_shared<mty::Pointer>( type->type ) )
					  .build() ) );
			}
		}
		else
		{
			infoList->add_msg( MSG_TYPE_ERROR, "lvalue required for dereference", ast );
			HALT();
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
			if ( fn->is_va_args && args.size() < fn->args.size() ||
				 !fn->is_va_args && args.size() != fn->args.size() )
			{
				infoList->add_msg(
				  MSG_TYPE_ERROR,
				  fmt( "too ", args.size() > fn->args.size() ? "many" : "few",
					   " arguments to function call, exprected ",
					   fn->is_va_args ? "at least " : "", fn->args.size(),
					   ", have ", args.size() ),
				  ast );
				HALT();
			}
			std::vector<Value *> args_val;
			for ( auto i = 0; i != args.size(); ++i )
			{
				if ( args[ i ].is_lvalue ) INTERNAL_ERROR();

				args_val.emplace_back(
				  i < fn->args.size() ? args[ i ].cast(
												   TypeView( std::make_shared<QualifiedType>(
													 fn->args[ i ].type ) ),
												   children[ i + 2 ] )
										  .get()
									  : args[ i ].get() );
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

	QualifiedValue &ensure_is_ptr_if_deref()
	{
		if ( deref_into_ptr_unwrap( this->type, this->val ) )
		{
			Json::Value ast;
			auto builder = DeclarationSpecifiers()
							 .add_type( this->type.into_type(), ast )
							 .into_type_builder( ast );
			this->type = TypeView( std::make_shared<QualifiedType>(
			  builder
				.add_level( std::make_shared<mty::Pointer>( this->type->type ) )
				.build() ) );
		}
		return *this;
	}

private:
	static bool deref_into_ptr_unwrap( TypeView &view, Value *&val );

public:
	static bool cast_binary_ptr( QualifiedValue &self, QualifiedValue &other, Json::Value &node, bool supress_warning = false );
	static void cast_binary_expr( QualifiedValue &self, QualifiedValue &other, Json::Value &node, bool allow_float = true,
								  BasicBlock *lhs = nullptr, BasicBlock *rhs = nullptr );
	static void cast_ternary_expr( QualifiedValue &self, QualifiedValue &other, Json::Value &node,
								   BasicBlock *lhs = nullptr, BasicBlock *rhs = nullptr );
	QualifiedValue &cast( const TypeView &dst, Json::Value &node, bool warn = true );
};
