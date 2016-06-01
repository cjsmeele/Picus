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

#include "uartcon.hh"
#include <mustore/memstore.hh>
#include <mustore/fatfs.hh>
#include "sdspi.hh"
#include "shell.hh"

#include <cstring>

Console *con;

using namespace MuStore;

static void dumpTree(FsNode &node, int level) {
    if (level > 8)
        return;
    char indent[8*2+1] = { };
    for (int i = 0; i < level; i++) {
        indent[i*2]   = ':';
        indent[i*2+1] = ' ';
    }

    FsError err;
    while (true) {
        FsNode child = node.readDir(err);
        if (err == FS_EOF)
            break;
        if (err) {
            con->printf("err: %d\n", err);
            break;
        }
        if (child.getName()[0] == '.')
            continue;
        con->puts(indent);
        con->puts(child.getName());
        if (child.isDirectory()) {
            con->puts("/\n");
            dumpTree(child, level+1);
        } else {
            con->puts("\n");
        }
    }
}

extern "C" void __libc_init_array();
extern "C" int main() {

    // CMSIS initialization.
    SystemInit();

    // Set up the SysTick timer for 1ms interrupts.
    if (SysTick_Config(SystemCoreClock / 1000))
        hang();

    // Disable the watchdog.
    WDT_Disable(WDT);

    // Init libc.
    __libc_init_array();

    // Obtain a console.
    con = &SamUartConsole::getInstance();

    // Due pin 13 (amber LED) is PB27.
    PIO_Configure(PIOB, PIO_OUTPUT_1, PIO_PB27, PIO_DEFAULT);
    PIOB->PIO_CODR = PIO_PB27;

    uint32_t z = 0;

    while (1) {
        int c;
        if ((c = con->getch(false)) >= 0) {
            if (c == '\r' || c == '\n')
                break;
        }
        if (z % 100 == 0) {
            // Blink LED slowly to indicate readiness.
            if (z % 200 == 0)
                PIOB->PIO_SODR = PIO_PB27;
            else
                PIOB->PIO_CODR = PIO_PB27;
        }

        Sleep(10);
        z++;
    }

    Sleep(10);

    con->clear();

    con->puts(
        "Picus 0.1 alpha.\n"
        "Copyright (c) 2016, Chris Smeele.\n"
        "This is free software with ABSOLUTELY NO WARRANTY.\n\n"
    );

    // Try to parse a FAT somewhere in flash storage.
    // MemStore store((const void*)(0x80000 + 0x8000), 128*1024);
    // FatFs fs(&store);

    SdSpi sd;
    FatFs fs(&sd);

    bool gotFat = fs.getFsSubType() != FatFs::SubType::NONE;
    if (gotFat) {
        con->printf("Found FAT%d filesystem `%s' on SPI SD card\n\n",
                    (fs.getFsSubType() == FatFs::SubType::FAT12 ? 12 :
                     fs.getFsSubType() == FatFs::SubType::FAT16 ? 16 :
                     fs.getFsSubType() == FatFs::SubType::FAT32 ? 32 : 99),
                    fs.getVolumeLabel());

        FsError err;
        auto root = fs.getRoot(err);

        if (err) {
            con->printf("err: %d\n", err);
        } else {
            FsNode banner = fs.get("/banner.txt", err);
            if (banner.doesExist()) {
                char buf[32];
                while (true) {
                    size_t readBytes = banner.read(buf, 32, err);
                    if (err && err != FS_EOF) {
                        con->printf("err: %d\n", err);
                        break;
                    } else {
                        for (size_t j = 0; j < readBytes; j++)
                            con->putch(buf[j]);
                        if (err == FS_EOF)
                            break;
                    }
                }
            }
        }
    }

    runShell(*con, fs);

    return 0;
}
