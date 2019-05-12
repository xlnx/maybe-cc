#pragma once

#include "common.h"
#include "symbolMap.h"

extern LLVMContext TheContext;
extern IRBuilder<> Builder;
extern std::unique_ptr<Module> TheModule;
extern Function *currentFunction;
extern BasicBlock *currentBB;
extern ffi::MsgList *infoList;
extern SymbolTable symTable;
extern bool stack_trace;
extern std::string decl_indent;
