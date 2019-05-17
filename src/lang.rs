#[allow(unused_imports)]
use myrpg::*;
use ref_thread_local::RefThreadLocal;
use regex::Regex;
use std::collections::HashSet;
use std::iter::FromIterator;

ref_thread_local! {
    static managed TYPE_SET: Vec<HashSet<String>> = vec![
        HashSet::from_iter([
                "__builtin_va_list"
            ]
            .into_iter()
            .map(|x| (*x).into())
        )
    ];
    static managed IS_TYPEDEF: bool = false;
}

lazy_static! {
    static ref SOURCE_MAP: Regex =
        Regex::new(r##"#\s+(\S+)\s+"((:?\\.|[^\\"])*)"\s*(.*)"##).unwrap();
}

fn contains_typedef<T>(ast: &Ast<T>) -> bool {
    for child in ast.children.iter() {
        match child {
            AstNode::Ast(ast) => {
                if contains_typedef(&ast) {
                    return true;
                }
            }
            AstNode::Token(token) => {
                if token.as_str() == "typedef" {
                    return true;
                }
            }
        }
    }
    false
}

fn register_types<T>(ast: &Ast<T>) {
    match ast.symbol.as_str() {
        "init_declarator" => {
            if let AstNode::Ast(ref ast) = ast.children[0] {
                register_types(&ast);
            }
        }
        "direct_declarator" => match ast.children[0] {
            AstNode::Ast(ref ast) => {
                register_types(&ast);
            }
            AstNode::Token(ref token) => {
                if token.symbol.as_str() == "IDENTIFIER" {
                    let len = TYPE_SET.borrow().len();
                    TYPE_SET.borrow_mut()[len - 1].insert(token.as_str().into());
                } else {
                    if let AstNode::Ast(ref ast) = ast.children[1] {
                        register_types(&ast);
                    }
                }
            }
        },
        _ => {
            for child in ast.children.iter() {
                if let AstNode::Ast(ast) = child {
                    register_types(&ast);
                }
            }
        }
    }
}

fn is_type(name: &String) -> bool {
    for namesp in TYPE_SET.borrow().iter().rev() {
        if namesp.contains(name) {
            return true;
        }
    }
    return false;
}

lang! {

    Name = C
    ValueType = ()

    ;;

    SOURCE_MAP => r"\n(#[^\n]*)\n" => |tok, ctrl| {
        // if let Some(caps) = SOURCE_MAP.captures(tok.val) {
        //     if let (Some(line), Some(file)) = (caps.get(1), caps.get(2)) {

        //         if let Some(flags) = caps.get(3) {
        //             let flags = flags.as_str().trim();
        //             let flags: Vec<_> = if flags.len() > 0 {
        //                 flags.split(" ").collect()
        //             } else {
        //                 vec![]
        //             };
        //         }

        //         ctrl.set_source_file(file.as_str())
        //             .set_location((line.as_str().parse::<isize>().unwrap() - 2, 0));
        //     }
        // }
        ctrl.discard();
    },
    TYPE_NAME => r"$^$^",
    FLOATING_POINT => r#"\d+[Ee][\+-]?\d+[fFlL]?\b|\d*\.\d+[fFlL]?\b|\d*\.\d+[Ee][\+-]?\d+[fFlL]?\b|\d+\.\d*[Ee][\+-]?\d+[fFlL]?\b|\d+\.\d*[fFlL]?\b"#,
    INTEGER => r#"0[xX][0-9A-Fa-f]+[uUlL]*\b|\d+[uUlL]*\b"#,
    CHAR => r#"[a-zA-Z_]?'(:?[^\\']|\\.)*'"#,
    STRING_LITERAL => r#"([a-zA-Z_]?"(:?[^\\"]|\\.)*")"#,
    IDENTIFIER => r"[a-zA-Z_]\w*\b"
        => |tok, ctrl| {
            let sym: String = tok.as_str().into();
            if is_type(&sym) {
                tok.symbol = Symbol::from("TYPE_NAME").as_terminal();
            }
        }

    ;;

    S => [
        translation_unit |@flatten|,
        _ |@flatten|
    ],

    primary_expression => [
        IDENTIFIER,
        INTEGER,
        CHAR,
        FLOATING_POINT,
        STRING_LITERAL,
        "(" expression ")"
    ],
    postfix_expression => [
        primary_expression |@flatten|,
        postfix_expression "[" expression "]",
        postfix_expression "(" ")",
        postfix_expression "(" argument_expression_list ")",
        postfix_expression "." IDENTIFIER,
        postfix_expression "->" IDENTIFIER,
        postfix_expression "++",
        postfix_expression "--"
    ],
    argument_expression_list => [
        assignment_expression |@flatten|,
        argument_expression_list "," assignment_expression |@flatten|
    ],
    unary_expression => [
        postfix_expression |@flatten|,
        "++" unary_expression,
        "--" unary_expression,
        unary_operator cast_expression,
        "sizeof" unary_expression,
        "sizeof" "(" type_name ")"
    ],
    unary_operator => [
        "&" |@flatten|,
        "*" |@flatten|,
        "+" |@flatten|,
        "-" |@flatten|,
        "~" |@flatten|,
        "!" |@flatten|
    ],
    cast_expression => [
        unary_expression |@flatten|,
        "(" type_name ")" cast_expression
    ],
    multiplicative_expression_ => [
        cast_expression |@flatten|,
        multiplicative_expression_ "*" cast_expression,
        multiplicative_expression_ "/" cast_expression,
        multiplicative_expression_ "%" cast_expression
    ],
    additive_expression_ => [
        multiplicative_expression_ |@flatten|,
        additive_expression_ "+" multiplicative_expression_,
        additive_expression_ "-" multiplicative_expression_
    ],
    shift_expression_ => [
        additive_expression_ |@flatten|,
        shift_expression_ "<<" additive_expression_,
        shift_expression_ ">>" additive_expression_
    ],
    relational_expression_ => [
        shift_expression_ |@flatten|,
        relational_expression_ "<" shift_expression_,
        relational_expression_ ">" shift_expression_,
        relational_expression_ "<=" shift_expression_,
        relational_expression_ ">=" shift_expression_
    ],
    equality_expression_ => [
        relational_expression_ |@flatten|,
        equality_expression_ "==" relational_expression_,
        equality_expression_ "!=" relational_expression_
    ],
    and_expression_ => [
        equality_expression_ |@flatten|,
        and_expression_ "&" equality_expression_
    ],
    exclusive_or_expression_ => [
        and_expression_ |@flatten|,
        exclusive_or_expression_ "^" and_expression_
    ],
    inclusive_or_expression_ => [
        exclusive_or_expression_ |@flatten|,
        inclusive_or_expression_ "|" exclusive_or_expression_
    ],
    logical_and_expression_ => [
        inclusive_or_expression_ |@flatten|,
        logical_and_expression_ "&&" inclusive_or_expression_
    ],
    logical_or_expression_ => [
        logical_and_expression_ |@flatten|,
        logical_or_expression_ "||" logical_and_expression_
    ],
    conditional_expression => [
        logical_or_expression_ |@flatten|,
        logical_or_expression_ "?" expression ":" conditional_expression
    ],
    assignment_expression => [
        conditional_expression |@flatten|,
        conditional_expression assignment_operator assignment_expression
    ],
    assignment_operator => [
        "=" |@flatten|,
        "*=" |@flatten|,
        "/=" |@flatten|,
        "%=" |@flatten|,
        "+=" |@flatten|,
        "-=" |@flatten|,
        "<<=" |@flatten|,
        ">>=" |@flatten|,
        "&=" |@flatten|,
        "^=" |@flatten|,
        "|=" |@flatten|
    ],
    expression => [
        assignment_expression |@flatten|,
        expression "," assignment_expression
    ],
    constant_expression => [
        conditional_expression |@flatten|
    ],
    declaration => [
        declaration_specifiers_i ";",
        declaration_specifiers_i init_declarator_list_i ";"
    ],
    declaration_specifiers_i => [
        declaration_specifiers => @reduce |ast| {
            *IS_TYPEDEF.borrow_mut() = contains_typedef(&ast);
        }
    ],
    declaration_specifiers => [
        storage_class_specifier |@flatten|,
        storage_class_specifier declaration_specifiers |@flatten|,
        type_specifier |@flatten|,
        type_specifier declaration_specifiers |@flatten|,
        type_qualifier |@flatten|,
        type_qualifier declaration_specifiers |@flatten|
    ],
    init_declarator_list_i => [
        init_declarator_list |@flatten| => @reduce |ast| {
            if *IS_TYPEDEF.borrow() {
                register_types(&ast);
            }
        }
    ],
    init_declarator_list => [
        init_declarator |@flatten|,
        init_declarator_list "," init_declarator |@flatten|
    ],
    init_declarator => [
        declarator,
        declarator "=" initializer
    ],
    storage_class_specifier => [
        "typedef" |@flatten|,
        "extern" |@flatten|,
        "static" |@flatten|,
        "auto" |@flatten|,
        "register" |@flatten|
    ],
    userdefined_type_name => [
        TYPE_NAME
    ],
    type_specifier => [
        "void" |@flatten|,
        "char" |@flatten|,
        "short" |@flatten|,
        "int" |@flatten|,
        "long" |@flatten|,
        "float" |@flatten|,
        "double" |@flatten|,
        "signed" |@flatten|,
        "unsigned" |@flatten|,
        struct_or_union_specifier |@flatten|,
        enum_specifier |@flatten|,
        userdefined_type_name |@flatten|
    ],
    enter_block => [
        enter_block_trig "{" |@flatten|
    ],
    enter_block_trig => [
        _ |@flatten| => @reduce |ast| {
            TYPE_SET.borrow_mut().push(HashSet::new());
        }
    ],
    leave_block => [
        leave_block_trig "}" |@flatten|
    ],
    leave_block_trig => [
        _ |@flatten| => @reduce |ast| {
            TYPE_SET.borrow_mut().pop();
        }
    ],
    struct_or_union_specifier => [
        struct_or_union IDENTIFIER enter_block struct_declaration_list_i leave_block,
        struct_or_union enter_block struct_declaration_list_i leave_block,
        struct_or_union IDENTIFIER
    ],
    struct_or_union => [
        "struct" |@flatten|, "union" |@flatten|
    ],
    struct_declaration_list_i => [
        struct_declaration_list
    ],
    struct_declaration_list => [
        _ |@flatten|,
        struct_declaration_list struct_declaration |@flatten|
    ],
    struct_declaration => [
        specifier_qualifier_list_i struct_declarator_list ";",
        specifier_qualifier_list_i ";"
    ],
    specifier_qualifier_list_i => [
        specifier_qualifier_list
    ],
    specifier_qualifier_list => [
        type_specifier specifier_qualifier_list |@flatten|,
        type_specifier |@flatten|,
        type_qualifier specifier_qualifier_list |@flatten|,
        type_qualifier |@flatten|
    ],
    struct_declarator_list => [
        struct_declarator |@flatten|,
        struct_declarator_list "," struct_declarator |@flatten|
    ],
    struct_declarator => [
        declarator,
        ":" constant_expression,
        declarator ":" constant_expression
    ],
    enum_specifier => [
        "enum" enter_block enumerator_list leave_block,
        "enum" IDENTIFIER enter_block enumerator_list leave_block,
        "enum" IDENTIFIER
    ],
    enumerator_list => [
        enumerator,
        enumerator_list "," enumerator
    ],
    enumerator => [
        IDENTIFIER,
        IDENTIFIER "=" constant_expression
    ],
    type_qualifier => [
        "const" |@flatten|, "volatile" |@flatten|
    ],
    declarator => [
        pointer direct_declarator,
        direct_declarator |@flatten|
    ],
    direct_declarator => [
        IDENTIFIER,
        "(" declarator ")",
        direct_declarator "[" constant_expression "]",
        direct_declarator "[" "]",
        direct_declarator "(" parameter_type_list ")",
        direct_declarator "(" identifier_list ")",
        direct_declarator "(" ")"
    ],
    pointer => [
        "*",
        "*" type_qualifier_list_i,
        "*" pointer,
        "*" type_qualifier_list_i pointer
    ],
    type_qualifier_list_i => [
        type_qualifier_list
    ],
    type_qualifier_list => [
        type_qualifier |@flatten|,
        type_qualifier_list type_qualifier |@flatten|
    ],
    parameter_type_list => [
        parameter_list |@flatten|,
        parameter_list "," "..." |@flatten|
    ],
    parameter_list => [
        parameter_declaration |@flatten|,
        parameter_list "," parameter_declaration |@flatten|
    ],
    parameter_declaration => [
        declaration_specifiers_p declarator,
        declaration_specifiers_p abstract_declarator,
        declaration_specifiers_p
    ],
    declaration_specifiers_p => [
        declaration_specifiers
    ],
    identifier_list => [
        IDENTIFIER,
        identifier_list "," IDENTIFIER
    ],
    type_name => [
        specifier_qualifier_list_i,
        specifier_qualifier_list_i abstract_declarator
    ],
    abstract_declarator => [
        pointer,
        direct_abstract_declarator,
        pointer direct_abstract_declarator
    ],
    direct_abstract_declarator => [
        "(" abstract_declarator ")",
        "[" "]",
        "[" constant_expression "]",
        direct_abstract_declarator "[" "]",
        direct_abstract_declarator "[" constant_expression "]",
        "(" ")",
        "(" parameter_type_list ")",
        direct_abstract_declarator "(" ")",
        direct_abstract_declarator "(" parameter_type_list ")"
    ],
    initializer => [
        assignment_expression,
        enter_block initializer_list leave_block,
        enter_block initializer_list "," leave_block
    ],
    initializer_list => [
        initializer,
        initializer_list "," initializer
    ],
    statement => [
        labeled_statement |@flatten|,
        compound_statement |@flatten|,
        declaration_statement |@flatten|,
        expression_statement |@flatten|,
        selection_statement |@flatten|,
        iteration_statement |@flatten|,
        jump_statement |@flatten|
    ],
    labeled_statement => [
        IDENTIFIER ":" statement,
        "case" constant_expression ":" statement,
        "default" ":" statement
    ],
    compound_statement => [
        enter_block leave_block,
        enter_block statement_list leave_block
    ],
    declaration_statement => [
        declaration |@flatten|
    ],
    declaration_list => [
        declaration |@flatten|,
        declaration_list declaration |@flatten|
    ],
    statement_list => [
        statement |@flatten|,
        statement_list statement |@flatten|
    ],
    expression_statement => [
        ";",
        expression ";"
    ],
    selection_statement => [
        "if" "(" expression ")" statement "else" statement,
        "if" "(" expression ")" statement,
        "switch" "(" expression ")" statement
    ],
    iteration_statement => [
        "while" "(" expression ")" statement,
        "do" statement "while" "(" expression ")" ";",
        "for" "(" expression_statement expression_statement ")" statement,
        "for" "(" expression_statement expression_statement expression ")" statement
    ],
    jump_statement => [
        "goto" IDENTIFIER ";",
        "continue" ";",
        "break" ";",
        "return" ";",
        "return" expression ";"
    ],
    translation_unit => [
        external_declaration |@flatten|,
        translation_unit external_declaration |@flatten|
    ],
    external_declaration => [
        function_definition |@flatten|,
        declaration |@flatten|
    ],
    function_definition_prefix => [
        declaration_specifiers_i declarator declaration_list |@flatten| => @reduce |ast| {
            if *IS_TYPEDEF.borrow() {
                if let AstNode::Ast(ref ast) = ast.children[1] {
                    register_types(&ast);
                }
            }
        },
        declaration_specifiers_i declarator |@flatten| => @reduce |ast| {
            if *IS_TYPEDEF.borrow() {
                if let AstNode::Ast(ref ast) = ast.children[1] {
                    register_types(&ast);
                }
            }
        }
    ],
    function_definition => [
        function_definition_prefix compound_statement,
        declarator declaration_list compound_statement,
        declarator compound_statement
    ]
}
