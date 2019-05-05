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
        .args(&["--libnames"])
        .output()
        .unwrap();

    let llvm_libs = String::from_utf8_lossy(&output.stdout);
    let llvm_libs: Vec<_> = llvm_libs
        .split(" ")
        .map(|x| x.trim()
            .trim_start_matches("lib")
            .trim_end_matches(".lib")
            .trim_end_matches(".a"))
        .collect();

    println!("cargo:rustc-link-search=native={}", dst.display());
    println!("cargo:rustc-link-lib=static=ir-gen");

    if llvm_libs.len() > 1 {
        for sta_lib in llvm_libs.iter() {
            println!("cargo:rustc-link-lib=static={}", sta_lib);
        }
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
    } else if target.contains("windows") {
        println!("cargo:rustc-link-lib=dylib=msvcrt");
    } else {
        println!("cargo:warning={} is not a supported platform.", target);
        unimplemented!();
    }
}
