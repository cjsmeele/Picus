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
#include "shell.hh"
#include "sam.hh"
#include <cstdint>
#include <cstdlib>
#include <cstring>

using namespace MuStore;

struct Command {
    const char *name;
    void (*func)(int, const char**);
};

#define CMD_DECL(name) \
    static void CMD_NAME(name) (int argc, const char **argv)

#define CMD_NAME(name) _cmd__ ## name
#define CMD(name) \
    { #name, CMD_NAME(name) }

static Fs      *fs;
static FsNode  *pwd;
static char     pwdPath[257] = { };
static Console *con;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

CMD_DECL(cat) {
    FsError err;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            FsNode node(fs);
            if (argv[i][0] == '/') {
                node = fs->get(argv[i], err);
            } else {
                node = pwd->get(argv[i], err);
            }
            if (err) {
                con->printf("err: %d\n", err);
            } else if (node.isDirectory()){
                con->printf("cat: '%s' is a directory\n", argv[i]);
            } else {
                char buffer[32] = { };
                while (true) {
                    size_t readBytes = node.read(buffer, 32, err);
                    if (err && err != FS_EOF) {
                        con->printf("err: %d\n", err);
                        break;
                    } else {
                        for (size_t j = 0; j < readBytes; j++)
                            con->putch(buffer[j]);
                        if (err == FS_EOF)
                            break;
                    }
                }
            }
        }
    } else {
        con->printf("usage: cat FILE...\n");
    }
}

CMD_DECL(cd) {
    FsError err;
    if (argc == 2) {
        if (argv[1][0] == '/') {
            auto node = fs->get(argv[1], err);
            if (!err && node.isDirectory()) {
                *pwd = node;
                strncpy(pwdPath, argv[1], 255);
            } else {
                con->printf("Path '%s' is not a directory\n", argv[1]);
            }
        } else {
            auto node = pwd->get(argv[1], err);
            if (!err && node.isDirectory()) {
                *pwd = node;
                if (strcmp(pwdPath, "/"))
                    strncat(pwdPath, "/", 255);
                strncat(pwdPath, argv[1], 255);
            } else {
                con->printf("Path '%s' is not a directory\n", argv[1]);
            }
        }
    } else {
        *pwd = fs->getRoot(err);
        strcpy(pwdPath, "/");
    }
}

CMD_DECL(cls) {
    con->clear();
}

CMD_DECL(dir) {
    con->printf("%s\n", pwdPath);
    size_t totalSize  = 0;
    size_t totalFiles = 0;
    size_t totalDirs  = 0;

    FsNode node = *pwd;
    FsError err;
    if (argc == 2) {
        if (argv[1][0] == '/')
            node = fs->get(argv[1], err);
        else
            node = pwd->get(argv[1], err);
    }
    if (node.isDirectory()) {
        node.rewind();
        while (true) {
            FsNode child = node.readDir(err);
            if (err == FS_EOF)
                break;
            if (err) {
                con->printf("err: %d\n", err);
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
}

CMD_DECL(echo) {
    for (int i = 1; i < argc; i++) {
        if (i > 1)
            con->putch(' ');
        con->puts(argv[i]);
    }
    con->putch('\n');
}

CMD_DECL(hello) {
    con->puts("Hello, world!\n");
}

CMD_DECL(help) {
    con->printf("%8s %8s %8s %8s %8s\n"
                "%8s %8s %8s %8s\n",
                "cat",
                "cd",
                "cls",
                "dir",
                "echo",
                "hello",
                "help",
                "log",
                "pwd"
               );
}

CMD_DECL(log) {
    FsError err;
    auto logFile = fs->get("/logFile", err);
    if (!logFile.doesExist()) {
        con->puts("Sorry, logfile does not exist.\n");
        return;
    }
    if (argc == 1) {
        const char *catArgs[] = {
            "cat", "/logfile"
        };
        // Delegate.
        CMD_NAME(cat)(2, catArgs);

    } else {
        // Add item to log.
        logFile.seek(logFile.getSize());
        logFile.write("log entry: ", 11, err);
        if (err) {
            con->printf("An error occured (1:%d)\n", err);
            return;
        }
        for (int i = 1; i < argc; i++) {
            // Since our Store doesn't buffer writes, this can get
            // very inefficient. Not the shell's problem though.
            logFile.write(argv[i], strlen(argv[i]), err);
            logFile.write(" ", 1, err);
            if (err) {
                con->printf("An error occured (2:%d)\n", err);
                return;
            }
        }
        logFile.write("\n", 1, err);
    }
}

CMD_DECL(pwd) {
    con->printf("%s\n", pwdPath);
}

#pragma GCC diagnostic pop

static Command cmds[] = {
    CMD(cat),
    CMD(cd),
    CMD(cls),
    CMD(dir),
    CMD(echo),
    CMD(hello),
    CMD(help),
    CMD(log),
    CMD(pwd),
};

#define CMD_COUNT (sizeof(cmds) / sizeof(*cmds))

/// Save a command string in the histfile if it exists.
static void saveCommand(const char *cmd) {
    FsError err;
    auto histfile = fs->get("/histfile", err);
    if (histfile.doesExist()) {
        histfile.seek(histfile.getSize());
        histfile.write(cmd, strlen(cmd), err);
        histfile.write("\n", 1, err); // Inefficient, but w/e.
    }
}


void runShell(Console &con_, MuStore::Fs &fs_) {
    con = &con_;
    fs  = &fs_;

    FsError fsErr;
    FsNode  root = fs->getRoot(fsErr);
    pwd = &root;

    if (fsErr || !root.doesExist()) {
        con->printf("Could not get root directory, aborting.\n");
        return;
    }

    strncpy(pwdPath, pwd->getName(), sizeof(pwdPath)-1);

    char cmdInput[256] = { };
    int  cmdInputI     = 0;
    int   argc         = 0;
    char *argv[16]     = { };
    uint32_t frame     = 0;

    auto printPrompt = []() {
        con->printf("%s:%s> ", fs->getVolumeLabel(), pwdPath);
    };

    printPrompt();

    while (true) {
        int c;

        if ((c = con->getch(false)) >= 0) {
            if (c == '\r' || c == '\n') {
                cmdInput[cmdInputI] = '\0';
                if (cmdInputI)
                    saveCommand(cmdInput);
                argc = 0;
                // Split command line into arguments.
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
                // Find a matching command.
                if (argc && strlen(argv[0])) {
                    bool cmdFound = false;
                    for (size_t i = 0; i < CMD_COUNT; i++) {
                        if (!strcmp(cmds[i].name, argv[0])) {
                            cmdFound = true;
                            cmds[i].func(argc, (const char**)argv);
                        }
                    }
                    if (!cmdFound)
                        con->printf("No such command `%s'\n", argv[0]);
                }

                printPrompt();

            } else if (c == '\b') {
                // Backspace.
                if (cmdInputI) {
                    cmdInputI--;
                    con->putch(' ');
                    con->putch((char)c);
                } else {
                    con->putch(' ');
                }
            } else {
                if (cmdInputI < 255)
                    cmdInput[cmdInputI++] = (char)c;
            }
        } else {
            if (frame % 10 == 0) {
                // Blink LED at pin 13.
                if (frame % 20 == 0)
                    PIOB->PIO_SODR = PIO_PB27;
                else
                    PIOB->PIO_CODR = PIO_PB27;
            }

            frame++;
            Sleep(10);
        }
    }
    
}
