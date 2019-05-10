#include "common.h"
#include "symbolMap.h"

LLVMContext TheContext;
IRBuilder<> Builder( TheContext );
std::unique_ptr<Module> TheModule;
std::map<std::string, Value *> NamedValues;
Function *currentFunction;
BasicBlock *currentBB;
MsgList *infoList;
SymbolTable symTable = SymbolTable();
bool stack_trace = false;