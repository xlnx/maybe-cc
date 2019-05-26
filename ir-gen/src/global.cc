#include "global.h"

LLVMContext TheContext;
IRBuilder<> Builder( TheContext );
std::unique_ptr<Module> TheModule;
std::unique_ptr<DataLayout> TheDataLayout;
std::shared_ptr<QualifiedValue> currentFunction;
std::string funcName;
std::stack<BasicBlock *> continueJump;
std::stack<BasicBlock *> breakJump;
std::map<std::string, std::vector<std::pair<BasicBlock *, Json::Value>>> gotoJump;
std::map<std::string, BasicBlock *> labelJump;
std::stack<std::map<ConstantInt *, BasicBlock *>> caseList;
std::stack<std::pair<bool, BasicBlock *>> defaultList;
std::stack<int> switchBits;
MsgList *infoList;
ScopedMap<Symbol> symTable;
ScopedMap<Global> globObjects;
bool stack_trace = false;
std::string decl_indent;
bool is_debug_mode = false;
