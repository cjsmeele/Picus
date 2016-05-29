/**
 * \file
 * \brief     Empty exception handlers.
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

extern "C" {
    void hang() {
        while (true)
            Sleep(1000); // Fire safety.
    }

    static void dummyHandler() {
        hang();
    }

    void NMI_Handler()        __attribute__((weak, alias("dummyHandler")));
    void HardFault_Handler()  __attribute__((weak, alias("dummyHandler")));
    void MemManage_Handler()  __attribute__((weak, alias("dummyHandler")));
    void BusFault_Handler()   __attribute__((weak, alias("dummyHandler")));
    void UsageFault_Handler() __attribute__((weak, alias("dummyHandler")));
    void SVC_Handler()        __attribute__((weak, alias("dummyHandler")));
    void DebugMon_Handler()   __attribute__((weak, alias("dummyHandler")));
    void PendSV_Handler()     __attribute__((weak, alias("dummyHandler")));

    void SysTick_Handler(void) {
        TimeTick_Increment();
    }

    // Handlers for peripheral interrupts.
    void ADC_Handler()    __attribute__((weak, alias("dummyHandler")));
    void CAN0_Handler()   __attribute__((weak, alias("dummyHandler")));
    void CAN1_Handler()   __attribute__((weak, alias("dummyHandler")));
    void DACC_Handler()   __attribute__((weak, alias("dummyHandler")));
    void DMAC_Handler()   __attribute__((weak, alias("dummyHandler")));
    void EFC0_Handler()   __attribute__((weak, alias("dummyHandler")));
    void EFC1_Handler()   __attribute__((weak, alias("dummyHandler")));
    void EMAC_Handler()   __attribute__((weak, alias("dummyHandler")));
    void HSMCI_Handler()  __attribute__((weak, alias("dummyHandler")));
    void PIOA_Handler()   __attribute__((weak, alias("dummyHandler")));
    void PIOB_Handler()   __attribute__((weak, alias("dummyHandler")));
    void PIOC_Handler()   __attribute__((weak, alias("dummyHandler")));
    void PIOD_Handler()   __attribute__((weak, alias("dummyHandler")));
    void PMC_Handler()    __attribute__((weak, alias("dummyHandler")));
    void PWM_Handler()    __attribute__((weak, alias("dummyHandler")));
    void RSTC_Handler()   __attribute__((weak, alias("dummyHandler")));
    void RTC_Handler()    __attribute__((weak, alias("dummyHandler")));
    void RTT_Handler()    __attribute__((weak, alias("dummyHandler")));
    void SMC_Handler()    __attribute__((weak, alias("dummyHandler")));
    void SPI0_Handler()   __attribute__((weak, alias("dummyHandler")));
    void SSC_Handler()    __attribute__((weak, alias("dummyHandler")));
    void SUPC_Handler()   __attribute__((weak, alias("dummyHandler")));
    void TC0_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC1_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC2_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC3_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC4_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC5_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC6_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC7_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TC8_Handler()    __attribute__((weak, alias("dummyHandler")));
    void TRNG_Handler()   __attribute__((weak, alias("dummyHandler")));
    void TWI0_Handler()   __attribute__((weak, alias("dummyHandler")));
    void TWI1_Handler()   __attribute__((weak, alias("dummyHandler")));
    void UART_Handler()   __attribute__((weak, alias("dummyHandler")));
    void UOTGHS_Handler() __attribute__((weak, alias("dummyHandler")));
    void USART0_Handler() __attribute__((weak, alias("dummyHandler")));
    void USART1_Handler() __attribute__((weak, alias("dummyHandler")));
    void USART2_Handler() __attribute__((weak, alias("dummyHandler")));
    void USART3_Handler() __attribute__((weak, alias("dummyHandler")));
    void WDT_Handler()    __attribute__((weak, alias("dummyHandler")));
}
