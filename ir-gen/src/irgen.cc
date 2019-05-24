#include "common.h"
#include "global.h"
#include "node/def.h"

JumpTable<NodeHandler> handlers = {
	{ "function_definition", pack_fn<VoidType, VoidType>( []( Json::Value &node, VoidType const & ) -> VoidType {
		  Json::Value &children = node[ "children" ];

		  auto declspec = get<DeclarationSpecifiers>( codegen( children[ 0 ], true ) );

		  if ( declspec.has_attribute( SC_TYPEDEF ) )
		  {
			  infoList->add_msg( MSG_TYPE_ERROR, "function definition declared `typedef`", children[ 0 ] );
			  HALT();
		  }
		  if ( declspec.has_attribute( SC_REGISTER ) || declspec.has_attribute( SC_AUTO ) )
		  {
			  infoList->add_msg( MSG_TYPE_ERROR, "illegal storage class on function", children[ 0 ] );
			  HALT();
		  }

		  auto builder = declspec.into_type_builder(
			children[ 0 ][ "type" ].asString() == "empty_declaration_specifiers" ? children[ 1 ] : children[ 0 ] );

		  auto decl = get<QualifiedDecl>( codegen( children[ 1 ], &builder ) );
		  auto type = decl.type;
		  auto name = decl.name.unwrap();

		  //  dbg( "in function_definition: ", decl );

		  if ( !type.is<mty::Function>() )
		  {
			  infoList->add_msg( MSG_TYPE_ERROR, "expected a function defination", children[ 0 ] );
			  HALT();
		  }

		  auto fn_type = type.as<mty::Function>();

		  if ( auto sym = symTable.find_in_scope( name ) )
		  {
			  if ( sym->is_type() )
			  {
				  infoList->add_msg(
					MSG_TYPE_ERROR,
					fmt( "redefination of `", name, "` as different kind of symbol" ),
					node );
				  HALT();
			  }
			  auto val = sym->as_value();
			  auto view = TypeView( std::make_shared<QualifiedType>( type ) );
			  if ( !val.get_type().is_same( view ) )
			  {
				  infoList->add_msg(
					MSG_TYPE_ERROR,
					fmt( "forward declaration of `", name, "` conflicts" ),
					node );
				  HALT();
			  }
		  }

		  auto fn = TheModule->getFunction( name );

		  if ( !fn )
		  {
			  if ( !( fn = Function::Create(
						static_cast<FunctionType *>( fn_type->type ),
						Function::ExternalLinkage, name, TheModule.get() ) ) )
			  {
				  INTERNAL_ERROR();
			  }
		  }

		  gotoJump.clear();
		  labelJump.clear();

		  auto func = QualifiedValue(
			std::make_shared<QualifiedType>( type ), fn, false );

		  symTable.insert(
			name,
			func,
			children[ 1 ] );

		  currentFunction = std::make_shared<QualifiedValue>( func );
		  funcName = name;
		  BasicBlock *BB = BasicBlock::Create( TheContext, "entry", fn );
		  Builder.SetInsertPoint( BB );

		  symTable.push();

		  auto fn_arg = fn->arg_begin();
		  for ( auto &arg : fn_type->args )
		  {
			  if ( arg.name.is_none() )
			  {
				  infoList->add_msg(
					MSG_TYPE_ERROR,
					fmt( "function parameter name omitted" ),
					children[ 1 ] );
				  HALT();
			  }
			  auto &name = arg.name.unwrap();
			  auto alloc = Builder.CreateAlloca( arg.type->type, 0, name );
			  Builder.CreateStore( fn_arg, alloc );
			  symTable.insert_if(
				name,
				QualifiedValue(
				  std::make_shared<QualifiedType>( arg.type ),
				  alloc,
				  true ),
				children[ 1 ] );
			  ++fn_arg;
		  }

		  //To codegen block
		  auto &basicBlock = children[ 2 ][ "children" ];
		  for ( int i = 1; i < basicBlock.size() - 1; i++ )
		  {
			  codegen( basicBlock[ i ] );
		  }

		  if ( !gotoJump.empty() )
		  {
			  for ( auto &entry : gotoJump )
			  {
				  for ( auto &goto_stmt : entry.second )
				  {
					  infoList->add_msg(
						MSG_TYPE_ERROR,
						fmt( "use of undeclared label `", entry.first, "`" ),
						goto_stmt.second );
				  }
			  }
			  HALT();
		  }

		  auto ret_ty = TypeView( std::make_shared<QualifiedType>( type ) ).next();
		  AllocaInst *retValue;
		  LoadInst *retLoad;
		  if ( !ret_ty->is<mty::Void>() && ( name != "main" || !ret_ty->is<mty::Integer>() ) )
		  {
			  retValue = Builder.CreateAlloca( ret_ty->type );
			  retLoad = Builder.CreateLoad( retValue );
			  retValue->setName( "retVal" );
		  }

		  if ( name != "main" )
		  {
			  if ( !ret_ty->is<mty::Void>() )
			  {
				  Builder.CreateRet( retLoad );
			  }
			  else
			  {
				  Builder.CreateRet( nullptr );
			  }
		  }
		  else
		  {
			  if ( ret_ty->is<mty::Integer>() )
			  {
				  Builder.CreateRet( Constant::getIntegerValue( ret_ty->type, APInt( 32, 0, false ) ) );
			  }
			  else if ( !ret_ty->is<mty::Void>() )
			  {
				  Builder.CreateRet( retLoad );
			  }
			  else
			  {
				  Builder.CreateRet( nullptr );
			  }
		  }

		  std::string fn_err;
		  raw_string_ostream fn_err_stream( fn_err );
		  if ( verifyFunction( *fn, &fn_err_stream ) )
		  {
			  fn_err_stream.flush();
			  TheModule->print( errs(), nullptr );
			  INTERNAL_ERROR( fmt( "\nLLVM Verify Function Failed:\n", fn_err ) );
		  }

		  dbg( symTable );
		  symTable.pop();

		  return VoidType{};
	  } ) }
};

