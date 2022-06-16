use crate::oci::OCIError;

use std::str::FromStr;

use serde::{Deserialize, Deserializer};

#[derive(Debug, Clone, Deserialize)]
pub struct Capabilities {
    #[serde(default)]
    pub bounding: Vec<Capability>,
    #[serde(default)]
    pub effective: Vec<Capability>,
    #[serde(default)]
    pub inheritable: Vec<Capability>,
    #[serde(default)]
    pub permitted: Vec<Capability>,
    #[serde(default)]
    pub ambient: Vec<Capability>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(u8)]
pub enum Capability {
    /// `CAP_CHOWN` (from POSIX)
    Chown = 0,
    /// `CAP_DAC_OVERRIDE` (from POSIX)
    DacOverride = 1,
    /// `CAP_DAC_READ_SEARCH` (from POSIX)
    DacReadSearch = 2,
    /// `CAP_FOWNER` (from POSIX)
    FileOwner = 3,
    /// `CAP_FSETID` (from POSIX)
    FileSetId = 4,
    /// `CAP_KILL` (from POSIX)
    Kill = 5,
    /// `CAP_SETGID` (from POSIX)
    SetGid = 6,
    /// `CAP_SETUID` (from POSIX)
    SetUid = 7,
    /// `CAP_SETPCAP` (from Linux)
    SetProcCap = 8,
    LinuxImmutable = 9,
    NetBindService = 10,
    NetBroadcast = 11,
    NetAdmin = 12,
    NetRaw = 13,
    IPCLock = 14,
    IPCOwner = 15,
    /// `CAP_SYS_MODULE` (from Linux)
    SysModule = 16,
    /// `CAP_SYS_RAWIO` (from Linux)
    SysRawIO = 17,
    /// `CAP_SYS_CHROOT` (from Linux)
    SysChroot = 18,
    /// `CAP_SYS_PTRACE` (from Linux)
    SysPtrace = 19,
    /// `CAP_SYS_PACCT` (from Linux)
    SysPacct = 20,
    /// `CAP_SYS_ADMIN` (from Linux)
    SysAdmin = 21,
    /// `CAP_SYS_BOOT` (from Linux)
    SysBoot = 22,
    /// `CAP_SYS_NICE` (from Linux)
    SysNice = 23,
    /// `CAP_SYS_RESOURCE` (from Linux)
    SysResource = 24,
    /// `CAP_SYS_TIME` (from Linux)
    SysTime = 25,
    /// `CAP_SYS_TTY_CONFIG` (from Linux)
    SysTTYConfig = 26,
    /// `CAP_SYS_MKNOD` (from Linux, >= 2.4)
    Mknod = 27,
    /// `CAP_LEASE` (from Linux, >= 2.4)
    Lease = 28,
    AuditWrite = 29,
    /// `CAP_AUDIT_CONTROL` (from Linux, >= 2.6.11)
    AuditControl = 30,
    SetFileCap = 31,
    MACOverride = 32,
    MACAdmin = 33,
    /// `CAP_SYSLOG` (from Linux, >= 2.6.37)
    Syslog = 34,
    /// `CAP_WAKE_ALARM` (from Linux, >= 3.0)
    WakeAlarm = 35,
    BlockSuspend = 36,
    /// `CAP_AUDIT_READ` (from Linux, >= 3.16).
    AuditRead = 37,
    /// `CAP_PERFMON` (from Linux, >= 5.8).
    PerformanceMonitoring = 38,
    /// `CAP_BPF` (from Linux, >= 5.8).
    BPF = 39,
    /// `CAP_CHECKPOINT_RESTORE` (from Linux, >= 5.9).
    CheckpointRestore = 40,
    Unknown,
}

impl FromStr for Capability {
    type Err = OCIError;

    fn from_str(s: &str) -> std::result::Result<Self, Self::Err> {
        match s {
            "CAP_CHOWN" => Ok(Capability::Chown),
            "CAP_DAC_OVERRIDE" => Ok(Capability::DacOverride),
            "CAP_DAC_READ_SEARCH" => Ok(Capability::DacReadSearch),
            "CAP_FOWNER" => Ok(Capability::FileOwner),
            "CAP_FSETID" => Ok(Capability::FileSetId),
            "CAP_KILL" => Ok(Capability::Kill),
            "CAP_SETGID" => Ok(Capability::SetGid),
            "CAP_SETUID" => Ok(Capability::SetUid),
            "CAP_SETPCAP" => Ok(Capability::SetProcCap),
            "CAP_LINUX_IMMUTABLE" => Ok(Capability::LinuxImmutable),
            "CAP_NET_BIND_SERVICE" => Ok(Capability::NetBindService),
            "CAP_NET_BROADCAST" => Ok(Capability::NetBroadcast),
            "CAP_NET_ADMIN" => Ok(Capability::NetAdmin),
            "CAP_NET_RAW" => Ok(Capability::NetRaw),
            "CAP_IPC_LOCK" => Ok(Capability::IPCLock),
            "CAP_IPC_OWNER" => Ok(Capability::IPCOwner),
            "CAP_SYS_MODULE" => Ok(Capability::SysModule),
            "CAP_SYS_RAWIO" => Ok(Capability::SysRawIO),
            "CAP_SYS_CHROOT" => Ok(Capability::SysChroot),
            "CAP_SYS_PTRACE" => Ok(Capability::SysPtrace),
            "CAP_SYS_PACCT" => Ok(Capability::SysPacct),
            "CAP_SYS_ADMIN" => Ok(Capability::SysAdmin),
            "CAP_SYS_BOOT" => Ok(Capability::SysBoot),
            "CAP_SYS_NICE" => Ok(Capability::SysNice),
            "CAP_SYS_RESOURCE" => Ok(Capability::SysResource),
            "CAP_SYS_TIME" => Ok(Capability::SysTime),
            "CAP_SYS_TTY_CONFIG" => Ok(Capability::SysTTYConfig),
            "CAP_MKNOD" => Ok(Capability::Mknod),
            "CAP_LEASE" => Ok(Capability::Lease),
            "CAP_AUDIT_WRITE" => Ok(Capability::AuditWrite),
            "CAP_AUDIT_CONTROL" => Ok(Capability::AuditControl),
            "CAP_SETFCAP" => Ok(Capability::SetFileCap),
            "CAP_MAC_OVERRIDE" => Ok(Capability::MACOverride),
            "CAP_MAC_ADMIN" => Ok(Capability::MACAdmin),
            "CAP_SYSLOG" => Ok(Capability::Syslog),
            "CAP_WAKE_ALARM" => Ok(Capability::WakeAlarm),
            "CAP_BLOCK_SUSPEND" => Ok(Capability::BlockSuspend),
            "CAP_AUDIT_READ" => Ok(Capability::AuditRead),
            "CAP_PERFMON" => Ok(Capability::PerformanceMonitoring),
            "CAP_BPF" => Ok(Capability::BPF),
            "CAP_CHECKPOINT_RESTORE" => Ok(Capability::CheckpointRestore),
            _ => Ok(Capability::Unknown),
        }
    }
}

impl<'de> Deserialize<'de> for Capability {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        Ok(Capability::from_str(&s).unwrap())
    }
}
