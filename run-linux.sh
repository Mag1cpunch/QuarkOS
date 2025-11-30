#!/bin/bash

sudo docker run --rm -it -v "$PWD:/src" quarkos-buildenv bash -c "make"

qemu-system-x86_64 \
  -cdrom output/QuarkOS.iso \
  -m 512M \
  -serial mon:stdio \
  -device virtio-net,netdev=net0 \
  -netdev user,id=net0 \
  -machine q35,accel=kvm \
  -cpu host \
  -smp 4 \
  -enable-kvm \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/ovmf/OVMF.fd

