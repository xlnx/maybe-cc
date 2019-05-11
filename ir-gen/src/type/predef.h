#pragma once

#include "../common.h"

extern LLVMContext TheContext;
extern MsgList *infoList;
extern IRBuilder<> Builder;

struct TypeView;

namespace mty
{
struct Qualified
{
	Type *type;
	bool is_const;
	bool is_volatile;

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
		return dynamic_cast<const T *>( this ) != nullptr;
	}

	template <typename T>
	const T *as() const
	{
		return dynamic_cast<const T *>( this );
	}

	virtual std::shared_ptr<Qualified> clone() const = 0;
	virtual void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const = 0;
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