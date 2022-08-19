package hooks

import (
	"encoding/json"
	"errors"
	"io"
	"io/ioutil"
	"path"
	"runtime"
	"strconv"

	"golang.org/x/sys/unix"
)

var (
	ErrContainerStateInvalid = errors.New("invalid container state")
	ErrNamespaceUnsupported  = errors.New("unsupported namespace")
)

type ContainerState struct {
	Id     string          `json:"id"`
	Pid    int             `json:"pid"`
	Bundle string          `json:"bundle"`
	Status ContainerStatus `json:"status"`
}

func ReadContainerState(reader io.Reader) (c ContainerState, err error) {
	bts, err := ioutil.ReadAll(reader)
	if err != nil {
		return c, ErrContainerStateInvalid
	}
	if err = json.Unmarshal(bts, &c); err != nil {
		err = ErrContainerStateInvalid
	}
	return c, err
}

type ContainerStatus string

const (
	ContainerStatusCreating ContainerStatus = "creating"
	ContainerStatusCreated  ContainerStatus = "created"
	ContainerStatusRunning  ContainerStatus = "running"
	ContainerStatusStopped  ContainerStatus = "stopped"
)

var namespaces map[int]string = map[int]string{
	unix.CLONE_NEWUSER:   "user",
	unix.CLONE_NEWPID:    "pid",
	unix.CLONE_NEWNS:     "mnt",
	unix.CLONE_NEWUTS:    "uts",
	unix.CLONE_NEWIPC:    "ipc",
	unix.CLONE_NEWNET:    "net",
	unix.CLONE_NEWCGROUP: "cgroup",
}

// DoInContainerNamespace is a utiltiy function that executes the user-defined
// function in the namespace of the container identified by containerPid
func DoInContainerNamespace(containerPid, namespace int, doer func() error) error {
	// Make sure to lock the current goroutine onto the current
	// hardware thread to avoid leaking the namespace in another goroutine
	runtime.LockOSThread()
	defer runtime.UnlockOSThread()

	ns, ok := namespaces[namespace]
	if !ok {
		return ErrNamespaceUnsupported
	}

	// Get a file descriptor handle to the old namespace
	// in order to return back to it after the doer finishes
	oldNsPath := path.Join("/proc", "self", "ns", ns)
	oldNsFd, err := unix.Open(oldNsPath, unix.O_RDONLY|unix.O_CLOEXEC, 0)
	if err != nil {
		return err
	}
	defer unix.Close(oldNsFd)

	// Now get a file descriptor to the new namespace
	newNsPath := path.Join("/proc", strconv.Itoa(containerPid), "ns", ns)
	newNsFd, err := unix.Open(newNsPath, unix.O_RDONLY|unix.O_CLOEXEC, 0)
	if err != nil {
		return err
	}
	defer unix.Close(newNsFd)

	// Join the new namespace and execute the user-defined function
	if err := unix.Setns(newNsFd, namespace); err != nil {
		return err
	}
	// Make sure to exit the new namespace at the end
	defer unix.Setns(oldNsFd, namespace)

	return doer()
}
