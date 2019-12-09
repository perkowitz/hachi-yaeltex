/*
 Copyright (C) 2011 James Coliz, Jr. <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

// Note that we use an internal "page size" of 16.  The chip has a page size of 64 bytes,
// and the wire library can only send 30 at a time.  So the largest even factor of 64 that's
// less than 30 is 16, so we write & read even chunks of 16.
//
// The public read/write functions can read/write any amount of data, to any location.
// Pages are automaticaly handled so we never do a page write over a page boundary.

// A chunk is the largest even factor of a page size that the Wire library can write,
// 16 bytes

// internal page size scales to the size of the Wire buffer.  Increase these values in Wire.h and utility/twi.h
// to get much faster speed.  The chip has a 5msec write cycle no matter how many bytes you push it, up to
// 64.



#include "extEEPROM.h"


uint16_t extEEPROMWriteChunk(uint16_t location, uint16_t length, uint8_t* data)
{
    uint16_t bytes_written = length < PAGE_SIZE ? length : PAGE_SIZE;
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////// ASF //////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//write_buffer[0] = (uint8_t)(location >> 8);
	//write_buffer[1] = (uint8_t)(location & 0xff);
	//
	//for(int i = 0; i < bytes_written; i++){
		//if(i < WR_BUFFER_LENGTH-2)
			//write_buffer[2+i] = data[i];
	//}
	//
	//packet.data_length = bytes_written+2;
	//packet.data        = write_buffer;
//
	//uint16_t timeout = 0;
	///* Write buffer to slave until success. */
	////! [write_packet]
	//while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
		///* Increment timeout counter and check if timed out. */
		//if (timeout++ == TIMEOUT) {
			//break;
		//}
	//}
	//return bytes_written;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////// SAM-BA ///////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	i2c_beginTransmission(SLAVE_ADDRESS);
	i2c_write((uint8_t)(location >> 8)); 
	i2c_write((uint8_t)(location & 0xff));
	for(int i = 0; i < bytes_written; i++){
		i2c_write(data[i]);
	}
	return i2c_endTransmission(true);

    /*
	
	Wire.beginTransmission(i2c_address);  // START condition
    Wire.write((uint8_t)(location >> 8));
    Wire.write((uint8_t)(location & 0xff));
    Wire.write(data, bytes_written);
    Wire.endTransmission();
	*/
    //delay_ms(5);// 5msec write cycle time (Datasheet, pg.4
}

void extEEPROMwrite(uint16_t location, void* buf, uint16_t len)
{
    uint16_t remaining = len;
    uint8_t* next_in = (uint8_t*)(buf);
    uint16_t next_out = location;

    // First, deal with page boundaries.  If location is NOT on an even 16 bytes,
    // we'll want to write out only enough to get to the end of this page.

    if ( location & (PAGE_SIZE-1) )
    {
        uint16_t chunk_length = min(remaining, PAGE_SIZE - ( location & (PAGE_SIZE-1) ));

        uint16_t written = extEEPROMWriteChunk( next_out, chunk_length, next_in );
        remaining -= written;
        next_in += written;
        next_out += written;
    }

    while ( remaining )
    {
        uint16_t written = extEEPROMWriteChunk( next_out, remaining, next_in );
        remaining -= written;
        next_in += written;
        next_out += written;
    }
}

