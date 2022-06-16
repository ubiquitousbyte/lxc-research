use crate::oci::ContainerState;

use std::path::Path;

pub enum RuntimeError {}

type Result<T> = std::result::Result<T, RuntimeError>;

pub trait Runtime {
    type Id;

    /// Query th state of a container
    /// This operation will generate an error if the container does not exist
    /// Otherwise, it will return the state of the container
    fn state(&self, container_id: Self::Id) -> Result<ContainerState>;

    /// Creates a container with the specified container_id from the bundle
    /// pointed to by the user-defined file path.
    ///
    /// This operation generates an error if the container_id provided
    /// is not unique across all containers within the scope of the runtime,
    /// or is not valid in any other way.
    ///
    /// All of the properties configured in the config.json file except for
    /// process will be applied.
    /// If the runtime cannot apply a property specified in the configuration,
    /// it will generate an error and a new container will not be created  
    fn create(&self, container_id: Self::Id, bundle: Path) -> Result<()>;

    /// Starts the container
    ///
    /// Attempting to start a container that is not in the created state will
    /// have no effect on the container and will generate an error
    /// This operation effectively runs the user-specified program as specified
    /// by the process property in the conffiguration.
    fn start(&self, container_id: Self::Id) -> Result<()>;

    /// Sends a signal to the container
    ///
    /// Attempting to send a signal to a container that is neither in the created
    /// nor in the running state will have no effect on the container and will
    /// generate an error
    fn kill(&self, container_id: Self::Id, signal: i32) -> Result<()>;

    /// Deletes the container
    ///
    /// Attempting to delete a container that is not in the stopped state
    /// will have no effect on the container and will generate an error
    /// This operation will delete all resources that were created during
    /// the create step.
    fn delete(&self, container_id: Self::Id) -> Result<()>;
}
