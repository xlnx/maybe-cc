use myrpg::{ast::*, log::*};

use std::ffi::{CStr, CString};
use std::os::raw::c_char;

const MSG_TYPE_ERROR: i32 = 0;
const MSG_TYPE_WARNING: i32 = 1;

#[repr(C)]
struct Msg {
	msg: *const c_char,
	msg_type: i32,
	has_loc: i32,
	pos: [usize; 4],
}

#[repr(C)]
struct MsgList {
	msgs: *const Msg,
	len: usize,
	cap: usize,
}

extern "C" {
	fn gen_llvm_ir(ast_json: *const c_char, msg: *mut *const MsgList) -> *const c_char;
	fn free_llvm_ir(msg: *const MsgList, ir: *const c_char);
}

pub fn ir_gen<T>(ast: &Ast<T>, logger: &mut Logger) -> Result<String, ()> {

	let mut msg_c: *const MsgList = std::ptr::null();
	let ir_c: *const c_char;

	unsafe {
		let ast_json_c = CString::new(ast.to_json().as_str()).unwrap();
		ir_c = gen_llvm_ir(ast_json_c.as_ptr(), &mut msg_c as *mut _);
	}

	let msg = if msg_c == std::ptr::null() {
		&MsgList {
			msgs: std::ptr::null(),
			len: 0,
			cap: 0,
		}
	} else {
		unsafe { &*msg_c }
	};
	let mut error = false;
	for i in 0..msg.len {
		unsafe {
			let msg_chunk = &*msg.msgs.offset(i as isize);
			let msg = CStr::from_ptr(msg_chunk.msg).to_str().unwrap();
			logger.log(&Item {
				level: match msg_chunk.msg_type {
					MSG_TYPE_ERROR => {
						error = true;
						Severity::Error
					}
					MSG_TYPE_WARNING => Severity::Warning,
					e @ _ => panic!(format!("unknown severity type: {}", e)),
				},
				location: if msg_chunk.has_loc != 0 {
					Some(SourceFileLocation {
						name: "ASDASD".into(),
						line: "fuck you.",
						from: (msg_chunk.pos[0], msg_chunk.pos[1]),
						to: (msg_chunk.pos[2], msg_chunk.pos[3]),
					})
				} else {
					None
				},
				message: msg.into(),
			})
		}
	}

	if error {
		return Err(());
	}

	let ir = String::from(if ir_c == std::ptr::null() {
		panic!("internal-error: ir-gen output null")
	} else {
		unsafe { CStr::from_ptr(ir_c).to_str().unwrap() }
	});

	unsafe {
		free_llvm_ir(msg_c, ir_c);
	}

	Ok(ir)

}