/*
 * extEEPROM.h
 *
 * Created: 3/12/2019 15:22:57
 *  Author: Franco
 */ 

#ifndef EXTEEPROM_H_
#define EXTEEPROM_H_

//#define ASF

#include <stdio.h>
#include <stdlib.h>

#include "board_driver_i2c.h"

#define EEPROM_SIZE		65536
#define PAGE_SIZE		128

#ifdef ASF
! [packet_data]
#define WR_BUFFER_LENGTH 18
static uint8_t write_buffer[WR_BUFFER_LENGTH];

static uint8_t read_buffer[WR_BUFFER_LENGTH];
//! [packet_data]
//! [packet_settings]
struct i2c_master_packet packet;
//! [packet_settings]

/* Init software module. */
//! [dev_inst]
struct i2c_master_module i2c_master_instance;
//! [dev_inst]
#endif
//! [address]
#define SLAVE_ADDRESS 0x50
//! [address]

/* Number of times to try to send packet if failed. */
//! [timeout]
#define TIMEOUT 10
//! [timeout]

#define WIRE_WRITE_FLAG		0
#define WIRE_READ_FLAG		1

uint16_t extEEPROMWriteChunk(uint16_t location, uint16_t length, uint8_t* data);
void extEEPROMwrite(uint16_t location, void* buf, uint16_t len);
uint16_t extEEPROMReadChunk(uint16_t location, uint16_t length, uint8_t* data);
uint16_t extEEPROMread(uint16_t location, void* buf, uint16_t len);
void configure_i2c_master(void);



#endif /* EXTEEPROM_H_ */