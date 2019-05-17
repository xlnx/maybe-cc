use super::msg::*;
use myrpg::*;

use std::ffi::{CStr, CString};
use std::os::raw::c_char;

extern "C" {
    fn gen_llvm_ir(ast_json: *const c_char) -> *const c_char;
    fn free_llvm_ir(ir: *const c_char);
}

pub fn ir_gen<T>(ast: &Ast<T>) -> Result<String, ()> {
    let ir_c: *const c_char;

    unsafe {
        let ast_json_c = CString::new(ast.to_json().as_str()).unwrap();
        ir_c = gen_llvm_ir(ast_json_c.as_ptr());
    }

    let ir = String::from(if ir_c == std::ptr::null() {
        "".into()
    } else {
        unsafe { CStr::from_ptr(ir_c).to_str().unwrap() }
    });

    unsafe {
        free_llvm_ir(ir_c);
    }

    Ok(ir)
}
