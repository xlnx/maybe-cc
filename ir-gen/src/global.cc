#include "common.h"
#include "symbolMap.h"

LLVMContext TheContext;
IRBuilder<> Builder( TheContext );
std::unique_ptr<Module> TheModule;
Function *currentFunction;
BasicBlock *currentBB;
MsgList *infoList;
SymbolTable symTable = SymbolTable();
bool stack_trace = false;