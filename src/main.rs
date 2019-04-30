use myrpg::{ast::*, *};

use std::fs::File;
use std::io::prelude::*;

lang! {

    Name = IfExpr
    ValueType = i32

    ;;

    Id => r"[a-zA-Z_]\w*",
    Number => r"[0-9]+"

    ;;

    S => [
        Glob => |glob| -> _ {
            println!("{:?}", glob);
            None
        }
    ],
    Glob => [
        Function
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
        Stmts Stmt,
        _
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
        Expr0
    ],

    Expr0 => [
        Expr0 "," Expr1,
        Expr1
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
        Expr2
    ],
    Expr2 => [
        Expr3 "?" Expr2 ":" Expr2,
        Expr3
    ],
    Expr3 => [
        Expr3 "||" Expr4,
        Expr4
    ],
    Expr4 => [
        Expr4 "^^" Expr5,
        Expr5
    ],
    Expr5 => [
        Expr5 "&&" Expr6,
        Expr6
    ],
    Expr6 => [
        Expr6 "|" Expr7,
        Expr7
    ],
    Expr7 => [
        Expr7 "^" Expr8,
        Expr8
    ],
    Expr8 => [
        Expr8 "&" Expr9,
        Expr9
    ],
    Expr9 => [
        Expr9 "==" Expr10,
        Expr9 "!=" Expr10,
        Expr10
    ],
    Expr10 => [
        Expr10 "<" Expr11,
        Expr10 "<=" Expr11,
        Expr10 ">" Expr11,
        Expr10 ">=" Expr11,
        Expr11
    ],
    Expr11 => [
        Expr11 "<<" Expr12,
        Expr11 ">>" Expr12,
        Expr12
    ],
    Expr12 => [
        Expr12 "+" Expr13,
        Expr12 "-" Expr13,
        Expr13
    ],
    Expr13 => [
        Expr13 "*" Expr14,
        Expr13 "/" Expr14,
        Expr13 "%" Expr14,
        Expr14
    ],
    Expr14 => [
        "++" Expr14,
        "--" Expr14,
        "+" Expr14,
        "-" Expr14,
        "~" Expr14,
        "!" Expr14,
        Expr15
    ],
    Expr15 => [
        Expr15 "++",
        Expr15 "--",
        Expr15 "." Id,
        Expr15 "(" UnnamedParams ")",
        Expr15 "[" Expr0 "]",
        Expr16
    ],
    Expr16 => [
        "(" Expr0 ")",
        Id,
        Number
    ]

}

fn main() {
    let parser = LRParser::<IfExpr>::new();
    let mut file = File::open("a.glsl").unwrap();
    let mut contents = String::new();
    file.read_to_string(&mut contents);

    match parser.parse(contents.as_str()) {
        Ok(val) => println!("{:?}", val),
        Err(err) => println!("{:?}", err),
    }
}