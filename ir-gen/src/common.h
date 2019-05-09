#pragma once

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <deque>

#include "msglist.h"
#include "utility.h"
#include "macros.h"

using namespace ffi;
using namespace llvm;
using namespace nonstd;
