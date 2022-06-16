use std::path::PathBuf;

use serde::Deserialize;

#[derive(Debug, Clone, Deserialize)]
pub struct Hook {
    pub path: PathBuf,
    #[serde(default)]
    pub args: Vec<String>,
    #[serde(default)]
    pub env: Vec<String>,
    pub timeout: Option<u32>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Hooks {
    #[serde(default)]
    pub create_runtime: Vec<Hook>,
    #[serde(default)]
    pub create_container: Vec<Hook>,
    #[serde(default)]
    pub start_container: Vec<Hook>,
    #[serde(default)]
    pub poststart: Vec<Hook>,
    #[serde(default)]
    pub poststop: Vec<Hook>,
}
