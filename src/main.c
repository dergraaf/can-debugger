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
/**
 * \brief	CAN Debugger
 *
 * \author	Fabian Greif <fabian.greif@rwth-aachen.de>
 * \version	$Id: main.c 6920 2008-12-03 15:26:58Z fabian $
 *
 * AT90CAN128
 * 
 * lfuse: 0xfe
 * hfuse: 0xd1
 * efuse: 0xfd
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include <util/delay.h>

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "config.h"
#include "shell.h"

#include "usbcan_protocol.h"
#include "shell_protocol.h"

#include "can.h"
#include "utils.h"

// ----------------------------------------------------------------------------
#define	LED_1_GREEN		\
	do { SET(LED_DUO1_1);	\
	RESET(LED_DUO1_2); } while (0)

#define	LED_1_RED		\
	do { RESET(LED_DUO1_1);	\
	SET(LED_DUO1_2); } while (0)

#define	LED_1_OFF		\
	do { RESET(LED_DUO1_1);	\
	RESET(LED_DUO1_2); } while (0)

#define	LED_2_GREEN		\
	do { SET(LED_DUO2_1);	\
	RESET(LED_DUO2_2); } while (0)

#define	LED_2_RED		\
	do { RESET(LED_DUO2_1);	\
	SET(LED_DUO2_2); } while (0)

#define	LED_2_OFF		\
	do { RESET(LED_DUO2_1);	\
	RESET(LED_DUO2_2); } while (0)

// ----------------------------------------------------------------------------
// globale Variablen

typedef enum {
	SHELL, DONGLE, UNKNOWN
} mode_t;

#if  HARDWARE_VERSION_MINOR >= 2
static mode_t mode_of_operation = DONGLE;
#endif

uint8_t volatile f_timer = FALSE;

static int putchar__(char c, FILE *stream) {
	term_putc_cr(c);
	return 0;
}

static int getchar__(FILE *stream) {
	if (term_data_available()) {
		return term_getc();
	}
	return -1;
}

static FILE mystdout = FDEV_SETUP_STREAM(putchar__, getchar__, _FDEV_SETUP_RW);

// ----------------------------------------------------------------------------
// Stellt fest in welchem Modus sich der Debugger befindet

static mode_t get_mode(void)
{
	#if  HARDWARE_VERSION_MINOR == 1
	
		if (!IS_SET(SW0) && !IS_SET(SW1))
			return DONGLE;
		else if (IS_SET(SW0) && IS_SET(SW1))
			return SHELL;
		
		return UNKNOWN;
	
	#else
	
		return mode_of_operation;
	
	#endif
}

// ----------------------------------------------------------------------------
// Setzt & initialisiert einen neuen Modus

static void change_mode(mode_t mode)
{
	#if  HARDWARE_VERSION_MINOR == 1
	// TODO gruene LED ansteuern => Interrupt
	RESET(LED_GREEN);
	RESET(LED_RED);
	#endif
	
	switch (mode)
	{
		case SHELL:
			#if  HARDWARE_VERSION_MINOR == 1
			// Timer 3 initialisieren, Fast PWM 10-Bit, clk = f_clk / 8
			TCCR1A = (1<<WGM11)|(1<<WGM10)|(1<<COM1C1);
			TCCR1B = (1<<WGM12)|(1<<CS11);
			OCR1C = 0;
			TIMSK1 = (1<<TOIE1);
			#else
			LED_1_OFF;
			LED_2_GREEN;
			#endif
			
			can_disable_filter(CAN_ALL_FILTER);
			break;
		
		case DONGLE:
			#if  HARDWARE_VERSION_MINOR == 1
			TCCR1A = (1<<WGM11)|(1<<WGM10);
			SET(LED_GREEN);
			#else
			LED_1_GREEN;
			LED_2_OFF;
			#endif
			
			can_filter_t filter = {
				.mask = 0,
				.id = 0,
				.flags.extended = 0,
				.flags.rtr = 0
			};
			
			for (uint8_t i=11;i<15;i++) {
				can_set_filter(i, &filter);
			}
			break;
		
		default:
			#if  HARDWARE_VERSION_MINOR == 1
			SET(LED_RED);
			
			TCCR1A = (1<<WGM11)|(1<<WGM10);
			#else
			LED_1_OFF;
			LED_2_OFF;
			#endif
			break;
	}
}

// ----------------------------------------------------------------------------
void
debugger_indicate_rx_traffic(void)
{
	static bool status = true;
	
	if (status) {
		RESET(LED_RX);
		SET(LED_RX_2);
	}
	else {
		RESET(LED_RX);
		RESET(LED_RX_2);
	}
	
	status = !status;
}

// ----------------------------------------------------------------------------
void
debugger_indicate_tx_traffic(void)
{
	static bool status = true;
	
	if (status) {
		RESET(LED_TX);
		SET(LED_TX_2);
	}
	else {
		RESET(LED_TX);
		RESET(LED_TX_2);
	}
	
	status = !status;
}

// ----------------------------------------------------------------------------
// Hauptprogramm

int main(void)
{
	// umleiten der Standardausgabe
	stdout = stdin = &mystdout;
	
	#if  HARDWARE_VERSION_MINOR == 1
	
		SET(LED_GREEN);
		SET(LED_YELLOW);
		SET(LED_RED);
		
		SET_OUTPUT(LED_GREEN);
		SET_OUTPUT(LED_YELLOW);
		SET_OUTPUT(LED_RED);
		
		SET_INPUT(SW0);
		SET_INPUT(SW1);
		SET(SW0);
		SET(SW1);
	
	#elif  HARDWARE_VERSION_MINOR >= 2
	
		SET(LED_PWR);
		SET(LED_RX);
		SET(LED_TX);
		
		SET_OUTPUT(LED_DUO1_1);
		SET_OUTPUT(LED_DUO1_2);
		SET_OUTPUT(LED_DUO2_1);
		SET_OUTPUT(LED_DUO2_2);
		
		LED_1_OFF;
		LED_2_OFF;
		
		SET_OUTPUT(LED_PWR);
		SET_OUTPUT(LED_RX);
		SET_OUTPUT(LED_TX);
		
		#if  HARDWARE_VERSION_MINOR == 3
		
			RESET(LED_RX_2);
			RESET(LED_TX_2);
			
			SET_OUTPUT(LED_RX_2);
			SET_OUTPUT(LED_TX_2);
		
		#endif
		
		RESET(CAN_TERM);
		SET_OUTPUT(CAN_TERM);
		
		// Timer 3, CTC, clk = f_clk / 64, => 10ms
		OCR1A = 2500;
		TIMSK1 = (1<<OCIE1A);
		
		TCCR1A = 0;
		TCCR1B = (1<<WGM12)|(1<<CS11)|(1<<CS10);
		
		SET_INPUT(SW2);
		SET(SW2);
	
	#endif
	
	SET_OUTPUT(RD);
	SET_OUTPUT(WR);
	SET(RD);
	RESET(WR);
	
	SET_INPUT(USB_RXF);
	SET_INPUT(USB_TXE);
	SET(USB_RXF);
	SET(USB_TXE);
	
	init_command_shell();
	
	// Interrupts aktivieren
	sei();
	
	_delay_ms(20);
	
	#if  HARDWARE_VERSION_MINOR == 1
	
		RESET(LED_RED);
		RESET(LED_YELLOW);
	
	#else
	
		RESET(LED_RX);
		RESET(LED_TX);
	
	#endif
	
	can_init(BITRATE_125_KBPS);
	
	while(1)
	{
		static mode_t mode = UNKNOWN;
		
		// Ueberpruefen ob sich der Modus geaendert hat
		mode_t tmode = get_mode();
		if (mode != tmode) 
		{
			mode = tmode;
			
			// neuen Modus initialisieren
			change_mode(tmode);
		}
		
		if (mode == SHELL)
		{
			shell_handle_protocol();
		}
		else if (mode == DONGLE)
		{
			usbcan_handle_protocol();
		}
	}
	
	return 0;
}

// ----------------------------------------------------------------------------
// Timer 3 Overflow-Interrupt (alle 0.512 ms)

#if  HARDWARE_VERSION_MINOR == 1
ISR(TIMER1_OVF_vect)
{
	static uint8_t counter = 0;
	static uint8_t step = 0;
	static bool up = true;
	
	counter++;
	if (counter == 100) {
		counter = 0;
		
		if (up) {
			step++;
			OCR1C = step * step - 1;
			
			if (step >= 32)
				up = false;
		}
		else {
			step--;
			OCR1C = step * step;
			
			if (step == 0) {
				up = true;
			}
		}
	}
}
#endif

#if  HARDWARE_VERSION_MINOR >= 2
ISR(TIMER1_COMPA_vect)
{
	static uint16_t switch_counter = 0;
	static bool select_mode = false;
	static bool pressed = false;
	static mode_t temp_mode;
	
	if (select_mode)
	{
		static bool led_status = true;
		static uint8_t led_counter = 0;
		static uint16_t return_counter = 0;
		
		if (!IS_SET(SW2)) 
		{
			if (!pressed)
				switch_counter++;
		}
		else {
			switch_counter = 0;
			pressed = false;
		}
		
		if (switch_counter > 10)
		{
			switch_counter = 0;
			pressed = true;
			return_counter = 0;
			
			if (temp_mode == SHELL)
				temp_mode = DONGLE;
			else
				temp_mode = SHELL;
		}
		
		
		return_counter++;
		if (return_counter > 400)
		{
			return_counter = 0;
			select_mode = false;
			switch_counter = 0;
			mode_of_operation = temp_mode;
			
			if (temp_mode == SHELL) {
				LED_1_OFF;
				LED_2_GREEN;
			}
			else {
				LED_1_GREEN;
				LED_2_OFF;
			}
			
			return;
		}
		
		// toggle leds
		led_counter++;
		if (led_counter > 10)
		{
			led_counter = 0;
			
			if (temp_mode == SHELL)
			{
				LED_1_OFF;
				
				if (led_status)
					LED_2_OFF;
				else
					LED_2_GREEN;
			}
			else
			{
				LED_2_OFF;
				
				if (led_status)
					LED_1_OFF;
				else
					LED_1_GREEN;
			}
			
			led_status = !led_status;
		}
	}
	else
	{
		if (!IS_SET(SW2))
		{
			switch_counter++;
			
			if (switch_counter >= 150)
			{
				switch_counter = 0;
				select_mode = true;
				temp_mode = mode_of_operation;
				pressed = true;
			}
		}
		else
		{
			switch_counter = 0;
			pressed = false;
		}
	}
}
#endif
