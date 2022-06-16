use std::str::FromStr;

use serde::de::Error as DeError;
use serde::{Deserialize, Deserializer};

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
        let variants = &[
            "SCMP_ACT_KILL",
            "SCMP_ACT_KILL_PROCESS",
            "SCMP_ACT_KILL_THREAD",
            "SCMP_ACT_TRAP",
            "SCMP_ACT_ERRNO",
            "SCMP_ACT_TRACE",
            "SCMP_ACT_ALLOW",
            "SCMP_ACT_LOG",
            "SCMP_ACT_NOTIFY",
        ];
        Self::from_str(&s).map_err(|_| DeError::unknown_variant(&s, variants))
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
        let variants = &[
            "SCMP_ARCH_X86",
            "SCMP_ARCH_X86_64",
            "SCMP_ARCH_X32",
            "SCMP_ARCH_ARM",
            "SCMP_ARCH_AARCH64",
            "SCMP_ARCH_MIPS",
            "SCMP_ARCH_MIPS64",
            "SCMP_ARCH_MIPS64N32",
            "SCMP_ARCH_MIPSEL",
            "SCMP_ARCH_MIPSEL64",
            "SCMP_ARCH_MIPSEL64N32",
            "SCMP_ARCH_PPC",
            "SCMP_ARCH_PPC64",
            "SCMP_ARCH_PPC64LE",
            "SCMP_ARCH_S390",
            "SCMP_ARCH_S390X",
            "SCMP_ARCH_PARISC",
            "SCMP_ARCH_PARISC64",
            "SCMP_ARCH_RISCV64",
        ];
        Self::from_str(&s).map_err(|_| DeError::unknown_variant(&s, variants))
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
        let variants = &[
            "SCMP_CMP_NE",
            "SCMP_CMP_LT",
            "SCMP_CMP_LE",
            "SCMP_CMP_EQ",
            "SCMP_CMP_GE",
            "SCMP_CMP_GT",
            "SCMP_CMP_MASKED_EQ",
        ];
        Self::from_str(&s).map_err(|_| DeError::unknown_variant(&s, variants))
    }
}

#[derive(Clone, Debug, Deserialize)]
pub struct SeccompArg {
    pub index: u32,
    #[serde(rename(deserialize = "value"))]
    pub first_arg: u64,
    #[serde(rename(deserialize = "valueTwo"))]
    pub second_arg: Option<u64>,
    pub op: SeccompOperator,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct SeccompSyscallFilter {
    pub names: Vec<String>,
    pub action: SeccompAction,
    pub errno_ret: Option<u32>,
    pub args: Option<Vec<SeccompArg>>,
}

#[derive(Clone, Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Seccomp {
    pub default_action: SeccompAction,
    pub default_errno_ret: Option<u32>,
    pub architectures: Option<Vec<SeccompArch>>,
    pub flags: Option<Vec<String>>,
    pub listener_path: Option<String>,
    pub listener_metadata: Option<String>,
    pub syscalls: Option<SeccompSyscallFilter>,
}
