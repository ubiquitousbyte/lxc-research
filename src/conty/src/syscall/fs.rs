use crate::syscall::{errno::errno, ffi};

use std::ffi::OsStr;

use libc as c;

/// mount attaches the filesystem managed by source (typically a device)
/// onto the (sub)tree of the file hierarchy defined by destination.
/// fstype specifies the type of the filesystem that source manages
/// See mount(2) for a description of flags
pub fn mount<S: AsRef<OsStr>>(
    source: S,
    destination: S,
    fstype: S,
    flags: u64,
) -> std::io::Result<i32> {
    let source = ffi::into_c_string(source)?;
    let destination = ffi::into_c_string(destination)?;
    let fstype = ffi::into_c_string(fstype)?;

    errno(unsafe {
        c::mount(
            source.as_ptr(),
            destination.as_ptr(),
            fstype.as_ptr(),
            flags,
            std::ptr::null(),
        )
    })
}
