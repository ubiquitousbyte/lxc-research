use crate::syscall::errno::errno;

use std::ffi::CStr;

use libc as c;

/// mount attaches the filesystem managed by source (typically a device)
/// onto the (sub)tree of the file hierarchy defined by destination.
/// fstype specifies the type of the filesystem that source manages
/// See mount(2) for a description of flags
pub fn mount<S: AsRef<CStr>>(
    source: S,
    destination: S,
    fstype: S,
    flags: u64,
) -> std::io::Result<i32> {
    let source = source.as_ref();
    let destination = destination.as_ref();
    let fstype = fstype.as_ref();

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
