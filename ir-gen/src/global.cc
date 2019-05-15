#include "common.h"
#include "symbolMap.h"

LLVMContext TheContext;
IRBuilder<> Builder( TheContext );
std::unique_ptr<Module> TheModule;
std::shared_ptr<QualifiedValue> currentFunction;
std::string funcName;
BasicBlock *currentBB;
MsgList *infoList;
SymbolTable symTable = SymbolTable();
bool stack_trace = false;
std::string decl_indent;
