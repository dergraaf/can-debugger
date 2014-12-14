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

#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include "termio.h"
#include "shell.h"
#include "shell_programs.h"

#include "config.h"

#include "can.h"
#include "utils.h"

// ----------------------------------------------------------------------------
uint8_t show_help(char *param, char data);
uint8_t show_man_page(char *param, char data);
uint8_t toggle_temp_output(char *param, char data);
uint8_t start_output(char *param, char data);
uint8_t stop_output(char *param, char data);
uint8_t send_can_messages(char *param, char data);
uint8_t set_filter(char *param, char data);
uint8_t set_bitrate(char *param, char data);
uint8_t get_values(char *param, char data);
uint8_t set_values(char *param, char data);
uint8_t restart(char *param, char data);
//uint8_t gateway(char *param, char data);
//uint8_t led_control(char *param, char data);
uint8_t print_hex(char *param, char data);
uint8_t clear_screen(char *param, char data);
uint8_t print_version(char *param, char data);

uint8_t send_bulk_messages(char *param, char data);
// ----------------------------------------------------------------------------

#define strncmp_flash(sram,sflash,n) strncmp_P(sram,PSTR(sflash),n)

static prog_char s_question[]	= "?";
static prog_char s_help[] 		= "help";

static prog_char s_clear[]		= "clear";

static prog_char s_restart[] 	= "restart";
static prog_char s_exit[]		= "exit";

//static prog_char s_led[] 		= "led";
static prog_char s_hex[]		= "hex";

static prog_char s_send_can[]	= "send";
static prog_char s_send_can2[]	= ">";

static prog_char s_get[]		= "get";
static prog_char s_set[]		= "set";

//static prog_char s_gateway[]	= "gateway";

static prog_char s_start[]		= "start";
static prog_char s_stop[]		= "stop";

static prog_char s_bulk[]		= "bulk";

static prog_char s_version[]	= "version";

// ----------------------------------------------------------------------------

const ShellProgram shell_program_list[] = {
	{ s_question,	1,	show_help		},
	{ s_help, 		4,	show_help		},
	{ s_restart, 	7,	restart 		},
	{ s_exit,		4,	restart			},
//	{ s_led, 		3,	led_control		},
	{ s_hex, 		3,	print_hex		},
	{ s_clear, 		5,	clear_screen	},
	
	{ s_send_can,	4,	send_can_messages },
	{ s_send_can2,	1,	send_can_messages },
	
	{ s_get,		3,	get_values		},
	{ s_set,		3,	set_values		},
//	{ s_gateway,	7,	gateway			},
	
	{ s_start,		5,	start_output	},
	{ s_stop,		4,	stop_output		},
	
	{ s_bulk,		4,	send_bulk_messages },
	
	{ s_version,	7,	print_version	},
};

uint8_t shell_program_list_length = sizeof(shell_program_list)/sizeof(ShellProgram);

// ----------------------------------------------------------------------------

static uint8_t tmp_stop_output = FALSE;
static uint8_t copy_stop_output = FALSE;

uint8_t flag_stop_output = FALSE;

// ----------------------------------------------------------------------------
static void vt100_setattr(uint8_t attr)
{
	term_puts_P("\x1b[");
	term_put_int(attr);
	term_putc('m');
}

// ----------------------------------------------------------------------------

#define error(s) term_report_error_p(PSTR(s))
void term_report_error_p(const char *s)
{
	term_puts_P("Error: ");
	term_puts_p(s);
	term_putc_cr('\n');
}

