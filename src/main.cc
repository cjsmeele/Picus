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
#include "mustore/mumemblockstore.hh"
#include "mustore/mufatfs.hh"
#include <utility>
#include "sdspi.hh"

#include <cstring>

Console *con;

static void dumpTree(MuFsNode &node, int level) {
    if (level > 8)
        return;
    char indent[8*2+1] = { };
    for (int i = 0; i < level; i++) {
        indent[i*2]   = ':';
        indent[i*2+1] = ' ';
    }

    MuFsError err;
    while (true) {
        MuFsNode child = node.readDir(err);
        if (err == MUFS_EOF)
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

    con->puts("\nPicus 0.1 alpha.\n");
    con->puts(    "Copyright (c) 2016, Chris Smeele. All rights reserved.\n\n");

    // (temporary)
    // Try to parse a FAT somewhere in flash storage.
    size_t storeAddr = (0x80000 + 0x8000);
    MuMemBlockStore store((const void*)storeAddr, 128*1024);
    MuFatFs fs(&store);

    bool gotFat = fs.getFsSubType() != MuFatFs::SubType::NONE;
    if (gotFat) {
        // loseWeight();
        con->printf("Got FAT%d filesystem '%s' in ramdisk @ %#08x\n\n",
                (fs.getFsSubType() == MuFatFs::SubType::FAT12 ? 12 :
                 fs.getFsSubType() == MuFatFs::SubType::FAT16 ? 16 :
                 fs.getFsSubType() == MuFatFs::SubType::FAT32 ? 32 : 99),
                fs.getVolumeLabel(),
                storeAddr);

        MuFsError err;
        auto root = fs.getRoot(err);

        if (err) {
            con->printf("err: %d\n", err);
        } else {
            con->puts("dumping tree\n");
            con->puts("/\n");
            dumpTree(root, 1);
        }
    }

    MuFsError fsErr;
    MuFsNode pwd = fs.getRoot(fsErr);
    char pwdPath[256] = { };
    strncpy(pwdPath, pwd.getName(), 256);

    char cmdInput[256] = { };
    int  cmdInputI     = 0;
    int   argc         = 0;
    char *argv[16]     = { };

    SdSpi sd;

    con->printf("\n%s:%s> ", fs.getVolumeLabel(), pwdPath);

    // Note: This mainloop was hacked together as a quick demo.
    // TODO: Rewrite this.
    while (true) {
        int c;

        if ((c = con->getch(false)) >= 0) {
            if (c == '\r' || c == '\n') {
                cmdInput[cmdInputI] = '\0';
                argc = 0;
                bool inPart = false;
                for (int i = 0; i < cmdInputI; i++) {
                    if (inPart) {
                        if (cmdInput[i] == ' ') {
                            cmdInput[i] = '\0';
                            inPart = false;
                            if (argc >= 15)
                                break;
                        }
                    } else {
                        if (cmdInput[i] != ' ') {
                            inPart = true;
                            argv[argc++] = cmdInput + i;
                        }
                    }
                }
                cmdInputI = 0;
                if (argc && strlen(argv[0])) {
                    if (!strcmp(argv[0], "hello")) {
                        con->puts("Hello, world!\n");
                    } else if (!strcmp(argv[0], "spi")) {
                    } else if (!strcmp(argv[0], "cls") || !strcmp(argv[0], "clear")) {
                        con->puts("\x1b[2J\x1b[0;0H");
                    } else if (!strcmp(argv[0], "echo")) {
                        for (int i = 1; i < argc; i++) {
                            if (i > 1)
                                con->putch(' ');
                            con->puts(argv[i]);
                        }
                        con->putch('\n');
                    } else if (!strcmp(argv[0], "help")) {
                        // con->puts("You're on your own, buddy\n");
                        con->printf("%8s %8s %8s %8s %8s\n"
                                    "%8s %8s %8s\n",
                                    "cat",
                                    "cd",
                                    "cls",
                                    "dir",
                                    "echo",
                                    "hello",
                                    "help",
                                    "pwd"
                                   );
                    } else if (!strcmp(argv[0], "pwd")) {
                        con->printf("%s\n", pwdPath);
                    } else if (!strcmp(argv[0], "cd")) {
                        if (argc == 2) {
                            if (argv[1][0] == '/') {
                                auto node = fs.get(argv[1], fsErr);
                                if (!fsErr && node.isDirectory()) {
                                    pwd = std::move(node);
                                    strncpy(pwdPath, argv[1], 255);
                                } else {
                                    con->printf("Path '%s' is not a directory\n", argv[1]);
                                }
                            } else {
                                auto node = pwd.get(argv[1], fsErr);
                                if (!fsErr && node.isDirectory()) {
                                    pwd = std::move(node);
                                    if (strcmp(pwdPath, "/"))
                                        strncat(pwdPath, "/", 255);
                                    strncat(pwdPath, argv[1], 255);
                                } else {
                                    con->printf("Path '%s' is not a directory\n", argv[1]);
                                }
                            }
                        } else {
                            pwd = fs.getRoot(fsErr);
                            strcpy(pwdPath, "/");
                        }
                        if(!fsErr) {
                        }
                    } else if (!strcmp(argv[0], "dir") || !strcmp(argv[0], "ls")) {
                        size_t totalSize  = 0;
                        size_t totalFiles = 0;
                        size_t totalDirs  = 0;

                        MuFsNode node = pwd;
                        if (argc == 2) {
                            if (argv[1][0] == '/')
                                node = fs.get(argv[1], fsErr);
                            else
                                node = pwd.get(argv[1], fsErr);
                        }
                        if (node.isDirectory()) {
                            node.rewind();
                            while (true) {
                                MuFsNode child = node.readDir(fsErr);
                                if (fsErr == MUFS_EOF)
                                    break;
                                if (fsErr) {
                                    con->printf("err: %d\n", fsErr);
                                    break;
                                }

                                con->printf("%13s", child.getName());
                                if (child.isDirectory()) {
                                    con->puts(  "  <DIR>\r\n");
                                    totalDirs++;
                                } else {
                                    con->printf("        %8'u Bytes\r\n", child.getSize());
                                    totalFiles++;
                                    totalSize += child.getSize();
                                }
                            }
                            con->printf("%5'u File(s)        %8'u Bytes\n", totalFiles, totalSize);
                            con->printf("%5'u Dir(s)\n", totalDirs);
                        } else if (argc > 1){
                            con->printf("Path '%s' is not a directory\n", argv[1]);
                        }
                    } else if (!strcmp(argv[0], "cat")) {
                        if (argc > 1) {
                            for (int i = 1; i < argc; i++) {
                                MuFsNode node(&fs);
                                if (argv[i][0] == '/') {
                                    node = fs.get(argv[i], fsErr);
                                } else {
                                    node = pwd.get(argv[i], fsErr);
                                }
                                if (fsErr) {
                                    con->printf("err: %d\n", fsErr);
                                } else if (node.isDirectory()){
                                    con->printf("cat: '%s' is a directory\n", argv[i]);
                                } else {
                                    char buffer[32] = { };
                                    while (true) {
                                        size_t readBytes = node.read(buffer, 32, fsErr);
                                        if (fsErr && fsErr != MUFS_EOF) {
                                            con->printf("err: %d\n", fsErr);
                                            break;
                                        } else {
                                            for (size_t j = 0; j < readBytes; j++)
                                                con->putch(buffer[j]);
                                            if (fsErr == MUFS_EOF)
                                                break;
                                        }
                                    }
                                }
                            }
                        } else {
                            con->printf("usage: cat FILE...\n");
                        }
                    } else {
                        con->printf("No such command '%s'\n", argv[0]);
                    }
                }
                con->printf("%s:%s> ", fs.getVolumeLabel(), pwdPath);
            } else if (c == '\b') {
                if (cmdInputI) {
                    cmdInputI--;
                    con->putch(' ');
                    con->putch((char)c);
                } else {
                    con->putch(' ');
                }
            } else if (c == '\x17') { // ^W
            } else {
                if (cmdInputI < 255)
                    cmdInput[cmdInputI++] = (char)c;
            }
        } else {
            if (z % 10 == 0) {
                // Blink LED at pin 13.
                if (z % 20 == 0)
                    PIOB->PIO_SODR = PIO_PB27;
                else
                    PIOB->PIO_CODR = PIO_PB27;
            }

            z++;
            Sleep(10);
        }
    }

    return 0;
}
