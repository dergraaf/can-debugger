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

#ifndef	TERMIO_H
#define	TERMIO_H

// -----------------------------------------------------------------------------

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <inttypes.h>

// -----------------------------------------------------------------------------
extern uint8_t term_data_available(void);
extern uint8_t term_getc(void);

// -----------------------------------------------------------------------------
extern void term_putc_cr(char c);
extern void term_putc(const char c);

// -----------------------------------------------------------------------------
extern void term_puts(const char *tx_data);
extern void term_puts_p(const char *progmem_tx_data);
#define term_puts_P(s) term_puts_p(PSTR(s))

// -----------------------------------------------------------------------------
extern void term_put_int(const int num);

// -----------------------------------------------------------------------------
extern void term_put_hex(const uint8_t val);

// -----------------------------------------------------------------------------
extern uint8_t term_get_long(char *s, uint32_t *num, uint8_t base);

// -----------------------------------------------------------------------------
extern uint8_t char_to_byte(char *s);

// -----------------------------------------------------------------------------
// Wandelt zwei Bytes des Strings in einen uint8_t Wert um

extern uint8_t hex_to_byte(char *s);

#endif	// TERMIO_H
