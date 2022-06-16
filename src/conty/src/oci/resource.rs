use crate::oci::OCIError;

use std::collections::HashMap;
use std::str::FromStr;

use serde::de::Error as DeError;
use serde::{Deserialize, Deserializer};

use libc as c;

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Resources {
    #[serde(default)]
    pub devices: Vec<DeviceCgroup>,
    pub memory: Option<Memory>,
    pub cpu: Option<CPU>,
    pub pids: Option<Pids>,
    #[serde(rename = "blockIO")]
    pub block_io: Option<BlockIO>,
    #[serde(default)]
    pub hugepage_limits: Vec<HugepageLimit>,
    pub network: Option<Network>,
    #[serde(default)]
    pub rdma: HashMap<String, Rdma>,
    #[serde(default)]
    pub unified: HashMap<String, String>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct DeviceCgroup {
    pub allow: bool,
    #[serde(rename = "type")]
    pub typ: Option<String>,
    pub major: Option<i64>,
    pub minor: Option<i64>,
    pub access: Option<i64>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Memory {
    pub limit: Option<i64>,
    pub reservation: Option<i64>,
    pub swap: Option<i64>,
    pub kernel: Option<i64>,
    #[serde(rename = "kernelTCP")]
    pub kernel_tcp: Option<i64>,
    pub swappiness: Option<u64>,
    #[serde(rename = "disableOOMKiller")]
    pub disable_oom_killer: Option<bool>,
    pub use_hierarchy: Option<bool>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct CPU {
    pub shares: Option<u64>,
    pub quota: Option<i64>,
    pub period: Option<u64>,
    pub realtime_runtime: Option<i64>,
    pub realtime_period: Option<u64>,
    pub cpus: Option<String>,
    pub mems: Option<String>,
    pub idle: Option<i64>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Pids {
    pub limit: i64,
}

#[derive(Debug, Clone, Deserialize)]
pub struct Network {
    #[serde(rename = "classID")]
    pub class_id: Option<u32>,
    #[serde(default)]
    pub priorities: Vec<InterfacePriority>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct InterfacePriority {
    pub name: String,
    pub priority: u32,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct HugepageLimit {
    pub page_size: String,
    pub limit: u64,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Rdma {
    pub hca_handles: Option<u32>,
    pub hca_objects: Option<u32>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct BlockIO {
    pub weight: Option<u16>,
    pub leaf_weight: Option<u16>,
    #[serde(default)]
    pub weight_device: Vec<WeightDevice>,
    #[serde(default)]
    pub throttle_read_bps_device: Vec<ThrottleDevice>,
    #[serde(default)]
    pub throttle_write_bps_device: Vec<ThrottleDevice>,
    #[serde(default, rename = "throttleReadIOPSDevice")]
    pub throttle_read_iops_device: Vec<ThrottleDevice>,
    #[serde(default, rename = "throttleWriteIOPSDevice")]
    pub throttle_write_iops_device: Vec<ThrottleDevice>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WeightDevice {
    pub major: i64,
    pub minor: i64,
    pub weight: Option<u16>,
    pub leaf_weight: Option<u16>,
}

#[derive(Debug, Clone, Deserialize)]
pub struct ThrottleDevice {
    pub major: i64,
    pub minor: i64,
    pub rate: Option<u64>,
}

#[repr(u32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
/// See https://github.com/torvalds/linux/blob/master/include/uapi/asm-generic/resource.h
pub enum ResourceLimitType {
    // CPU time in seconds
    CPU = c::RLIMIT_CPU,
    // Max File size
    FileSize = c::RLIMIT_FSIZE,
    // Max data section size
    Data = c::RLIMIT_DATA,
    // Max stack section size
    Stack = c::RLIMIT_STACK,
    // Max core file size
    Core = c::RLIMIT_CORE,
    // Max resident set size
    RSS = c::RLIMIT_RSS,
    // Max number of processes
    NProc = c::RLIMIT_NPROC,
    // Max number of open files
    NOFiles = c::RLIMIT_NOFILE,
    // Max locked-in memory address space
    MemLock = c::RLIMIT_MEMLOCK,
    // Address space limit
    AddressSpace = c::RLIMIT_AS,
    // Maximum file locks held
    Locks = c::RLIMIT_LOCKS,
    // Maximum number of pending signals
    SigPending = c::RLIMIT_SIGPENDING,
    // Maximum bytes in POSIX msgqueues,
    MsgQueue = c::RLIMIT_MSGQUEUE,
    // Max nice prio
    Nice = c::RLIMIT_NICE,
    // Maximum realtime priority
    RealtimePrio = c::RLIMIT_RTPRIO,
    // Timeout for RT tasks in us
    Realtime = c::RLIMIT_RTTIME,
}

impl FromStr for ResourceLimitType {
    type Err = OCIError;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "RLIMIT_CPU" => Ok(Self::CPU),
            "RLIMIT_FSIZE" => Ok(Self::FileSize),
            "RLIMIT_DATA" => Ok(Self::Data),
            "RLIMIT_STACK" => Ok(Self::Stack),
            "RLIMIT_CORE" => Ok(Self::Core),
            "RLIMIT_RSS" => Ok(Self::RSS),
            "RLIMIT_NPROC" => Ok(Self::NProc),
            "RLIMIT_NOFILE" => Ok(Self::NOFiles),
            "RLIMIT_MEMLOCK" => Ok(Self::MemLock),
            "RLIMIT_AS" => Ok(Self::AddressSpace),
            "RLIMIT_LOCKS" => Ok(Self::Locks),
            "RLIMIT_SIGPENDING" => Ok(Self::SigPending),
            "RLIMIT_MSGQUEUE" => Ok(Self::MsgQueue),
            "RLIMIT_NICE" => Ok(Self::Nice),
            "RLIMIT_RTPRIO" => Ok(Self::RealtimePrio),
            "RLIMIT_RTTIME" => Ok(Self::Realtime),
            _ => Err(OCIError::InvalidConfig("resource limit type")),
        }
    }
}

impl<'de> Deserialize<'de> for ResourceLimitType {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        let variants = &[
            "RLIMIT_CPU",
            "RLIMIT_FSIZE",
            "RLIMIT_DATA",
            "RLIMIT_STACK",
            "RLIMIT_CORE",
            "RLIMIT_RSS",
            "RLIMIT_NPROC",
            "RLIMIT_NOFILE",
            "RLIMIT_MEMLOCK",
            "RLIMIT_AS",
            "RLIMIT_LOCKS",
            "RLIMIT_SIGPENDING",
            "RLIMIT_MSGQUEUE",
            "RLIMIT_NICE",
            "RLIMIT_RTPRIO",
            "RLIMIT_RTTIME",
        ];
        ResourceLimitType::from_str(&s).map_err(|_| DeError::unknown_variant(&s, variants))
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Deserialize)]
pub struct ResourceLimit {
    #[serde(rename = "type")]
    pub typ: ResourceLimitType,
    pub soft: u64,
    pub hard: u64,
}
