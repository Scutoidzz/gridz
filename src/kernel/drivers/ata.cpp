#include "ata.hpp"
#include "io.hpp"

extern "C" void serial_print(const char* s);
extern "C" void itoa(uint64_t n, char* s, int base);

namespace ata {

void io_wait() {
    for (int i = 0; i < 4; i++) inb(ATA_PRIMARY_STATUS);
}

bool wait_bsy() {
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint8_t status = inb(ATA_PRIMARY_STATUS);
        if (status == 0xFF) return false; // Floating bus
        if (!(status & 0x80)) return true;
    }
    return false;
}

bool wait_drq() {
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint8_t status = inb(ATA_PRIMARY_STATUS);
        if (status == 0xFF) return false;
        if (!(status & 0x80) && (status & 0x08)) return true;
        if (status & 0x01) return false; // Error
        if (status & 0x20) return false; // Drive Fault
    }
    return false;
}

bool exists(bool slave) {
    uint8_t sel = slave ? 0xB0 : 0xA0;
    outb(ATA_PRIMARY_DRIVE_SEL, sel);
    for(int i=0; i<10; i++) io_wait(); // Wait a bit longer for selection
    
    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0xFF) return false; // Floating bus
    if (status == 0x00) return false; // No drive on this port
    
    return true;
}

bool identify(uint16_t* buffer, bool slave) {
    if (!exists(slave)) return false;
    outb(ATA_PRIMARY_DRIVE_SEL, slave ? 0xB0 : 0xA0);
    outb(ATA_PRIMARY_COMMAND, 0xEC);
    
    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0) return false;
    
    if (!wait_bsy()) {
        serial_print("[ATA] Identify: Timeout waiting for BSY\n");
        return false;
    }
    
    // Some drives under emulation return non-zero signatures after 0xEC
    // but still provide valid ATA IDENTIFY data. We'll be more permissive.
    
    if (!wait_drq()) {
        uint8_t s = inb(ATA_PRIMARY_STATUS);
        if (s & 0x01) {
            // Error bit set, might be ATAPI. Try 0xA1?
            // For now just skip.
            return false;
        }
        serial_print("[ATA] Identify: Timeout waiting for DRQ\n");
        return false;
    }
    
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_PRIMARY_DATA);
    }

    // Print model name for debug
    char model[41];
    for (int i = 0; i < 20; i++) {
        uint16_t word = buffer[27 + i];
        model[i * 2] = (char)(word >> 8);
        model[i * 2 + 1] = (char)(word & 0xFF);
    }
    model[40] = '\0';
    serial_print("[ATA] Model: "); serial_print(model); serial_print("\n");

    return true;
}

bool read_sectors(uint32_t lba, uint8_t count, uint16_t* buffer, bool slave) {
    if (!exists(slave)) return false;
    if (!wait_bsy()) return false;
    outb(ATA_PRIMARY_DRIVE_SEL, (slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F));
    io_wait();
    outb(ATA_PRIMARY_SECCOUNT, count);
    outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x20); // Read sectors
    io_wait();

    for (int j = 0; j < (count == 0 ? 256 : count); j++) {
        if (!wait_bsy()) return false;
        if (!wait_drq()) return false;
        for (int i = 0; i < 256; i++) {
            buffer[i + j * 256] = inw(ATA_PRIMARY_DATA);
        }
    }
    return true;
}

bool write_sectors(uint32_t lba, uint8_t count, uint16_t* buffer, bool slave) {
    if (!exists(slave)) return false;
    if (!wait_bsy()) return false;
    outb(ATA_PRIMARY_DRIVE_SEL, (slave ? 0xF0 : 0xE0) | ((lba >> 24) & 0x0F));
    io_wait();
    outb(ATA_PRIMARY_SECCOUNT, count);
    outb(ATA_PRIMARY_LBA_LOW, (uint8_t)lba);
    outb(ATA_PRIMARY_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_PRIMARY_LBA_HIGH, (uint8_t)(lba >> 16));
    outb(ATA_PRIMARY_COMMAND, 0x30); // Write sectors
    io_wait();

    for (int j = 0; j < (count == 0 ? 256 : count); j++) {
        if (!wait_bsy()) return false;
        if (!wait_drq()) return false;
        for (int i = 0; i < 256; i++) {
            outw(ATA_PRIMARY_DATA, buffer[i + j * 256]);
        }
    }
    
    // Cache flush
    outb(ATA_PRIMARY_COMMAND, 0xE7);
    wait_bsy();
    
    return true;
}

}
