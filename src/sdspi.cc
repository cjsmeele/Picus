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
#include "console.hh"

#include <cstdlib>
#include <cstring>
#include <cmath>

extern Console *con; // XXX: For debugging purposes.

uint8_t SdSpi::wait() {
    // Wait for the card to become ready for accepting new commands.
    uint8_t  x = 0;
    uint32_t i = 0;
    do {
        if (i++ > 200)
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

uint8_t SdSpi::recvR1() {
    // Receive R1 response byte.
    uint8_t  x = 0;
    uint32_t i = 0;
    do {
        if (i++ > 200)
            while(true);
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

MuBlockStoreError SdSpi::seek(size_t lba) {
    // TODO.
    return MUBLOCKSTORE_ERR_IO;
}

MuBlockStoreError SdSpi::read (void *buffer) {
    // TODO.
    return MUBLOCKSTORE_ERR_IO;
}

MuBlockStoreError SdSpi::write(const void *buffer) {
    // TODO.
    return MUBLOCKSTORE_ERR_IO;
}

SdSpi::SdSpi() {
    con->puts("Initing SD SPI on NPCS0\n");

    SPI_Disable(SPI0);

    // Setup SPI pins.
    // Note: CS0 is pin 10 on the due.
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA28A_SPI0_NPCS0, PIO_DEFAULT);
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA25A_SPI0_MISO,  PIO_DEFAULT);
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA26A_SPI0_MOSI,  PIO_DEFAULT);
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA27A_SPI0_SPCK,  PIO_DEFAULT);

    // Set SPI configuration parameters.
	con->puts("Config SPI\n");
    SPI_Configure(SPI0, ID_SPI0,
                  0x21         // WDRBT enabled, PS Fixed, Master.
                  | SPI_PCS(0) // Select first peripheral (NPCS0 - CS on Due pin 10).
                 );

    // SPI_DLYBCT(0, SystemCoreClock)
    // | SPI_DLYBS(0, SystemCoreClock)
    // | SPI_SCBR(32, SystemCoreClock)

    // Configure SPI peripheral 0 (CS0).
	con->puts("Config NPCS\n");
    SPI_ConfigureNPCS(SPI0, 0,
                      // XXX: These clock values are just a guess but they seem to work for SD.
                      // TODO: Tune this for performance?
                        (uint32_t)8 << 24
                      | (uint32_t)8 << 16
                      | (uint32_t)8 << 8 // baud rate divisor thingy.
                      | 0x2);            // CPOL = 0, NCHPA = 1

	con->puts("Enable SPI\n");
    SPI_Enable(SPI0);

    // Wait for the SD card to become ready.
    if (wait() != 0xff)
        return; // Probably no card present.

    cardPresent = true;

    uint8_t result;
    // Send CMD0, reset the card.
    result = send(SdCommand{0, 0});
    con->printf("result0:   <%02xh>\n", result);

    // Check if the card can handle our voltage levels.
    con->printf("result8:   <%02xh>\n", send(SdCommand{8, 0x1a5})); // 2.7 - 3.6V.
    con->printf("result8.1: <%02xh>\n", recv());
    con->printf("result8.2: <%02xh>\n", recv());
    con->printf("result8.3: <%02xh>\n", recv());
    con->printf("result8.4: <%02xh>\n", recv());
    // (TODO: actually check the result)

    // Wait for the card to leave idle state (wait for it to finish initializing).
    int j = 300;
    do {
        if (j-- <= 0)
            while (true);
        con->printf("result55: <%02xh>\n", send(SdCommand{55, 0}));
        result = send(SdCommand{41, 1UL << 30}); // Set the second highest bit to indicate SDHC/SDXC support.
        con->printf("result41: <%02xh>\n", result);
    } while (result == 1);

    // TODO.
}
