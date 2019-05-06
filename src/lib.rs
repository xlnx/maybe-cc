extern crate clap;

#[macro_use]
extern crate lazy_static;

use clap::{App, Arg};

#[allow(unused_imports)]
use myrpg::{ast::*, log::*, symbol::*, *};

use std::fs::File;
use std::io::prelude::*;

mod prep;
use prep::Preprocessor;

mod ir;
use ir::ir_gen;

mod lang;
use lang::C;

macro_rules! error_exit {
    () => {
        |_: ()| std::process::exit(1)
    };
}

fn main_rs() -> Result<(), std::io::Error> {
    let stdin = std::io::stdin();
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
        )
        .arg(
            Arg::with_name("target")
                .help("select a compile target: ir | ast")
                .takes_value(true)
                .short("t")
                .long("target")
                .multiple(false)
        )
        .get_matches();

    let ir_stuff = ("ir", ".ii");
    let ast_stuff = ("ast", ".ast.json");

    let (target, suf) = match matches.value_of("target") {
        Some("ir") => ir_stuff,
        Some("ast") => ast_stuff,
        None => ir_stuff,
        Some(what) => {
            println!("unknown target: {}", what);
            std::process::exit(0);
        }
    };

    let mut default_out_file = format!("a{}", suf);
    let mut in_file: Box<Read> = if let Some(input) = matches.value_of("input") {
        default_out_file = format!(
            "{}{}",
            &input[..input.rfind('.').unwrap_or(input.len())],
            suf
        );
        Box::new(File::open(input)?)
    } else {
        Box::new(stdin)
    };
    let out_file = matches
        .value_of("output")
        .unwrap_or(default_out_file.as_str());

    let mut logger = Logger::from(&mut stderr);

    let mut contents = String::new();
    in_file.read_to_string(&mut contents)?;

    /* preprocessing */
    let preprocessor = Preprocessor::new();

    contents = String::from("\n") + preprocessor
        .parse(contents.as_str(), &mut logger)
        .unwrap_or_else(error_exit!()).as_str() + "\n";

    /* parsing */
    let parser = LRParser::<C>::new();

    let ast = parser
        .parse(contents.as_str(), &mut logger)
        .unwrap_or_else(error_exit!());

    if target == "ast" {
        let mut out = File::create(out_file)?;
        out.write(ast.to_json_pretty().as_bytes())?;
        std::process::exit(0);
    }

    /* ir-generation */
    let ir = ir_gen(&ast, &mut logger).unwrap_or_else(error_exit!());

    if target == "ir" {
        let mut out = File::create(out_file)?;
        out.write(ir.as_bytes())?;
        std::process::exit(0);
    }

    Ok(())
}

#[no_mangle]
pub fn main() -> i32 {
    main_rs().unwrap();
    0
}
