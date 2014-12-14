// coding: utf-8
// -----------------------------------------------------------------------------
/*
 * Copyright (C) 2008 Fabian Greif, Roboterclub Aachen e.V.
 * 
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your 
 * option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>.
 */
// -----------------------------------------------------------------------------

#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#include "termio.h"
#include "config.h"
#include "utils.h"

// ------------------------------------------------------------------------
uint8_t term_data_available(void) {
	return (!IS_SET(USB_RXF));
}

// ------------------------------------------------------------------------
uint8_t term_getc()
{
	uint8_t t;
	
	// read databyte
	RESET(RD);
	asm ("nop");
	
	t = PIN(USB_DATA);
	SET(RD);
	
	return t;
}

// ------------------------------------------------------------------------
void term_putc(const char c)
{
	// wait until ft245 ready to receive byte
	while (IS_SET(USB_TXE))
		;
	
	DDR(USB_DATA) = 0xff;
	PORT(USB_DATA) = c;
	
	// write data
	SET(WR);
	asm ("nop");
	RESET(WR);
	
	// use port as input, pull-ups on
	DDR(USB_DATA) = 0;
	PORT(USB_DATA) = 0xff;
}

// ----------------------------------------------------------------------------
void term_putc_cr(char c)
{
	if (c == '\n')	
		term_putc('\r');
	
	term_putc(c);
}

// ----------------------------------------------------------------------------
void term_puts(const char* tx_data)
{
	char c;
	while( (c = *tx_data++) ) 
		term_putc_cr(c); 
}

// ----------------------------------------------------------------------------
void term_puts_p(const char *progmem_tx_data)
{
	uint8_t c=0;  

	while( (c = pgm_read_byte(progmem_tx_data++)) ) 
		term_putc_cr(c);
}

// ----------------------------------------------------------------------------
void term_put_int(const int num)
{
	char buffer[8 * sizeof (int) + 1];
	
	itoa(num,buffer,10);
	
	term_puts(buffer);
}

// ----------------------------------------------------------------------------
void term_put_hex(const uint8_t val)
{
	uint8_t tmp = val >> 4;
	
	if (tmp > 9)
		tmp += 'A' - 10;
	else 
		tmp += '0';
	term_putc(tmp);
	
	tmp = val & 0x0f;
	
	if (tmp > 9) 
		tmp += 'A' - 10;
	else 
		tmp += '0';
	term_putc(tmp);
}

// ----------------------------------------------------------------------------
uint8_t term_get_long(char *s, uint32_t *num, uint8_t base)
{
	char b;
	char *t = &b;
	
	*num = strtoul(s, &t, base);
	
	if ((*t != '\0' && !isspace(*t)) || errno == ERANGE) {
		// Fehler im String
		return 0;
	}
	
	return 1;
}

// -----------------------------------------------------------------------------
uint8_t char_to_byte(char *s)
{
	uint8_t t = *s;
	
	if (t >= 'a')
		t = t - 'a' + 10;
	else if (t >= 'A')
		t = t - 'A' + 10;
	else
		t = t - '0';
	
	return t;
}

// -----------------------------------------------------------------------------
// Wandelt zwei Bytes des Strings in einen uint8_t Wert um

uint8_t hex_to_byte(char *s)
{
	return (char_to_byte(s) << 4) | char_to_byte(s + 1);
}
