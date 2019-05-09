#pragma once

#include "common.h"
#include "qualifiedType.h"

#define SC_TYPEDEF 0x100
#define SC_EXTERN 0x200
#define SC_STATIC 0x400
#define SC_AUTO 0x800
#define SC_REGISTER 0x1000

#define STORAGE_SPECIFIER 0xff00

#define TQ_CONST 0x1
#define TQ_VOLATILE 0x2

#define TYPE_QUALIFIER 0xff

class DeclarationSpecifiers
{
private:
	Option<QualifiedType> type;
	unsigned attrs = 0;

private:
	int get_attr_from( const std::string &name ) const
	{
		int attr = -1;
		if ( name == "typedef" ) attr = SC_TYPEDEF;
		if ( name == "extern" ) attr = SC_EXTERN;
		if ( name == "static" ) attr = SC_STATIC;
		if ( name == "auto" ) attr = SC_AUTO;
		if ( name == "register" ) attr = SC_REGISTER;

		if ( name == "const" ) attr = TQ_CONST;
		if ( name == "volatile" ) attr = TQ_VOLATILE;

		if ( attr == -1 )
		{
			infoList->add_msg( MSG_TYPE_ERROR, "internal error: unknown attribute" );
			HALT();
		}

		return attr;
	}

public:
	DeclarationSpecifiers() = default;

	void add_type( const QualifiedType &type, Json::Value const &ast )
	{
		if ( this->type.is_none() )
		{
			this->type = type;
		}
		else
		{
			infoList->add_msg( MSG_TYPE_ERROR, "multiple type specifiers", ast );
			HALT();
		}
	}

	void add_attribute( const std::string &name, Json::Value const &ast )
	{
		auto attr = get_attr_from( name );
		if ( ( attr & STORAGE_SPECIFIER ) && ( this->attrs & STORAGE_SPECIFIER ) )
		{
			infoList->add_msg( MSG_TYPE_ERROR, "multiple storage specifiers", ast );
			HALT();
		}
		this->attrs |= attr;
	}

	bool has_attribute( const std::string &name ) const
	{
		auto attr = get_attr_from( name );
		return ( this->attrs & attr ) != 0;
	}

	Option<QualifiedType> const &get_type() const
	{
		return this->type;
	}

	QualifiedTypeBuilder into_type_builder( Json::Value const &ast ) const
	{
		if ( attrs & SC_TYPEDEF )
		{
			infoList->add_msg( MSG_TYPE_ERROR, "expected type specifiers", ast );
			HALT();
		}
		auto type = this->type;
		if ( type.is_none() )
		{
			type = std::make_shared<Qualified>( Type::getInt32Ty( TheContext ) );
			infoList->add_msg( MSG_TYPE_WARNING, "type defaults to `int`", ast );
		}
		auto builder = QualifiedTypeBuilder();
		builder.add_level( std::make_shared<Qualified>( type.unwrap() ) );
		return builder;
	}

	friend std::ostream &operator<<( std::ostream &os, const DeclarationSpecifiers &declspec )
	{
		os << "{ ";

		if ( declspec.type.is_some() ) os << "<type> ";

		if ( declspec.attrs & SC_TYPEDEF ) os << "typedef ";
		if ( declspec.attrs & SC_EXTERN ) os << "extern ";
		if ( declspec.attrs & SC_STATIC ) os << "static ";
		if ( declspec.attrs & SC_AUTO ) os << "auto ";
		if ( declspec.attrs & SC_REGISTER ) os << "register ";

		if ( declspec.attrs & TQ_CONST ) os << "const ";
		if ( declspec.attrs & TQ_VOLATILE ) os << "volatile ";

		os << "}";

		return os;
	}
};
