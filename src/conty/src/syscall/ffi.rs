use std::ffi::{CString, OsStr};

use std::io::{Error, ErrorKind, Result};

/// into_c_string converts an operating system string into a C-style string
/// This function copies the bytes from s into a newly allocated buffer and appends the nul byte.
/// It also validates that there aren't any nul bytes before the end of the string
/// If the latter does not hold, an error is returned
pub fn into_c_string<S: AsRef<OsStr>>(s: S) -> Result<CString> {
    use std::os::unix::ffi::OsStrExt;
    let s = s.as_ref();
    CString::new(s.as_bytes()).map_err(|e| Error::new(ErrorKind::InvalidInput, e))
}
