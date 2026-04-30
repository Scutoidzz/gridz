# Physical Memory Management (PMM)

Physical memory management involves tracking which pages of physical RAM are available and which are in use. For an x86_64 kernel using the sboot protocol, the common approach is a **Bitmap Allocator**.

## 1. Bitmap Allocator Concept
A bitmap uses 1 bit to represent the status of a single 4KiB page:
- `0`: Page is free
- `1`: Page is used

For a system with 4GiB of RAM, you would need:
- $4 \times 1024^3$ bytes / 4096 bytes/page = $1,048,576$ pages.
- $1,048,576$ bits / 8 bits/byte = $131,072$ bytes (128 KiB) for the bitmap.

## 2. sboot Memory Map
sboot provides a memory map describing usable and reserved regions.
- **`sboot_MEMMAP_USABLE`**: RAM you can use.
- **`sboot_MEMMAP_RESERVED`**: BIOS/UEFI reserved areas.
- **`sboot_MEMMAP_ACPI_*`**: ACPI data.
- **`sboot_MEMMAP_KERNEL_AND_MODULES`**: Where your code is.

## 3. Implementation Steps
1.  **Request Memmap & HHDM**: Use sboot requests to get the map and the physical memory offset.
2.  **Find Highest Address**: Iterate through the map to find the total physical RAM.
3.  **Setup Bitmap**: Reserve a region of usable RAM for the bitmap itself.
4.  **Mark Usable RAM**: Initially mark all as used, then unset bits for all `sboot_MEMMAP_USABLE` regions.
5.  **Reserve Kernel & Modules**: Ensure the areas used by the kernel and modules are marked as used.

## 4. Useful Links
- [OSDev Wiki: Page Frame Allocation](https://wiki.osdev.org/Page_Frame_Allocation)
- [sboot Protocol Specification](https://github.com/sboot-bootloader/sboot/blob/v3.0-branch/PROTOCOL.md)
