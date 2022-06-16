use crate::oci::OCIError;

use std::path::PathBuf;
use std::str::FromStr;

use libc as c;

use serde::de::Error as DeError;
use serde::{Deserialize, Deserializer};

#[repr(i32)]
#[derive(Debug, Clone, Copy)]
pub enum NamespaceType {
    Pid = c::CLONE_NEWPID,
    Network = c::CLONE_NEWNET,
    Mount = c::CLONE_NEWNS,
    IPC = c::CLONE_NEWIPC,
    UTS = c::CLONE_NEWUTS,
    Cgroup = c::CLONE_NEWCGROUP,
    User = c::CLONE_NEWUSER,
}

impl FromStr for NamespaceType {
    type Err = OCIError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "pid" => Ok(Self::Pid),
            "net" | "network" => Ok(Self::Network),
            "mount" => Ok(Self::Mount),
            "ipc" => Ok(Self::IPC),
            "uts" => Ok(Self::UTS),
            "cgroup" => Ok(Self::Cgroup),
            "user" => Ok(Self::User),
            _ => Err(OCIError::InvalidConfig("namespace type")),
        }
    }
}

impl<'de> Deserialize<'de> for NamespaceType {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let variants = &[
            "pid", "net", "network", "mount", "ipc", "uts", "cgroup", "user",
        ];
        Self::from_str(&s).map_err(|_| DeError::unknown_variant(&s, variants))
    }
}

#[derive(Debug, Clone, Deserialize)]
pub struct Namespace {
    #[serde(rename = "type")]
    pub typ: NamespaceType,
    pub path: Option<PathBuf>,
}
