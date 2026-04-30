

#ifndef sboot_H
#define sboot_H 1

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#ifdef sboot_NO_POINTERS
#  define sboot_PTR(TYPE) uint64_t
#else
#  define sboot_PTR(TYPE) TYPE
#endif

#ifndef sboot_API_REVISION
#  define sboot_API_REVISION 0
#endif

#if sboot_API_REVISION > 2
#  error "sboot.h API revision unsupported"
#endif

#ifdef __GNUC__
#  define sboot_DEPRECATED __attribute__((__deprecated__))
#  define sboot_DEPRECATED_IGNORE_START \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#  define sboot_DEPRECATED_IGNORE_END \
    _Pragma("GCC diagnostic pop")
#else
#  define sboot_DEPRECATED
#  define sboot_DEPRECATED_IGNORE_START
#  define sboot_DEPRECATED_IGNORE_END
#endif

#define sboot_REQUESTS_START_MARKER \
    uint64_t sboot_requests_start_marker[4] = { 0xf6b8f4b39de7d1ae, 0xfab91a6940fcb9cf, \
                                                 0x785c6ed015d3e316, 0x181e920a7852b9d9 };
#define sboot_REQUESTS_END_MARKER \
    uint64_t sboot_requests_end_marker[2] = { 0xadc0e0531bb10d03, 0x9572709f31764c62 };

#define sboot_REQUESTS_DELIMITER sboot_REQUESTS_END_MARKER

#define sboot_BASE_REVISION(N) \
    uint64_t sboot_base_revision[3] = { 0xf9562b2d5c95a6c8, 0x6a7b384944536bdc, (N) };

#define sboot_BASE_REVISION_SUPPORTED (sboot_base_revision[2] == 0)

#define sboot_LOADED_BASE_REV_VALID (sboot_base_revision[1] != 0x6a7b384944536bdc)
#define sboot_LOADED_BASE_REVISION (sboot_base_revision[1])

#define sboot_COMMON_MAGIC 0xc7b1dd30df4c8b88, 0x0a82e883a194f07b

struct sboot_uuid {
    uint32_t a;
    uint16_t b;
    uint16_t c;
    uint8_t d[8];
};

#define sboot_MEDIA_TYPE_GENERIC 0
#define sboot_MEDIA_TYPE_OPTICAL 1
#define sboot_MEDIA_TYPE_TFTP 2

struct sboot_file {
    uint64_t revision;
    sboot_PTR(void *) address;
    uint64_t size;
    sboot_PTR(char *) path;
    sboot_PTR(char *) cmdline;
    uint32_t media_type;
    uint32_t unused;
    uint32_t tftp_ip;
    uint32_t tftp_port;
    uint32_t partition_index;
    uint32_t mbr_disk_id;
    struct sboot_uuid gpt_disk_uuid;
    struct sboot_uuid gpt_part_uuid;
    struct sboot_uuid part_uuid;
};

/* Boot info */

#define sboot_BOOTLOADER_INFO_REQUEST { sboot_COMMON_MAGIC, 0xf55038d8e2a1202f, 0x279426fcf5f59740 }

struct sboot_bootloader_info_response {
    uint64_t revision;
    sboot_PTR(char *) name;
    sboot_PTR(char *) version;
};

struct sboot_bootloader_info_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_bootloader_info_response *) response;
};


#define sboot_FIRMWARE_TYPE_REQUEST { sboot_COMMON_MAGIC, 0x8c2f75d90bef28a8, 0x7045a4688eac00c3 }

#define sboot_FIRMWARE_TYPE_X86BIOS 0
#define sboot_FIRMWARE_TYPE_UEFI32 1
#define sboot_FIRMWARE_TYPE_UEFI64 2
#define sboot_FIRMWARE_TYPE_SBI 3

struct sboot_firmware_type_response {
    uint64_t revision;
    uint64_t firmware_type;
};

struct sboot_firmware_type_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_firmware_type_response *) response;
};


#define sboot_STACK_SIZE_REQUEST { sboot_COMMON_MAGIC, 0x224ef0460a8e8926, 0xe1cb0fc25f46ea3d }

struct sboot_stack_size_response {
    uint64_t revision;
};

struct sboot_stack_size_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_stack_size_response *) response;
    uint64_t stack_size;
};

/* HHDM */

