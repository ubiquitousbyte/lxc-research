build-io: 
	@docker build -f ./Dockerfile.fio . -t lxc-research/fio 
	@docker export $(shell docker create lxc-research/fio) --output="fio.tar"
	@mkdir -p ./rootfs/io
	@tar -xf fio.tar -C ./rootfs/io
	@touch ./rootfs/io/SanDisk_microSD_UHS-I-Container

build-net:
	@docker build -f ./Dockerfile.iperf3 . -t lxc-research/iperf3
	@docker export $(shell docker create lxc-research/iperf3) --output="iperf3.tar"
	@mkdir -p ./rootfs/net
	@tar -xf iperf3.tar -C ./rootfs/net
	@cd hooks; go build -o ./bin/net-hook ./cmd/network/main.go 
	sudo chmod u+x ./hooks/bin/net-hook
	sudo chown root ./hooks/bin/net-hook
	sudo chmod u+s ./hooks/bin/net-hook

install-io: build-io
	@jq --arg x $(shell pwd)/rootfs/io '.root.path = $$x' io-container-template.json > io-container.json

install-net: build-net
	@jq --arg x $(shell pwd)/rootfs/net --arg y $(shell pwd)/hooks/bin/net-hook '.root.path = $$x | (.hooks.on_runtime_create[0].path, .hooks.on_container_stopped[0].path) |= $$y' net-container-server-template.json > net-container-server.json
	@jq --arg x $(shell pwd)/rootfs/net --arg y $(shell pwd)/hooks/bin/net-hook '.root.path = $$x | (.hooks.on_runtime_create[0].path) |= $$y' net-container-client-template.json > net-container-client.json
