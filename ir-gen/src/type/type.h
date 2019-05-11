#pragma once

#include "predef.h"

class QualifiedTypeBuilder;
class TypeView;

class QualifiedType
{
	friend class QualifiedTypeBuilder;
	friend class TypeView;

	friend std::ostream &operator<<( std::ostream &os, const TypeView &view );

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
	static TypeView const &getCharTy( bool is_signed );
	static TypeView const &getShortTy( bool is_signed );
	static TypeView const &getIntTy( bool is_signed );
	static TypeView const &getLongTy( bool is_signed );
	static TypeView const &getLongLongTy( bool is_signed );
	static TypeView const &getFloatTy();
	static TypeView const &getDoubleTy();
	static TypeView const &getLongDoubleTy();

	// static TypeView from( const std::shared_ptr<mty::Qualified> &direct )
	// {
	// 	return TypeView( std::make_shared<QualifiedType>( direct ), 0 );
	// }
};

struct QualifiedDecl
{
	QualifiedType type;
	Option<std::string> name;

public:
	QualifiedDecl( const QualifiedType &type, const std::string &name ) :
	  type( type ),
	  name( name )
	{
	}
	QualifiedDecl( const QualifiedType &type ) :
	  type( type )
	{
	}

	friend std::ostream &operator<<( std::ostream &os, const QualifiedDecl &decl );
};

struct QualifiedType;

namespace mty
{
struct Array;
}

class QualifiedTypeBuilder
{
private:
	std::vector<std::shared_ptr<mty::Qualified>> list;

public:
	QualifiedTypeBuilder() = default;
	QualifiedTypeBuilder( const std::shared_ptr<mty::Qualified> &type ) :
	  list{ type }
	{
	}
	QualifiedTypeBuilder( const QualifiedType &type, bool is_const = false, bool is_volatile = false ) :
	  list{ type.list }
	{
		auto idx = list.size() - 1;
		while ( list[ idx ]->is<mty::Array>() )
		{
			idx -= 1;
		}
		for ( auto i = idx; i != list.size(); ++i )
		{  // if typedef const/volatile array
			auto item = list[ i ]->clone();
			if ( i == idx )
			{
				item->is_const = item->is_const || is_const;
				item->is_volatile = item->is_volatile || is_volatile;
			}
			list[ i ] = std::move( item );
		}
	}

	QualifiedTypeBuilder &add_level( const std::shared_ptr<mty::Qualified> &obj )
	{
		list.emplace_back( obj );
		return *this;
	}
	Type *get_type() const
	{
		return list.back()->type;
	}
	QualifiedType build()
	{
		QualifiedType type;
		type.list = std::move( list );
		return type;
	}
};

inline std::ostream &operator<<( std::ostream &os, const QualifiedDecl &decl )
{
	os << decl.name << " = " << decl.type;
	return os;
}

inline std::ostream &operator<<( std::ostream &os, const QualifiedType &type )
{
	type.list.front()->print( os, type.list, 0 );
	return os;
}

inline std::ostream &operator<<( std::ostream &os, const TypeView &view )
{
	auto tb = QualifiedTypeBuilder();
	for ( int i = 0; i <= view.index; ++i )
	{
		tb.add_level( view.type->list[ i ] );
	}
	return os << tb.build();
}
