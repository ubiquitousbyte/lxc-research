pub type Result<T> = std::result::Result<T, std::io::Error>;

mod clone;
pub use clone::*;

mod ffi;
