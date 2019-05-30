use myrpg::*;

use std::ffi::CStr;
use std::os::raw::c_char;

const MSG_TYPE_ERROR: i32 = 0;
const MSG_TYPE_WARNING: i32 = 1;

#[repr(C)]
pub struct Msg {
    pub msg: *const c_char,
    pub msg_type: i32,
    pub has_loc: i32,
    pub pos: [usize; 4],
}

#[repr(C)]
pub struct MsgList {
    pub msgs: *const Msg,
    pub len: usize,
    pub cap: usize,
}

impl MsgList {
    pub fn log(&self, text: &String, logger: &mut Logger, source_map: &Vec<SourceFileMark>) -> bool {
        let mut error = false;
        for i in 0..self.len {
            unsafe {
                let msg_chunk = &*self.msgs.offset(i as isize);
                let msg = CStr::from_ptr(msg_chunk.msg).to_str().unwrap();
                logger.log(&LogItem {
                    level: match msg_chunk.msg_type {
                        MSG_TYPE_ERROR => {
                            error = true;
                            Severity::Error
                        }
                        MSG_TYPE_WARNING => Severity::Warning,
                        e @ _ => panic!(format!("unknown severity type: {}", e)),
                    },
                    location: if msg_chunk.has_loc != 0 {
                        let from = (msg_chunk.pos[0], msg_chunk.pos[1]);
                        let to = (msg_chunk.pos[2], msg_chunk.pos[3]);
                        let (provider, from, to) = source_map
                            .map_error(&from, &to)
                            .unwrap();
                        Some(SourceFileLocation {
                            provider, from, to,
                        })
                    } else {
                        None
                    },
                    message: msg.into(),
                })
            }
        }
        !error
    }
}