// ----------------------------------------------------------------------------
uint8_t show_help(char *param, char data)
{
	char *s = get_parameter(param, 1);
	uint8_t length = get_parameter_length(s);
	
	if (length == 0) {
		term_puts_P("help   - show this help\n" \
					"send   - send a CAN packet\n" \
					"set    - set filter, bitrate etc.\n" \
					"get    - get filter and other values\n" \
					"start  - restarts output\n" \
					"stop   - stops output immediately\n" \
					"led    - control LED status\n" \
					"clear  - clear screen\n" \
					"exit   - restart AVR\n" \
					"\nTo get more information about a specific command type \"help %name%\"\n");
	}
	else {
		if (!strncmp_P(s, s_send_can, 4)) {
			vt100_setattr(1);
			term_puts_P("send id length [rtr|data]\n\n");
			vt100_setattr(0);
			
			term_puts_P("Example:\n" \
			"If you want send a message with CAN Id 0x123 and four "\
			"bytes of data you have to type:\n" \
			"  $ send 123 4 abcd5678\n"
			"For better readability the arguments and the data tuples can "
			"be separated by optional whitespaces.\n"
			);
		}
		else if (!strncmp_P(s, s_get, 3)) {
			vt100_setattr(1);
			term_puts_P("get filter [number]\n\n");
			vt100_setattr(0);
			
			term_puts_P(
			"Prints information about the specified filter. Without a given number " \
			"the command generates a table with an overview of all filter.\n"
			);
		}
		else if (!strncmp_P(s, s_set, 3)) {
			vt100_setattr(1);
			term_puts_P("set bitrate|filter ...\n\n");
			vt100_setattr(0);
			
			term_puts_P("1. ");
			vt100_setattr(1);
			term_puts_P("set bitrate [125|250|500|1000]\n\n");
			vt100_setattr(0);
			
			term_puts_P("Set a new bitrate for the CAN bus.\n\n");
			
			term_puts_P("2. ");
			vt100_setattr(1);
			term_puts_P("set filter number [disable|mask id]\n\n");
			vt100_setattr(0);
			
			term_puts_P("To receive all messages you simply have to type:\n" \
			"  $ set filter n 0 0\n" \
			"n is a one of the message-objects (0..14).\n\n"
			);
			
			#if  HARDWARE_VERSION_MINOR >= 2
			term_puts_P("3. ");
			vt100_setattr(1);
			term_puts_P("set term on|off\n\n");
			vt100_setattr(0);
			
			term_puts_P("Activate/Deactivate a 120 Ohm terminating resistor.\n\n");
			#endif
		}
		else {
			error("no help page available for this command");
		}
	}
	
	return 1;
}

// ----------------------------------------------------------------------------
uint8_t start_output(char *param, char data)
{
	copy_stop_output = FALSE;
	tmp_stop_output = FALSE;
	flag_stop_output = FALSE;
	return 1;
}

// ----------------------------------------------------------------------------
uint8_t stop_output(char *param, char data)
{
	copy_stop_output = TRUE;
	flag_stop_output = TRUE;
	return 1;
}

// ----------------------------------------------------------------------------
// send id length [rtr|data]

uint8_t send_can_messages(char *param, char data)
{
	if (param != NULL) {
		char *s = get_parameter(param, 1);
		uint8_t length = get_parameter_length(s);
		
		can_t m;
		if (length > 8 || length == 0)
			goto error;
		
		if (length > 3)
			m.flags.extended = TRUE;
		else
			m.flags.extended = FALSE;
		
		// CAN ID auslesen
		if (!term_get_long(s, &m.id, 16)) {
			error("Invalid characters in CAN-ID");		// Fehler im String
			return 1;
		}
		
		// Laenge auslesen
		s = get_next_parameter(s);
		if (get_parameter_length(s) != 1 || ((uint8_t)( *s - '0')) > 8 ) {
			error("Invalid length code");
			return 1;
		}
		
		m.length = *s - '0';
		
		// RTR oder Daten auslesen
		s = get_next_parameter(s);
		length = get_parameter_length(s);
		
		if (length == 3 && !strncmp(s, "rtr", 3)) {
			// RTR Frame
			m.flags.rtr = TRUE;
		}
		else {
			// Daten auslesen
			m.flags.rtr = FALSE;
			
			uint8_t i = 0;
			while (length)
			{
				if (length & 0x01) {
					// ungerade Datenanzahl
					error("Only tupels of 2*n byte allowed in data segment");
					return 1;
				}
				
				char *p = s;
				do {
					if (!isxdigit(*s) || !isxdigit(*(s+1)))
						goto error;
					
					m.data[i] = hex_to_byte(s);
					s += 2;
					i++;
					
					if (i > 8) {
						// Datenteil zu lang
						error("Data segment to long");
						return 1;
					}
					length -= 2;
				} while (length);
				
				s = get_next_parameter(p);
				length = get_parameter_length(s);
			}
			
			if (i != m.length) {
				// Falsche Laenge des Datenteils
				error("Given data length differs from length of data segment");
				return 1;
			}
		}
		
		if (!can_send_message(&m))
			error("Could not send message");
		
		return 1;
		
	error:
		error("Wrong format");
		return 1;
	}
	
	return 1;
}

