package hooks

import (
	"io"
)

type Hook interface {
	OnContainerCreating(state ContainerState) error
	OnContainerCreated(state ContainerState) error
	OnContainerRunning(state ContainerState) error
	OnContainerStopped(state ContainerState) error
}

// Runs the hook, reading the container state from the stateReader beforehand
func Run(hook Hook, stateReader io.Reader) error {
	state, err := ReadContainerState(stateReader)
	if err != nil {
		return err
	}

	var cb func(ContainerState) error

	switch state.Status {
	case ContainerStatusCreating:
		cb = hook.OnContainerCreating
	case ContainerStatusCreated:
		cb = hook.OnContainerCreated
	case ContainerStatusRunning:
		cb = hook.OnContainerRunning
	case ContainerStatusStopped:
		cb = hook.OnContainerStopped
	}

	return cb(state)
}
