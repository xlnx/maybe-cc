#pragma once

#include "common.h"
#include "symbolMap.h"

extern LLVMContext TheContext;
extern IRBuilder<> Builder;
extern std::unique_ptr<Module> TheModule;
extern std::shared_ptr<QualifiedValue> currentFunction;
extern std::string funcName;
extern BasicBlock *currentBB;
extern ffi::MsgList *infoList;
extern SymbolTable symTable;
extern bool stack_trace;
extern std::string decl_indent;