// ----------------------------------------------------------------------------
// filter number [e|ext|extended] [r|rtr] [disable|mask id]

uint8_t set_filter(char *param, char data)
{
	char *s = get_next_parameter(param);
	uint8_t length = get_parameter_length(s);
	
	if (!length) {
		error("No Parameters");
		return 1;
	}
	
	// Filter anlegen
	can_filter_t filter;
	
	int number;
	if (sscanf_P(s, PSTR("%i"), &number) != 1 || number > 14) {
		error("Invaild filter number");
		return 1;
	}
	
	s = get_next_parameter(s);
	length = get_parameter_length(s);
	
	// check for [e|ext|extended]
	if ((length == 1 && !strncmp_flash(s, "e", 1)) ||
		(length == 3 && !strncmp_flash(s, "ext", 3)) ||
		(length == 8 && !strncmp_flash(s, "extended", 8)))
	{
		filter.flags.extended = 0x3;
		
		s = get_next_parameter(s);
		length = get_parameter_length(s);
	}
	// check for [s|std|standard]
	else if ((length == 1 && !strncmp_flash(s, "s", 1)) ||
		(length == 3 && !strncmp_flash(s, "std", 3)) ||
		(length == 8 && !strncmp_flash(s, "standard", 8)))
	{
		filter.flags.extended = 0x2;
		
		s = get_next_parameter(s);
		length = get_parameter_length(s);
	}
	else {
		filter.flags.extended = 0;
	}
	
	// check for [r|rtr]
	if ((length == 1 && !strncmp_flash(s, "r", 1)) ||
		(length == 3 && !strncmp_flash(s, "rtr", 3)))
	{
		filter.flags.rtr = 3;
		s = get_next_parameter(s);
		length = get_parameter_length(s);
	}
	// check for [nr|non-rtr]
	else if ((length == 2 && !strncmp_flash(s, "nr", 2)) ||
			 (length == 7 && !strncmp_flash(s, "non-rtr", 7)))
	{
		filter.flags.rtr = 2;
		s = get_next_parameter(s);
		length = get_parameter_length(s);
	}
	else {
		filter.flags.rtr = 0;
	}
	
	if (sscanf_P(s, PSTR("%lx %lx"), &filter.mask, &filter.id) != 2) {
		// Ueberpruefen ob nicht vielleicht ein "disable"
		// im String steht.
		length = get_parameter_length(s);
		if (length == 7 && !strncmp_flash(s, "disable", 7)) {
			if (!can_disable_filter(number)) {
				error("Could not disable filter");
			}
			return 1;
		}
		error("Invalid characters in CAN-mask or id");
		return 1;
	}
	
	if (!can_set_filter(number, &filter))
		error("Could not set filter");
	return 1;
}

// ----------------------------------------------------------------------------
// bitrate [125|250|500|1000]

