/**
 * \file
 * \author    Chris Smeele
 * \copyright Copyright (c) 2016, Chris Smeele
 *
 * \page License
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "sam.hh"
#include "sdspi.hh"

#include <cstdlib>
#include <cstring>
#include <cmath>

using namespace MuStore;

/**
 * \brief SD Card-Specific Data structure.
 *
 * Fields are named as in the simplified SD spec.
 */
struct SdCsd {
    // Offset 0.
    uint8_t  version    : 2; // We don't support cards below version 2 (value 0b01).
    uint8_t  _reserved1 : 6;

    // Offset 1.
    uint8_t  taac      : 8;
    uint8_t  nsac      : 8;
    uint8_t  tranSpeed : 8;

    // Offset 4.
    uint16_t ccc              : 12;
    uint8_t  readBlLen        :  4;

    // Offset 6.
    uint8_t  readBlPartial    :  1;
    uint8_t  writeBlkMisalign :  1;
    uint8_t   readBlkMisalign :  1;
    uint8_t  dsrImp           :  1;
    uint8_t  _reserved2       :  6;
    uint32_t cSize            : 22;

    // Offset 10.
    uint8_t  _reserved3       :  1;
    uint8_t  eraseBlkEn       :  1;
    uint8_t  sectorSize       :  7;
    uint8_t  wpGrpSize        :  7;

    // Offset 12.
    uint8_t  wpGrpEnable      :  1;
    uint8_t  _reserved4       :  2;
    uint8_t  r2wFactor        :  3;
    uint8_t  writeBlLen       :  4;
    uint8_t  writeBlPartial   :  1;
    uint8_t  _reserved5       :  5;

    // Offset 14.
    uint8_t  fileFormatGrp    :  1;
    uint8_t  copy             :  1;
    uint8_t  permWriteProtect :  1;
    uint8_t  tmpWriteProtect  :  1;
    uint8_t  fileFormat       :  2;
    uint8_t  _reserved6       :  2;

    // Offset 15.
    uint8_t  crc7             :  7;
    uint8_t  _endBit          :  1;
} __attribute__((packed));

uint8_t SdSpi::wait() {
    // Wait for the card to become ready for accepting new commands.
    uint8_t  x = 0;
    uint32_t i = 0;
    do {
        if (i++ > cmdTimeoutClocks)
            return x;
        SPI_Write(SPI0, 0, 0xff);
        x = (uint8_t)SPI_Read(SPI0);
    } while (x != 0xff);

    return x;
}

uint8_t SdSpi::recv() {
    return send(0xff);
}

void SdSpi::recv(uint8_t *buffer, size_t length) {
    for (size_t i = 0; i < length; i++)
        buffer[i] = recv();
}

StoreError SdSpi::recvBlock(uint8_t *buffer, size_t length) {
    size_t i = 0;
    uint8_t ch;

    do {
        if (i++ > cmdTimeoutClocks)
            return STORE_ERR_IO;
        // Wait for start block token (0xfe).
    } while ((ch = recv()) != 0xfe);

    for (i = 0; i < length; i++)
        buffer[i] = recv();

    return STORE_ERR_OK;
}

uint8_t SdSpi::recvR1() {
    // Receive R1 response byte.
    uint8_t  x = 0;
    uint32_t i = 0;
    do {
        if (i++ > cmdTimeoutClocks)
            return 0xff; // Invalid.
        SPI_Write(SPI0, 0, 0xff);
        x = (uint8_t)SPI_Read(SPI0);

        // First bit of R1 must be zero.
    } while (x & 0x80);

    return x;
}

uint8_t SdSpi::send(uint8_t byte) {
    SPI_Write(SPI0, 0, byte);
    return (uint8_t)SPI_Read(SPI0);
}

uint8_t SdSpi::send(uint8_t *buffer, size_t length) {
    uint8_t ret = 0;
    for (size_t i = 0; i < length; i++)
        ret = send(buffer[i]);
    return ret;
}

uint8_t SdSpi::send(SdCommand cmd) {
    uint8_t str[6];
    str[0] = (uint8_t)(0x40 | (cmd.cmd & 0x3f));
    str[1] = (uint8_t)(cmd.arg >> 24);
    str[2] = (uint8_t)(cmd.arg >> 16);
    str[3] = (uint8_t)(cmd.arg >>  8);
    str[4] = (uint8_t)(cmd.arg      );

    if (cmd.cmd == 0) {
        str[5] = 0x95; // CRC7 for CMD0, includes terminating end bit.
    } else if (cmd.cmd == 8) {
        str[5] = 0x69; // CRC7 for CMD8 with 0x1a5 as params.
    } else {
        str[5] = 0xff; // ¯\_(ツ)_/¯
    }

    wait();
    send(str, sizeof(str));

    return recvR1();
}

StoreError SdSpi::sendBlock(const uint8_t *buffer, size_t length) {
    if (wait() != 0xff)
        return STORE_ERR_IO;

    send(0xfe); // Data start token.

    for (size_t i = 0; i < length; i++)
        send(buffer[i]);

    // CRC, unused.
    send(0xff);
    send(0xff);

    uint8_t respToken = recv();
    if ((respToken & 0x1f) == 0x05) {
        // 'Data accepted'.
        if (wait() != 0xff)
            return STORE_ERR_IO;

        return STORE_ERR_OK;
    } else {
        return STORE_ERR_IO;
    }
}

