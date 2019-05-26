#pragma once

#include "common.h"
#include "scopedMap.h"

struct Global
{
	QualifiedValue value;
	bool is_internal;
	bool is_allocated;

	Global( const QualifiedValue &val, bool is_internal = false,
			bool is_allocated = false ) :
	  value( val ),
	  is_internal( is_internal ),
	  is_allocated( is_allocated )
	{
	}
};

inline bool declare_global( const std::string &name,
							const Global &prev, const Global &curr,
							Json::Value &ast )
{
	auto &prev_type = prev.value.get_type();
	auto &curr_type = curr.value.get_type();
	dbg( name, " with type ", prev_type, " ", curr_type, " ", prev.is_internal, " ", curr.is_internal, " ", prev.is_allocated, " ", curr.is_allocated );
	if ( !prev_type.is_same( curr_type ) )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "conflicting types for `", name, "`" ),
		  ast );
		HALT();
	}
	if ( prev.is_allocated )
	{
		if ( curr.is_allocated )
		{
			infoList->add_msg(
			  MSG_TYPE_ERROR,
			  fmt( "redefination of `", name, "`" ),
			  ast );
			HALT();
		}
		return false;
	}
	if ( prev.is_internal && !curr.is_internal )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "non-static declaration of `", name, "` follows static declaration" ),
		  ast );
		HALT();
		return false;
	}
	if ( !prev.is_internal && curr.is_internal )
	{
		infoList->add_msg(
		  MSG_TYPE_ERROR,
		  fmt( "static declaration of `", name, "` follows non-static declaration" ),
		  ast );
		HALT();
		return false;
	}
	return true;
}

extern LLVMContext TheContext;
extern IRBuilder<> Builder;
extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<DataLayout> TheDataLayout;
extern std::shared_ptr<QualifiedValue> currentFunction;
extern std::string funcName;
extern std::stack<BasicBlock *> continueJump;
extern std::stack<BasicBlock *> breakJump;
extern std::map<std::string, std::vector<std::pair<BasicBlock *, Json::Value>>> gotoJump;
extern std::map<std::string, BasicBlock *> labelJump;
extern std::stack<std::map<ConstantInt *, BasicBlock *>> caseList;
extern std::stack<std::pair<bool, BasicBlock *>> defaultList;
extern ffi::MsgList *infoList;
extern ScopedMap<Symbol> symTable;
extern ScopedMap<Global> globObjects;
extern bool stack_trace;
extern std::string decl_indent;
extern bool is_debug_mode;
