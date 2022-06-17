use crate::oci::IdMapping;

use std::path::PathBuf;

use serde::Deserialize;

#[derive(Debug, Clone, Deserialize)]
pub struct RootFs {
    // The absolute path to the container's root filesystem
    pub path: PathBuf,
    #[serde(default)]
    pub readonly: bool,
}

/// Additional mount points for a container beyond the root filesystem
#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Mount {
    // The destination inside the container
    pub destination: String,
    // The device that owns the file system
    pub source: Option<String>,
    #[serde(rename = "type")]
    // The type of file system
    pub typ: Option<String>,
    pub options: Option<String>,
    // The mapping that converts user ids from the source file system
    // to the destination mount point
    #[serde(default)]
    pub uid_mappings: Vec<IdMapping>,
    // The mapping that converts group ids from the source file system
    // to the destination mount point
    #[serde(default)]
    pub gid_mappings: Vec<IdMapping>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Device {
    pub path: PathBuf,
    #[serde(rename = "type")]
    pub typ: String,
    pub major: i64,
    pub minor: i64,
    pub file_mode: Option<u32>,
    pub uid: Option<u32>,
    pub gid: Option<u32>,
}
