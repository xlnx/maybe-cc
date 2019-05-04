use std::io::prelude::*;

use myrpg::log::{Item, Logger, Severity, SourceFileLocation};

pub struct Preprocessor {}

pub struct PreprocessError {}

impl PreprocessError {
    pub fn as_string(&self) -> String {
        String::from("asd")
    }
    pub fn from() -> Self {
        PreprocessError {}
    }
}

impl<'a> std::convert::From<PreprocessError> for Item<'a> {
    fn from(err: PreprocessError) -> Self {
        Item {
            level: Severity::Warning,
            location: SourceFileLocation {
                name: "asd".into(),
                line: "ASDASDASD\n",
                from: (0, 0),
                to: (0, 9),
            },
            message: "wtf warning message".into(),
        }
    }
}

impl Preprocessor {
    pub fn new() -> Self {
        Preprocessor {}
    }
    pub fn parse(&self, text: &str, logger: &mut Logger) -> Result<String, ()> {
        Ok(text.into())
        // Err(PreprocessError::from())
    }
}
