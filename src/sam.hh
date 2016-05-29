/**
 * \file
 * \brief Include libsam and CMSIS headers for the Arduino Due board.
 */
#pragma once

#define __SAM3X8E__

extern "C" {
#include "sam/libsam/chip.h"
#include "sam/CMSIS/Device/ATMEL/sam.h"

    void hang() __attribute__((noreturn));
}