#define sboot_HHDM_REQUEST { sboot_COMMON_MAGIC, 0x48dcf1cb8ad2b852, 0x63984e959a98244b }

struct sboot_hhdm_response {
    uint64_t revision;
    uint64_t offset;
};

struct sboot_hhdm_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_hhdm_response *) response;
};

/* Framebuffer */

#define sboot_FRAMEBUFFER_REQUEST { sboot_COMMON_MAGIC, 0x9d5827dcd881dd75, 0xa3148604f6fab11b }

#define sboot_FRAMEBUFFER_RGB 1

struct sboot_video_mode {
    uint64_t pitch;
    uint64_t width;
    uint64_t height;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
};

struct sboot_framebuffer {
    sboot_PTR(void *) address;
    uint64_t width;
    uint64_t height;
    uint64_t pitch;
    uint16_t bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
    uint8_t unused[7];
    uint64_t edid_size;
    sboot_PTR(void *) edid;
    /* Response revision 1 */
    uint64_t mode_count;
    sboot_PTR(struct sboot_video_mode **) modes;
};

struct sboot_framebuffer_response {
    uint64_t revision;
    uint64_t framebuffer_count;
    sboot_PTR(struct sboot_framebuffer **) framebuffers;
};

struct sboot_framebuffer_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_framebuffer_response *) response;
};

/* Terminal */

#define sboot_TERMINAL_REQUEST { sboot_COMMON_MAGIC, 0xc8ac59310c2b0844, 0xa68d0c7265d38878 }

#define sboot_TERMINAL_CB_DEC 10
#define sboot_TERMINAL_CB_BELL 20
#define sboot_TERMINAL_CB_PRIVATE_ID 30
#define sboot_TERMINAL_CB_STATUS_REPORT 40
#define sboot_TERMINAL_CB_POS_REPORT 50
#define sboot_TERMINAL_CB_KBD_LEDS 60
#define sboot_TERMINAL_CB_MODE 70
#define sboot_TERMINAL_CB_LINUX 80

#define sboot_TERMINAL_CTX_SIZE ((uint64_t)(-1))
#define sboot_TERMINAL_CTX_SAVE ((uint64_t)(-2))
#define sboot_TERMINAL_CTX_RESTORE ((uint64_t)(-3))
#define sboot_TERMINAL_FULL_REFRESH ((uint64_t)(-4))

/* Response revision 1 */
#define sboot_TERMINAL_OOB_OUTPUT_GET ((uint64_t)(-10))
#define sboot_TERMINAL_OOB_OUTPUT_SET ((uint64_t)(-11))

#define sboot_TERMINAL_OOB_OUTPUT_OCRNL (1 << 0)
#define sboot_TERMINAL_OOB_OUTPUT_OFDEL (1 << 1)
#define sboot_TERMINAL_OOB_OUTPUT_OFILL (1 << 2)
#define sboot_TERMINAL_OOB_OUTPUT_OLCUC (1 << 3)
#define sboot_TERMINAL_OOB_OUTPUT_ONLCR (1 << 4)
#define sboot_TERMINAL_OOB_OUTPUT_ONLRET (1 << 5)
#define sboot_TERMINAL_OOB_OUTPUT_ONOCR (1 << 6)
#define sboot_TERMINAL_OOB_OUTPUT_OPOST (1 << 7)

sboot_DEPRECATED_IGNORE_START

struct sboot_DEPRECATED sboot_terminal;

typedef void (*sboot_terminal_write)(struct sboot_terminal *, const char *, uint64_t);
typedef void (*sboot_terminal_callback)(struct sboot_terminal *, uint64_t, uint64_t, uint64_t, uint64_t);

struct sboot_DEPRECATED sboot_terminal {
    uint64_t columns;
    uint64_t rows;
    sboot_PTR(struct sboot_framebuffer *) framebuffer;
};

struct sboot_DEPRECATED sboot_terminal_response {
    uint64_t revision;
    uint64_t terminal_count;
    sboot_PTR(struct sboot_terminal **) terminals;
    sboot_PTR(sboot_terminal_write) write;
};

struct sboot_DEPRECATED sboot_terminal_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_terminal_response *) response;
    sboot_PTR(sboot_terminal_callback) callback;
};

sboot_DEPRECATED_IGNORE_END

/* Paging mode */

#define sboot_PAGING_MODE_REQUEST { sboot_COMMON_MAGIC, 0x95c1a0edab0944cb, 0xa4e5cb3842f7488a }

