#pragma once

#include "common.h"
#include "symbolMap.h"

extern LLVMContext TheContext;
extern IRBuilder<> Builder;
extern std::unique_ptr<Module> TheModule;
extern std::unique_ptr<DataLayout> TheDataLayout;
extern std::shared_ptr<QualifiedValue> currentFunction;
extern std::string funcName;
extern std::stack<BasicBlock *> continueJump;
extern std::stack<BasicBlock *> breakJump;
extern std::map<std::string, std::vector<BasicBlock *>> gotoJump;
extern std::map<std::string, BasicBlock *> labelJump;
extern ffi::MsgList *infoList;
extern SymbolTable symTable;
extern bool stack_trace;
extern std::string decl_indent;
extern bool is_debug_mode;
