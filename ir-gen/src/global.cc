#include "common.h"
#include "symbolMap.h"

LLVMContext TheContext;
IRBuilder<> Builder( TheContext );
std::unique_ptr<Module> TheModule;
std::unique_ptr<DataLayout> TheDataLayout;
std::shared_ptr<QualifiedValue> currentFunction;
std::string funcName;
std::stack<BasicBlock *> continueJump;
std::stack<BasicBlock *> breakJump;
std::map<std::string, std::vector<BasicBlock *>> gotoJump;
std::map<std::string, BasicBlock *> labelJump;
MsgList *infoList;
SymbolTable symTable = SymbolTable();
bool stack_trace = false;
std::string decl_indent;
bool is_debug_mode = false;
