#include "common.h"

LLVMContext TheContext;
IRBuilder<> Builder( TheContext );
std::unique_ptr<Module> TheModule;
std::map<std::string, Value *> NamedValues;
Function *currentFunction;
BasicBlock *currentBB;
MsgList *infoList;
symbolTable symTable = symbolTable();