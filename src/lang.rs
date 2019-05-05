#[allow(unused_imports)]
use myrpg::{ast::*, symbol::*, *};

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
