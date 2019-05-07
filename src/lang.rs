#[allow(unused_imports)]
use myrpg::{ast::*, symbol::*, *};
use regex::Regex;

lazy_static! {
    static ref SOURCE_MAP: Regex = Regex::new(r"#\s+(\S+)\s+(\S+)\s*(.*)").unwrap();
}

lang! {

    Name = C
    ValueType = i32

    ;;

    IDENTIFIER => r"[a-zA-Z_]\w*\b",
        // => |tok| -> _ {
        //     tok.symbol = Symbol::from("Number")
        // },
    Number => r"[0-9]+\b",
    SOURCE_MAP => r"\n(#[^\n]*)\n" => |tok, ctrl| {
        let caps = SOURCE_MAP.captures(tok.val).unwrap();
        let line = caps.get(1).unwrap().as_str();
        let file = caps.get(2).unwrap().as_str();
        let flags = caps.get(3).unwrap().as_str().trim();
        let flags: Vec<_> = if flags.len() > 0 {
            flags.split(" ").collect()
        } else {
            vec![]
        };

        ctrl.set_source_file(file.trim_matches('"'))
            .set_location((0, line.parse::<usize>().unwrap()))
            .discard();
    },
    TYPE_NAME => r"$^$^",
    CONSTANT => r#"
        0[xX][0-9A-Fa-f]+(?:u|U|l|L)*? |
        [0-9]+(?:u|U|l|L)*? |
        [a-zA-Z_]?'(?:\\.|[^\\'])+' |
        [0-9]+[Ee][\+-]?[0-9]+(?:f|F|l|L)? |
        [0-9]*"."[0-9]+(?:[Ee][\+-]?[0-9]+)?(?:f|F|l|L)?
    "#,
    STRING_LITERAL => r#"[a-zA-Z_]?"(\\.|[^\\"])*"#

    ;;

    S => [
        translation_unit
    ],

    primary_expression => [
        IDENTIFIER,
        CONSTANT,
        STRING_LITERAL,
        "(" expression ")"
    ],
    postfix_expression => [
        primary_expression,
        postfix_expression "[" expression "]",
        postfix_expression "(" ")",
        postfix_expression "(" argument_expression_list ")",
        postfix_expression "." IDENTIFIER,
        postfix_expression "->" IDENTIFIER,
        postfix_expression "++",
        postfix_expression "--"
    ],
    argument_expression_list => [
        assignment_expression,
        argument_expression_list "," assignment_expression
    ],
    unary_expression => [
        postfix_expression,
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
        unary_expression,
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
        declaration_specifiers ";",
        declaration_specifiers init_declarator_list ";"
    ],
    declaration_specifiers => [
        storage_class_specifier,
        storage_class_specifier declaration_specifiers,
        type_specifier,
        type_specifier declaration_specifiers,
        type_qualifier,
        type_qualifier declaration_specifiers
    ],
    init_declarator_list => [
        init_declarator,
        init_declarator_list "," init_declarator
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
        direct_declarator
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
        "*" type_qualifier_list,
        "*" pointer,
        "*" type_qualifier_list pointer
    ],
    type_qualifier_list => [
        type_qualifier,
        type_qualifier_list type_qualifier
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
        declaration_specifiers declarator,
        declaration_specifiers abstract_declarator,
        declaration_specifiers
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
        external_declaration,
        translation_unit external_declaration
    ],
    external_declaration => [
        function_definition,
        declaration
    ],
    function_definition => [
        declaration_specifiers declarator declaration_list compound_statement,
        declaration_specifiers declarator compound_statement,
        declarator declaration_list compound_statement,
        declarator compound_statement
    ]

}
