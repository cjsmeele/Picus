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
#include "uartcon.hh"
#include "queue.hh"

#include <cstdlib>
#include <cstring>

Queue<uint8_t, 64> rxbuf;

SamUartConsole &SamUartConsole::getInstance() {
    static SamUartConsole console(UART);
    return console;
}

void SamUartConsole::doPutch(uint8_t ch) {
    // Wait for the transmitter to become ready.
    while (!(uart->UART_SR & UART_SR_TXRDY));

    // Set the transmit holding register.
    uart->UART_THR = ch;
}

void SamUartConsole::putch(char ch) {

    static char lastChar = '\0';

    // Convert LF -> CRLF.
    if (lastChar != '\r' && ch == '\n')
        doPutch('\r');

    doPutch((uint8_t)ch);
    lastChar = ch;
}

int SamUartConsole::doGetch(bool block) {
    if (block) {
        // Wait for the receiver to become ready.
        while (!(uart->UART_SR & UART_SR_RXRDY)) {
            // XXX: Blink LED for debugging.
            PIOB->PIO_CODR = PIO_PB27;
            Sleep(10);
            PIOB->PIO_SODR = PIO_PB27;
        }
    } else {
        if (!(uart->UART_SR & UART_SR_RXRDY))
            return -1;
    }

    // Read from receive holding register.
    return (uint8_t)uart->UART_RHR;
}

int SamUartConsole::getch(bool block) {
    if (block) {
        while (!rxbuf.getLength())
            Sleep(1);
    }
    if (rxbuf.getLength()) {
        return (uint8_t) --rxbuf;
    } else {
        return -1;
    }
}

void SamUartConsole::clear() {
    // Assume the terminal supports ECMA-48 CSI sequences.
    puts( "\x1b[2J\x1b[1;1H");
    // erase dpy ^        ^ move cursor to origin.
}

extern "C" void UART_Handler(void) {
    int c;
    SamUartConsole &con = SamUartConsole::getInstance();

    if (con.uart->UART_IMR & UART_IMR_RXRDY) {
        // Append the received character to the receive buffer.
        if ((c = con.doGetch(false)) >= 0) {
            rxbuf += (uint8_t)c;
        }
    }
}

SamUartConsole::SamUartConsole(Uart *uart_)
    : uart(uart_) {

    // Configure UART pins.
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA8A_URXD, PIO_PULLUP);
    PIO_Configure(PIOA, PIO_PERIPH_A, PIO_PA9A_UTXD, PIO_PULLUP);

    pmc_enable_periph_clk(ID_UART);

    // Reset and disable receiver and transmitter.
    uart->UART_CR = UART_CR_RSTRX | UART_CR_RSTTX | UART_CR_RXDIS | UART_CR_TXDIS;

    // Set the baudrate to 115200.
    uart->UART_BRGR = 45; // MASTER_CLK_FREQ / (16 * 45) = 116666 (close enough).
    //uart->UART_BRGR = 546; // For ~9600 baud.

    // No parity, normal channel mode. (We could enable echo by setting CHMODE AUTOMATIC)
    uart->UART_MR = UART_MR_PAR_NO;

    uart->UART_IDR = 0xFFFFFFFF;        // Disable all interrupts.
    NVIC_EnableIRQ((IRQn_Type)ID_UART); // Configure UART isr.
    uart->UART_IER = UART_IER_RXRDY;    // Enable receive-ready interrupt.
    //uart->UART_IER = UART_IER_RXRDY | UART_IER_TXRDY;

    // Enable the receiver and the trasmitter.
    uart->UART_CR = UART_CR_RXEN | UART_CR_TXEN;
}
