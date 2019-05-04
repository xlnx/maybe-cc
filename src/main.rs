extern crate clap;

use clap::{App, Arg};

#[allow(unused_imports)]
use myrpg::{ast::*, log::*, symbol::*, *};

use std::ffi::{CStr, CString};
use std::fs::File;
use std::io::prelude::*;
use std::os::raw::c_char;

mod prep;
use prep::Preprocessor;

lang! {

    Name = C
    ValueType = i32

    ;;

    Id => r"[a-zA-Z_]\w*\b",
        // => |tok: &mut Token| {
        //     tok.symbol = Symbol::from("Number")
        // },
    Number => r"[0-9]+\b"

    ;;

    S => [
        Global |@flatten|
    ],
    Global => [
        Function |@flatten|
    ],
    Type => [
        "int", "void"
    ],
    Function => [
        Type Id "(" NamedParams ")" Block
    ],
    Block => [
        "{" Stmts "}"
    ],
    Stmts => [
        Stmts Stmt |@flatten|,
        _ |@flatten|
    ],
    Stmt => [
        IfStmt,
        ForStmt,
        WhileStmt,
        Expr0 ";",
        Decl,
        Block
    ],
    IfStmt => [
        "if" "(" Cond ")" Stmt "else" Stmt,
        "if" "(" Cond ")" Stmt
    ],
    ForStmt => [
        "for" "("  ";" Cond ";"  ")" Stmt
    ],
    WhileStmt => [
        "while" "(" Cond ")" Stmt
    ],
    Decl => [
        Type Id DeclRight
    ],
    DeclRight => [
        "," Id DeclRight,
        ";"
    ],
    NamedParams => [
        Type Id NamedParamsRight,
        _
    ],
    NamedParamsRight => [
        "," Type Id NamedParamsRight,
        ",",
        _
    ],
    UnnamedParams => [
        Expr1 UnnamedParamsRight,
        _
    ],
    UnnamedParamsRight => [
        "," Expr1 UnnamedParamsRight,
        ",",
        _
    ],
    Cond => [
        Expr0 |@flatten|
    ],

    // AddressSpaceQualifier => [
    //     "global",
    //     "__global",
    //     "local",
    //     "__local",
    //     "constant",
    //     "__constant",
    //     "private",
    //     "__private"
    // ],
    // AccessQualifiers => [
    //     "read_only",
    //     "__read_only",
    //     "write_only",
    //     "__write_only",
    //     "read_write",
    //     "__read_write"
    // ],
    // FunctionQualifiers => [
    //     "kernel",
    //     "__kernel"
    // ],
    // OptionalAttributeQualifiers => [
    //     "__attribute__"
    // ],
    // StorageClassSpecifiers => [
    //     "extern",
    //     "static"
    // ],

    NoptrDeclarator => [
        Id,
        NoptrDeclarator "[" Qualifiers Expr0 "]",
        "(" Declarator ")",
        NoptrDeclarator "[" Qualifiers "]",
        NoptrDeclarator "[" Qualifiers "]"
    ],
    PtrDeclarator => [
        "*" Qualifiers Declarator
    ],
    Declarator => [
        PtrDeclarator,
        NoptrDeclarator
    ],

    Expr0 => [
        Expr0 "," Expr1,
        Expr1 |@flatten|
    ],
    Expr1 => [
        Expr2 "=" Expr1,
        Expr2 "+=" Expr1,
        Expr2 "-=" Expr1,
        Expr2 "*=" Expr1,
        Expr2 "/=" Expr1,
        Expr2 "%=" Expr1,
        Expr2 "<<=" Expr1,
        Expr2 ">>=" Expr1,
        Expr2 "&=" Expr1,
        Expr2 "^=" Expr1,
        Expr2 "|=" Expr1,
        Expr2 |@flatten|
    ],
    Expr2 => [
        Expr3 "?" Expr2 ":" Expr2,
        Expr3 |@flatten|
    ],
    Expr3 => [
        Expr3 "||" Expr4,
        Expr4 |@flatten|
    ],
    Expr4 => [
        Expr4 "^^" Expr5,
        Expr5 |@flatten|
    ],
    Expr5 => [
        Expr5 "&&" Expr6,
        Expr6 |@flatten|
    ],
    Expr6 => [
        Expr6 "|" Expr7,
        Expr7 |@flatten|
    ],
    Expr7 => [
        Expr7 "^" Expr8,
        Expr8 |@flatten|
    ],
    Expr8 => [
        Expr8 "&" Expr9,
        Expr9 |@flatten|
    ],
    Expr9 => [
        Expr9 "==" Expr10,
        Expr9 "!=" Expr10,
        Expr10 |@flatten|
    ],
    Expr10 => [
        Expr10 "<" Expr11,
        Expr10 "<=" Expr11,
        Expr10 ">" Expr11,
        Expr10 ">=" Expr11,
        Expr11 |@flatten|
    ],
    Expr11 => [
        Expr11 "<<" Expr12,
        Expr11 ">>" Expr12,
        Expr12 |@flatten|
    ],
    Expr12 => [
        Expr12 "+" Expr13,
        Expr12 "-" Expr13,
        Expr13 |@flatten|
    ],
    Expr13 => [
        Expr13 "*" Expr14,
        Expr13 "/" Expr14,
        Expr13 "%" Expr14,
        Expr14 |@flatten|
    ],
    Expr14 => [
        "++" Expr14,
        "--" Expr14,
        "+" Expr14,
        "-" Expr14,
        "~" Expr14,
        "!" Expr14,
        Expr15 |@flatten|
    ],
    Expr15 => [
        Expr15 "++",
        Expr15 "--",
        Expr15 "." Id,
        Expr15 "(" UnnamedParams ")",
        Expr15 "[" Expr0 "]",
        Expr16 |@flatten|
    ],
    Expr16 => [
        "(" Expr0 ")",
        Id,
        Number
    ]

}

