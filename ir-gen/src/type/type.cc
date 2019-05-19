#include "type.h"
#include "def.h"

TypeView const &TypeView::getVoidPtrTy()
{
	static auto s_v = ( [] {
		Json::Value ast;
		return TypeView(
		  std::make_shared<QualifiedType>(
			DeclarationSpecifiers()
			  .add_type( QualifiedType( std::make_shared<mty::Void>() ), ast )
			  .into_type_builder( ast )
			  .add_level( std::make_shared<mty::Pointer>( mty::Void().type ) )
			  .build() ) );
	} )();

	return s_v;
}
TypeView const &TypeView::getBoolTy()
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 1, true ) );

	static auto s_v = TypeView( s_ty );

	return s_v;
}
TypeView const &TypeView::getCharTy( bool is_signed )
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 8, true ) );
	static auto u_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 8, false ) );

	static auto s_v = TypeView( s_ty );
	static auto u_v = TypeView( s_ty );

	return is_signed ? s_v : u_v;
}
TypeView const &TypeView::getShortTy( bool is_signed )
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 16, true ) );
	static auto u_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 16, false ) );

	static auto s_v = TypeView( s_ty );
	static auto u_v = TypeView( s_ty );

	return is_signed ? s_v : u_v;
}
TypeView const &TypeView::getIntTy( bool is_signed )
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 32, true ) );
	static auto u_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 32, false ) );

	static auto s_v = TypeView( s_ty );
	static auto u_v = TypeView( s_ty );

	return is_signed ? s_v : u_v;
}
TypeView const &TypeView::getLongTy( bool is_signed )
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 64, true ) );
	static auto u_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 64, false ) );

	static auto s_v = TypeView( s_ty );
	static auto u_v = TypeView( s_ty );

	return is_signed ? s_v : u_v;
}
TypeView const &TypeView::getLongLongTy( bool is_signed )
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 64, true ) );
	static auto u_ty = std::make_shared<QualifiedType>( std::make_shared<mty::Integer>( 64, false ) );

	static auto s_v = TypeView( s_ty );
	static auto u_v = TypeView( s_ty );

	return is_signed ? s_v : u_v;
}
TypeView const &TypeView::getFloatTy()
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::FloatingPoint>( 32 ) );

	static auto s_v = TypeView( s_ty );

	return s_v;
}
TypeView const &TypeView::getDoubleTy()
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::FloatingPoint>( 64 ) );

	static auto s_v = TypeView( s_ty );

	return s_v;
}
TypeView const &TypeView::getLongDoubleTy()
{
	static auto s_ty = std::make_shared<QualifiedType>( std::make_shared<mty::FloatingPoint>( 128 ) );

	static auto s_v = TypeView( s_ty );

	return s_v;
}
