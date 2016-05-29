/**
 * \file
 * \brief     Due UART Console class.
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

#include "console.hh"
#include "sam.hh"

class SamUartConsole : public Console {

    friend void UART_Handler();

    Uart *uart;

    void doPutch(uint8_t ch);
    int  doGetch(bool block = true);

    SamUartConsole(Uart *uart_);

public:
    void putch(char ch);
    int getch(bool block = true);

    static SamUartConsole &getInstance();

    SamUartConsole(SamUartConsole const&) = delete;
    void operator=(SamUartConsole const&) = delete;

    virtual ~SamUartConsole() = default;
};
