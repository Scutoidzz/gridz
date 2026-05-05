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

    // Boot signature
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

bool is_initialized() { return is_init; }
uint32_t root_cluster() { return is_init ? current_bpb.root_cluster : 2; }

static uint32_t cluster_to_lba(uint32_t cluster) {
    uint32_t data_start = current_bpb.reserved_sectors +
                          current_bpb.fat_count * current_bpb.sectors_per_fat_32;
    return data_start + (cluster - 2) * current_bpb.sectors_per_cluster;
}

static uint32_t next_cluster(uint32_t cluster, bool slave) {
    uint32_t fat_byte   = cluster * 4;
    uint32_t fat_sector = current_bpb.reserved_sectors + fat_byte / 512;
    uint32_t fat_index  = (fat_byte % 512) / 4;

    uint16_t buf[256];
    if (!ata::read_sectors(fat_sector, 1, buf, slave)) return 0x0FFFFFFF;
    return ((uint32_t*)buf)[fat_index] & 0x0FFFFFFF;
}

// Static sector buffer: up to 8 sectors (one cluster) = 4 KB
static uint8_t dir_sector_buf[8 * 512];

int list_directory(uint32_t cluster, DirInfo* out, int max_out, bool slave) {
    if (!is_init || max_out <= 0) return 0;

    int count = 0;
    uint32_t cur = cluster;

    while (cur < 0x0FFFFFF8 && count < max_out) {
        uint32_t lba = cluster_to_lba(cur);
        uint8_t  spc = current_bpb.sectors_per_cluster;
        if (spc > 8) spc = 8;

        for (uint8_t s = 0; s < spc; s++) {
            if (!ata::read_sectors(lba + s, 1,
                    (uint16_t*)(dir_sector_buf + s * 512), slave))
                return count;
        }

        int entries_in_cluster = (spc * 512) / 32;
        bool eod = false;

        for (int i = 0; i < entries_in_cluster && count < max_out; i++) {
            directory_entry_t* e =
                (directory_entry_t*)(dir_sector_buf + i * 32);

            if ((uint8_t)e->name[0] == 0x00) { eod = true; break; }
            if ((uint8_t)e->name[0] == 0xE5) continue; // deleted
            if (e->attr == 0x0F)              continue; // LFN
            if (e->attr & 0x08)               continue; // volume label

            DirInfo& info = out[count];

            // Build 8.3 display name
            int pos = 0;
            for (int j = 0; j < 8 && e->name[j] != ' '; j++)
                info.name[pos++] = e->name[j];
            if (e->name[8] != ' ') {
                info.name[pos++] = '.';
                for (int j = 8; j < 11 && e->name[j] != ' '; j++)
                    info.name[pos++] = e->name[j];
            }
            info.name[pos] = '\0';

            info.size    = e->size;
            info.is_dir  = (e->attr & 0x10) != 0;
            info.cluster = ((uint32_t)e->first_cluster_high << 16) |
                            e->first_cluster_low;
            count++;
        }

        if (eod) break;
        cur = next_cluster(cur, slave);
    }

    return count;
}

