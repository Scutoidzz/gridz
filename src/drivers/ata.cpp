#include "ata.hpp"
#include "../io.hpp"

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
    outb(ATA_PRIMARY_DRIVE_SEL, slave ? 0xB0 : 0xA0);
    io_wait();
    outb(ATA_PRIMARY_SECCOUNT, 0x55);
    outb(ATA_PRIMARY_LBA_LOW, 0xAA);
    if (inb(ATA_PRIMARY_SECCOUNT) != 0x55) return false;
    if (inb(ATA_PRIMARY_LBA_LOW) != 0xAA) return false;
    
    uint8_t status = inb(ATA_PRIMARY_STATUS);
    if (status == 0xFF) return false;
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
