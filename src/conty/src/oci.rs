use std::{collections::HashMap, path::PathBuf, str::FromStr};

use libc as c;
use serde::de::{Error as DeError, Unexpected};
use serde::{Deserialize, Deserializer};

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Config {
    oci_version: String,
    process: Option<Process>,
    root: Option<RootFs>,
    hostname: Option<String>,
    mounts: Option<Vec<Mount>>,
    hooks: Option<Hooks>,
    annotations: Option<HashMap<String, String>>,
    linux: Option<Linux>,
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
    create_runtime: Option<Vec<Hook>>,
    create_container: Option<Vec<Hook>>,
    start_container: Option<Vec<Hook>>,
    poststart: Option<Vec<Hook>>,
    poststop: Option<Vec<Hook>>,
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
    uid_mappings: Option<Vec<IdMapping>>,
    gid_mappings: Option<Vec<IdMapping>>,
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
    bounding: Option<Vec<String>>,
    permitted: Option<Vec<String>>,
    effective: Option<Vec<String>>,
    inheritable: Option<Vec<String>>,
    ambient: Option<Vec<String>>,
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
    rlimits: Option<Vec<ResourceLimit>>,
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
pub struct NamespaceType {
    flag: i32,
}

impl FromStr for NamespaceType {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let flag = match s {
            "cgroup" => Ok(c::CLONE_NEWCGROUP),
            "ipc" => Ok(c::CLONE_NEWIPC),
            "network" => Ok(c::CLONE_NEWNET),
            "mount" => Ok(c::CLONE_NEWNS),
            "pid" => Ok(c::CLONE_NEWPID),
            "user" => Ok(c::CLONE_NEWUSER),
            "uts" => Ok(c::CLONE_NEWUTS),
            "time" => Err("time namespace currently not supported"),
            _ => Err("invalid namespace"),
        };
        flag.map(|flag| NamespaceType { flag })
    }
}

impl Into<i32> for NamespaceType {
    fn into(self) -> i32 {
        self.flag
    }
}

impl<'de> Deserialize<'de> for NamespaceType {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let s = s.as_str();
        NamespaceType::from_str(s).map_err(|_| {
            DeError::invalid_value(
                Unexpected::Str(s),
                &"^(cgroup|ipc|network|mount|pid|user|uts)$",
            )
        })
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
    weight_device: Option<Vec<WeightDevice>>,
    throttle_read_bps_device: Option<Vec<ThrottleDevice>>,
    throttle_write_bps_device: Option<Vec<ThrottleDevice>>,
    throttle_read_iops_device: Option<Vec<ThrottleDevice>>,
    throttle_write_iops_device: Option<Vec<ThrottleDevice>>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Resources {
    #[serde(default)]
    devices: Option<Vec<DeviceCgroup>>,
    memory: Option<Memory>,
    cpu: Option<CPU>,
    pids: Option<ProcessLimit>,
    #[serde(rename(deserialize = "blockIO"))]
    block_io: Option<BlockIO>,
    hugepage_limits: Option<HugePageLimit>,
    network: Option<Network>,
    rdma: Option<Rdma>,
    unified: Option<HashMap<String, String>>,
}

#[derive(Clone, Debug)]
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
    type Err = &'static str;

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
            _ => Err("unsupported seccomp action"),
        }
    }
}

impl<'de> Deserialize<'de> for SeccompAction {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let s = s.as_str();
        SeccompAction::from_str(s).map_err(|_| {
            DeError::invalid_value(
                Unexpected::Str(s),
                &"^SCMP_ACT_(KILL|KILL_PROCESS|KILL_THREAD|TRAP|ERRNO|TRACE|ALLOW|LOG|NOTIFY)$)",
            )
        })
    }
}

#[derive(Clone, Debug)]
pub enum SeccompArch {
    X86,
    X86_64,
    X32,
    ARM,
    AARCH64,
    MIPS,
    MIPS64,
    MIPS64N32,
    MIPSEL,
    MIPSEL64,
    MIPSEL64N32,
    PPC,
    PPC64,
    PPC64LE,
    S390,
    S390X,
    PARISC,
    PARISC64,
    RISCV64,
}

