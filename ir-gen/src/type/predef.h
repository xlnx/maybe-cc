#pragma once

#include "../common.h"

extern LLVMContext TheContext;
extern MsgList *infoList;
extern IRBuilder<> Builder;
extern std::string decl_indent;

struct TypeView;

namespace mty
{
enum TypeName
{
	VoidType,
	IntegerType,
	FloatingPointType,
	PointerType,
	FunctionType,
	ArrayType,
	StructType
};

struct Qualified
{
	Type *type;
	bool is_const;
	bool is_volatile;
	TypeName type_name;

	Qualified( Type *type, bool is_const = false, bool is_volatile = false ) :
	  type( type ),
	  is_const( is_const ),
	  is_volatile( is_volatile )
	{
	}

	virtual ~Qualified() = default;

	template <typename T>
	bool is() const
	{
		static_assert( std::is_base_of<mty::Qualified, T>::value, "invalid cast" );
		return dynamic_cast<const T *>( this ) != nullptr;
	}

	template <typename T>
	const T *as() const
	{
		static_assert( std::is_base_of<mty::Qualified, T>::value, "invalid cast" );
		return dynamic_cast<const T *>( this );
	}

	bool is_same_without_cv( const Qualified &other ) const
	{
		if ( this->type_name != other.type_name ) return false;
		return this->impl_is_same_without_cv( other );
	}

	bool is_same( const Qualified &other ) const
	{
		return this->is_same_without_cv( other ) &&
			   this->is_const == other.is_const &&
			   this->is_volatile == other.is_volatile;
	}

	bool is_complete() const
	{
		if ( auto ty = dyn_cast_or_null<llvm::StructType>( type ) )
		{
			return !ty->isOpaque();
		}
		return true;
	}

	bool is_allocable() const
	{
		return this->is_complete() && !type->isVoidTy();
	}

	virtual std::shared_ptr<Qualified> clone() const = 0;
	virtual void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const = 0;

protected:
	virtual bool impl_is_same_without_cv( const Qualified &other ) const = 0;
};

class Derefable : public Qualified
{
public:
	using Qualified::Qualified;

	virtual Value *deref( TypeView &view, Value *val, Json::Value &ast ) const = 0;
	virtual Value *offset( TypeView &view, Value *val, Value *off, Json::Value &ast ) const = 0;
};

class Address : public Derefable
{
public:
	using Derefable::Derefable;
};

class Arithmetic : public Qualified
{
public:
	using Qualified::Qualified;
};

}  // namespace mty
