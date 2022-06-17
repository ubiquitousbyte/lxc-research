use std::collections::HashMap;
use std::path::PathBuf;

use serde::Deserialize;

mod container_state;
pub use container_state::*;

mod process;
pub use process::*;

mod capability;
pub use capability::*;

mod hook;
pub use hook::*;

mod device;
pub use device::*;

mod user;
pub use user::*;

mod namespace;
pub use namespace::*;

mod resource;
pub use resource::*;

mod seccomp;
pub use seccomp::*;

#[derive(Debug, Clone)]
pub enum OCIError {
    InvalidConfig(&'static str),
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Spec {
    pub oci_version: String,
    pub process: Option<Process>,
    pub root: Option<RootFs>,
    pub hostname: Option<String>,
    #[serde(default)]
    pub mounts: Vec<Mount>,
    pub hooks: Option<Hooks>,
    #[serde(default)]
    pub annotations: HashMap<String, String>,
    pub linux: Option<Linux>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Linux {
    #[serde(default)]
    pub uid_mappings: Vec<IdMapping>,
    #[serde(default)]
    pub gid_mappings: Vec<IdMapping>,
    #[serde(default)]
    pub sysctl: HashMap<String, String>,
    pub resources: Option<Resources>,
    pub cgroups_path: Option<String>,
    #[serde(default)]
    pub namespaces: Vec<Namespace>,
    #[serde(default)]
    pub devices: Vec<Device>,
    pub seccomp: Option<Seccomp>,
    pub rootfs_propagation: Option<String>,
    #[serde(default)]
    pub masked_paths: Vec<PathBuf>,
    #[serde(default)]
    pub readonly_paths: Vec<PathBuf>,
    pub mount_label: Option<String>,
    pub personality: Option<Personality>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct Personality {
    pub domain: String,
    #[serde(default)]
    pub flags: Vec<String>,
}
