docker run --rm -it -v %cd%:/src quarkos-buildenv bash -c make
qemu-system-x86_64 -cdrom output/QuarkOS.iso -m 512M -serial mon:stdio -device i8042 -device virtio-net -machine q35 -cpu Broadwell