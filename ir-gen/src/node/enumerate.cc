#include "enumerate.h"

int Enumerate::reg()
{
	static decltype( handlers ) __ = {
		{ "enum_specifier", pack_fn<bool, QualifiedType>( []( Json::Value &node, const bool &curr_scope ) -> QualifiedType {
			  auto &children = node[ "children" ];
			  auto name = children[ 1 ][ 1 ].asString();
			  bool has_id = name != "{";
			  int an = has_id ? 2 : 1;

			  if ( children.size() < 3 )
			  {
				  return forward_decl<mty::Enum>( name, "enum." + name, curr_scope, node );
			  }
			  else
			  {
				  auto enum_ty = has_id ? std::make_shared<mty::Enum>( name ) : std::make_shared<mty::Enum>();

				  auto type = QualifiedType( enum_ty );

				  auto int_ty = TypeView::getIntTy( true );
				  QualifiedValue init = QualifiedValue(
					int_ty,
					Constant::getIntegerValue(
					  int_ty->type,
					  APInt( 32, 0, true ) ) );

				  for ( int i = an + 1; i < children.size() - 1; ++i )
				  {
					  if ( children[ i ].isObject() )
					  {
						  auto &node = children[ i ];
						  auto &children = node[ "children" ];
						  Option<QualifiedValue> val;
						  if ( children.size() > 2 )
						  {
							  init = get<QualifiedValue>( codegen( children[ 2 ] ) )
									   .value( children[ 2 ] );
							  if ( !init.get_type()->is<mty::Integer>() ||
								   !dyn_cast_or_null<Constant>( init.get() ) )
							  {
								  infoList->add_msg(
									MSG_TYPE_ERROR,
									fmt( "expression is not an integer constant expression" ),
									children[ 2 ] );
								  HALT();
							  }
							  val = init.cast( TypeView::getIntTy( true ), children[ 2 ] );
						  }
						  if ( val.is_none() )
						  {
							  val = init;
						  }
						  init = QualifiedValue(
							int_ty,
							Builder.CreateAdd(
							  init.get(),
							  Constant::getIntegerValue(
								int_ty->type,
								APInt( 32, 1, true ) ) ) );

						  symTable.insert(
							children[ 0 ][ 1 ].asCString(),
							QualifiedValue(
							  TypeView( std::make_shared<QualifiedType>( type ) ),
							  val.unwrap().get() ),
							node );
					  }
				  }

				  enum_ty->set_body( node );

				  if ( has_id )
				  {
					  auto fullName = "enum." + name;

					  fix_forward_decl( fullName, type );
				  }

				  return type;
			  }
		  } ) }
	};

	for ( auto &_ : __ )
	{
		handlers.emplace( _.first, _.second );
	}

	return 0;
}