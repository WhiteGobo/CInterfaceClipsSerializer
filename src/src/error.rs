use std::io::Error;
//use std::str::Utf8Error;
//use std::ffi::NulError;
//use oxrdf::IriParseError;
use std::fmt;

pub enum CICSError {
    QQ(Error),
}

impl fmt::Display for CICSError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            //CICSError::QQ => e.fmt(f),
            CICSError::QQ(e) => e.fmt(f),
        }
    }
}

impl CICSError {
    pub fn other(value: &str) -> Self {
        CICSError::QQ(Error::other(value))
    }
}
