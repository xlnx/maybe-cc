#include "common.h"
#include "global.h"

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

static int irc_into_obj_cxx(const char *out_file)
{
    auto triple = sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(triple);

    std::string err;
    auto target = TargetRegistry::lookupTarget( triple, err);

    if (!target)
    {
        infoList->add_msg(MSG_TYPE_ERROR, err);
        return 1;
    }

    auto cpu = "generic";
    auto features = "";

    TargetOptions opt;
    auto rm = Optional<Reloc::Model>();
    auto machine = target->createTargetMachine( triple, cpu, features, opt, rm );

    TheModule->setDataLayout( machine->createDataLayout() );

    std::error_code errc;
    raw_fd_ostream dest( out_file, errc, sys::fs::F_None );

    if ( errc )
    {
        infoList->add_msg(MSG_TYPE_ERROR, fmt("Could not open file: ", errc.message()));
        return 1;
    }

    legacy::PassManager pass;
    auto file_type = TargetMachine::CGFT_ObjectFile;

    if ( machine->addPassesToEmitFile( pass, dest, nullptr, file_type ) )
    {
        infoList->add_msg(MSG_TYPE_ERROR, fmt("TheTargetMachine can't emit a file of this type"));
        return 1;
    }

    pass.run( *TheModule );
    dest.flush();

    return 0;
}

extern "C" {

int irc_into_obj(const char *out_file)
{
    int val = 1;
    secure_exec([&]{
        val = irc_into_obj_cxx( out_file );
    });
    return val;
}

}
