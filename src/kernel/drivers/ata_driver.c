#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "misc/io.h"
#include "drivers/ata_driver.h"

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
#define IDENTIFY_DEVICE 0xEC

typedef enum
{
    MASTER,
    SLAVE
} disk_t;

typedef enum
{
    ATA_DATA = 0x1F0,
    ATA_STATUS = 0x1F7,
    ATA_ALT_STATUS = 0x3F6, // reading this doesn't clear interrupts
    ATA_COMMAND = 0x1F7
} ata_port_t;

// i always use lba
static void send_ata_command(uint32_t lba, disk_t drive, uint8_t sec_count, uint8_t command)
{
    uint8_t options = ((drive == MASTER) ? 0xE0 : 0xF0);
    outb(0x1F6, ((uint8_t)(lba >> 24) & 0x0F) | options);
    if (command != IDENTIFY_DEVICE)
    {
        outb(0x1F5, (uint8_t)(lba >> 16));
        outb(0x1F4, (uint8_t)(lba >> 8));
        outb(0x1F3, (uint8_t)(lba));
        outb(0x1F2, sec_count);
    }
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
    do
    {
        *status_byte = inb(ATA_STATUS);
    } while (*status_byte & ATA_DRIVE_BUSY);
    if (*status_byte & ATA_OPERATION_ERROR || (data_left ? !(*status_byte & ATA_DATA_READY) : false))
        return false; // we don't check the type from 0x1F1 in this simple design.

    return true;
}

static bool identify_device(void *buf)
{
    uint8_t status_byte;
    wait_for_disk_avl(&status_byte);

    send_ata_command(0, MASTER, 0, IDENTIFY_DEVICE);

    for (uint8_t i = 0; i < 4; ++i)
        inb(ATA_ALT_STATUS);    // introduce delay before confirming
    if (inb(ATA_STATUS) == 0x00)
        return false; // no disk exists

    if (!poll_disk(&status_byte, true))
        return false;

    uint16_t *buffer = (uint16_t *)buf; // to write 2 bytes conveniently.
    for (uint16_t i = 0; i < 256; ++i)
    {
        buffer[i] = inw(ATA_DATA);
    }

    return true;
}

bool ata_fill_disk_info(ata_disk_info_t *disk_info)
{
    char info_buf[512];
    if (!identify_device((void *)info_buf))
        return false;
    const uint16_t *info_buffer = (const uint16_t *)info_buf;
    disk_info->sector_count = (info_buffer[61] << 16) | info_buffer[60];
    return true;
}

bool ata_read_sectors(uint32_t lba, uint8_t sec_count, void *buf)
{
    uint8_t status_byte;
    wait_for_disk_avl(&status_byte);

    send_ata_command(lba, MASTER, sec_count, READ_SECTOR);
    for (uint8_t i = 0; i < 4; ++i)
        inb(ATA_ALT_STATUS); // 4 dummy reads for 400 ns delay
    // ATA bus much slower than cpu we may check status byte before BSY is even set to 1 lol

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

    return true;
}

bool ata_write_sectors(uint32_t lba, uint8_t sec_count, const void *buf)
{
    uint8_t status_byte;
    wait_for_disk_avl(&status_byte);

    send_ata_command(lba, MASTER, sec_count, WRITE_SECTOR);
    for (uint8_t i = 0; i < 4; ++i)
        inb(ATA_ALT_STATUS);

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
        return false; // finish writing last sector
    return true;
}
