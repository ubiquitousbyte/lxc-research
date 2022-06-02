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

#[derive(Clone, Debug, Deserialize)]
pub struct DeviceCgroup {
    allow: bool,
    #[serde(rename(deserialize = "type"))]
    typ: Option<String>,
    major: Option<i64>,
    minor: Option<i64>,
    access: Option<String>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct Memory {
    limit: Option<i64>,
    reservation: Option<i64>,
    swap: Option<i64>,
    kernel: Option<i64>,
    #[serde(rename(deserialize = "kernelTCP"))]
    kernel_tcp: Option<i64>,
    swappiness: Option<u64>,
    #[serde(rename(deserialize = "disableOOMKiller"))]
    disable_oom_killer: Option<bool>,
    #[serde(rename(deserialize = "useHierarchy"))]
    use_hierarchy: Option<bool>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CPU {
    shares: Option<u64>,
    quota: Option<i64>,
    period: Option<u64>,
    realtime_runtime: Option<i64>,
    realtime_period: Option<u64>,
    cpus: Option<String>,
    mems: Option<String>,
    idle: Option<i64>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct ProcessLimit {
    limit: i64,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct HugePageLimit {
    page_size: String,
    limit: u64,
}

#[derive(Clone, Debug, Deserialize)]
pub struct InterfacePriority {
    name: String,
    priority: u32,
}

#[derive(Clone, Debug, Deserialize)]
pub struct BlockIODevice {
    major: i64,
    minor: i64,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WeightDevice {
    major: i64,
    minor: i64,
    weight: Option<u16>,
    leaf_weight: Option<u16>,
}

#[derive(Clone, Debug, Deserialize)]
pub struct ThrottleDevice {
    major: i64,
    minor: i64,
    rate: u64,
}

#[derive(Clone, Debug, Deserialize)]
pub struct Network {
    #[serde(rename(deserialize = "classID"))]
    class_id: Option<u32>,
    #[serde(default)]
    priorities: Vec<InterfacePriority>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Rdma {
    hca_handles: Option<u32>,
    hca_objects: Option<u32>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BlockIO {
    weight: Option<u16>,
    leaf_weight: Option<u16>,
    #[serde(default)]
    weight_device: Vec<WeightDevice>,
    #[serde(default)]
    throttle_read_bps_device: Vec<ThrottleDevice>,
    #[serde(default)]
    throttle_write_bps_device: Vec<ThrottleDevice>,
    #[serde(default)]
    throttle_read_iops_device: Vec<ThrottleDevice>,
    #[serde(default)]
    throttle_write_iops_device: Vec<ThrottleDevice>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Resources {
    #[serde(default)]
    devices: Vec<DeviceCgroup>,
    memory: Option<Memory>,
    cpu: Option<CPU>,
    pids: Option<ProcessLimit>,
    #[serde(rename(deserialize = "blockIO"))]
    block_io: Option<BlockIO>,
    hugepage_limits: Option<HugePageLimit>,
    network: Option<Network>,
    rdma: Option<Rdma>,
    #[serde(default)]
    unified: HashMap<String, String>,
}

pub enum SeccompAction {
    Kill,
    KillProcess,
    KillThread,
    Trap,
    Errno,
    Trace,
    Allow,
    Log,
    Notify,
}

impl FromStr for SeccompAction {
    type Err = NamespaceError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "SCMP_ACT_KILL" => Ok(Self::Kill),
            "SCMP_ACT_KILL_PROCESS" => Ok(Self::KillProcess),
            "SCMP_ACT_KILL_THREAD" => Ok(Self::KillThread),
            "SCMP_ACT_TRAP" => Ok(Self::Trap),
            "SCMP_ACT_ERRNO" => Ok(Self::Errno),
            "SCMP_ACT_TRACE" => Ok(Self::Trace),
            "SCMP_ACT_ALLOW" => Ok(Self::Allow),
            "SCMP_ACT_LOG" => Ok(Self::Log),
            "SCMP_ACT_NOTIFY" => Ok(Self::Notify),
            _ => Err(NamespaceError::StrConversion),
        }
    }
}

impl<'de> Deserialize<'de> for SeccompAction {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let typ = String::deserialize(deserializer)?;
        let typ = typ.as_str();
        SeccompAction::from_str(typ)
            .map_err(|_| DeError::invalid_value(Unexpected::Str(typ), &"valid namespace type"))
    }
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Linux {
    #[serde(default)]
    uid_mappings: Vec<IdMapping>,
    #[serde(default)]
    gid_mappings: Vec<IdMapping>,
    #[serde(default)]
    sysctl: HashMap<String, String>,
    resources: Option<Resources>,
    cgroups_path: Option<String>,
    #[serde(default)]
    namespaces: Vec<Namespace>,
    #[serde(default)]
    devices: Vec<Device>,
}
