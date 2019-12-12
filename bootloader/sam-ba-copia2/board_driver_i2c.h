/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  Copyright (c) 2015 Atmel Corporation/Thibaut VIARD.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _BOARD_DRIVER_I2C_
#define _BOARD_DRIVER_I2C_

#include <sam.h>
#include <stdbool.h>
#include "board_definitions.h"

#define TRANSMISSION_OK 0
#define ADDRESS_ERROR	2
#define NACK_ERROR		3

void i2c_init(uint32_t baud);
void i2c_end();
uint8_t i2c_requestFrom(uint8_t address, uint8_t quantity, bool stopBit);
void i2c_beginTransmission(uint8_t address);
uint8_t i2c_endTransmission(bool stopBit);
uint8_t i2c_write(uint8_t ucData);
uint8_t i2c_read(uint8_t index);

#define PIN_PA22C_SERCOM3_PAD0            22L  /**< \brief SERCOM3 signal: PAD0 on PA22 mux C */
#define MUX_PA22C_SERCOM3_PAD0             2L
#define PINMUX_PA22C_SERCOM3_PAD0  ((PIN_PA22C_SERCOM3_PAD0 << 16) | MUX_PA22C_SERCOM3_PAD0)
#define PORT_PA22C_SERCOM3_PAD0    (1ul << 22)
#define PIN_PA23C_SERCOM3_PAD1            23L  /**< \brief SERCOM3 signal: PAD1 on PA23 mux C */
#define MUX_PA23C_SERCOM3_PAD1             2L
#define PINMUX_PA23C_SERCOM3_PAD1  ((PIN_PA23C_SERCOM3_PAD1 << 16) | MUX_PA23C_SERCOM3_PAD1)
#define PORT_PA23C_SERCOM3_PAD1    (1ul << 23)

/*- Definitions -------------------------------------------------------------*/
#define I2C_SERCOM            SERCOM3
#define I2C_SERCOM_GCLK_ID    GCLK_CLKCTRL_ID_SERCOM3_CORE_Val
#define I2C_SERCOM_CLK_GEN    2
#define I2C_SERCOM_APBCMASK   PM_APBCMASK_SERCOM3


#endif // _BOARD_DRIVER_I2C_
