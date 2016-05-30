/**
 * \file
 * \brief
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
#pragma once

#include "mustore/mublockstore.hh"

class SdSpi : public MuBlockStore {

    struct SdCommand {
        uint8_t  cmd;
        uint32_t arg;
    };

    bool cardPresent = false;
    bool inited      = false;

    uint8_t wait();

    uint8_t send(uint8_t byte);
    uint8_t send(uint8_t *buffer, size_t length);
    uint8_t send(SdCommand cmd);

    uint8_t recv();
    void    recv(uint8_t *buffer, size_t length);
    void    recvBlock(uint8_t *buffer, size_t length);

    uint8_t recvR1();

public:
    MuBlockStoreError seek(size_t lba);

    MuBlockStoreError read (void *buffer);
    MuBlockStoreError write(const void *buffer);

    using MuBlockStore::read;
    using MuBlockStore::write;

    SdSpi();
    ~SdSpi() = default;
};