#if defined (__x86_64__) || defined (__i386__)
#define sboot_PAGING_MODE_X86_64_4LVL 0
#define sboot_PAGING_MODE_X86_64_5LVL 1
#define sboot_PAGING_MODE_MIN sboot_PAGING_MODE_X86_64_4LVL
#define sboot_PAGING_MODE_DEFAULT sboot_PAGING_MODE_X86_64_4LVL
#elif defined (__aarch64__)
#define sboot_PAGING_MODE_AARCH64_4LVL 0
#define sboot_PAGING_MODE_AARCH64_5LVL 1
#define sboot_PAGING_MODE_MIN sboot_PAGING_MODE_AARCH64_4LVL
#define sboot_PAGING_MODE_DEFAULT sboot_PAGING_MODE_AARCH64_4LVL
#elif defined (__riscv) && (__riscv_xlen == 64)
#define sboot_PAGING_MODE_RISCV_SV39 0
#define sboot_PAGING_MODE_RISCV_SV48 1
#define sboot_PAGING_MODE_RISCV_SV57 2
#define sboot_PAGING_MODE_MIN sboot_PAGING_MODE_RISCV_SV39
#define sboot_PAGING_MODE_DEFAULT sboot_PAGING_MODE_RISCV_SV48
#elif defined (__loongarch__) && (__loongarch_grlen == 64)
#define sboot_PAGING_MODE_LOONGARCH64_4LVL 0
#define sboot_PAGING_MODE_MIN sboot_PAGING_MODE_LOONGARCH64_4LVL
#define sboot_PAGING_MODE_DEFAULT sboot_PAGING_MODE_LOONGARCH64_4LVL
#else
#error Unknown architecture
#endif

struct sboot_paging_mode_response {
    uint64_t revision;
    uint64_t mode;
};

struct sboot_paging_mode_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_paging_mode_response *) response;
    uint64_t mode;
    uint64_t max_mode;
    uint64_t min_mode;
};

/* 5-level paging */

#define sboot_5_LEVEL_PAGING_REQUEST { sboot_COMMON_MAGIC, 0x94469551da9b3192, 0xebe5e86db7382888 }

sboot_DEPRECATED_IGNORE_START

struct sboot_DEPRECATED sboot_5_level_paging_response {
    uint64_t revision;
};

struct sboot_DEPRECATED sboot_5_level_paging_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_5_level_paging_response *) response;
};

sboot_DEPRECATED_IGNORE_END

/* MP */

#if sboot_API_REVISION >= 1
#  define sboot_MP_REQUEST { sboot_COMMON_MAGIC, 0x95a67b819a1b857e, 0xa0b61b723b6a73e0 }
#  define sboot_MP(TEXT) sboot_mp_##TEXT
#else
#  define sboot_SMP_REQUEST { sboot_COMMON_MAGIC, 0x95a67b819a1b857e, 0xa0b61b723b6a73e0 }
#  define sboot_MP(TEXT) sboot_smp_##TEXT
#endif

struct sboot_MP(info);

typedef void (*sboot_goto_address)(struct sboot_MP(info) *);

#if defined (__x86_64__) || defined (__i386__)

#if sboot_API_REVISION >= 1
#  define sboot_MP_X2APIC (1 << 0)
#else
#  define sboot_SMP_X2APIC (1 << 0)
#endif

struct sboot_MP(info) {
    uint32_t processor_id;
    uint32_t lapic_id;
    uint64_t reserved;
    sboot_PTR(sboot_goto_address) goto_address;
    uint64_t extra_argument;
};

struct sboot_MP(response) {
    uint64_t revision;
    uint32_t flags;
    uint32_t bsp_lapic_id;
    uint64_t cpu_count;
    sboot_PTR(struct sboot_MP(info) **) cpus;
};

#elif defined (__aarch64__)

struct sboot_MP(info) {
    uint32_t processor_id;
    uint32_t reserved1;
    uint64_t mpidr;
    uint64_t reserved;
    sboot_PTR(sboot_goto_address) goto_address;
    uint64_t extra_argument;
};

struct sboot_MP(response) {
    uint64_t revision;
    uint64_t flags;
    uint64_t bsp_mpidr;
    uint64_t cpu_count;
    sboot_PTR(struct sboot_MP(info) **) cpus;
};

#elif defined (__riscv) && (__riscv_xlen == 64)

