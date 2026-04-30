#pragma once
#include <stdint.h>

namespace ata {

const uint16_t ATA_PRIMARY_DATA         = 0x1F0;
const uint16_t ATA_PRIMARY_ERR          = 0x1F1;
const uint16_t ATA_PRIMARY_SECCOUNT     = 0x1F2;
const uint16_t ATA_PRIMARY_LBA_LOW      = 0x1F3;
const uint16_t ATA_PRIMARY_LBA_MID      = 0x1F4;
const uint16_t ATA_PRIMARY_LBA_HIGH     = 0x1F5;
const uint16_t ATA_PRIMARY_DRIVE_SEL    = 0x1F6;
const uint16_t ATA_PRIMARY_COMMAND      = 0x1F7;
const uint16_t ATA_PRIMARY_STATUS       = 0x1F7;

bool wait_bsy();
bool exists(bool slave);
bool wait_drq();
bool read_sectors(uint32_t lba, uint8_t count, uint16_t* buffer, bool slave = false);
bool write_sectors(uint32_t lba, uint8_t count, uint16_t* buffer, bool slave = false);
bool identify(uint16_t* buffer, bool slave = false);

}