impl FromStr for SeccompArch {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "SCMP_ARCH_X86" => Ok(Self::X86),
            "SCMP_ARCH_X86_64" => Ok(Self::X86_64),
            "SCMP_ARCH_X32" => Ok(Self::X32),
            "SCMP_ARCH_ARM" => Ok(Self::ARM),
            "SCMP_ARCH_AARCH64" => Ok(Self::AARCH64),
            "SCMP_ARCH_MIPS" => Ok(Self::MIPS),
            "SCMP_ARCH_MIPS64" => Ok(Self::MIPS64),
            "SCMP_ARCH_MIPS64N32" => Ok(Self::MIPS64N32),
            "SCMP_ARCH_MIPSEL" => Ok(Self::MIPSEL),
            "SCMP_ARCH_MIPSEL64" => Ok(Self::MIPSEL64),
            "SCMP_ARCH_MIPSEL64N32" => Ok(Self::MIPSEL64N32),
            "SCMP_ARCH_PPC" => Ok(Self::PPC),
            "SCMP_ARCH_PPC64" => Ok(Self::PPC64),
            "SCMP_ARCH_PPC64LE" => Ok(Self::PPC64LE),
            "SCMP_ARCH_S390" => Ok(Self::S390),
            "SCMP_ARCH_S390X" => Ok(Self::S390X),
            "SCMP_ARCH_PARISC" => Ok(Self::PARISC),
            "SCMP_ARCH_PARISC64" => Ok(Self::PARISC64),
            "SCMP_ARCH_RISCV64" => Ok(Self::RISCV64),
            _ => Err("unsupported seccomp architecture"),
        }
    }
}

impl<'de> Deserialize<'de> for SeccompArch {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let s = s.as_str();
        SeccompArch::from_str(s).map_err(|_| {
            let msg = "^SCMP_ARCH_(X86|X86_64|X32|ARM|AARCH64|MIPS|MIPS64|MIPS64N32|MIPSEL|\
                MIPSEL64|MIPSEL64N32|PPC|PPC64|PPC64LE|S390|S390X|PARISC|PARISC64|RISCV64)$)";
            DeError::invalid_value(Unexpected::Str(s), &msg)
        })
    }
}

#[derive(Clone, Debug)]
pub enum SeccompOperator {
    NotEqual,
    LessThan,
    LessEqual,
    Equal,
    GreaterEqual,
    GreaterThan,
    MaskedEqual,
}

impl FromStr for SeccompOperator {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "SCMP_CMP_NE" => Ok(Self::NotEqual),
            "SCMP_CMP_LT" => Ok(Self::LessThan),
            "SCMP_CMP_LE" => Ok(Self::LessEqual),
            "SCMP_CMP_EQ" => Ok(Self::Equal),
            "SCMP_CMP_GE" => Ok(Self::GreaterEqual),
            "SCMP_CMP_GT" => Ok(Self::GreaterThan),
            "SCMP_CMP_MASKED_EQ" => Ok(Self::MaskedEqual),
            _ => Err("unsupported seccomp operator"),
        }
    }
}

impl<'de> Deserialize<'de> for SeccompOperator {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let s = s.as_str();
        SeccompOperator::from_str(s).map_err(|_| {
            DeError::invalid_value(
                Unexpected::Str(s),
                &"^SCMP_CMP_(NE|LT|LE|EQ|GE|GT|MASKED_EQ)$",
            )
        })
    }
}

#[derive(Clone, Debug, Deserialize)]
pub struct SeccompArg {
    index: u32,
    #[serde(rename(deserialize = "value"))]
    first_arg: u64,
    #[serde(rename(deserialize = "valueTwo"))]
    second_arg: Option<u64>,
    op: SeccompOperator,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct SeccompSyscallFilter {
    names: Vec<String>,
    action: SeccompAction,
    errno_ret: Option<u32>,
    args: Option<Vec<SeccompArg>>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Seccomp {
    default_action: SeccompAction,
    default_errno_ret: Option<u32>,
    architectures: Option<Vec<SeccompArch>>,
    flags: Option<Vec<String>>,
    listener_path: Option<String>,
    listener_metadata: Option<String>,
    syscalls: Option<SeccompSyscallFilter>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Linux {
    uid_mappings: Option<Vec<IdMapping>>,
    gid_mappings: Option<Vec<IdMapping>>,
    sysctl: Option<HashMap<String, String>>,
    resources: Option<Resources>,
    cgroups_path: Option<String>,
    namespaces: Option<Vec<Namespace>>,
    devices: Option<Vec<Device>>,
    seccomp: Option<Seccomp>,
    rootfs_propagation: Option<String>,
    masked_paths: Option<Vec<String>>,
    readonly_paths: Option<Vec<String>>,
    mount_label: Option<String>,
}
