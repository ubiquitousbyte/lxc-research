#!/bin/bash
# Do NOT run this script manually. Use cmake, it will do the heavy lifting

if [ $# -ne 1 ]; then
  echo "expected path to store root filesystem tar"
  exit 1
fi

out=$1

if [ ! -d $out ]; then
  mkdir -p $out
  mkdir $out/net
  mkdir $out/disk
  mkdir $out/cpu
fi

arch=$(uname -m)

url="https://dl-cdn.alpinelinux.org/alpine/v3.16/releases/$arch"
filename="alpine-minirootfs-3.16.2-$arch"
tar_uri="$url/$filename.tar.gz"

# Download the base root filesystem image from alpine cdn
if [ ! -f "$out/$filename.tar.gz" ]; then
  wget --directory-prefix=$out $tar_uri
  if [ $? -ne 0 ]; then
    echo "cannot download root filesystem $tar_uri"
    exit 1
  fi
else
  echo "tar ball $filename.tar.gz already exists.. skipping download"
fi

# Extract the image, once per workload
tar -zxvf $out/$filename.tar.gz -C $out/net
if [ $? -ne 0 ]; then
  echo "cannot extract rootfs to $out/net"
  exit 1
fi

tar -zxvf $out/$filename.tar.gz -C $out/disk
if [ $? -ne 0 ]; then
  echo "cannot extract rootfs to $out/disk"
  exit 1
fi

tar -zxvf $out/$filename.tar.gz -C $out/cpu
if [ $? -ne 0 ]; then
  echo "cannot extract rootfs to $out/cpu"
  exit 1
fi
