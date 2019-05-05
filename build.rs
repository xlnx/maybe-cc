extern crate cmake;

use std::env;
use std::process::Command;

fn main() {
    let dst = cmake::build("ir-gen");

    let output = Command::new("llvm-config")
        .args(&["--libdir"])
        .output()
        .unwrap_or_else(|_| {
            println!("cargo:warning={}", "llvm-config not in PATH");
            std::process::exit(0);
        });

    let llvm_lib_dir = String::from_utf8_lossy(&output.stdout);
    println!("cargo:rustc-link-search=native={}", llvm_lib_dir);

    let output = Command::new("llvm-config")
        .args(&["--libs"])
        .output()
        .unwrap();

    let llvm_libs = String::from_utf8_lossy(&output.stdout);
    let llvm_libs: Vec<_> = llvm_libs
        .split(" ")
        .map(|x| x.trim_start_matches("-l"))
        .collect();

    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=ir-gen");

    if llvm_libs.len() > 1 {
        println!("cargo:rustc-link-lib=static=LLVMCore");
        println!("cargo:rustc-link-lib=static=LLVMSupport");
    } else {
        for dyn_lib in llvm_libs.iter() {
            println!("cargo:rustc-link-lib={}", dyn_lib);
        }
    }

    let target = env::var("TARGET").unwrap();
    if target.contains("apple") {
        println!("cargo:rustc-link-lib=dylib=c++");
    } else if target.contains("linux") {
        println!("cargo:rustc-link-lib=dylib=stdc++");
    } else {
        println!("cargo:warning={}", "windows is not a supported platform.");
        unimplemented!();
    }
}
