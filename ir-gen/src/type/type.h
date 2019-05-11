#pragma once

#include "predef.h"

class QualifiedTypeBuilder;
class TypeView;

class QualifiedType
{
	friend class QualifiedTypeBuilder;
	friend class TypeView;

private:
	std::vector<std::shared_ptr<mty::Qualified>> list;

	QualifiedType() = default;

public:
	QualifiedType( const std::shared_ptr<mty::Qualified> &type )
	{
		list.emplace_back( type );
	}

	QualifiedType( const QualifiedType & ) = default;
	QualifiedType( QualifiedType && ) = default;
	QualifiedType &operator=( const QualifiedType & ) = default;
	QualifiedType &operator=( QualifiedType && ) = default;

	operator Type *() const
	{
		return list.back()->type;
	}

	Type *get() const
	{
		return list.back()->type;
	}

	Type *operator->() const
	{
		return this->list.back()->type;
	}

	template <typename T>
	T *as() const
	{
		return static_cast<T *>( (Type *)*this );
	}

	template <typename T>
	bool is() const
	{
		return dynamic_cast<const T *>( this->list.back().get() );
	}

	TypeView view() const;

	friend std::ostream &operator<<( std::ostream &os, const QualifiedType &type );
};

class TypeView
{
private:
	std::shared_ptr<QualifiedType> type;
	std::size_t index;

	TypeView( const std::shared_ptr<QualifiedType> &type, std::size_t index ) :
	  type( type ),
	  index( index )
	{
	}

public:
	TypeView( const std::shared_ptr<QualifiedType> &type ) :
	  type( type ),
	  index( type->list.size() - 1 )
	{
	}

	const mty::Qualified *operator->() const
	{
		return this->type->list[ this->index ].get();
	}

	const mty::Qualified *get() const
	{
		return this->type->list[ this->index ].get();
	}

	TypeView &next()
	{
		index--;
		return *this;
	}

	friend std::ostream &operator<<( std::ostream &os, const TypeView &view );

public:
	static TypeView &getCharTy( bool is_signed );
	static TypeView &getShortTy( bool is_signed );
	static TypeView &getIntTy( bool is_signed );
	static TypeView &getLongTy( bool is_signed );
	static TypeView &getLongLongTy( bool is_signed );
	static TypeView &getFloatTy();
	static TypeView &getDoubleTy();
	static TypeView &getLongDoubleTy();
};

inline std::ostream &operator<<( std::ostream &os, const QualifiedType &type )
{
	for ( int i = type.list.size() - 1; i >= 0; --i )
	{
		type.list[ i ]->print( os );
	}
	return os;
}

inline std::ostream &operator<<( std::ostream &os, const TypeView &view )
{
	view->print( os );
	return os;
}
