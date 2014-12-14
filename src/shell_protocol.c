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

#include "shell_protocol.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "can.h"
#include "utils.h"

#include "termio.h"
#include "shell.h"
#include "shell_programs.h"

// ----------------------------------------------------------------------------
void shell_handle_protocol(void)
{
	// Shell ausfuehren
	command_shell();
	
	// eventl. vorhandene Nachrichten abrufen/ausgeben
	if (can_check_message())
	{
		can_t message;
		
		// Nachricht abrufen
		uint8_t mob = can_get_message(&message);
		if (mob)
		{
			uint8_t length = message.length;
			
			#if CAN_RX_BUFFER_SIZE == 0
			
			if (message.flags.extended) {
				printf_P(PSTR("%x: %08lx %u"), mob - 1, message.id, length);
			}
			else {
				uint16_t id = message.id;
				printf_P(PSTR("%x: %8x %u"), mob - 1, id, length);
			}
			
			#else
			
			if (message.flags.extended) {
				printf_P(PSTR("%6u: %08lx %u"), message.timestamp, message.id, length);
			}
			else {
				uint16_t id = message.id;
				printf_P(PSTR("%6u: %8x %u"), message.timestamp, id, length);
			}
			
			#endif
			
			if (!message.flags.rtr)
			{
				if (length)
					term_puts_P(" >");
				
				for (uint8_t i=0;i<length;i++) {
					term_putc(' ');
					term_put_hex(message.data[i]);
				}
			}
			else
			{
				term_puts_P(" rtr");
			}
			
			term_putc_cr('\n');
		}
	}
}
