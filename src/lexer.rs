use myrpg::*;
use regex::Regex;

use std::collections::HashMap;

macro_rules! map(
    { $($key:expr),+ } => {
        {
            let mut m = ::std::collections::HashMap::new();
            $(
                m.insert($key, into_symbol_lit($key));
            )+
            m
        }
     };
);

macro_rules! yank(
	{$value:expr, $idx:expr, $default:expr, $($key:expr),+} => {
		match &$value[..$value.len().min($idx)] {
			$(
				$key => Some((into_symbol_lit($key), $idx)),
			)+
			_ => $default
		}
	}
);

pub struct CLexer {
    reserved: HashMap<&'static str, Symbol>,
    delegate: DefaultRegexLexer,
}

impl CLexer {
    fn sym_get(&self, input: &str) -> Option<(Symbol, usize)> {
        yank! {
            input, 3,
            yank!{
                input, 2,
                yank! {
                    input, 1,
                    None,
                    "(", ")", "[", "]", ".", ",", "&", "*", "+",
                    "-", "~", "!", "/", "%", "<", ">", "^", "|",
                    "?", ":", ";", "=", "{", "}"
                },
                "->", "++", "--", "<<", ">>", "<=", ">=",
                "==", "!=", "&&", "||", "*=", "/=", "%=",
                "+=", "-=", "&=", "^=", "|="
            },
            ">>=", "<<="
        }
    }
    fn trie_get(&self, input: &str) -> (Symbol, usize) {
        let end = input
            .find(|x: char| {
                if let 'a'..='z' | 'A'..='Z' | '_' | '0'..='9' = x {
                    false
                } else {
                    true
                }
            })
            .unwrap_or(input.len());
        if let Some(symbol) = self.reserved.get(&input[..end]) {
            (*symbol, end)
        } else {
            (into_symbol("IDENTIFIER"), end)
        }
    }
}

fn into_symbol(tok: &str) -> Symbol {
    Symbol::from(tok).as_terminal()
}

fn into_symbol_lit(tok: &str) -> Symbol {
    Symbol::from(format!(r#""{}""#, tok).as_str()).as_terminal()
}

impl LexerProvider for CLexer {
    fn new(rules: Vec<(Symbol, Regex)>) -> Self {
        let rules: Vec<_> = rules
            .into_iter()
            .filter(|x| {
                if let "FLOATING_POINT" | "INTEGER" | "CHAR" | "STRING_LITERAL" = x.0.as_str() {
                    true
                } else {
                    false
                }
            })
            .collect();
        CLexer {
            reserved: map! {
                "typedef",
                "extern",
                "static",
                "auto",
                "register",
                "void",
                "char",
                "short",
                "int",
                "long",
                "float",
                "sizeof",
                "double",
                "signed",
                "unsigned",
                "struct",
                "union",
                "enum",
                "const",
                "volatile",
                "case",
                "default",
                "if",
                "else",
                "switch",
                "while",
                "do",
                "for",
                "goto",
                "continue",
                "break",
                "return"
            },
            delegate: DefaultRegexLexer::new(rules),
        }
    }
    fn emit(&self, input: &str) -> Option<(Symbol, usize)> {
        let mut chars = input.chars();
        match chars.next().unwrap() {
            'a'..='z' | 'A'..='Z' | '_' => Some(self.trie_get(input)),
            '#' => Some((
                into_symbol("SOURCE_MAP"),
                input.find('\n').unwrap_or(input.len()),
            )),
            '0'..='9' | '\'' | '"' => self.delegate.emit(input),
            '.' => {
                let a = chars.next();
                let b = chars.next();
                match (a, b) {
                    (Some('0'..='9'), _) => self.delegate.emit(input),
                    (Some('.'), Some('.')) => Some((into_symbol_lit("..."), 3)),
                    _ => Some((into_symbol_lit("."), 1)),
                    (None, _) => None,
                }
            }
            _ => self.sym_get(input),
        }
    }
}
