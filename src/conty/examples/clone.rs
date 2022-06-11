use std::ffi::OsString;
use std::path::{Path, PathBuf};

use conty::syscall;
use libc as c;

fn print_namespaces() -> std::io::Result<()> {
    use std::os::unix::prelude::MetadataExt;

    let path = format!("/proc/{}/ns", unsafe { c::getpid() });
    let path = Path::new(&path);

    println!("{0: <33}  {1}", "Namespace", "Id");

    for entry in path.read_dir()? {
        let entry = entry?;
        let entry_path = entry.path();
        let metadata = std::fs::metadata(&entry_path)?;

        let namespace = Namespace {
            ns_type: entry.file_name(),
            path: entry_path,
            inode: metadata.ino(),
            dev: metadata.dev(),
        };

        println!("{}", namespace);
    }

    Ok(())
}

fn mount_proc() -> std::io::Result<()> {
    use std::ffi::CString;

    let device = CString::new("proc").unwrap();
    let typ = CString::new("proc").unwrap();
    let target = CString::new("/proc").unwrap();

    let res = unsafe {
        c::mount(
            device.as_ptr(),
            target.as_ptr(),
            typ.as_ptr(),
            0,
            std::ptr::null_mut(),
        )
    };

    match res {
        -1 => Err(std::io::Error::last_os_error()),
        _ => Ok(()),
    }
}

#[derive(Debug, Clone)]
pub struct Namespace {
    pub ns_type: OsString,
    pub path: PathBuf,
    pub inode: u64,
    pub dev: u64,
}

impl std::fmt::Display for Namespace {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{0: <32} [{1}]", self.path.display(), self.inode)
    }
}

fn main() {
    let child_func = || {
        if let Err(mount_result) = mount_proc() {
            println!("Could not mount procfs in child: {}", mount_result);
            return 1;
        }

        if let Err(print_result) = print_namespaces() {
            println!(
                "Could not print namespace environment of child: {}",
                print_result
            );
            return 1;
        }

        return 0;
    };

    const STACK_SIZE: usize = 4 * 1024 * 1024;
    let ref mut stack: [u8; STACK_SIZE] = [0; STACK_SIZE];
    let flags = c::CLONE_NEWUSER
        | c::CLONE_NEWUTS
        | c::CLONE_NEWNET
        | c::CLONE_NEWIPC
        | c::CLONE_NEWNS
        | c::CLONE_NEWCGROUP
        | c::CLONE_NEWPID;

    let result = syscall::clone(Box::new(child_func), stack, flags);
    if let Err(clone_result) = result {
        println!("Could not create execution environment: {}", clone_result);
    }
}
