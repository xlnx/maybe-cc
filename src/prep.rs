use std::fs::File;
use std::io::prelude::*;

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

impl std::fmt::Debug for PreprocessError {
	fn fmt(&self, f: &mut std::fmt::Formatter) -> std::fmt::Result {
		write!(f, "{}", self.as_string())
	}
}

impl Preprocessor {
	pub fn new() -> Self {
		Preprocessor {}
	}
	pub fn parse(&self, text: &str) -> Result<String, PreprocessError> {
		Ok(text.into())
		// Err(PreprocessError::from())
	}
}