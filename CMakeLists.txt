cmake_minimum_required(VERSION 3.0)
project(maybe-cc)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake-rs/cmake")

if (MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/DEBUG")
	# set(CMAKE_CXX_FLAGS_RELEASE )
else()
	set(CMAKE_CXX_FLAGS_DEBUG "-g -pg")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

enable_language(Rust)
include(CMakeCargo)

set(LLVM_LIBRARIES LLVMSupport LLVMCore)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

add_executable(mcc ./null.c)

cargo_build(NAME fe-lib)
add_subdirectory(ir-gen)

if(WIN32)
    target_link_libraries(mcc fe-lib ws2_32 userenv)
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_libraries(mcc fe-lib resolv)
else()
    target_link_libraries(mcc fe-lib pthread dl)
endif()

# if (FALSE)
llvm_map_components_to_libnames(llvm_libs
LTO Passes ObjCARCOpts WindowsManifest FuzzMutate TextAPI XCoreDisassembler XCoreCodeGen XCoreDesc XCoreInfo XCoreAsmPrinter WebAssemblyDisassembler WebAssemblyCodeGen WebAssemblyDesc WebAssemblyAsmPrinter WebAssemblyAsmParser WebAssemblyInfo SystemZDisassembler SystemZCodeGen SystemZAsmParser SystemZDesc SystemZInfo SystemZAsmPrinter SparcDisassembler SparcCodeGen SparcAsmParser SparcDesc SparcInfo SparcAsmPrinter PowerPCDisassembler PowerPCCodeGen PowerPCAsmParser PowerPCDesc PowerPCInfo PowerPCAsmPrinter NVPTXCodeGen NVPTXDesc NVPTXInfo NVPTXAsmPrinter MSP430Disassembler MSP430CodeGen MSP430AsmParser MSP430Desc MSP430Info MSP430AsmPrinter MipsDisassembler MipsCodeGen MipsAsmParser MipsDesc MipsInfo MipsAsmPrinter LanaiDisassembler LanaiCodeGen LanaiAsmParser LanaiDesc LanaiAsmPrinter LanaiInfo HexagonDisassembler HexagonCodeGen HexagonAsmParser HexagonDesc HexagonInfo BPFDisassembler BPFCodeGen BPFAsmParser BPFDesc BPFInfo BPFAsmPrinter ARMDisassembler ARMCodeGen ARMAsmParser ARMDesc ARMInfo ARMAsmPrinter ARMUtils AMDGPUDisassembler AMDGPUCodeGen AMDGPUAsmParser AMDGPUDesc AMDGPUInfo AMDGPUAsmPrinter AMDGPUUtils AArch64Disassembler AArch64CodeGen AArch64AsmParser AArch64Desc AArch64Info AArch64AsmPrinter AArch64Utils ObjectYAML
Interpreter TableGen DlltoolDriver OptRemarks XRay OrcJIT MIRParser MCA LineEditor Symbolize DebugInfoPDB DebugInfoDWARF LibDriver Option Coroutines ipo Instrumentation Vectorize Linker IRReader AsmParser Coverage X86Disassembler X86AsmParser MCJIT ExecutionEngine RuntimeDyld X86CodeGen GlobalISel X86Desc X86Info MCDisassembler X86AsmPrinter X86Utils SelectionDAG AsmPrinter CodeGen Target ScalarOpts InstCombine AggressiveInstCombine TransformUtils BitWriter Analysis ProfileData Object MCParser MC DebugInfoCodeView DebugInfoMSF BitReader Core BinaryFormat Support Demangle
)
# else ()
# set(llvm_libs LLVM-8)
# endif()

target_link_libraries(mcc ir-gen)
target_link_libraries(mcc ${llvm_libs})
