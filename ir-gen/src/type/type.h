#pragma once

#include "predef.h"

class QualifiedTypeBuilder;
class TypeView;

namespace mty
{
struct Function;
struct Struct;
}  // namespace mty

class QualifiedType
{
	friend class QualifiedTypeBuilder;
	friend class TypeView;

	friend std::ostream &operator<<( std::ostream &os, const TypeView &view );

private:
	std::vector<std::shared_ptr<mty::Qualified>> list;

	QualifiedType() = default;

	bool is_same_without_cv_from_index(
	  const QualifiedType &other, std::size_t idx ) const
	{
		if ( !this->list[ idx ]->is_same_without_cv( *other.list[ idx ] ) ) return false;
		if ( idx-- == 0 ) return true;
		do
		{
			if ( !this->list[ idx ]->is_same( *other.list[ idx ] ) ) return false;
		} while ( idx-- != 0 );
		return true;
	}

	bool is_same_discard_qualifiers_from_index(
	  const QualifiedType &other, std::size_t idx ) const
	{
		do
		{
			if ( !this->list[ idx ]->is_same_without_cv( *other.list[ idx ] ) ) return false;
		} while ( idx-- != 0 );
		return true;
	}

	bool is_qualifiers_compatible_from_index(
	  const QualifiedType &other, std::size_t idx ) const
	{
		auto need_strict = this->list[ idx ]->is<mty::Function>();
		if ( !( int( this->list[ idx ]->is_const ) >= int( other.list[ idx ]->is_const ) &&
				int( this->list[ idx ]->is_volatile ) >= int( other.list[ idx ]->is_volatile ) ) ) return false;
		if ( idx-- == 0 ) return true;

		do
		{
			if ( need_strict )
			{
				if ( !( int( this->list[ idx ]->is_const ) == int( other.list[ idx ]->is_const ) &&
						int( this->list[ idx ]->is_volatile ) == int( other.list[ idx ]->is_volatile ) ) ) return false;
			}
			else
			{
				if ( !( int( this->list[ idx ]->is_const ) >= int( other.list[ idx ]->is_const ) &&
						int( this->list[ idx ]->is_volatile ) >= int( other.list[ idx ]->is_volatile ) ) ) return false;
			}

		} while ( idx-- != 0 );

		return true;
	}

public:
	QualifiedType( const std::shared_ptr<mty::Qualified> &type )
	{
		list.emplace_back( type );
	}

	QualifiedType( const QualifiedType & ) = default;
	QualifiedType( QualifiedType && ) = default;
	QualifiedType &operator=( const QualifiedType & ) = default;
	QualifiedType &operator=( QualifiedType && ) = default;

	bool is_same_without_cv( const QualifiedType &other ) const
	{
		if ( this->list.size() != other.list.size() ) return false;
		return is_same_without_cv_from_index(
		  other, this->list.size() - 1 );
	}

	bool is_same_discard_qualifiers( const QualifiedType &other ) const
	{
		if ( this->list.size() != other.list.size() ) return false;
		return is_same_discard_qualifiers_from_index(
		  other, this->list.size() - 1 );
	}

	bool is_qualifiers_compatible( const QualifiedType &other ) const
	{
		if ( this->list.size() != other.list.size() ) return false;
		return is_qualifiers_compatible_from_index(
		  other, this->list.size() - 1 );
	}

	// operator Type *() const
	// {
	// 	return list.back()->type;
	// }

	const mty::Qualified *get() const
	{
		return this->list.back().get();
	}

	const mty::Qualified *operator->() const
	{
		return this->list.back().get();
	}

	template <typename T>
	T *as()
	{
		static_assert( std::is_base_of<mty::Qualified, T>::value, "invalid cast" );
		return dynamic_cast<T *>( this->list.back().get() );
	}

	template <typename T>
	const T *as() const
	{
		static_assert( std::is_base_of<mty::Qualified, T>::value, "invalid cast" );
		return dynamic_cast<const T *>( this->list.back().get() );
	}

	template <typename T>
	bool is() const
	{
		static_assert( std::is_base_of<mty::Qualified, T>::value, "invalid cast" );
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

	// bool is_same_without_cv( const TypeView &other ) const
	// {
	// 	if ( this->index != other.index ) return false;
	// 	return this->type->is_same_without_cv_from_index(
	// 	  *other.type, index );
	// }

	bool is_same_discard_qualifiers( const TypeView &other ) const
	{
		if ( this->index != other.index ) return false;
		return this->type->is_same_discard_qualifiers_from_index(
		  *other.type, index );
	}

	bool is_qualifiers_compatible( const TypeView &other ) const
	{
		if ( this->index != other.index ) return false;
		return this->type->is_qualifiers_compatible_from_index(
		  *other.type, index );
	}

	bool has_next() const
	{
		return this->index != 0;
	}

	TypeView &next()
	{
		index--;
		return *this;
	}

	QualifiedType into_type() const
	{
		QualifiedType ty;
		for ( int i = 0; i <= index; ++i )
		{
			ty.list.emplace_back( type->list[ i ] );
		}
		return ty;
	}

	friend std::ostream &operator<<( std::ostream &os, const TypeView &view );

public:
	static TypeView const &getVoidPtrTy();
	static TypeView const &getBoolTy();
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
	const mty::Qualified *get_type() const
	{
		return list.back().get();
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
