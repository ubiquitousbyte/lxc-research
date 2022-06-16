use crate::oci::OCIError;

use std::collections::HashMap;
use std::path::PathBuf;
use std::str::FromStr;

use serde::de::Error as DeError;
use serde::{Deserialize, Deserializer};

/// ContainerStatus represents the runtime status of a container
#[derive(Debug, Clone, Copy)]
pub enum ContainerStatus {
    Creating,
    Created,
    Running,
    Stopped,
}

impl FromStr for ContainerStatus {
    type Err = OCIError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "creating" => Ok(Self::Creating),
            "created" => Ok(Self::Created),
            "running" => Ok(Self::Running),
            "stopped" => Ok(Self::Stopped),
            _ => Err(OCIError::InvalidConfig("container status")),
        }
    }
}

impl<'de> Deserialize<'de> for ContainerStatus {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let variants = &["creating", "created", "running", "stopped"];
        ContainerStatus::from_str(&s).map_err(|_| DeError::unknown_variant(&s, variants))
    }
}

/// ContainerState represents the runtime state of a container
#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct ContainerState {
    // The version of the OCI specification with which the state complies
    pub oci_version: String,
    // Unique container identifier on this host system
    pub id: String,
    // Runtime status of the container
    pub status: ContainerStatus,
    // The process identifier of the container
    // This field varies depending on the namespace
    // That is, if the container state is sent to an external process
    // the pid must be set based on the namespace that the external process
    // resides in
    pub pid: i32,
    // The absolute path to the container's bundle directory, which holds
    // the container's configuration and root filesystem
    pub bundle: PathBuf,
    // Optional list of annotations
    #[serde(default)]
    pub annotations: HashMap<String, String>,
}
