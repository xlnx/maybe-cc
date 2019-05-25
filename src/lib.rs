#![feature(exclusive_range_pattern)]

extern crate clap;

#[macro_use]
extern crate lazy_static;

#[macro_use]
extern crate ref_thread_local;

use clap::{App, Arg};

use myrpg::*;

mod lexer;
use lexer::CLexer;

mod prep;
use prep::Preprocessor;

mod ir;
use ir::ir_gen;

mod lang;
use lang::C;
mod msg;
use msg::*;

use std::ffi::{CStr, CString};
use std::fs::File;
use std::io::prelude::*;
use std::iter::FromIterator;
use std::os::raw::c_char;
use std::process::Command;

macro_rules! error_exit {
    () => {
        |_: ()| std::process::exit(1)
    };
}

extern "C" {
    fn init_be(dev: i32) -> *const MsgList;
    fn clear_msg();
    fn deinit_be();
    fn irc_into_obj(out_file: *const c_char) -> i32;
}

trait NextName {
    fn next_name(&mut self) -> &mut Self;
}

impl NextName for String {
    fn next_name(&mut self) -> &mut Self {
        //        self.clear();
        self.insert_str(0, "0");
        self
    }
}

fn main_rs(args: Vec<&str>) -> Result<(), std::io::Error> {
    let mut stderr = std::io::stderr();

    let matches = App::new("my")
        .version("0.1.0")
        .author("KoishiChan")
        .arg(
            Arg::with_name("output")
                .help("output file")
                .takes_value(true)
                .short("o")
                .long("output")
                .multiple(false)
                // .required(true),
        )
        .arg(
            Arg::with_name("input")
                .help("input file")
                .index(1)
                .multiple(true)
        )
        .arg(
            Arg::with_name("print")
                .help("print ast in tree structure")
                .short("p")
                .long("print")
        )
        .arg(
            Arg::with_name("target")
                .help("select a compile target")
                .takes_value(true)
                .short("t")
                .long("target")
                .multiple(false)
                .possible_values(&[
                    "ir", "ast", "obj", "elf"
                ])
        )
        .arg(
            Arg::with_name("compile")
                .help("compile but do not link")
                .short("c")
                .long("compile")
        )
        .arg(
            Arg::with_name("link")
                .help("link static library")
                .short("l")
                .takes_value(true)
                .multiple(true)
        )
        .arg(
            Arg::with_name("dev")
                .help("dev mode")
                .long("dev")
        ).get_matches_from(args.iter());

    let elf_stuff = ("elf", "");
    let obj_stuff = ("obj", ".o");
    let ir_stuff = ("ir", ".ll");
    let ast_stuff = ("ast", ".ast.json");

    let (target, suf) = if !matches.is_present("compile") {
        match matches.value_of("target") {
            Some("elf") => elf_stuff,
            Some("obj") => obj_stuff,
            Some("ir") => ir_stuff,
            Some("ast") => ast_stuff,
            None => elf_stuff,
            Some(what) => {
                println!("unknown target: {}", what);
                std::process::exit(0);
            }
        }
    } else {
        obj_stuff
    };

    let in_files = if let Some(input) = matches.values_of_lossy("input") {
        input
    } else {
        println!("no input files");
        std::process::exit(0);
    };

    let mut logger = Logger::from(&mut stderr);

    // let mut contents = String::new();
    // in_file.read_to_string(&mut contents)?;

    /* preprocessing */
    let preprocessor = Preprocessor::new();
    let parser = LRParser::<C, CLexer>::new();
    let msg;
    unsafe {
        msg = init_be(if matches.is_present("dev") { 1 } else { 0 });
    }
    let msg = if msg == std::ptr::null() {
        &MsgList {
            msgs: std::ptr::null(),
            len: 0,
            cap: 0,
        }
    } else {
        unsafe { &*msg }
    };

    let mut objs = vec![];
    let mut name = String::new();

    for in_file in in_files.iter() {
        let mut default_out_file = format!("a{}", suf);
        default_out_file = format!(
            "{}{}",
            &in_file.as_str()[in_file
                .rfind('\\')
                .or(in_file.rfind('/'))
                .map_or(0, |x| x + 1)
                ..in_file.rfind('.').unwrap_or(in_file.len())],
            suf
        );
        let out_file = matches
            .value_of("output")
            .unwrap_or(default_out_file.as_str());

        let contents = String::from("\n")
            + preprocessor
                .parse(in_file, &mut logger)
                .unwrap_or_else(error_exit!())
                .as_str()
            + "\n";

        /* parsing */
        let ast = parser
            .parse(contents.as_str(), &mut logger)
            .unwrap_or_else(error_exit!());

        if target == "ast" {
            let mut out = File::create(out_file)?;
            out.write(ast.to_json_pretty().as_bytes())?;
            if matches.is_present("print") {
                ast.print_tree();
            }
            continue;
        }

        /* ir-generation */
        let ir = ir_gen(&ast).unwrap_or_else(error_exit!());

        if !msg.log(&contents, &mut logger) {
            error_exit!()(());
        }
        unsafe {
            clear_msg();
        }

        if target == "ir" {
            let mut out = File::create(out_file)?;
            out.write(ir.as_bytes())?;
            continue;
        }

        let obj_out: String = if target == "obj" {
            out_file.into()
        } else {
            String::from("/tmp/") + name.next_name().as_str() + ".o"
        };
        let irc_val = unsafe { irc_into_obj(CString::new(obj_out.as_str()).unwrap().as_ptr()) };
        objs.push(obj_out);

        if !msg.log(&contents, &mut logger) || irc_val != 0 {
            error_exit!()(());
        }
        unsafe {
            clear_msg();
        }
    }

    if target == "elf" {
        let mut args: Vec<String> = vec!["-o", matches.value_of("output").unwrap_or("a.out")]
            .into_iter()
            .map(|x| String::from(x))
            .collect();
        args.append(&mut objs);
        let mut libs: Vec<String> = matches
            .values_of_lossy("link")
            .unwrap_or(vec![])
            .iter()
            .map(|x| String::from("-l") + x.as_str())
            .collect();
        args.append(&mut libs);

        let child = Command::new("gcc").args(args.as_slice()).output().unwrap();

        let errs = String::from_utf8(child.stderr.to_vec()).unwrap();

        if errs != "" {
            logger.log(&LogItem {
                level: Severity::Error,
                location: None,
                message: errs.trim().into(),
            });
            error_exit!()(());
        }
    }

    unsafe {
        deinit_be();
    }

    Ok(())
}

#[no_mangle]
pub fn main(argc: i32, argv: *const *const i8) -> i32 {
    unsafe {
        let args = Vec::from_iter(
            (0..argc as isize).map(|i| CStr::from_ptr(*argv.offset(i)).to_str().unwrap()),
        );
        match main_rs(args) {
            Ok(_) => {}
            Err(e) => {
                println!("uncaught error: {}", e);
                std::process::exit(1);
            }
        }
        0
    }
}
