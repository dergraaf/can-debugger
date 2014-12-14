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

#include "usbcan_protocol.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "can.h"
#include "utils.h"

#include "termio.h"

static bool use_timestamps = false;

// Indicates the state of the Lawicel communication channel.
static bool channel_open = false;

// Indicates if we received a S command to set the bitrate.
// This is required before opening the communication channel.
static bool bitrate_set = false;

// ----------------------------------------------------------------------------
bool usbcan_decode_message(char *str, uint8_t length)
{
	can_t msg;
	uint8_t dlc_pos;
	bool extended;
	
	if (str[0] == 'R' || str[0] == 'T') {
		extended = true;
		dlc_pos = 9;
	}
	else {
		extended = false;
		dlc_pos = 4;
	}
	
	if (length < dlc_pos + 1)
		return false;
	
	// get the number of data-bytes for this message
	msg.length = str[dlc_pos] - '0';
	
	if (msg.length > 8)
		return false;		// too many data-bytes
	
	
	if (str[0] == 'r' || str[0] == 'R') {
		msg.flags.rtr = true;
		if (length != (dlc_pos + 1))
			return false;
	}
	else {
		msg.flags.rtr = false;
		if (length != (msg.length * 2 + dlc_pos + 1))
			return false;
	}
	
	// read the messge-identifier
	if (extended)
	{
		uint16_t id;
		uint16_t id2;
		
		id  = hex_to_byte(&str[1]) << 8;
		id |= hex_to_byte(&str[3]);
		
		id2  = hex_to_byte(&str[5]) << 8;
		id2 |= hex_to_byte(&str[7]);
		
		msg.id = (uint32_t) id << 16 | id2;
	}
	else {
		uint16_t id;
		
		id  = char_to_byte(&str[1]) << 8;
		id |= hex_to_byte(&str[2]);
		
		msg.id = id;
	}
	
	msg.flags.extended = extended;
	
	// read data if the message is no rtr-frame
	if (!msg.flags.rtr)
	{
		char *buf = str + dlc_pos + 1;
		uint8_t i;
		
		for (i=0; i < msg.length; i++)
		{
			msg.data[i] = hex_to_byte(buf);
			buf += 2;
		}
	}
	
	// finally try to send the message
	if (can_send_message( &msg ))
		return true;
	else
		return false;
}

