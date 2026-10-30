use std::ptr;
use std::os::raw::{c_char, c_uchar};

use crate::chelper::copy2cstring;
use crate::clipsserializer::ClipsSerializer;

//#[unsafe(no_mangle)]
//pub extern "C" fn free_ClipsSerializer(config: *mut ClipsSerializer){
//    if !config.is_null(){
//        unsafe { let _ = Box::from_raw(config); }
//    }
//}

#[unsafe(no_mangle)]
pub extern "C" fn ClipsSerializer_new() -> *mut ClipsSerializer {
    let x = match ClipsSerializer::new(){
        Ok(x) => x,
        Err(_) => {
            return ptr::null_mut();
        }
    };
    let mybox = Box::new(x);
    let config = Box::into_raw(mybox);
    config
}

#[unsafe(no_mangle)]
pub extern "C" fn ClipsSerializer_add(
        subject: *const c_char, subject_type: u8,
        predicate: *const c_char,
        object: *const c_char, object_suffix: *const c_char,
        object_type: u8,
        _graph_id: *const c_char, _graph_type: u8,
        serializer: *mut ClipsSerializer,
        ) -> i8
{
    if serializer.is_null() {
        return -1;
    }
    unsafe{
        (*serializer).add_triple(subject, subject_type, predicate, object, object_suffix, object_type)
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn ClipsSerializer_finish(config: *mut ClipsSerializer) -> *mut c_uchar
{
    if !config.is_null(){
        unsafe {
            let mut cfg = Box::from_raw(config);
            match cfg.finish() {
                Ok(mut x) => {
                    x.extend_from_slice(b"\0");
                    copy2cstring(x) //allocated with C's malloc
                },
                Err(e) => {
                    eprintln!("ClipsSerializer_finish failed: {:?}", e);
                    ptr::null_mut()
                },
            }
        }
    } else {
        ptr::null_mut()
    }
}
