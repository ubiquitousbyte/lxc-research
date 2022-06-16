use crate::oci::{Capabilities, User};

use std::path::PathBuf;

use serde::Deserialize;

/// Process is a platform-agnostic representation of a sandboxed application
/// in execution
#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Process {
    // Specifies whether a terminal is attached to the process
    // Should default to false
    // As an example, if set to true on Linux, a pseudoterminal pair
    // is allocated for the process and the pseudoterminal pty is duplicated
    // on the process' standard streams
    #[serde(default)]
    pub terminal: bool,
    // Specifies the console size in characters of the terminal
    // Must be ignored if terminal is false
    pub console_size: Option<Rectangle>,
    // The working directory that will be set for the executable
    // The value must be an absolute path
    pub cwd: PathBuf,
    // Proc environment, must have same semantics as environ
    // This argument is optional
    #[serde(default)]
    pub env: Vec<String>,
    // Arguments to be passed in to the executable
    // Must have same semantics as argv
    #[serde(default)]
    pub args: Vec<String>,
    // Resource limits to apply to the process
    // If the vector has elements that refer to the same resource limit,
    // an error must be generate
    // #[serde(default)]
    // pub rlimits: Vec<ResourceLimit>,
    // The capabilities of the process
    // OCI states that the runtime should not fail if the container configuration
    // requests capabilites that cannot be granted
    pub capabilities: Option<Capabilities>,

    // The user for the process
    pub user: User,

    #[serde(default)]
    pub no_new_privileges: bool,

    // Adjusts the out of memory killer score in /proc/pid/oom_score_adj
    pub oom_score_adj: Option<i32>,

    pub apparmor_profile: Option<String>,
    pub selinux_label: Option<String>,
}

#[derive(Debug, Clone, Copy, Deserialize)]
pub struct Rectangle {
    pub width: u32,
    pub height: u32,
}
