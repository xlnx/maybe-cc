#[allow(unused_imports)]
use myrpg::{ast::*, symbol::*, *};
use regex::Regex;

lazy_static! {
    static ref SOURCE_MAP: Regex =
        Regex::new(r##"#\s+(\S+)\s+"((:?\\.|[^\\"])*)"\s*(.*)"##).unwrap();
}

lang! {

    Name = C
    ValueType = ()

    ;;

    SOURCE_MAP => r"\n(#[^\n]*)\n" => |tok, ctrl| {
        if let Some(caps) = SOURCE_MAP.captures(tok.val) {
            if let (Some(line), Some(file)) = (caps.get(1), caps.get(2)) {

                if let Some(flags) = caps.get(3) {
                    let flags = flags.as_str().trim();
                    let flags: Vec<_> = if flags.len() > 0 {
                        flags.split(" ").collect()
                    } else {
                        vec![]
                    };
                }

                ctrl.set_source_file(file.as_str())
                    .set_location((0, line.as_str().parse::<usize>().unwrap()));
            }
        }
        ctrl.discard();
    },
    IDENTIFIER => r"[a-zA-Z_]\w*\b",
        // => |tok| -> _ {
        //     tok.symbol = Symbol::from("Number")
        // },
    TYPE_NAME => r"$^$^",
    CONSTANT => r#"0[xX][0-9A-Fa-f]+(?:u|U|l|L)*?|[0-9]+(?:u|U|l|L)*?|[a-zA-Z_]?'(?:\\.|[^\\'])+'|[0-9]+[Ee][\+-]?[0-9]+(?:f|F|l|L)?|[0-9]*"."[0-9]+(?:[Ee][\+-]?[0-9]+)?(?:f|F|l|L)?"#,
    STRING_LITERAL => r#"[a-zA-Z_]?"(\\.|[^\\"])*"#

    ;;

    S => [
        translation_unit |@flatten|,
        _ |@flatten|
    ],

    primary_expression => [
        IDENTIFIER |@flatten|,
        CONSTANT |@flatten|,
        STRING_LITERAL |@flatten|,
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
        argument_expression_list "," assignment_expression
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
        "&","*","+","-","~","!"
    ],
    cast_expression => [
        unary_expression |@flatten|,
        "(" type_name ")" cast_expression
    ],
    multiplicative_expression => [
        cast_expression |@flatten|,
        multiplicative_expression "*" cast_expression,
        multiplicative_expression "/" cast_expression,
        multiplicative_expression "%" cast_expression
    ],
    additive_expression => [
        multiplicative_expression |@flatten|,
        additive_expression "+" multiplicative_expression,
        additive_expression "-" multiplicative_expression
    ],
    shift_expression => [
        additive_expression |@flatten|,
        shift_expression "<<" additive_expression,
        shift_expression ">>" additive_expression
    ],
    relational_expression => [
        shift_expression |@flatten|,
        relational_expression "<" shift_expression,
        relational_expression ">" shift_expression,
        relational_expression "<=" shift_expression,
        relational_expression ">=" shift_expression
    ],
    equality_expression => [
        relational_expression |@flatten|,
        equality_expression "==" relational_expression,
        equality_expression "!=" relational_expression
    ],
    and_expression => [
        equality_expression |@flatten|,
        and_expression "&" equality_expression
    ],
    exclusive_or_expression => [
        and_expression |@flatten|,
        exclusive_or_expression "^" and_expression
    ],
    inclusive_or_expression => [
        exclusive_or_expression |@flatten|,
        inclusive_or_expression "|" exclusive_or_expression
    ],
    logical_and_expression => [
        inclusive_or_expression |@flatten|,
        logical_and_expression "&&" inclusive_or_expression
    ],
    logical_or_expression => [
        logical_and_expression |@flatten|,
        logical_or_expression "||" logical_and_expression
    ],
    conditional_expression => [
        logical_or_expression |@flatten|,
        logical_or_expression "?" expression ":" conditional_expression
    ],
    assignment_expression => [
        conditional_expression |@flatten|,
        unary_expression assignment_operator assignment_expression
    ],
    assignment_operator => [
        "=","*=","/=","%=","+=","-=","<<=",">>=","&=","^=","|="
    ],
    expression => [
        assignment_expression |@flatten|,
        expression "," assignment_expression
    ],
    constant_expression => [
        conditional_expression
    ],
    declaration => [
        declaration_specifiers_i ";",
        declaration_specifiers_i init_declarator_list_i ";"
    ],
    declaration_specifiers_i => [
        declaration_specifiers
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
        init_declarator_list
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
        "typedef",
        "extern",
        "static",
        "auto",
        "register"
    ],
    type_specifier => [
        "void",
        "char",
        "short",
        "int",
        "long",
        "float",
        "double",
        "signed",
        "unsigned",
        struct_or_union_specifier,
        enum_specifier,
        TYPE_NAME
    ],
    struct_or_union_specifier => [
        struct_or_union IDENTIFIER "{" struct_declaration_list "}",
        struct_or_union "{" struct_declaration_list "}",
        struct_or_union IDENTIFIER
    ],
    struct_or_union => [
        "struct", "union"
    ],
    struct_declaration_list => [
        struct_declaration,
        struct_declaration_list struct_declaration
    ],
    struct_declaration => [
        specifier_qualifier_list struct_declarator_list ";"
    ],
    specifier_qualifier_list => [
        type_specifier specifier_qualifier_list,
        type_specifier,
        type_qualifier specifier_qualifier_list,
        type_qualifier
    ],
    struct_declarator_list => [
        struct_declarator,
        struct_declarator_list "," struct_declarator
    ],
    struct_declarator => [
        declarator,
        ":" constant_expression,
        declarator ":" constant_expression
    ],
    enum_specifier => [
        "enum" "{" enumerator_list "}",
        "enum" IDENTIFIER "{" enumerator_list "}",
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
        "const", "volatile"
    ],
    declarator => [
        pointer direct_declarator,
        direct_declarator |@flatten|
    ],
    direct_declarator => [
        IDENTIFIER |@flatten|,
        "(" declarator ")",
        direct_declarator "[" constant_expression "]",
        direct_declarator "[" "]",
        direct_declarator "(" parameter_type_list ")",
        direct_declarator "(" identifier_list ")",
        direct_declarator "(" ")"
    ],
    pointer => [
        "*",
        "*" type_qualifier_list,
        "*" pointer,
        "*" type_qualifier_list pointer
    ],
    type_qualifier_list => [
        type_qualifier |@flatten|,
        type_qualifier_list type_qualifier |@flatten|
    ],
    parameter_type_list => [
        parameter_list,
        parameter_list "," "..."
    ],
    parameter_list => [
        parameter_declaration,
        parameter_list "," parameter_declaration
    ],
    parameter_declaration => [
        declaration_specifiers_i declarator,
        declaration_specifiers_i abstract_declarator,
        declaration_specifiers_i
    ],
    identifier_list => [
        IDENTIFIER,
        identifier_list "," IDENTIFIER
    ],
    type_name => [
        specifier_qualifier_list,
        specifier_qualifier_list abstract_declarator
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
        "{" initializer_list "}",
        "{" initializer_list "," "}"
    ],
    initializer_list => [
        initializer,
        initializer_list "," initializer
    ],
    statement => [
        labeled_statement,
        compound_statement,
        expression_statement,
        selection_statement,
        iteration_statement,
        jump_statement
    ],
    labeled_statement => [
        IDENTIFIER ":" statement,
        "case" constant_expression ":" statement,
        "default" ":" statement
    ],
    compound_statement => [
        "{" "}",
        "{" statement_list "}",
        "{" declaration_list "}",
        "{" declaration_list statement_list "}"
    ],
    declaration_list => [
        declaration,
        declaration_list declaration
    ],
    statement_list => [
        statement,
        statement_list statement
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
    function_definition => [
        declaration_specifiers_i declarator declaration_list compound_statement,
        declaration_specifiers_i declarator compound_statement,
        declarator declaration_list compound_statement,
        declarator compound_statement
    ]
}
