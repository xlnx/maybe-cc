#pragma once

#include "predef.h"
#include "integerType.h"
#include "type.h"

namespace mty
{
struct Enum : Integer
{
	int id = -1;
	Option<std::string> name;

	static constexpr auto self_type = TypeName::EnumType;

	Enum( const std::string &name, bool is_const = false, bool is_volatile = false ) :
	  Integer( 32, true, is_const, is_volatile )
	{
		this->name = name;
		type_name = self_type;
	}

	Enum( bool is_const = false, bool is_volatile = false ) :
	  Integer( 32, true, is_const, is_volatile )
	{
		type_name = self_type;
	}

	void set_body( Json::Value &ast )
	{
		if ( this->id >= 0 )
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "`enum ", name.unwrap(), "` redefined" ),
			  ast );
			HALT();
		}
		this->id = get_enum_id();
	}

	bool is_complete() const override
	{
		return this->id >= 0;
	}

	void print( std::ostream &os, const std::vector<std::shared_ptr<Qualified>> &st, int id ) const override
	{
		if ( is_const ) os << "const ";
		if ( is_volatile ) os << "volatile ";
		os << "enum";
		if ( this->name.is_some() ) os << " " << this->name.unwrap();
		if ( st.size() != ++id )
		{
			os << " ";
			st[ id ]->print( os, st, id );
		}
	}

	std::shared_ptr<Qualified> clone() const override
	{
		return std::make_shared<Enum>( *this );
	}

public:
	bool impl_is_same_without_cv( const Qualified &other ) const override
	{
		auto &ref = static_cast<Enum const &>( other );
		return this->id == ref.id;
	}

private:
	static int get_enum_id()
	{
		static int id = 0;
		return id++;
	}
};

}  // namespace mty