// ----------------------------------------------------------------------------
void usbcan_decode_command(char *str, uint8_t length)
{
	uint8_t temp;
	
	if (length == 0)
		return;
	
	switch (str[0]) {
		case 'S':	// set bitrate
			// Lawicel has the folowing settings:
			// S0   10 kbps
			// S1   20 kbps
			// S2   50 kbps
			// S3  100 kbps
			// S4  125 kbps
			// S5  250 kbps
			// S6  500 kbps
			// S7  800 kbps
			// S8    1 mbps; is index 7 on USB2CAN
			// Note that the USB2CAN adapter does not support 800 kbps.
			// With index 7 USB2CAN runs on 1 mbps.
			// Report error if the range is wrong, communication channel
			// already open, Parameter wrong or the user tries to set
			// 800 kbps, which is not supported.
			if ( ((temp = str[1] - '0') > 8) || channel_open || length != 2 || temp == 7 ) {
				goto error;
			
			} else {
				// Take care of different index usage for 1 mbps.
				if ( temp == 8 ) {
					temp--;
				}
				// Set new bitrate, remember all MOB are cleared!
				can_init(temp);
				bitrate_set = true;
			}
			break;
		
		case 'r':	// send frames
		case 't':
		case 'R':
		case 'T':
			if ( !channel_open ) {
				goto error;
			
			} else {
				if ( !usbcan_decode_message(str, length) ) {
					goto error;
				}
			}
			break;
		
		case 'M':	// Set acceptance code for SJA1000
		case 'm':	// Set acceptance mask for SJA1000
		case 's':	// Set BTR0/BTR1 for SJA1000
			goto error;
			// TODO
			break;
		
		case 'O':	// Open channel, i.e. connect CAN to output.
			if ( channel_open || !bitrate_set ) {
				goto error;
			
			} else {
				can_set_mode(NORMAL_MODE);
				
				// In case the baudrate changed, re-enable some filters.
				can_filter_t filter = {
					.mask = 0,
					.id = 0,
					.flags.extended = 0,
					.flags.rtr = 0
				};
				for (uint8_t i=11; i<15; i++) {
					can_set_filter(i, &filter);
				}
				
				// Mark the channel as open.
				channel_open = true;
			}
			break;
		
		case 'L':	// Open channel, i.e. connect CAN to output in listen only mode.
			if ( channel_open || !bitrate_set ) {
				goto error;
			
			} else {
				can_set_mode(LISTEN_ONLY_MODE);
				channel_open = true;
			}
			break;
		
		case 'C':	// Close channel, i.e. disconnect CAN from output.
			if ( !channel_open ) {
				goto error;
			
			} else {
				channel_open = false;
			}
			break;
		
		case 'F':	// read Status Flags
			term_put_hex(0);
			// TODO
			break;
		
		case 'N':	// read serial number
			term_putc( 'N' );
			term_put_hex( 0 );
			term_put_hex( 0 );
			break;
		
		case 'V':	// read hardware and firmeware version
			term_putc( 'V' );
			term_put_hex( (HARDWARE_VERSION_MAJOR << 4) | HARDWARE_VERSION_MINOR );
			term_put_hex( (SOFTWARE_VERSION_MAJOR << 4) | SOFTWARE_VERSION_MINOR );
			break;
		
		case 'Z':
			// Switch on or off timestamps.
			// On Lawicel this value is stored in EEPROM.
			if ( channel_open || length != 2) {
				goto error;
			
			} else {
				use_timestamps = (str[1] - '0' == 0) ? false : true;
				CANTIM = 0;
			}
			break;
	}

	term_putc('\r');	// command could be executed
	return;
	
error:
	term_putc(7);		// Error in command
}

// ----------------------------------------------------------------------------
// processes commands in usbcan-protocol mode

void usbcan_handle_protocol(void)
{
	static char buffer[40];
	static uint8_t pos;
	static can_error_register_t last_error = { 0, 0 };
	can_error_register_t error;
	
	// check for new messages
	if (can_check_message()) {
		can_t message;
		
		// Only communicate data if channel is open.
		// Due to left -> right evaluation messages are extracted from the 
		// MOB to avoid overflow, even if no communication is desired.
		if ( can_get_message(&message) && channel_open ) {
			uint8_t length = message.length;
			
			if (message.flags.rtr)
			{
				// print identifier
				if (message.flags.extended) {
					printf_P(PSTR("R%08lx"), message.id);
				} else {
					uint16_t id = message.id;
					printf_P(PSTR("r%03x"), id);
				}
				term_putc(length + '0');
			}
			else
			{
				// print identifier
				if (message.flags.extended) {
					printf_P(PSTR("T%08lx"), message.id);
				} else {
					uint16_t id = message.id;
					printf_P(PSTR("t%03x"), id);
				}
				term_putc(length + '0');
				
				// print data
				for (uint8_t i = 0; i < length; i++)
					term_put_hex(message.data[i]);
			}

			if (use_timestamps)
				printf_P(PSTR("%04x"), message.timestamp);

			term_putc('\r');
		}
	}
	
	// get commands
	while (term_data_available())
	{
		char chr = term_getc();
		
		if (chr != '\r')
		{
			buffer[pos] = chr;
			pos++;
			
			if (pos >= sizeof(buffer)) {
				// format-error: command to long!
				pos = 0;
			}
		}
		else {
			buffer[pos] = '\0';
			usbcan_decode_command(buffer, pos);
			pos = 0;
		}
	}
	
	// get error-register
	error = can_read_error_register();
	
	if (last_error.tx != error.tx || last_error.rx != error.rx) {
		printf_P(PSTR("E%02x%02x\r"), error.rx, error.tx);
		
		last_error = error;
	}
}
