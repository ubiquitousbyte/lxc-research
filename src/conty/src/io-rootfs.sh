#!/bin/bash

if [ $# -ne 2 ]; then
  echo "expected path to store root filesystem tar"
  exit 1
fi

out=$1
s=$2

if [ ! -f "io-rootfs.tar" ]; then
  docker export $(docker create xridge/fio) --output="io-rootfs.tar"
fi

if [ ! -d $out/$s ]; then
  mkdir -p $out/$s
fi

tar -xvf "io-rootfs.tar" -C $out/$s
if [ $? -ne 0 ]; then
  echo "cannot extract rootfs to $out/$s"
  exit 1
fi
