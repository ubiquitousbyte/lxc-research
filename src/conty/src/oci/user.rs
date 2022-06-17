use serde::Deserialize;

#[derive(Debug, Clone, Deserialize)]
pub struct IdMapping {
    #[serde(rename = "containerID")]
    pub container_id: u32,
    #[serde(rename = "hostID")]
    pub host_id: u32,
    pub size: u32,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct User {
    // The user id in the container namespace
    pub uid: u32,
    // The group id in the container namespace
    pub gid: u32,
    // The umask of the user
    // If unspecified, it should not be changed from the calling process' umask
    pub umask: Option<u32>,
    #[serde(default)]
    // Additional gids to add to the user inside the container namespace
    pub additional_gids: Vec<u32>,
}
