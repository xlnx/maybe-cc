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
    
    println!("cargo:rustc-link-search=native={}", String::from_utf8_lossy(&output.stdout));

    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=ir-gen");

    println!("cargo:rustc-link-lib=static=LLVMCore");
    println!("cargo:rustc-link-lib=static=LLVMSupport");

    let target = env::var("TARGET").unwrap();
    if target.contains("apple") {
        println!("cargo:rustc-link-lib=dylib=c++");
    } else if target.contains("linux") {
        println!("cargo:rustc-link-lib=dylib=stdc++");
    // } else {
    //     unimplemented!();
    }
}
