#pragma once

#include "common.h"
#include "qualifiedType.h"

#define SC_TYPEDEF 0x10
#define SC_EXTERN 0x20
#define SC_STATIC 0x40
#define SC_AUTO 0x80
#define SC_REGISTER 0x100

#define STORAGE_SPECIFIER 0xff0

#define TQ_CONST 0x1
#define TQ_VOLATILE 0x2

#define TYPE_QUALIFIER 0xf

#define TS_VOID 0x1000
#define TS_CHAR 0x2000
#define TS_INT 0x4000
#define TS_FLOAT 0x8000
#define TS_DOUBLE 0x10000

#define TM_SHORT 0x100000
#define TM_LONG 0x200000
#define TM_SIGNED 0x400000
#define TM_UNSIGNED 0x800000

#define TYPE_MODIFIER 0xf00000
#define TYPE_SPECIFIER 0xff000

class DeclarationSpecifiers
{
private:
	Option<QualifiedType> type;
	unsigned attrs = 0;

private:
	int get_attr_from( const std::string &name ) const
	{
		static std::map<std::string, unsigned> attr_map = {
			{ "typedef", SC_TYPEDEF },
			{ "extern", SC_EXTERN },
			{ "static", SC_STATIC },
			{ "auto", SC_AUTO },
			{ "register", SC_REGISTER },

			{ "const", TQ_CONST },
			{ "volatile", TQ_VOLATILE },

			{ "void", TS_VOID },
			{ "char", TS_CHAR },
			{ "int", TS_INT },
			{ "float", TS_FLOAT },
			{ "double", TS_DOUBLE },

			{ "short", TM_SHORT },
			{ "long", TM_LONG },
			{ "signed", TM_LONG },
			{ "unsigned", TM_UNSIGNED },
		};

		if ( attr_map.find( name ) != attr_map.end() )
		{
			return attr_map[ name ];
		}
		else
		{
			INTERNAL_ERROR( "unknown attribute: ", name );
		}
	}

public:
	DeclarationSpecifiers() = default;

	void add_type( const QualifiedType &type, Json::Value const &ast )
	{
		if ( ( this->type.is_none() ) && !( this->attrs & TYPE_SPECIFIER ) )
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
		if ( ( attr & TYPE_SPECIFIER ) && ( ( this->attrs & TYPE_SPECIFIER ) || ( this->type.is_some() ) ) )
		{
			infoList->add_msg( MSG_TYPE_ERROR, "multiple type specifiers", ast );
			HALT();
		}
		if ( ( this->attrs & attr ) != 0 )
		{
			infoList->add_msg( MSG_TYPE_WARNING, fmt( "duplicate `", name, "` type specifier" ), ast );
		}
		this->attrs |= attr;
		if ( ( this->attrs & TM_LONG ) && ( this->attrs & TM_SHORT ) )
		{
			infoList->add_msg( MSG_TYPE_ERROR, "`long short` is invalid", ast );
			HALT();
		}
		if ( ( this->attrs & TM_SIGNED ) && ( this->attrs & TM_UNSIGNED ) )
		{
			infoList->add_msg( MSG_TYPE_ERROR, "`signed unsigned` is invalid", ast );
			HALT();
		}
	}

	bool has_attribute( const std::string &name ) const
	{
		auto attr = get_attr_from( name );
		return ( this->attrs & attr ) != 0;
	}
	bool has_attribute( int attr ) const
	{
		return ( this->attrs & attr ) != 0;
	}

	// Option<QualifiedType> const &get_type() const
	// {
	// 	return this->type;
	// }

	QualifiedTypeBuilder into_type_builder( Json::Value const &ast ) const
	{
		Option<QualifiedType> type = this->type;

		if ( type.is_none() && ( attrs & ( TYPE_SPECIFIER | TYPE_MODIFIER ) ) != 0 )
		{
			auto base_type = TS_INT;
			int num_bits = 32;
			bool is_signed = true;

			if ( ( attrs & TYPE_SPECIFIER ) != 0 )
			{
				auto ty_spec = attrs & TYPE_SPECIFIER;
				switch ( ty_spec )
				{
				case TS_DOUBLE:
				{
					if ( ( attrs & ( TYPE_MODIFIER & ~TM_LONG ) ) != 0 )
					{
						infoList->add_msg( MSG_TYPE_ERROR, "invalid type specifier", ast );
					}
					base_type = TS_FLOAT;
					num_bits = 64;
					break;
				}
				case TS_VOID:
				case TS_CHAR:
				case TS_FLOAT:
				{
					if ( ( attrs & TYPE_MODIFIER ) != 0 )
					{
						infoList->add_msg( MSG_TYPE_ERROR, "invalid type specifier", ast );
					}
					switch ( ty_spec )
					{
					case TS_VOID: base_type = TS_VOID; break;
					case TS_CHAR: num_bits = 8; break;
					case TS_FLOAT: base_type = TS_FLOAT; break;
					}
					break;
				}
				}
			}
			if ( ( attrs & TYPE_MODIFIER ) != 0 )
			{  // only modifier
				if ( ( attrs & TM_SHORT ) != 0 ) num_bits = num_bits >> 1;
				if ( ( attrs & TM_LONG ) != 0 ) num_bits = num_bits << 1;

				if ( ( attrs & TM_SIGNED ) != 0 ) is_signed = true;
				if ( ( attrs & TM_UNSIGNED ) != 0 ) is_signed = false;
			}
			switch ( base_type )
			{
			case TS_VOID: type = std::make_shared<Qualified>( Type::getVoidTy( TheContext ) ); break;
			case TS_INT:
			{
				auto llvm_ty = Type::getIntNTy( TheContext, num_bits );
				TODO( "signed unsigned not implemented" );
				type = std::make_shared<Qualified>( llvm_ty );
				break;
			}
			case TS_FLOAT:
			{
				switch ( num_bits )
				{
				case 16: type = std::make_shared<Qualified>( Type::getHalfTy( TheContext ) ); break;
				case 32: type = std::make_shared<Qualified>( Type::getFloatTy( TheContext ) ); break;
				case 64: type = std::make_shared<Qualified>( Type::getDoubleTy( TheContext ) ); break;
				case 128: type = std::make_shared<Qualified>( Type::getFP128Ty( TheContext ) ); break;
				default:
				{
					UNIMPLEMENTED( "unknown floating point type: f", num_bits );
				}
				}
				break;
			}
			}
		}
		else
		{
			if ( ( attrs & TYPE_MODIFIER ) != 0 )
			{
				UNIMPLEMENTED( "qualify custom type" );
			}
		}

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