uint8_t set_bitrate(char *param, char data)
{
	char *s = get_parameter(param, 1);
	uint8_t length = get_parameter_length(s);
	
	if (!length) {
		error("no parameters");
		return 1;
	}
	
	// read bitrate
	int bitrate;
	if (sscanf_P(s, PSTR("%i"), &bitrate) != 1) {
		goto bitrate_error;
	}
	
	// try to set bitrate
	if (bitrate == 125)
		can_init(BITRATE_125_KBPS);
	else if (bitrate == 250)
		can_init(BITRATE_250_KBPS);
	else if (bitrate == 500)
		can_init(BITRATE_500_KBPS);
	else if (bitrate == 1000)
		can_init(BITRATE_1_MBPS);
	else
		goto bitrate_error;
	return 1;
	
bitrate_error:
	error("Invalid bitrate");
	return 1;
}

// ----------------------------------------------------------------------------
// get filter [number]

uint8_t get_values(char *param, char data)
{
	char *s = get_parameter(param, 1);
	uint8_t length = get_parameter_length(s);
	
	if (!length) {
		// no parameters
		return 1;
	}
	
	if (!strncmp_flash(s, "filter", 6) && length == 6)
	{
		s = get_next_parameter(s);
		length = get_parameter_length(s);
		
		if (length == 0) {
			// print all filters
			term_puts_P("   :     mask       id  options\n" \
						"---:----------------------------\n");
			
			for (uint8_t i=0;i<15;i++) {
				printf_P(PSTR("%2d : "), i);
				
				can_filter_t filter;
				uint8_t res = can_get_filter(i, &filter);
				if (res == 0xff || res == 0) {
					term_puts_P("       *        *\n");
				}
				else if (res == 2) {
					term_puts_P("       -        -\n");
				}
				else {
					printf_P(PSTR("%8lx %8lx "), filter.mask, filter.id);
					
					bool output = false;
					if (filter.flags.extended) {
						if (filter.flags.extended == 0x2)
							term_puts_P(" only standard");
						else
							term_puts_P(" only extended");
						
						output = true;
						if (filter.flags.rtr)
							term_puts_P(" and");
					}
					else if (filter.flags.rtr)
						output = true;
					
					if (filter.flags.rtr == 2)
						term_puts_P(" non-rtr");
					else if (filter.flags.rtr == 3)
						term_puts_P(" rtr");
					
					if (!output)
						term_puts_P("all");
					
					term_puts_P(" frames\n");
				}
			}
		}
		else {
			// only one filter requested => get number
			int number;
			if (sscanf_P(s, PSTR("%d"), &number) != 1 || number > 14) {
				error("Invalid filter number");
				return 1;
			}
			
			can_filter_t filter;
			uint8_t res = can_get_filter(number, &filter);
			if (res == 0)
				error("could not read filter.");
			else if (res == 2)
				term_puts_P("filter currently not used.\n");
			else if (res == 0xff)
				term_puts_P("currently no statement posible.\n");
			else {
				if (filter.flags.extended != 0x2) {
					printf_P(PSTR("mask : %8lx\n" \
								  "  id : %8lx\n"), filter.mask, filter.id);
				} else {
					printf_P(PSTR("mask : %4lx\n" \
								  "  id : %4lx\n"), filter.mask, filter.id);
				}
				
				if (filter.flags.rtr == 0 && filter.flags.extended == 0)
					term_puts_P("all");
				else {
					term_puts_P("only ");
					
					if (filter.flags.extended) {
						if (filter.flags.extended == 0x2)
							term_puts_P("standard");
						else
							term_puts_P("extended");
						
						if (filter.flags.rtr)
							term_puts_P(", ");
					}
					
					if (filter.flags.rtr == 2)
						term_puts_P("non-rtr");
					else if (filter.flags.rtr == 3)
						term_puts_P("rtr");
				}
				
				term_puts_P(" frames\n");
			}
		}
	}
	
	return 1;
}