uint16_t extEEPROMReadChunk(uint16_t location, uint16_t length, uint8_t* data)
{
    uint16_t bytes_received = 0;
    uint16_t bytes_requested = min(length,PAGE_SIZE);
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////// ASF //////////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	//// Send address from where to read
	//write_buffer[0] = (uint8_t)(location >> 8);
	//write_buffer[1] = (uint8_t)(location & 0xff);
	//
	//packet.data_length		= 2;
	//packet.data				= write_buffer;
//
	//uint16_t timeout = 0;
	///* Write buffer to slave until success. */
	////! [write_packet]
	//while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
		///* Increment timeout counter and check if timed out. */
		//if (timeout++ == TIMEOUT) {
			//break;
		//}
	//}
	//
	//uint16_t timeout = 0;
	 //Read response
	///* Read from slave until success. */
	//! [read_packet]
	//packet.data_length		= bytes_requested;
	//packet.data				= data;
	//
	//while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK) {
		///* Increment timeout counter and check if timed out. */
		//if (timeout++ == TIMEOUT) {
			//break;
		//}
	//}
	////! [read_packet]
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////// SAM-BA ///////////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	i2c_beginTransmission(SLAVE_ADDRESS);
	i2c_write((uint8_t)(location >> 8));
	i2c_write((uint8_t)(location & 0xff));
	i2c_endTransmission(1);
	
	
	i2c_requestFrom(SLAVE_ADDRESS, bytes_requested, 1);
	for(int i = 0; i < bytes_requested; i++){
		data[i] = i2c_read(i);
	}
	
	
	/*
    Wire.beginTransmission(i2c_address);  // START condition
    Wire.write((uint8_t)(location >> 8));
    Wire.write((uint8_t)(location & 0xff
    Wire.endTransmission();
	
    Wire.requestFrom(i2c_address,bytes_requested);
    uint16_t remaining = bytes_requested;
    uint8_t* next = data;

    while (Wire.available() && remaining--)
    {
        *next++ = Wire.read();
        ++bytes_received;
    }
	*/
	
//    delay_ms(1);
    return bytes_received;
}

uint16_t extEEPROMread(uint16_t location, void* buf, uint16_t len)
{
    uint16_t bytes_received = 0;
    uint16_t remaining = len;
    uint8_t* next_memory = (uint8_t*)(buf);
    uint16_t next_chip = location;

    // First, deal with page boundaries.  If location is NOT on an even 16 bytes,
    // we'll want to write out only enough to get to the end of this page.

    if ( location & 0xf )
    {
        uint16_t chunk_length = min( remaining, 0x10 - ( location & 0xf ) );

        uint16_t chunk_received = extEEPROMReadChunk(next_chip,chunk_length,next_memory);
        remaining -= chunk_received;
        next_memory += chunk_received;
        next_chip += chunk_received;
        bytes_received += chunk_received;
    }

    while ( remaining )
    {
        uint16_t chunk_received = extEEPROMReadChunk(next_chip,remaining,next_memory);
        remaining -= chunk_received;
        next_memory += chunk_received;
        next_chip += chunk_received;
        bytes_received += chunk_received;
        if ( ! chunk_received ) // if we don't get any data, fall out so we don't hang here forever.
            break;
    }

    return bytes_received;
}

//! [initialize_i2c]
//void configure_i2c_master(void)
//{
	///* Initialize config structure and software module. */
	////! [init_conf]
	//struct i2c_master_config config_i2c_master;
	//i2c_master_get_config_defaults(&config_i2c_master);
	////! [init_conf]
//
	///* Change buffer timeout to something longer. */
	////! [conf_change]
	//config_i2c_master.baud_rate = I2C_MASTER_BAUD_RATE_400KHZ;
	//config_i2c_master.pinmux_pad0 = PINMUX_PA22C_SERCOM3_PAD0;
	//config_i2c_master.pinmux_pad1 = PINMUX_PA23C_SERCOM3_PAD1;
	//config_i2c_master.buffer_timeout = 10000;
	//
	////! [conf_change]
	///* Initialize and enable device with config. */
	////! [init_module]
	//i2c_master_init(&i2c_master_instance, CONF_I2C_MASTER_MODULE, &config_i2c_master);
	////! [init_module]
//
	////! [enable_module]
	//i2c_master_enable(&i2c_master_instance);
	////! [enable_module]
	//
	//
	//packet.address			= SLAVE_ADDRESS;
	//packet.ten_bit_address	= false;
	//packet.high_speed		= false;
	//packet.hs_master_code	= 0x0;
	//
//}
//! [initialize_i2c]
