#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "misc/io.h"

typedef enum
{
    ATA_DRIVE_BUSY = 0x80,
    ATA_DRIVE_READY = 0x40,
    ATA_OPERATION_ERROR = 0x01,
    ATA_DATA_READY = 0x08
} ata_status_t;

#define SECTOR_SIZE 512
#define READ_SECTOR 0x20
#define WRITE_SECTOR 0x30

typedef enum
{
    MASTER,
    SLAVE
} disk_t;

typedef enum
{
    ATA_DATA = 0x1F0,
    ATA_STATUS = 0x1F7,
    ATA_COMMAND = 0x1F7
} ata_port_t;

// i always use lba
static void send_ata_command(uint32_t lba, bool drive, uint8_t sec_count, uint8_t command)
{
    outb(0x1F6, ((uint8_t)(lba >> 24) & 0x0F) | ((drive == MASTER) ? 0xE0 : 0xF0));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F3, (uint8_t)(lba));
    outb(0x1F2, sec_count);
    outb(ATA_COMMAND, command);
}

static void wait_for_disk_avl(uint8_t *status_byte)
{
    do
    {
        *status_byte = inb(ATA_STATUS);
    } while ((*status_byte & ATA_DRIVE_BUSY) || !(*status_byte & ATA_DRIVE_READY));
}

static bool poll_disk(uint8_t *status_byte, bool data_left)
{
    for (uint8_t i = 0; i < 4; ++i)
        inb(ATA_STATUS); // 4 dummy reads for 400 ns delay
    // ATA bus much slower than cpu we may check status byte before BSY is even set to 1 lol

    do
    {
        *status_byte = inb(ATA_STATUS);
    } while (*status_byte & ATA_DRIVE_BUSY);
    if (*status_byte & ATA_OPERATION_ERROR || (data_left ? !(*status_byte & ATA_DATA_READY) : false))
        return false; // we don't check the type from 0x1F1 in this simple design.

    return true;
}

bool read_sectors(uint32_t lba, uint8_t sec_count, void *buf)
{
    uint8_t status_byte;
    wait_for_disk_avl(&status_byte);

    send_ata_command(lba, MASTER, sec_count, READ_SECTOR);

    for (uint16_t j = 0; j < sec_count; ++j)
    {
        if (!poll_disk(&status_byte, true))
            return false;

        uint16_t *buffer = (uint16_t *)buf; // to write 2 bytes conveniently.
        for (uint16_t i = 0; i < 256; ++i)
        {
            buffer[j * 256 + i] = inw(ATA_DATA);
        }
    }

    if (!poll_disk(&status_byte, false))
        return false;
    return true;
}

bool write_sectors(uint32_t lba, uint8_t sec_count, const void *buf)
{
    uint8_t status_byte;
    wait_for_disk_avl(&status_byte);

    send_ata_command(lba, MASTER, sec_count, WRITE_SECTOR);

    for (uint16_t j = 0; j < sec_count; ++j)
    {
        if (!poll_disk(&status_byte, true))
            return false;

        const uint16_t *buffer = (const uint16_t *)buf; // to write 2 bytes conveniently.
        for (uint16_t i = 0; i < 256; ++i)
        {
            outw(ATA_DATA, buffer[j * 256 + i]);
        }
    }

    if (!poll_disk(&status_byte, false))
        return false;
    return true;
}
