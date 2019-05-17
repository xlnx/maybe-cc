#pragma once

#include "msglist.h"

extern "C" {

char *gen_llvm_ir( const char *ast_json );
void free_llvm_ir( char *ir );
}