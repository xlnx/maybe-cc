use std::io::prelude::*;
use std::process::{Command, Stdio};

use myrpg::*;

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

impl<'a> std::convert::From<PreprocessError> for LogItem<'a> {
    fn from(err: PreprocessError) -> Self {
        LogItem {
            level: Severity::Warning,
            location: Some(SourceFileLocation {
                name: "asd".into(),
                line: "ASDASDASD\n",
                from: (0, 0),
                to: (0, 9),
            }),
            message: "wtf warning message".into(),
        }
    }
}

impl Preprocessor {
    pub fn new() -> Self {
        Preprocessor {}
    }
    pub fn parse(&self, in_file: &str, logger: &mut Logger) -> Result<String, ()> {
        let child = Command::new("gcc")
            .args(&[
                "-E",
                "-std=c89",
                "-U__GNUC__",
                "-U__GNUC_MINOR__",
                "-U__GNUC_PATCHLEVEL__",
                in_file,
            ])
            .output()
            .unwrap();

        if child.status.success() {
            Ok(String::from_utf8(child.stdout.to_vec()).unwrap())
        } else {
            logger.log(&LogItem {
                level: Severity::Error,
                location: None,
                message: format!(
                    "preprocessing error:\n{}",
                    String::from_utf8(child.stderr.to_vec()).unwrap().trim()
                ),
            });
            Err(())
        }
    }
}
