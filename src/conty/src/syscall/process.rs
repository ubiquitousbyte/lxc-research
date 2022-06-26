use crate::syscall::errno::errno;

use libc as c;

/// Pid is a wrapper around a c pid_t
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Pid(c::pid_t);

impl Pid {
    /// current returns the process identifier of the current process
    pub fn current() -> Self {
        Self {
            0: unsafe { c::getpid() },
        }
    }

    /// parent returns the process identifier of the parent process
    pub fn parent() -> Self {
        Self {
            0: unsafe { c::getppid() },
        }
    }

    /// as_raw_pid returns the raw process identifier of self
    pub fn as_raw_pid(self) -> c::pid_t {
        self.0
    }
}

/// Wraps a c::pid_t into a Pid
impl From<c::pid_t> for Pid {
    fn from(pid: c::pid_t) -> Self {
        Self { 0: pid }
    }
}

impl std::fmt::Display for Pid {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.0)
    }
}

type CloneFn<'a> = Box<dyn FnMut() -> isize + 'a>;

/// Creates a new child process that commences execution by calling the function
/// pointed to the argument cb.
/// When the function returns, the child process terminates.
/// The integer returned by cb is the exit status for the child process.
///
/// The stack argument specifies the location of the stack used by the child process.
///
/// Using the flags argument, the caller can control various properties of the child's
/// excution environment. For example, the parent can place the child process
/// in different namespaces, decice whether or the child should see its parent's
/// file descriptor table and so on.
pub fn clone(
    mut cb: CloneFn,
    stack: &mut [u8],
    flags: i32,
    signal: Option<c::c_int>,
) -> std::io::Result<Pid> {
    // There are a lot of things happening here, so let me explain.
    //
    // First, cb points to a closure that can capture mutable references to memory
    // from its outside scope. Its signature is not int (*f)(*void) but int (*f)(*self)
    // where self is an anonymous struct used to capture the references.
    // This means that we cannot pass CloneFn directly to the clone system call, which expects
    // a function pointer of the form int (*f)(*void) where void is an arbitrary argument.
    // We can, however, pass the pointer to the closure as the argument to an int (*f)(*void),
    // unwrap it, call it and return its result, thereby making it compatible with the C ABI.
    // This is what callback does.
    //
    // Secondly, glibc requires that the parent process provide a stack for the child.
    // glibc will transparently push the function pointer onto the stack and call
    // the kernel entry function which will pop our function from the stack and call it.
    // See https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/aarch64/clone.S
    // In addition, glibc uses the stack to pop the return value of our function pointer
    // and exit the child with it.
    // Note that on all contemporary architectures that run Linux, the stack grows downwards.
    // However, the clone system call expects the highest address of the stack.
    // So we need to pass in base_stack_address + stack_length to clone

    extern "C" fn callback(f: *mut c::c_void) -> c::c_int {
        let cb = unsafe { &mut *(f as *mut Box<dyn FnMut() -> isize>) };
        (*cb)() as c::c_int
    }

    let pid = unsafe {
        // Stack grows downwards, so add the stack length
        let ptr = stack.as_mut_ptr().add(stack.len());
        // Will we break the ABI if ptr is unaligned because of stack.len() ?
        // https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/aarch64/clone.S#L51
        // says we won't, but we'll align regardless
        let ptr_aligned = ptr.sub(ptr as usize % 16);
        // Finally, create the child
        c::clone(
            callback,
            ptr_aligned as *mut c::c_void,
            flags | signal.unwrap_or(0),
            // We pass in the closure as the *void parameter to callback
            &mut cb as *mut _ as *mut c::c_void,
        )
    };

    let pid = errno(pid)?;
    Ok(pid.into())
}

pub fn fork() -> std::io::Result<Pid> {
    let pid = errno(unsafe { c::fork() })?;
    Ok(pid.into())
}