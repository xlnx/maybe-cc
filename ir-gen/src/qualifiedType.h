#pragma once

#include "common.h"

struct Qualified
{
	Type *type;

	Qualified( Type *type ) :
	  type( type )
	{
	}

	virtual ~Qualified() = default;
};

class QualifiedTypeBuilder;

class QualifiedType
{
	friend class QualifiedTypeBuilder;

private:
	std::vector<std::shared_ptr<Qualified>> list;

public:
	Type *get_type() const
	{
		return list.back()->type;
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

struct QualifiedPointer : Qualified
{
	bool is_const;
	bool is_volatile;

	QualifiedPointer( Type *base_type, bool is_const, bool is_volatile ) :
	  Qualified( PointerType::getUnqual( base_type ) ),
	  is_const( is_const ),
	  is_volatile( is_volatile )
	{
	}
};

struct QualifiedType;

struct QualifiedFunction : Qualified
{
	std::vector<QualifiedDecl> args;

	QualifiedFunction( Type *result_type, const std::vector<QualifiedDecl> &args ) :
	  Qualified( FunctionType::get( result_type, map_type( args ), false ) ),
	  args( args )
	{
		WARN( "function arg name ignored" );
	}

private:
	static std::vector<Type *> map_type( const std::vector<QualifiedDecl> &args )
	{
		std::vector<Type *> new_args;
		for ( auto arg : args ) new_args.push_back( arg.type.get_type() );
		return new_args;
	}
};

class QualifiedTypeBuilder
{
private:
	std::vector<std::shared_ptr<Qualified>> list;

public:
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
		auto type = QualifiedType{};
		type.list = std::move( list );
		return type;
	}
};

std::ostream &operator<<( std::ostream &os, const QualifiedType &type )
{
	for ( int i = type.list.size() - 1; i >= 0; --i )
	{
		auto ptr = type.list[ i ].get();
		if ( auto fn = dynamic_cast<QualifiedFunction *>( ptr ) )
		{
			os << "Fn (";
			for ( auto arg : fn->args )
			{
				os << arg << ", ";
			}
			os << "): ";
		}
		else if ( dynamic_cast<QualifiedPointer *>( ptr ) )
		{
			os << "Ptr: ";
		}
		else if ( dynamic_cast<Qualified *>( ptr ) )
		{
			os << "Ty";
		}
		else
		{
			INTERNAL_ERROR();
		}
	}
	return os;
}

std::ostream &operator<<( std::ostream &os, const QualifiedDecl &decl )
{
	os << decl.name << " = " << decl.type;
	return os;
}
