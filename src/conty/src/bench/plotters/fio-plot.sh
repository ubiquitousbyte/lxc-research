#!/bin/bash


if [ $# -ne 2 ]; then
	echo "***************************************"
	echo "Usage: $0 <container_results_dir> <host_results_dir>"
	echo "***************************************"
fi

container_results_dir=$1
host_results_dir=$2

fio-plot -i "$container_results" --source "" -T "SanDisk microSD UHS-I on Pi 4 (Container)" -l -r read --output sandisk-libaio-read-queue-depth-iops-latency.png
fio-plot -i "$container_results"--source "" -T "SanDisk microSD UHS-I on Pi 4 (Container)" -N -r read --output sandisk-libaio-read-numjobs-iops-latency.png
fio-plot -i "$container_results" --source "" -T "SanDisk microSD UHS-I on Pi 4 (Container)" -l -r write --output sandisk-libaio-write-queue-depth-iops-latency.png
fio-plot -i "$container_results" --source "" -T "SanDisk microSD UHS-I on Pi 4 (Container)" -N -r write --output sandisk-libaio-write-num-jobs-iops-latency.png
fio-plot -i "$host_results_dir" --source "" -T "SanDisk microSD UHS-I on Pi 4" -l -r read --output sandisk-host-libaio-read-queue-depth-iops-latency.png
fio-plot -i "$host_results_dir" --source "" -T "SanDisk microSD UHS-I on Pi 4" -N -r read --output sandisk-host-libaio-read-numjobs-iops-latency.png
fio-plot -i "$host_results_dir" --source "" -T "SanDisk microSD UHS-I on Pi 4" -l -r write --output sandisk-host-libaio-write-queue-depth-iops-latency.png
fio-plot -i "$host_results_dir" --source "" -T "SanDisk microSD UHS-I on Pi 4" -N -r write --output sandisk-host-libaio-write-numjobs-iops-latency.png
fio-plot -i "$container_results" "$host_results_dir" --source "" -T "IOPS comparison over time" -g -t iops -r read --output sandisk-libaio-iops-read-comparison.png
fio-plot -i "$container_results" "$host_results_dir" --source "" -T "IOPS comparison over time" -g -t iops -r write --output sandisk-libaio-iops-read-comparison.png
fio-plot -i "$container_results" "$host_results_dir" --source "" -T "Latency comparison over time" -g -t lat -r read --output sandisk-libaio-latency-read-comparison.png
fio-plot -i "$container_results" "$host_results_dir" --source "" -T "Latency comparison over time" -g -t lat -r write --output sandisk-libaio-latency-write-comparison.png