struct sboot_MP(info) {
    uint64_t processor_id;
    uint64_t hartid;
    uint64_t reserved;
    sboot_PTR(sboot_goto_address) goto_address;
    uint64_t extra_argument;
};

struct sboot_MP(response) {
    uint64_t revision;
    uint64_t flags;
    uint64_t bsp_hartid;
    uint64_t cpu_count;
    sboot_PTR(struct sboot_MP(info) **) cpus;
};

#elif defined (__loongarch__) && (__loongarch_grlen == 64)

struct sboot_MP(info) {
    uint64_t reserved;
};

struct sboot_MP(response) {
    uint64_t cpu_count;
    sboot_PTR(struct sboot_MP(info) **) cpus;
};

#else
#error Unknown architecture
#endif

struct sboot_MP(request) {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_MP(response) *) response;
    uint64_t flags;
};

/* Memory map */

#define sboot_MEMMAP_REQUEST { sboot_COMMON_MAGIC, 0x67cf3d9d378a806f, 0xe304acdfc50c3c62 }

#define sboot_MEMMAP_USABLE                 0
#define sboot_MEMMAP_RESERVED               1
#define sboot_MEMMAP_ACPI_RECLAIMABLE       2
#define sboot_MEMMAP_ACPI_NVS               3
#define sboot_MEMMAP_BAD_MEMORY             4
#define sboot_MEMMAP_BOOTLOADER_RECLAIMABLE 5
#if sboot_API_REVISION >= 2
#  define sboot_MEMMAP_EXECUTABLE_AND_MODULES 6
#else
#  define sboot_MEMMAP_KERNEL_AND_MODULES 6
#endif
#define sboot_MEMMAP_FRAMEBUFFER            7

struct sboot_memmap_entry {
    uint64_t base;
    uint64_t length;
    uint64_t type;
};

struct sboot_memmap_response {
    uint64_t revision;
    uint64_t entry_count;
    sboot_PTR(struct sboot_memmap_entry **) entries;
};

struct sboot_memmap_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_memmap_response *) response;
};

/* Entry point */

#define sboot_ENTRY_POINT_REQUEST { sboot_COMMON_MAGIC, 0x13d86c035a1cd3e1, 0x2b0caa89d8f3026a }

typedef void (*sboot_entry_point)(void);

struct sboot_entry_point_response {
    uint64_t revision;
};

struct sboot_entry_point_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_entry_point_response *) response;
    sboot_PTR(sboot_entry_point) entry;
};

/* Executable File */

#if sboot_API_REVISION >= 2
#  define sboot_EXECUTABLE_FILE_REQUEST { sboot_COMMON_MAGIC, 0xad97e90e83f1ed67, 0x31eb5d1c5ff23b69 }
#else
#  define sboot_KERNEL_FILE_REQUEST { sboot_COMMON_MAGIC, 0xad97e90e83f1ed67, 0x31eb5d1c5ff23b69 }
#endif

#if sboot_API_REVISION >= 2
struct sboot_executable_file_response {
#else
struct sboot_kernel_file_response {
#endif
    uint64_t revision;
#if sboot_API_REVISION >= 2
    sboot_PTR(struct sboot_file *) executable_file;
#else
    sboot_PTR(struct sboot_file *) kernel_file;
#endif
};

#if sboot_API_REVISION >= 2
struct sboot_executable_file_request {
#else
struct sboot_kernel_file_request {
#endif
    uint64_t id[4];
    uint64_t revision;
#if sboot_API_REVISION >= 2
    sboot_PTR(struct sboot_executable_file_response *) response;
#else
    sboot_PTR(struct sboot_kernel_file_response *) response;
#endif
};

/* Module */

#define sboot_MODULE_REQUEST { sboot_COMMON_MAGIC, 0x3e7e279702be32af, 0xca1c4f3bd1280cee }

#define sboot_INTERNAL_MODULE_REQUIRED (1 << 0)
#define sboot_INTERNAL_MODULE_COMPRESSED (1 << 1)

struct sboot_internal_module {
    sboot_PTR(const char *) path;
    sboot_PTR(const char *) cmdline;
    uint64_t flags;
};

struct sboot_module_response {
    uint64_t revision;
    uint64_t module_count;
    sboot_PTR(struct sboot_file **) modules;
};

struct sboot_module_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_module_response *) response;

