/*
 * test.cpp
 *
 * Created: 27/5/2020 11:53:19 a. m.
 *  Author: Franco
 */ 
#include <stdint.h>

void foo(void)
{
	uint8_t a = 5;
	uint8_t b = 5;
	uint8_t c;
	c = a+b;
	b = c - a;
}