StoreError SdSpi::seek(size_t lba) {
    if (!cardPresent || !inited)
        return STORE_ERR_IO;
    if (lba >= blockCount)
        return STORE_ERR_OUT_OF_BOUNDS;

    pos = lba;

    return STORE_ERR_OK;
}

StoreError SdSpi::read(void *buffer) {
    if (!cardPresent || !inited)
        return STORE_ERR_IO;
    if (pos >= blockCount)
        return STORE_ERR_OUT_OF_BOUNDS;

    wait();

    // NOTE: SDHC/SDXC use LBAs, SDSC uses byte addresses.
    uint8_t result = send(SdCommand{17, (uint32_t)pos});
    if (result != 0) {
        return STORE_ERR_IO;
    }
    recvBlock((uint8_t*)buffer, blockSize);

    pos++;

    // Receive and discard 16-bit CRC.
    recv();
    recv();

    if (wait() != 0xff)
        return STORE_ERR_IO;

    return STORE_ERR_OK;
}

StoreError SdSpi::write(const void *buffer) {
    if (!cardPresent || !inited)
        return STORE_ERR_IO;
    if (pos >= blockCount)
        return STORE_ERR_OUT_OF_BOUNDS;

    if (wait() != 0xff)
        return STORE_ERR_IO;

    uint8_t result = send(SdCommand{24, (uint32_t)pos});
    if (result != 0) {
        return STORE_ERR_IO;
    }

    pos++;

    return sendBlock((uint8_t*)buffer, 512);
}

SdSpi::SdSpi() {
    // con->puts("Initing SD SPI on NPCS0\n");

    SPI_Disable(SPI0);

    // Setup SPI pins.
    // Note: CS0 is pin 10 on the due.
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA28A_SPI0_NPCS0, PIO_DEFAULT);
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA25A_SPI0_MISO,  PIO_DEFAULT);
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA26A_SPI0_MOSI,  PIO_DEFAULT);
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA27A_SPI0_SPCK,  PIO_DEFAULT);

    // Set SPI configuration parameters.
    SPI_Configure(SPI0, ID_SPI0,
                  0x21         // WDRBT enabled, PS Fixed, Master.
                  | SPI_PCS(0) // Select first peripheral (NPCS0 - CS on Due pin 10).
                 );

    // Configure SPI peripheral 0 (CS0).
    SPI_ConfigureNPCS(SPI0, 0,
                      // XXX: These clock values are just a guess but they seem to work for SD.
                      // TODO: Tune this for performance?
                        (uint32_t)32 << 24
                      | (uint32_t)32 << 16
                      | (uint32_t)16 << 8 // baud rate divisor thingy.
                      | 0x2);            // CPOL = 0, NCHPA = 1

	// con->puts("Enable SPI\n");
    SPI_Enable(SPI0);

    // Wait for the SD card to become ready.
    if (wait() != 0xff)
        return; // Probably no card present.

    cardPresent = true;

    uint8_t result;
    // Send CMD0, reset the card.
    result = send(SdCommand{0, 0});
    if (result != 1)
        return;

    // Check if the card can handle our voltage levels.
    result = send(SdCommand{8, 0x1a5});
    if (result != 1)
        return;

    {
        uint8_t cmd8Buf[4] = { };
        recv(cmd8Buf, 4);
        if ((cmd8Buf[2] & 0x0f) != 1)
            return; // Voltage range unacceptable.
        if (cmd8Buf[3] != 0xa5)
            return; // Check pattern mismatch.
    }

    // Wait for the card to leave idle state (wait for it to finish initializing).
    int j = cmdTimeoutClocks;
    do {
        if (j-- <= 0)
            return;
        result = send(SdCommand{55, 0});
        result = send(SdCommand{41, 1UL << 30}); // Set the second highest bit to indicate SDHC/SDXC support.
    } while (result == 1);

    // Get Card-Specific Data.
    result = send(SdCommand{9, 0});
    if (result != 0)
        return;

    SdCsd csd;

    // For some reason bitfields are broken. We accecss it as a byte
    // array instead. Meh.
    uint8_t *csdb = (uint8_t*)&csd;
    recvBlock(csdb, sizeof(csd));

    if ((csdb[0] & 0xc0) == 0x40) {
        // con->printf("Card is of type SDHC/SDXC\n");
    } else {
        // con->printf("Card is of standard capacity (unimplemented)\n");
        return; // Currently no support for standard capacity.
    }

    blockSize  = 512;
    blockCount = (
          (uint32_t)(csdb[7] & 0x3f) << 16
        | (uint32_t)csdb[8] << 8
        | (uint32_t)csdb[9]
    ) * 1024;

    // con->printf("Block count: %'u (%'u KB)\n", blockCount, blockCount / 1024 * 512);

    // N/A in SDHC/SDXC.
    // result = send(SdCommand{16, 512}); // Set block length.
    // con->printf("result16: <%02xh>\n", result);

    inited = true;
}