    /* Request revision 1 */
    uint64_t internal_module_count;
    sboot_PTR(struct sboot_internal_module **) internal_modules;
};

/* RSDP */

#define sboot_RSDP_REQUEST { sboot_COMMON_MAGIC, 0xc5e77b6b397e7b43, 0x27637845accdcf3c }

struct sboot_rsdp_response {
    uint64_t revision;
#if sboot_API_REVISION >= 1
    uint64_t address;
#else
    sboot_PTR(void *) address;
#endif
};

struct sboot_rsdp_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_rsdp_response *) response;
};

/* SMBIOS */

#define sboot_SMBIOS_REQUEST { sboot_COMMON_MAGIC, 0x9e9046f11e095391, 0xaa4a520fefbde5ee }

struct sboot_smbios_response {
    uint64_t revision;
#if sboot_API_REVISION >= 1
    uint64_t entry_32;
    uint64_t entry_64;
#else
    sboot_PTR(void *) entry_32;
    sboot_PTR(void *) entry_64;
#endif
};

struct sboot_smbios_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_smbios_response *) response;
};

/* EFI system table */

#define sboot_EFI_SYSTEM_TABLE_REQUEST { sboot_COMMON_MAGIC, 0x5ceba5163eaaf6d6, 0x0a6981610cf65fcc }

struct sboot_efi_system_table_response {
    uint64_t revision;
#if sboot_API_REVISION >= 1
    uint64_t address;
#else
    sboot_PTR(void *) address;
#endif
};

struct sboot_efi_system_table_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_efi_system_table_response *) response;
};

/* EFI memory map */

#define sboot_EFI_MEMMAP_REQUEST { sboot_COMMON_MAGIC, 0x7df62a431d6872d5, 0xa4fcdfb3e57306c8 }

struct sboot_efi_memmap_response {
    uint64_t revision;
    sboot_PTR(void *) memmap;
    uint64_t memmap_size;
    uint64_t desc_size;
    uint64_t desc_version;
};

struct sboot_efi_memmap_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_efi_memmap_response *) response;
};

/* Boot time */

#define sboot_BOOT_TIME_REQUEST { sboot_COMMON_MAGIC, 0x502746e184c088aa, 0xfbc5ec83e6327893 }

struct sboot_boot_time_response {
    uint64_t revision;
    int64_t boot_time;
};

struct sboot_boot_time_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_boot_time_response *) response;
};

/* Executable address */

#if sboot_API_REVISION >= 2
#  define sboot_EXECUTABLE_ADDRESS_REQUEST { sboot_COMMON_MAGIC, 0x71ba76863cc55f63, 0xb2644a48c516a487 }
#else
#  define sboot_KERNEL_ADDRESS_REQUEST { sboot_COMMON_MAGIC, 0x71ba76863cc55f63, 0xb2644a48c516a487 }
#endif

#if sboot_API_REVISION >= 2
struct sboot_executable_address_response {
#else
struct sboot_kernel_address_response {
#endif
    uint64_t revision;
    uint64_t physical_base;
    uint64_t virtual_base;
};

#if sboot_API_REVISION >= 2
struct sboot_executable_address_request {
#else
struct sboot_kernel_address_request {
#endif
    uint64_t id[4];
    uint64_t revision;
#if sboot_API_REVISION >= 2
    sboot_PTR(struct sboot_executable_address_response *) response;
#else
    sboot_PTR(struct sboot_kernel_address_response *) response;
#endif
};

/* Device Tree Blob */

#define sboot_DTB_REQUEST { sboot_COMMON_MAGIC, 0xb40ddb48fb54bac7, 0x545081493f81ffb7 }

struct sboot_dtb_response {
    uint64_t revision;
    sboot_PTR(void *) dtb_ptr;
};

struct sboot_dtb_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_dtb_response *) response;
};

/* RISC-V Boot Hart ID */

#define sboot_RISCV_BSP_HARTID_REQUEST { sboot_COMMON_MAGIC, 0x1369359f025525f9, 0x2ff2a56178391bb6 }

struct sboot_riscv_bsp_hartid_response {
    uint64_t revision;
    uint64_t bsp_hartid;
};

struct sboot_riscv_bsp_hartid_request {
    uint64_t id[4];
    uint64_t revision;
    sboot_PTR(struct sboot_riscv_bsp_hartid_response *) response;
};

#ifdef __cplusplus
}
#endif

#endif
