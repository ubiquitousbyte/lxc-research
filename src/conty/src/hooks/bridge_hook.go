package hooks

import (
	"errors"
	"fmt"
	"log"
	"net"

	"github.com/vishvananda/netlink"
	"golang.org/x/sys/unix"
)

var (
	ErrLinkNotBridge  = errors.New("link is not a bridge device")
	ErrBridgeNotFound = errors.New("bridge not found")
)

type BridgeHook struct {
	// The bridge device to connect the virtual ethernet pair to
	Bridge string
	// The addresses to assign to the container
	Addresses []*net.IPNet
}

var _ (Hook) = (*BridgeHook)(nil)

func (b *BridgeHook) OnContainerCreating(state ContainerState) error {
	// First off, get a handle to the bridge device where we'll
	// attach the virtual ethernet pair for the container
	br, err := getBridge(b.Bridge)
	if err != nil {
		return err
	}

	var (
		hostName string = fmt.Sprintf("veth%s", state.Id)
		peerName string = fmt.Sprintf("vethc%s", state.Id)
	)

	// Create a virtual ethernet cable
	veth := &netlink.Veth{
		LinkAttrs: netlink.LinkAttrs{
			Name: hostName,
			MTU:  br.MTU,
		},
		PeerName: peerName,
	}

	defer func() {
		if err != nil {
			netlink.LinkDel(veth)
			log.Fatal(err)
		}
	}()

	if err = netlink.LinkAdd(veth); err != nil {
		return err
	}

	// Attach the host's end of the virtual ethernet cable to the
	// bridge. This makes network communication between the container
	// and all peers connected to the bridge possible, at least at layer 2
	if err = netlink.LinkSetMaster(veth, br); err != nil {
		return err
	}

	// Now the interesting bit,
	// get a handle to the other end of the virtual ethernet device and
	// move it into the container's network namespace
	peer, err := netlink.LinkByName(peerName)
	if err != nil {
		return err
	}

	if err = netlink.LinkSetNsPid(peer, state.Pid); err != nil {
		return err
	}

	// Join the network namespace of the container, rename the virtual
	// ethernet device to eth0, and add all IP addresses to the device
	err = DoInContainerNamespace(state.Pid, unix.CLONE_NEWNET, func() error {
		// Rename the virtual ethernet device to eth0
		if err := netlink.LinkSetName(peer, "eth0"); err != nil {
			return err
		}

		// Add all IP addresses to the device
		for _, addr := range b.Addresses {
			nlAddr := &netlink.Addr{IPNet: addr}
			if err := netlink.AddrAdd(peer, nlAddr); err != nil {
				return err
			}
		}

		return nil
	})

	if err != nil {
		return err
	}

	// Bring the host end's pair of the veth cable up
	err = netlink.LinkSetUp(veth)
	return err
}

func (b *BridgeHook) OnContainerCreated(state ContainerState) error {
	veth, err := netlink.LinkByName("eth0")
	if err != nil {
		return err
	}
	return netlink.LinkSetUp(veth)
}

func (b *BridgeHook) OnContainerRunning(state ContainerState) error {
	return nil
}

func (b *BridgeHook) OnContainerStopped(state ContainerState) error {
	vethName := fmt.Sprintf("veth%s", state.Id)
	veth, err := netlink.LinkByName(vethName)
	if err != nil {
		return err
	}
	return netlink.LinkDel(veth)
}

// getBridge queries the kernel for a bridge device with the given name
func getBridge(name string) (br *netlink.Bridge, err error) {
	bridge, err := netlink.LinkByName(name)
	if err != nil {
		return nil, err
	}

	if bridge.Type() != "bridge" {
		return nil, ErrLinkNotBridge
	}

	br = bridge.(*netlink.Bridge)
	return br, nil
}