static auto const volatile __decl = Declaration::reg();
static auto const volatile __stmt = Statement::reg();
static auto const volatile __expr = Expression::reg();
static auto const volatile __init = Initializer::reg();
static auto const volatile __enum = Enumerate::reg();

std::string indent( int num )
{
	std::string res;
	for ( int i = 0; i < num; ++i )
	{
		res += "  ";
	}
	return res;
}

AstType codegen( Json::Value &node, const ArgsType &arg )
{
	auto type = node[ "type" ].asCString();
	if ( type[ std::strlen( type ) - 1 ] == '_' )
		type = "binary_expression";

	static int ind = 0;

	if ( stack_trace )
	{
		dbg( indent( ind++ ), "+ ", type );
	}

	if ( handlers.find( type ) != handlers.end() )
	{
		auto res = handlers[ type ]( node, arg );
		if ( stack_trace )
		{
			dbg( indent( --ind ), "- ", type );
		}
		return res;
	}
	else
	{
		UNIMPLEMENTED( node );
	}
}

char *gen_llvm_ir_cxx( const char *ast_json )
{
	Json::Reader reader;
	Json::Value root;

	dbg( "enter ir-gen" );

	// init
	TheModule = make_unique<Module>( "asd", TheContext );
	TheDataLayout = make_unique<DataLayout>( TheModule.get() );
	currentFunction = nullptr;
	funcName = "";
	while ( !continueJump.empty() ) continueJump.pop();
	while ( !breakJump.empty() ) breakJump.pop();

	dbg( "parsing ast" );

	// finish
	if ( !reader.parse( ast_json, root ) )
	{
		INTERNAL_ERROR( fmt( "jsoncpp failed to parse ast.json" ) );
	}

	symTable.push();
	globObjects.push();

	dbg( "building va_list" );

	Json::Value dummy;
	std::vector<QualifiedDecl> comps;

	comps.emplace_back( TypeView::getCharTy( true ).into_type(), "val" );
	auto struct_ty = std::make_shared<mty::Struct>();
	struct_ty->set_body( comps, dummy );

	dbg( "register va_list" );

	auto type = DeclarationSpecifiers()
				  .add_type( QualifiedType( struct_ty ), dummy )
				  .into_type_builder( dummy )
				  .build();
	dbg( type );

	symTable.insert_if( "__builtin_va_list", type, dummy );

	dbg( "enter global" );

	// StackTrace _;

	try
	{
		for ( auto i = 0; i < root.size(); ++i )
		{
			codegen( root[ i ] );
		}
	}
	catch ( std::exception &_ )
	{
		dbg( symTable );
		globObjects.pop();
		symTable.pop();
		throw;
	}

	dbg( symTable );
	globObjects.pop();
	symTable.pop();

	if ( is_debug_mode )
	{
		TheModule->print( errs(), nullptr );
	}

	std::string module_err;
	raw_string_ostream module_err_stream( module_err );
	if ( verifyModule( *TheModule, &module_err_stream ) )
	{
		module_err_stream.flush();
		TheModule->print( errs(), nullptr );
		INTERNAL_ERROR( fmt( "\nLLVM Verify Module Failed:\n", module_err ) );
	}

	std::string cxx_ir;
	raw_string_ostream str_stream( cxx_ir );
	TheModule->print( str_stream, nullptr );

	auto ir = new char[ cxx_ir.length() + 1 ];
	memcpy( ir, cxx_ir.c_str(), cxx_ir.length() + 1 );

	return ir;
}

extern "C" {
char *gen_llvm_ir( const char *ast_json )
{
	char *val = nullptr;
	secure_exec( [&] {
		val = gen_llvm_ir_cxx( ast_json );
	} );
	return val;
}

void free_llvm_ir( char *ir )
{
	delete ir;
}
}