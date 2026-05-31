#include <stdint.h>
#include <stdbool.h>
#include "io.h"

#define ATA_DATA 0x1F0
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7

bool read_sectors(uint32_t lba, uint8_t sec_count, void *buf)
{
    uint8_t status_byte;
    do
    {
        status_byte = inb(ATA_STATUS);
    } while ((status_byte & 0x80) || !(status_byte & 0x40));

    outb(0x1F6, ((uint8_t)(lba >> 24) & 0x0F) | 0xE0);
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F3, (uint8_t)(lba));
    outb(0x1F2, sec_count);
    outb(ATA_COMMAND, 0x20);

    for (uint16_t j = 0; j < sec_count; ++j)
    {

        for (uint8_t i = 0; i < 4; ++i)
            inb(ATA_STATUS); // 4 dummy reads for 400 ns delay
        // ATA bus much slower than cpu we may check status byte before BSY is even set to 1 lol

        do
        {
            status_byte = inb(ATA_STATUS);
        } while (status_byte & 0x80);
        if (status_byte & 0x01)
            return false; // we don't check the type from 0x1F1 in this simple design.
        if (!(status_byte & 0x08))
            return false;

        uint16_t *buffer = (uint16_t *)buf; // to write 2 bytes conveniently.
        for (uint16_t i = 0; i < 256; ++i)
        {
            buffer[j * 256 + i] = inw(ATA_DATA);
        }
    }
    return true;
}