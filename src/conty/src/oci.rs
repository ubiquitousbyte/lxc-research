use std::str::FromStr;
use std::{collections::HashMap, path::PathBuf};

use libc as c;
use serde::de::{Error as DeError, Unexpected};
use serde::{Deserialize, Deserializer};

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Config {
    oci_version: String,
    hooks: Option<Hooks>,
    #[serde(default)]
    annotations: HashMap<String, String>,
    hostname: Option<String>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct Hook {
    path: PathBuf,
    args: Option<Vec<String>>,
    env: Option<Vec<String>>,
    timeout: Option<u32>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Hooks {
    #[serde(default)]
    create_runtime: Vec<Hook>,
    #[serde(default)]
    create_container: Vec<Hook>,
    #[serde(default)]
    start_container: Vec<Hook>,
    #[serde(default)]
    poststart: Vec<Hook>,
    #[serde(default)]
    poststop: Vec<Hook>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct IdMapping {
    #[serde(rename(deserialize = "containerID"))]
    container_id: u32,
    #[serde(rename(deserialize = "hostID"))]
    host_id: u32,
    size: u32,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Mount {
    destination: PathBuf,
    source: Option<PathBuf>,
    options: Option<Vec<String>>,
    #[serde(rename(deserialize = "type"))]
    typ: Option<String>,
    #[serde(default)]
    uid_mappings: Vec<IdMapping>,
    #[serde(default)]
    gid_mappings: Vec<IdMapping>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct RootFs {
    path: PathBuf,
    #[serde(default)]
    readonly: bool,
}

#[derive(Clone, Debug, Deserialize)]
pub struct ConsoleSize {
    height: u64,
    width: u64,
}

#[derive(Clone, Debug, Deserialize)]
pub struct User {
    uid: u32,
    gid: u32,
    umask: u32,
    additional_gids: Option<Vec<u32>>,
    username: Option<String>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct Capabilities {
    #[serde(default)]
    bounding: Vec<String>,
    #[serde(default)]
    permitted: Vec<String>,
    #[serde(default)]
    effective: Vec<String>,
    #[serde(default)]
    inheritable: Vec<String>,
    #[serde(default)]
    ambient: Vec<String>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct ResourceLimit {
    soft: u64,
    hard: u64,
    #[serde(rename(deserialize = "type"))]
    typ: String,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Process {
    cwd: String,
    args: Option<Vec<String>>,
    env: Option<Vec<String>>,
    command_line: Option<String>,
    console_size: Option<ConsoleSize>,
    #[serde(default)]
    terminal: bool,
    user: Option<User>,
    capabilities: Option<Capabilities>,
    apparmor_profile: Option<String>,
    oom_score_adj: Option<i32>,
    selinux_label: Option<String>,
    #[serde(default)]
    no_new_privileges: bool,
    #[serde(default)]
    rlimits: Vec<ResourceLimit>,
}

#[derive(Clone, Debug)]
pub enum DeviceType {
    Char,
    Block,
    U,
    Pipe,
}

impl<'de> Deserialize<'de> for DeviceType {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        match s.as_str() {
            "c" => Ok(DeviceType::Char),
            "b" => Ok(DeviceType::Block),
            "u" => Ok(DeviceType::U),
            "p" => Ok(DeviceType::Pipe),
            other => Err(DeError::invalid_value(Unexpected::Str(other), &"^[cbup]$")),
        }
    }
}

#[derive(Clone, Debug, Deserialize)]
pub struct Device {
    #[serde(rename(deserialize = "type"))]
    typ: DeviceType,
    path: PathBuf,
    file_mode: Option<u32>,
    major: Option<i64>,
    minor: Option<i64>,
    uid: Option<u32>,
    gid: Option<u32>,
}

#[derive(Clone, Debug)]
pub enum NamespaceType {
    Cgroup,
    IPC,
    Network,
    Mount,
    PID,
    Time,
    User,
    UTS,
}

#[derive(Clone, Debug)]
pub enum NamespaceError {
    StrConversion,
    IntConversion(i32),
}

impl TryFrom<i32> for NamespaceType {
    type Error = NamespaceError;

    fn try_from(value: i32) -> Result<Self, Self::Error> {
        match value {
            c::CLONE_NEWCGROUP => Ok(Self::Cgroup),
            c::CLONE_NEWIPC => Ok(Self::IPC),
            c::CLONE_NEWNET => Ok(Self::Network),
            c::CLONE_NEWNS => Ok(Self::Mount),
            c::CLONE_NEWPID => Ok(Self::PID),
            c::CLONE_NEWUSER => Ok(Self::User),
            c::CLONE_NEWUTS => Ok(Self::UTS),
            _ => Err(NamespaceError::IntConversion(value)),
        }
    }
}

impl FromStr for NamespaceType {
    type Err = NamespaceError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "cgroup" => Ok(Self::Cgroup),
            "ipc" => Ok(Self::IPC),
            "network" => Ok(Self::Network),
            "mount" => Ok(Self::Mount),
            "pid" => Ok(Self::PID),
            "time" => Ok(Self::Time),
            "user" => Ok(Self::User),
            "uts" => Ok(Self::UTS),
            _ => Err(NamespaceError::StrConversion),
        }
    }
}

impl<'de> Deserialize<'de> for NamespaceType {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let typ = String::deserialize(deserializer)?;
        let typ = typ.as_str();
        NamespaceType::from_str(typ)
            .map_err(|_| DeError::invalid_value(Unexpected::Str(typ), &"valid namespace type"))
    }
}

#[derive(Clone, Debug, Deserialize)]
pub struct Namespace {
    #[serde(rename(deserialize = "type"))]
    typ: NamespaceType,
    path: Option<PathBuf>,
}
