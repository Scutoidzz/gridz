#pragma once
#include <stdint.h>
#include <stddef.h>

namespace fat32 {

struct bpb_t {
    uint8_t jump[3];
    char oem[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed));

struct directory_entry_t {
    char name[11];
    uint8_t attr;
    uint8_t nt_res;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t size;
} __attribute__((packed));

struct DirInfo {
    char     name[13]; // 8.3: "XXXXXXXX.XXX\0"
    uint32_t size;
    bool     is_dir;
    uint32_t cluster;
};

bool     format(uint32_t total_sectors, const char* label, bool slave = false);
bool     init(bool slave = false);
bool     is_initialized();
uint32_t root_cluster();
int      list_directory(uint32_t cluster, DirInfo* out, int max_out, bool slave = false);
void*    read_file(const char* filename, size_t* out_size);
bool     write_file(const char* filename, void* data, size_t size);

}
