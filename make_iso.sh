mkdir -p iso_root/boot/limine
mkdir -p iso_root/EFI/BOOT
cp kernel.elf iso_root/boot/
cp limine-bin-repo/limine-bios.sys limine-bin-repo/limine-bios-cd.bin limine-bin-repo/limine-uefi-cd.bin iso_root/boot/limine/
cp limine-bin-repo/BOOTX64.EFI limine-bin-repo/BOOTIA32.EFI iso_root/EFI/BOOT/
xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    --efi-boot boot/limine/limine-uefi-cd.bin \
    -efi-boot-part --efi-boot-image --protective-msdos-label \
    iso_root -o gridz.iso
./limine_bin/limine bios-install gridz.iso
