#include "fat32.hpp"
#include "drivers/ata.hpp"
#include "terminal.hpp"

extern Terminal* global_term;

namespace fat32 {

static bpb_t current_bpb;
static bool is_init = false;

bool format(uint32_t total_sectors, const char* label, bool slave) {
    uint16_t buffer[256];
    for(int i=0; i<256; i++) buffer[i] = 0;

    bpb_t* bpb = (bpb_t*)buffer;
    bpb->jump[0] = 0xEB; bpb->jump[1] = 0x3C; bpb->jump[2] = 0x90;
    for(int i=0; i<8; i++) bpb->oem[i] = 'G';
    bpb->bytes_per_sector = 512;
    bpb->sectors_per_cluster = 8; // 4KB clusters
    bpb->reserved_sectors = 32;
    bpb->fat_count = 2;
    bpb->media_type = 0xF8;
    bpb->total_sectors_32 = total_sectors;
    
    uint32_t fat_size = (total_sectors / 8 * 4) / 512 + 1;
    bpb->sectors_per_fat_32 = fat_size;
    bpb->root_cluster = 2;
    bpb->boot_signature = 0x29;
    for(int i=0; i<11; i++) bpb->volume_label[i] = label[i] ? label[i] : ' ';
    for(int i=0; i<8; i++) bpb->fs_type[i] = "FAT32   "[i];
  
    buffer[255] = 0xAA55;

    // Write boot sector
    if (!ata::write_sectors(0, 1, buffer, slave)) return false;

    // Clear FATs and Reserved sectors (simple version)
    for(int i=0; i<256; i++) buffer[i] = 0;
    for(uint32_t i=1; i < bpb->reserved_sectors + (bpb->fat_count * fat_size); i++) {
        if (!ata::write_sectors(i, 1, buffer, slave)) return false;
    }

    // Init first few FAT entries
    // FAT32 entries are 32-bit
    uint32_t* fat = (uint32_t*)buffer;
    fat[0] = 0x0FFFFFF8; // Media type
    fat[1] = 0x0FFFFFFF; // EOC
    fat[2] = 0x0FFFFFFF; // Root directory EOC
    
    if (!ata::write_sectors(bpb->reserved_sectors, 1, buffer, slave)) return false;
    
    if (global_term) global_term->print("Disk formatted successfully.\n");
    return true;
}

bool init(bool slave) {
    uint16_t buffer[256];
    if (!ata::read_sectors(0, 1, buffer, slave)) return false;
    bpb_t* bpb = (bpb_t*)buffer;
    
    if (bpb->boot_signature != 0x29) return false;
    
    current_bpb = *bpb;
    is_init = true;
    return true;
}

// Placeholder for full file IO - will be expanded if needed
void* read_file(const char* filename, size_t* out_size) { (void)filename; (void)out_size; return nullptr; }
bool write_file(const char* filename, void* data, size_t size) { (void)filename; (void)data; (void)size; return false; }

}
