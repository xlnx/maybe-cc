#pragma once

#include "predef.h"
#include "type.h"

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
		while ( list[ idx ]->is_array_type() )
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

	void add_level( const std::shared_ptr<mty::Qualified> &obj )
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

inline std::ostream &operator<<( std::ostream &os, const QualifiedDecl &decl )
{
	os << decl.name << " = " << decl.type;
	return os;
}
