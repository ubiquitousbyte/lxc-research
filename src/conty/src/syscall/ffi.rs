use std::ffi::{CString, OsStr};

use std::io::{Error, ErrorKind, Result};

pub fn into_c_string<S: AsRef<OsStr>>(s: S) -> Result<CString> {
    use std::os::unix::ffi::OsStrExt;
    let s = s.as_ref();
    CString::new(s.as_bytes()).map_err(|e| Error::new(ErrorKind::InvalidInput, e))
}
