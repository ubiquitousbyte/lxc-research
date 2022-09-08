# Prerequisites

On Fedora:
```bash 
sudo dnf install make cmake gcc gcc-c++ git golang iperf3 fio jq gnuplot
```

You'll need `docker` installed. 
We use it to build root filesystem images for the container runner.
Follow [this installation guide](https://docs.docker.com/engine/install/fedora/), if on Fedora. 

Make sure to disable docker's firewall rules via:
```bash
echo 0 > /proc/sys/net/bridge/bridge-nf-call-iptables
```
The docker engine prevents us from measuring network performance across network namespace boundaries
because it drops packets that aren't protected by `iptables`.

# Build 

Clone the repo, compile the project and install the binaries:
```bash
git clone https://github.com/ubiquitousbyte/lxc-research.git
cd lxc-research/src/conty
mkdir build
cd build 
cmake ../
make --build . --target install
```

Validate that the container runner is installed:
```bash
conty-runner --help
Usage: conty-runner [OPTION...] NAME
conty-runner -- A program that runs containers

  -b, --bundle=PATH          Path to container bundle
  -t, --timeout[=NUMBER]     Time to wait for container to exit before killing
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to htw-berlin.de.
```

# Run the benchmarks

Head over to the benchmark directory:
```bash
# From the repository root
cd src/conty/src/bench
```

## Network benchmark

This benchmark has three modes of operation:
1. Run both the client and the server in containers
2. Run just the server in a container and the client on the host
3. Run just the client in a container and the server on the host
4. Run both the client and the server on the host

### Mode 1

Run the following:
```bash
make install-net
```
This will create a root filesystem for both containers at `./rootfs/net` and 
two configuration files `net-container-client.json` and `net-container-server.json` 
derived from `net-container-client-template.json` and `net-container-server-template.json`, respectively.

Now you're ready to run the benchmark: 
```bash
conty-runner --timeout=120 --bundle=./net-container-server.json srv &
conty-runner --bundle=./net-container-client.json cln
```

The first command starts the server as a background container. 
Don't worry about managing the process, the runner will kill it after 120 seconds.
The second command starts the client as a foreground process. The client will collect all necessary
performance metrics and store them inside the root filesystem - `./rootfs/net/test.iperf3`.

**Generating the plots is a bit tedious, sorry about that.**
Copy the file `./rootfs/net/test.iperf3` into the plotters directory and generate the plots:
```bash
cd plotters
cp ../rootfs/net/test.iperf3 perf-output.json
./iperf-plot.sh perf-output.json
cd results
ls 
RTT.pdf bytes.pdf throughput.pdf MTU.pdf retransmits.pdf RTT_Var.pdf
```

