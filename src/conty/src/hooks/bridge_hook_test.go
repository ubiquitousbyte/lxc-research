package hooks

import (
	"net"
	"testing"

	"github.com/vishvananda/netlink"
	"golang.org/x/sys/unix"
)

func TestBridgeHookOnContainerCreating(t *testing.T) {
	bridge := &netlink.Bridge{
		LinkAttrs: netlink.LinkAttrs{
			Name: "brcc1",
		},
	}

	if err := netlink.LinkAdd(bridge); err != nil {
		t.Error(err)
		return
	}

	state := ContainerState{
		Id:     "cc1",
		Pid:    unix.Getpid(),
		Bundle: "",
		Status: ContainerStatusCreating,
	}

	hook := &BridgeHook{
		Bridge: "brcc1",
		Addresses: []*net.IPNet{
			{
				IP:   net.IPv4(192, 168, 168, 1),
				Mask: net.IPv4Mask(255, 255, 255, 0),
			},
		},
	}

	if err := hook.OnContainerCreating(state); err != nil {
		t.Error(err)
	}
}
