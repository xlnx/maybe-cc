#pragma once

#include "msglist.h"

extern "C" {

char *gen_llvm_ir( const char *ast_json, ffi::MsgList **msg );
void free_llvm_ir( ffi::MsgList *msg, char *ir );
}