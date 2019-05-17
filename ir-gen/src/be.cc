#include "common.h"
#include "global.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

extern "C" {

MsgList *init_be(int debug)
{
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    is_debug_mode = debug != 0;

    auto msg_l = new MsgList();
    infoList = msg_l;
    return msg_l;
}

void clear_msg()
{
    infoList->clear();
}

void deinit_be()
{
    delete infoList;
}

}