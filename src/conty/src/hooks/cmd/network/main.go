package main

import (
	"flag"
	"log"
	"net"
	"os"

	"github.com/ubiquitousbyte/lxc-research/nsbench/hooks"
)

type ipAddresses []*net.IPNet

func (i *ipAddresses) Set(value string) error {
	ip, net, err := net.ParseCIDR(value)
	if err != nil {
		return err
	}
	net.IP = ip
	*i = append(*i, net)
	return nil
}

func (i *ipAddresses) String() string {
	return "192.168.0.101/24"
}

var (
	module       string
	bridge       string
	bridgeIps    ipAddresses
	containerIps ipAddresses
	deleteBridge bool
)

func main() {
	flag.StringVar(
		&module,
		"module",
		"shared-bridge",
		"The network module to use for connecting the container",
	)
	flag.StringVar(
		&bridge,
		"bridge-name",
		"",
		"The name of the bridge to connect the container to",
	)
	flag.Var(&bridgeIps, "bridge-ips", "IP addresses of the form <ip>/<prefix>")
	flag.Var(&containerIps, "container-ips", "IP addresses of the form <ip>/<prefix>")
	flag.BoolVar(&deleteBridge, "delete-bridge", false, "Delete bridge if the container stops")
	flag.Parse()

	var hook hooks.Hook

	switch module {
	case "shared-bridge":
		if bridge == "" {
			log.Fatal("shared-bridge module requires bridge-name to be specified")
		}
		hook = &hooks.BridgeHook{
			Bridge:             bridge,
			BridgeAddresses:    bridgeIps,
			ContainerAddresses: containerIps,
		}
	default:
		log.Fatalf("module %s not supported", module)
	}

	if err := hooks.Run(hook, os.Stdin); err != nil {
		log.Fatal(err)
	}
}
