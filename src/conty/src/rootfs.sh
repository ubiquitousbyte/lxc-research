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

# Now download golang and add it to each root filesystem

# We need to change arch because go uses a different scheme
if [ "$arch" = "x86_64" ]; then
  arch="amd64"
elif [ "$arch" = "aarch64" ]; then
  arch="arm64"
fi

if [ ! -f "$out/go1.19.linux-$arch.tar.gz" ]; then
  wget --directory-prefix=$out "https://go.dev./dl/go1.19.linux-$arch.tar.gz"
else
  echo "tar ball go1.19.linux-$arch.tar.gz already exists.. skipping download"
fi

tar -zxvf $out/go1.19.linux-$arch.tar.gz -C $out/net/usr/local
if [ $? -ne 0 ]; then
  echo "cannot extract golang to $out/net/usr/local"
  exit 1
fi

tar -zxvf $out/go1.19.linux-$arch.tar.gz -C $out/disk/usr/local
if [ $? -ne 0 ]; then
  echo "cannot extract golang to $out/disk/usr/local"
  exit 1
fi

tar -zxvf $out/go1.19.linux-$arch.tar.gz -C $out/cpu/usr/local
if [ $? -ne 0 ]; then
  echo "cannot extract golang to $out/cpu/usr/local"
  exit 1
fi