// Convert a filename like "GAME.PY" to FAT 8.3 format: "GAME    PY "
static void to_83(const char* filename, char out[11]) {
    for (int i = 0; i < 11; i++) out[i] = ' ';

    // Skip leading path components
    const char* base = filename;
    for (const char* p = filename; *p; p++)
        if (*p == '/' || *p == '\\') base = p + 1;

    int i = 0;
    for (; i < 8 && base[i] && base[i] != '.'; i++) {
        char c = base[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        out[i] = c;
    }
    // Find extension
    const char* dot = nullptr;
    for (const char* p = base; *p; p++) if (*p == '.') dot = p;
    if (dot) {
        int e = 0;
        for (const char* p = dot + 1; *p && e < 3; p++, e++) {
            char c = *p;
            if (c >= 'a' && c <= 'z') c -= 32;
            out[8 + e] = c;
        }
    }
}

// Walk a directory cluster chain looking for a file with the given 8.3 name.
// Returns the directory entry if found, zeroed otherwise.
static directory_entry_t find_in_dir(uint32_t cluster, const char name83[11], bool slave) {
    directory_entry_t result = {};
    uint32_t cur = cluster;

    while (cur < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(cur);
        uint8_t  spc = current_bpb.sectors_per_cluster;
        if (spc > 8) spc = 8;

        for (uint8_t s = 0; s < spc; s++)
            if (!ata::read_sectors(lba + s, 1, (uint16_t*)(dir_sector_buf + s * 512), slave))
                return result;

        int entries = (spc * 512) / 32;
        for (int i = 0; i < entries; i++) {
            directory_entry_t* e = (directory_entry_t*)(dir_sector_buf + i * 32);
            if ((uint8_t)e->name[0] == 0x00) return result; // end of dir
            if ((uint8_t)e->name[0] == 0xE5) continue;      // deleted
            if (e->attr == 0x0F)              continue;      // LFN
            if (e->attr & 0x08)               continue;      // volume label
            bool match = true;
            for (int j = 0; j < 11; j++)
                if (e->name[j] != name83[j]) { match = false; break; }
            if (match) { result = *e; return result; }
        }
        cur = next_cluster(cur, slave);
    }
    return result;
}

extern "C" void* malloc(size_t);

void* read_file(const char* filename, size_t* out_size) {
    if (!is_init) return nullptr;

    // Walk path components: "python/guess.py" → find "PYTHON  " dir, then "GUESS   PY "
    uint32_t dir_cluster = current_bpb.root_cluster;
    const char* p = filename;
    // Skip leading slash
    if (*p == '/') p++;

    while (true) {
        // Find next '/'
        const char* slash = p;
        while (*slash && *slash != '/') slash++;
        if (*slash == '/') {
            // This component is a directory name
            char comp83[11];
            // Temporarily null-terminate
            char tmp[13] = {};
            int len = (int)(slash - p);
            if (len > 12) len = 12;
            for (int i = 0; i < len; i++) tmp[i] = p[i];
            to_83(tmp, comp83);
            directory_entry_t de = find_in_dir(dir_cluster, comp83, false);
            if (!(de.attr & 0x10)) return nullptr; // not a directory
            dir_cluster = ((uint32_t)de.first_cluster_high << 16) | de.first_cluster_low;
            p = slash + 1;
        } else {
            break; // p now points to the filename
        }
    }

    char name83[11];
    to_83(p, name83);
    directory_entry_t entry = find_in_dir(dir_cluster, name83, false);

    if (entry.size == 0 && entry.first_cluster_low == 0 && entry.first_cluster_high == 0)
        return nullptr; // not found

    size_t size = entry.size;
    if (out_size) *out_size = size;
    if (size == 0) return nullptr;

    uint8_t* buf = (uint8_t*)malloc(size + 1);
    if (!buf) return nullptr;
    buf[size] = 0; // null-terminate for text files

    uint32_t cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    size_t   written = 0;
    uint8_t  spc     = current_bpb.sectors_per_cluster;
    if (spc > 8) spc = 8;
    size_t cluster_bytes = (size_t)spc * 512;

    static uint8_t file_sector_buf[8 * 512];

    while (cluster < 0x0FFFFFF8 && written < size) {
        uint32_t lba = cluster_to_lba(cluster);
        for (uint8_t s = 0; s < spc; s++)
            if (!ata::read_sectors(lba + s, 1, (uint16_t*)(file_sector_buf + s * 512), false))
                goto done;

        size_t to_copy = cluster_bytes;
        if (written + to_copy > size) to_copy = size - written;
        for (size_t b = 0; b < to_copy; b++) buf[written + b] = file_sector_buf[b];
        written += to_copy;
        cluster = next_cluster(cluster, false);
    }
done:
    return buf;
}

bool write_file(const char* filename, void* data, size_t size) { (void)filename; (void)data; (void)size; return false; }

} // namespace fat32

// C-linkage wrappers for the MicroPython C module
extern "C" {
    int fat32_ensure_init(void) {
        if (fat32::is_initialized()) return 1;
        return (fat32::init(false) || fat32::init(true)) ? 1 : 0;
    }
    void* fat32_read_file(const char* filename, size_t* out_size) {
        fat32_ensure_init();
        return fat32::read_file(filename, out_size);
    }
    int fat32_list_directory(uint32_t cluster, void* out, int max_out, int slave) {
        return fat32::list_directory(cluster, (fat32::DirInfo*)out, max_out, slave != 0);
    }
    uint32_t fat32_root_cluster(void) {
        return fat32::root_cluster();
    }
    int fat32_is_initialized(void) {
        return fat32_ensure_init();
    }
}