// ----------------------------------------------------------------------------
uint8_t set_values(char *param, char data)
{
	char *s = get_parameter(param, 1);
	uint8_t length = get_parameter_length(s);
	
	if (!strncmp_flash(s, "bitrate", 7) && length == 7) {
		set_bitrate(s, 0);
	}
	else if (!strncmp_flash(s, "filter", 6) && length == 6) {
		set_filter(s, 0);
	}
	#if  HARDWARE_VERSION_MINOR >= 2
	else if (!strncmp_flash(s, "term", 4) && length == 4) {
		s = get_next_parameter(s);
		length = get_parameter_length(s);
		
		if (!strncmp_flash(s, "on", 2) && length == 2) {
			SET(CAN_TERM);
		}
		else if (!strncmp_flash(s, "off", 3) && length == 3) {
			RESET(CAN_TERM);
		}
		else {
			error("Unknown option. Should be \"on\" or \"off\"");
		}
	}
	#endif
	else {
		error("Command not yet implemented!");
	}
	
	return 1;
}

// ----------------------------------------------------------------------------
uint8_t restart(char *param, char data)
{
	term_puts_P("shell restart\n");
	wdt_enable(WDTO_500MS);
	
	for(;;);
	
	return 1;
}

// ----------------------------------------------------------------------------
uint8_t print_hex(char *param, char data)
{
	if (data != 0)
	{
		// Beenden sobald ein <STRG> + C empfangen wird
		if (data == 0x03) {
			return 1;
		}
		
		term_putc(data);
		term_puts_P(": 0x");
		term_put_hex(data);
		term_putc_cr('\n');
	}
	
	return 0;
}

// ----------------------------------------------------------------------------
uint8_t clear_screen(char *param, char data)
{
	term_puts_P("\033[;H\033[2J");		// VT100 -> Terminal loeschen
	term_puts_P("\033[H");				// Cursor oben links positionieren
	
	return 1;
}

// ----------------------------------------------------------------------------
uint8_t print_version(char *param, char data)
{
	term_puts_P("Firmware Version: ");
	term_put_int(SOFTWARE_VERSION_MAJOR);
	term_putc('.');
	term_put_int(SOFTWARE_VERSION_MINOR);
	term_puts_P("   (compiled " __DATE__ ")\n");
	term_puts_P("Hardware Version: ");
	term_put_int(HARDWARE_VERSION_MAJOR);
	term_putc('.');
	term_put_int(HARDWARE_VERSION_MINOR);
	term_putc_cr('\n');
	
	#if CAN_RX_BUFFER_SIZE > 0 || CAN_TX_BUFFER_SIZE > 0
	term_puts_P("\nbuild with:\n");
	#if CAN_RX_BUFFER_SIZE > 0
		term_puts_P("- ");
		term_put_int(CAN_RX_BUFFER_SIZE);
		term_puts_P(" messages rx buffer\n");
	#endif
	#if CAN_TX_BUFFER_SIZE > 0
		term_puts_P("- ");
		term_put_int(CAN_TX_BUFFER_SIZE);
		term_puts_P(" messages tx buffer\n");
		#if CAN_FORCE_TX_ORDER
			term_puts_P("- force tx order support\n");
		#endif
	#endif
	#endif
	
	return 1;
}

// ----------------------------------------------------------------------------
uint8_t send_bulk_messages(char *param, char data)
{
	can_t msg;
	
	msg.flags.extended = true;
	msg.flags.rtr = false;
	
	msg.length = 8;
	
	msg.id = 0x200;
	can_send_message( &msg );
	_delay_ms(2);
	
	msg.id = 0x1ff;
	can_send_message( &msg );
	msg.id = 0x1fe;
	can_send_message( &msg );
	msg.id = 0x1fd;
	can_send_message( &msg );
	_delay_ms(1);
	
	msg.id = 0x1fc;
	can_send_message( &msg );
	_delay_ms(1);
	msg.id = 0x1fb;
	can_send_message( &msg );
	_delay_ms(20);
	
	msg.id = 0x1fa;
	can_send_message( &msg );
	_delay_ms(10);
	
	uint8_t i;
	for (i = 0; i < 20; i++)
	{
		msg.id = 0x100 - i;
		can_send_message( &msg );
	}
	
	return 1;
}
