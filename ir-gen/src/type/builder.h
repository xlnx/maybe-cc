#pragma once

#include "qualified.h"

class QualifiedTypeBuilder;

class QualifiedType
{
	friend class QualifiedTypeBuilder;

private:
	std::vector<std::shared_ptr<Qualified>> list;

	QualifiedType() = default;

public:
	QualifiedType( const std::shared_ptr<Qualified> &type )
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

	Type *operator->() const
	{
		return this->list.back()->type;
	}

	template <typename T>
	T *as() const
	{
		return static_cast<T *>( (Type *)*this );
	}

	friend std::ostream &operator<<( std::ostream &os, const QualifiedType &type );
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

class QualifiedTypeBuilder
{
private:
	std::vector<std::shared_ptr<Qualified>> list;

public:
	QualifiedTypeBuilder() = default;
	QualifiedTypeBuilder( const std::shared_ptr<Qualified> &type ) :
	  list{ type }
	{
	}
	QualifiedTypeBuilder( const QualifiedType &type, bool is_const = false, bool is_volatile = false ) :
	  list{ type.list }
	{
		if ( is_const || is_volatile )
		{
			auto back = list.back()->clone();
			back->is_const = is_const;
			back->is_volatile = is_volatile;
			list.back() = back;
		}
	}

	void add_level( const std::shared_ptr<Qualified> &obj )
	{
		list.emplace_back( obj );
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

inline std::ostream &operator<<( std::ostream &os, const QualifiedType &type )
{
	for ( int i = type.list.size() - 1; i >= 0; --i )
	{
		type.list[ i ]->print( os );
	}
	return os;
}

inline std::ostream &operator<<( std::ostream &os, const QualifiedDecl &decl )
{
	os << decl.name << " = " << decl.type;
	return os;
}
