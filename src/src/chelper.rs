use std::ptr;
use std::os::raw::{c_uchar};

unsafe extern "C" {
    pub unsafe fn malloc(size: usize) -> *mut c_uchar;
    pub unsafe fn memcpy(dest: *mut c_uchar, source: *mut c_uchar, size: usize) -> *mut c_uchar;
}

pub fn copy2cstring(mut input: Vec<u8>) -> *mut c_uchar {
    input.push(0);
    let cinput: *mut c_uchar = input.as_mut_ptr();
    unsafe {
        let ret = malloc(input.len());
        if ret.is_null(){
            return ptr::null_mut();
        }
        memcpy(ret, cinput, input.len());
        return ret;
    }
}
