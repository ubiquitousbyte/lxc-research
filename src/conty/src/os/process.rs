use std::ffi::{CString, NulError, OsStr, OsString};
use std::path::PathBuf;

use libc as c;

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum CommandError {
    NulByte(NulError),
    SpawnError(std::io::ErrorKind),
}

impl std::fmt::Display for CommandError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::NulByte(n) => write!(f, "command error: invalid nul byte: {}", n),
            Self::SpawnError(kind) => write!(f, "command error: spawn: {}", kind),
        }
    }
}

impl std::error::Error for CommandError {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            Self::NulByte(nul_err) => Some(nul_err),
            _ => None,
        }
    }
}

type CommandResult<T> = Result<T, CommandError>;

/// Contains a set of parameters to be passed to a process
/// Parameters can be passed in any arbitrary form, e.g as
/// execution parameters, environment variables, etc.
#[derive(Clone, PartialEq, Eq, Debug)]
struct Params {
    // Invariants:
    // 1. Every parameter must not contain a nul byte in the middle
    // 2. Every parameter must contain a nul byte at the end
    // 3. When materialized, a null ptr must be appended at the end
    inner: Vec<CString>,
}

impl Default for Params {
    fn default() -> Self {
        Self { inner: Vec::new() }
    }
}

impl Params {
    /// Constructs a new set of parameters
    fn new() -> Self {
        Default::default()
    }

    /// Appends the argument to the parameters
    fn arg<S>(&mut self, arg: S) -> &mut Self
    where
        S: Into<CString>,
    {
        self.inner.push(arg.into());
        self
    }

    /// Appends the argument list to the parameters
    fn args<I, S>(&mut self, args: I) -> &mut Self
    where
        I: IntoIterator<Item = S>,
        S: Into<CString>,
    {
        self.inner.extend(args.into_iter().map(|arg| arg.into()));
        self
    }

    /// Converts the parameter list to a vector of pointers
    /// # Safety
    /// There is no way to guarantee that the caller won't drop the Params object
    /// but keep referencing the pointers. Therefore, this function is unsafe.
    unsafe fn execve(&self) -> Vec<*const c::c_char> {
        let mut ptrs: Vec<*const c::c_char> = self.inner.iter().map(|s| s.as_ptr()).collect();
        ptrs.push(std::ptr::null());
        ptrs
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
struct OsParams {
    inner: Params,
    err: Option<CommandError>,
}

impl Default for OsParams {
    fn default() -> Self {
        Self {
            inner: Params::new(),
            err: None,
        }
    }
}

impl OsParams {
    fn new() -> Self {
        Default::default()
    }

    fn command_str<S>(&mut self, arg: S)
    where
        S: AsRef<OsStr>,
    {
        match command_str(&arg) {
            Ok(c_str) => {
                self.inner.arg(c_str);
            }
            Err(err) => {
                if let None = self.err {
                    self.err = Some(err);
                }
            }
        }
    }

    /// Appends the argument to the parameters
    fn arg<S>(&mut self, arg: S) -> &mut Self
    where
        S: AsRef<OsStr>,
    {
        self.command_str(arg);
        self
    }

    /// Appends the argument list to the parameters
    fn args<I, S>(&mut self, args: I) -> &mut Self
    where
        I: IntoIterator<Item = S>,
        S: AsRef<OsStr>,
    {
        args.into_iter().for_each(|arg| self.command_str(arg));
        self
    }

    /// Converts the parameter list to a vector of pointers
    /// # Safety
    /// There is no way to guarantee that the caller won't drop the Params object
    /// but keep referencing the pointers. Therefore, this function is unsafe.
    unsafe fn execve(&self) -> CommandResult<Vec<*const i8>> {
        match &self.err {
            None => Ok(self.inner.execve()),
            Some(err) => Err(err.clone()),
        }
    }
}

// Converts an operating system string into a CString
fn command_str<S: AsRef<OsStr>>(os_str: S) -> CommandResult<CString> {
    use std::os::unix::prelude::OsStrExt;
    CString::new(os_str.as_ref().as_bytes()).map_err(|err| CommandError::NulByte(err))
}

#[derive(Clone, PartialEq, Eq, Debug)]
pub struct Command {
    path: PathBuf,
    args: OsParams,
    env: OsParams,
}

impl Command {
    /// Creates a new command referencing the program stored in path
    pub fn new<P: Into<PathBuf>>(path: P) -> Self {
        Self {
            path: path.into(),
            args: OsParams::new(),
            env: OsParams::new(),
        }
    }

    /// Appends the argument to the set of arguments to pass into the command
    pub fn arg<S>(&mut self, arg: S) -> &mut Self
    where
        S: AsRef<OsStr>,
    {
        self.args.arg(arg);
        self
    }

    /// Appends the argument list to the set of arguments to pass into the command
    pub fn args<I, S>(&mut self, args: I) -> &mut Self
    where
        I: IntoIterator<Item = S>,
        S: AsRef<OsStr>,
    {
        self.args.args(args);
        self
    }

    /// Adds the environment variable to the command
    pub fn env<S>(&mut self, env: S) -> &mut Self
    where
        S: AsRef<OsStr>,
    {
        self.env.arg(env);
        self
    }

    /// Adds the environment variables to the command
    pub fn envs<I, S>(&mut self, envs: I) -> &mut Self
    where
        I: IntoIterator<Item = S>,
        S: AsRef<OsStr>,
    {
        self.env.args(envs);
        self
    }

    /// Spawns the command, returning a handle to the child process
    pub fn spawn(&self) -> CommandResult<i32> {
        let path = command_str(&self.path)?;

        // Safety: The pointers are owned by self, so they are valid in this scope
        let argv = unsafe { self.args.execve()? };
        let environ = unsafe { self.env.execve()? };

        // Fork a copy of the current process
        let pid = unsafe { c::fork() };
        if pid == 0 {
            // Overlay the binary onto the virtual memory space of the child
            let _ = unsafe { c::execve(path.as_ptr(), argv.as_ptr(), environ.as_ptr()) };
            // The instruction pointer has no business being here, i.e
            // execve has failed and we need to exit the child with an error
            unsafe { c::exit(c::EXIT_FAILURE) };
        }

        if pid < 0 {
            // We couldn't fork a new child.
            // Read from __errno_location and wrap the error
            let error = std::io::Error::last_os_error();
            Err(CommandError::SpawnError(error.kind()))
        } else {
            Ok(pid)
        }
    }
}

#[cfg(test)]
mod tests {

    use std::os::unix::prelude::OsStrExt;

    use super::*;

    fn test() {
        let a = OsString::from("asdasd");
        let bts = a.as_bytes();
        let cstr = CString::new(bts);
    }
}
