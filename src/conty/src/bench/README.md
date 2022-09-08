# Prerequisites
You'll need `docker` installed.
We use it to export root filesystems for our own containers.
You'll also need the `jq` utility program. 
The Makefile depends on that to automatically setup the container configuration paths 

Make sure to disable 
```bash
echo 0 > /proc/sys/net/bridge/bridge-nf-call-iptables
```