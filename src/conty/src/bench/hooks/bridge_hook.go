package hooks

import (
	"errors"
	"fmt"
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
	// The addresses to be assigned to the bridge, if created
	BridgeAddresses []*net.IPNet
	// The addresses to assign to the container
	ContainerAddresses []*net.IPNet
	DeleteBridge       bool
}

var _ (Hook) = (*BridgeHook)(nil)

func (b *BridgeHook) OnContainerCreating(state ContainerState) error {
	// First off, get a handle to the bridge device where we'll
	// attach the virtual ethernet pair for the container
	var created bool
	br, err := getBridge(b.Bridge)
	if err != nil {
		if err != ErrBridgeNotFound {
			fmt.Println("Cannot get bridge device")
			return err
		}

		// Bridge doesn't exist, so create it
		br = &netlink.Bridge{
			LinkAttrs: netlink.LinkAttrs{
				MTU:  1500,
				Name: b.Bridge,
			},
		}
		if err = netlink.LinkAdd(br); err != nil {
			fmt.Println("Cannot add bridge device")
			return err
		}
		created = true

		defer func() {
			if err != nil && created {
				netlink.LinkDel(br)
			}
		}()

		for _, addr := range b.BridgeAddresses {
			nlAddr := &netlink.Addr{IPNet: addr}
			if err := netlink.AddrAdd(br, nlAddr); err != nil {
				fmt.Println("Cannot add address to bridge device")
				return err
			}
		}

		if err = netlink.LinkSetUp(br); err != nil {
			fmt.Println("Cannot set bridge device up")
			return err
		}
	}

	var (
		hostName string = fmt.Sprintf("veth%s", state.Id[:3])
		peerName string = fmt.Sprintf("vethc%s", state.Id[:3])
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
		}
	}()

	fmt.Printf("Creating veth device %s", hostName)

	if err = netlink.LinkAdd(veth); err != nil {
		fmt.Println("Cannot add veth device")
		return err
	}

	// Attach the host's end of the virtual ethernet cable to the
	// bridge. This makes network communication between the container
	// and all peers connected to the bridge possible, at least at layer 2
	if err = netlink.LinkSetMaster(veth, br); err != nil {
		fmt.Println("Cannot set link master of veth device")
		return err
	}

	// Now the interesting bit,
	// get a handle to the other end of the virtual ethernet device and
	// move it into the container's network namespace
	peer, err := netlink.LinkByName(peerName)
	if err != nil {
		fmt.Println("Cannot get veth device")
		return err
	}

	if err = netlink.LinkSetNsPid(peer, state.Pid); err != nil {
		fmt.Println("Cannot get namespace pid")
		return err
	}

	// Join the network namespace of the container, rename the virtual
	// ethernet device to eth0, and add all IP addresses to the device
	err = DoInContainerNamespace(state.Pid, unix.CLONE_NEWNET, func() error {
		// Rename the virtual ethernet device to eth0
		//if err := netlink.LinkSetName(peer, "eth0"); err != nil {
		//	fmt.Println("Cannot set name to eth0")
		//	return err
		//}

		// Add all IP addresses to the device
		for _, addr := range b.ContainerAddresses {
			nlAddr := &netlink.Addr{IPNet: addr}
			if err := netlink.AddrAdd(peer, nlAddr); err != nil {
				fmt.Println("Cannot add address to veth device")
				return err
			}
		}

		localhost, err := netlink.LinkByName("lo")
		if err != nil {
			fmt.Println("Cannot get loopback")
			return err
		}

		if err = netlink.LinkSetUp(localhost); err != nil {
			fmt.Println("Cannot set loopback up")
			return err
		}

		if err = netlink.LinkSetUp(peer); err != nil {
			fmt.Println("Cannot set veth device up")
		}

		return err
	})

	if err != nil {
		return err
	}

	// Bring the host end's pair of the veth cable up
	if err = netlink.LinkSetUp(veth); err != nil {
		fmt.Println("Cannot set veth device up on host")
	}

	return err
}

func (b *BridgeHook) OnContainerCreated(state ContainerState) error {
	return nil
}

func (b *BridgeHook) OnContainerRunning(state ContainerState) error {
	return nil
}

func (b *BridgeHook) OnContainerStopped(state ContainerState) error {
	veth, _ := netlink.LinkByName(fmt.Sprintf("veth%s", state.Id[:3]))
	netlink.LinkDel(veth)
	if b.DeleteBridge {
		link, err := netlink.LinkByName(b.Bridge)
		if err != nil {
			// Nothing to delete, so whatever
			return nil
		}
		netlink.LinkSetDown(link)
		return netlink.LinkDel(link)
	}
	return nil
}

// getBridge queries the kernel for a bridge device with the given name
func getBridge(name string) (br *netlink.Bridge, err error) {
	bridge, err := netlink.LinkByName(name)
	if err != nil {
		return nil, ErrBridgeNotFound
	}

	if bridge.Type() != "bridge" {
		return nil, ErrLinkNotBridge
	}

	br = bridge.(*netlink.Bridge)
	return br, nil
}