fn main() -> Result<(), std::io::Error> {
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

    contents = preprocessor
        .parse(contents.as_str(), &mut logger)
        .unwrap_or_else(|_| {
            std::process::exit(1);
        });

    /* parsing */
    let parser = LRParser::<C>::new();

    let ast = parser
        .parse(contents.as_str(), &mut logger)
        .unwrap_or_else(|_| {
            std::process::exit(1);
        });

    if target == "ast" {
        let mut out = File::create(out_file)?;
        out.write(ast.to_json_pretty().as_bytes())?;
        std::process::exit(0);
    }

    /* ir-generation */
    #[repr(C)]
    struct Msg {
        pos: [usize; 4],
        msg: *const c_char,
    }
    #[repr(C)]
    struct MsgList {
        msgs: *const Msg,
        len: usize,
        cap: usize,
    }

    extern "C" {
        fn gen_llvm_ir(ast_json: *const c_char, msg: *mut *const MsgList) -> *const c_char;
        fn free_llvm_ir(msg: *const MsgList, ir: *const c_char);
    }

    let mut msg_c: *const MsgList = std::ptr::null();
    let ir_c: *const c_char;

    unsafe {
        let ast_json_c = CString::new(ast.to_json().as_str()).unwrap();
        ir_c = gen_llvm_ir(ast_json_c.as_ptr(), &mut msg_c as *mut _);
    }

    let ir = if ir_c == std::ptr::null() {
        panic!("internal-error: ir-gen output null")
    } else {
        unsafe { CStr::from_ptr(ir_c).to_str().unwrap() }
    };
    let msg = if msg_c == std::ptr::null() {
        &MsgList {
            msgs: std::ptr::null(),
            len: 0,
            cap: 0,
        }
    } else {
        unsafe { &*msg_c }
    };

    for i in 0..msg.len {
        unsafe {
            let msg_chunk = &*msg.msgs.offset(i as isize);
            let msg = CStr::from_ptr(msg_chunk.msg).to_str().unwrap();
            logger.log(&Item {
                level: Severity::Error,
                location: SourceFileLocation {
                    name: "ASDASD".into(),
                    line: "fuck you.",
                    from: (msg_chunk.pos[0], msg_chunk.pos[1]),
                    to: (msg_chunk.pos[2], msg_chunk.pos[3]),
                },
                message: msg.into(),
            })
        }
    }

    let ir = String::from(ir);

    unsafe {
        free_llvm_ir(msg_c, ir_c);
    }

    if target == "ir" {
        let mut out = File::create(out_file)?;
        out.write(ir.as_bytes())?;
        std::process::exit(0);
    }

    Ok(())
}
