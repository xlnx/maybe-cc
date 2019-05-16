#include "statement.h"

int Statement::reg()
{
	static decltype( handlers ) stmt = {
		{ "compound_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];

			  symTable.push();

			  // Ignore the { and }
			  for ( int i = 1; i < children.size() - 1; i++ )
			  {
				  codegen( children[ i ] );
			  }

			  dbg( symTable );
			  symTable.pop();

			  return VoidType();
		  } ) },
		{ "expression_statement", pack_fn<VoidType, Option<QualifiedValue>>( []( Json::Value &node, VoidType const & ) -> Option<QualifiedValue> {
			  auto &children = node[ "children" ];

			  if ( children.size() > 1 )  // expr ;
			  {
				  return get<QUalifiedValue>( codegen( children[ 0 ] ) );
			  }

			  return NoneOpt{};
		  } ) },
		{ "iteration_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  auto typeIter = children[ 0 ][ 1 ].asCString();
			  static JumpTable<VoidType( Json::Value & children, Json::Value & ast )> __ = {
				  { "while", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } },
				  { "do", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } },
				  { "for", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto func = currentFunction->get();
					   auto loopEnd = BasicBlock::Create( TheContext, "for.end", static_cast<Function *>( func ) );
					   auto loopBody = BasicBlock::Create( TheContext, "for.body", static_cast<Function *>( func ), loopEnd );
					   auto loopInc = BasicBlock::Create( TheContext, "for.inc", static_cast<Function *>( func ), loopBody );
					   auto loopCond = BasicBlock::Create( TheContext, "for.cond", static_cast<Function *>( func ), loopInc );

					   codegen( children[ 2 ] );
					   //    auto branch =
					   Builder.SetInsertPoint( loopCond );
					//    auto branch = get<QualifiedValue>( codegen( children[ 3 ] ) )
					//    .value(children[3])
					//    .cast(TypeView:;
					   

					   //    Builder.CreateCondBr();
					   UNIMPLEMENTED();
					   return VoidType();
				   } }
			  };
			  if ( __.find( typeIter ) != __.end() )
			  {
				  return __[ typeIter ]( children, node );
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
		{ "jump_statement", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
			  auto &children = node[ "children" ];
			  auto typeRet = children[ 0 ][ 1 ].asCString();

			  static JumpTable<VoidType( Json::Value & children, Json::Value & ast )> __ = {
				  { "return", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   auto fn_res_ty = currentFunction->get_type();
					   fn_res_ty.next();
					   if ( children.size() == 3 )
					   {
						   if ( fn_res_ty->is<mty::Void>() )
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "void function `", funcName, "` should not return a value" ),
								 children[ 1 ] );
							   HALT();
						   }
						   auto retValue = get<QualifiedValue>( codegen( children[ 1 ] ) )
											 .value( children[ 1 ] )
											 .cast( fn_res_ty, children[ 1 ] );
						   Builder.CreateRet( retValue.get() );
					   }
					   else if ( children.size() == 2 )
					   {
						   if ( fn_res_ty->is<mty::Void>() )
						   {
							   Builder.CreateRet( nullptr );
						   }
						   else
						   {
							   infoList->add_msg(
								 MSG_TYPE_ERROR,
								 fmt( "non-void function `", funcName, "` should return a value" ),
								 ast );
						   }
					   }
					   else
					   {
						   INTERNAL_ERROR();
					   }
				   } },
				  { "goto", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } },
				  { "continue", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } },
				  { "break", []( Json::Value &children, Json::Value &ast ) -> VoidType {
					   UNIMPLEMENTED();
				   } }
			  };
			  if ( __.find( typeRet ) != __.end() )
			  {
				  return __[ typeRet ]( children, node );
			  }
			  else
			  {
				  INTERNAL_ERROR();
			  }
		  } ) },
	};

	handlers.insert( stmt.begin(), stmt.end() );

	return 0;
}
