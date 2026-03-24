#!/bin/bash
set -e
# call makefile to compile
make clean
make progs # make user programs
make -j$(nproc) 

# assumes limine is built and executable at ./limine/limine

# Create a directory which will be our ISO root.
mkdir -p iso_root

# Copy the relevant files over.
mkdir -p iso_root/boot
cp -v bin/LykOS iso_root/boot/
mkdir -p iso_root/boot/limine
cp -v limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin \
      limine/limine-uefi-cd.bin iso_root/boot/limine/

# Create the EFI boot tree and copy Limine's EFI executables over.
mkdir -p iso_root/EFI/BOOT
cp -v limine/BOOTX64.EFI iso_root/EFI/BOOT/
cp -v limine/BOOTIA32.EFI iso_root/EFI/BOOT/

# Create the bootable ISO.
xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        iso_root -o image.iso

# Install Limine stage 1 and 2 for legacy BIOS boot.
./limine/limine bios-install image.iso

DEBUG_FLAGS=""
if [[ "$1" == "dbg" ]]; then
    DEBUG_FLAGS="-s -S"
fi

ISO="image.iso"
DISK="fat16.img"
RAM="4G"

QEMU_CMD=(
    qemu-system-x86_64
    -m "$RAM"
    -cdrom "$ISO"
    -boot d
    -d strace,int
    -no-reboot
    -D qemu.lg
    -serial file:serial.lg #-serial stdio
    $DEBUG_FLAGS
    -hda "$DISK"
    #-drive if=pflash,format=raw,file=OVMF.4m.fd # UEFI
)
# -d int,cpu,guest_errors
# need to diswon and & for gdb and clion
if [[ $DEBUG_FLAGS ]]; then
    "${QEMU_CMD[@]}" &
        disown
else
    "${QEMU_CMD[@]}"
fi
