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

#ifndef	CONFIG_H
#define	CONFIG_H

// ----------------------------------------------------------------------------
// hardware and software version

#define HARDWARE_VERSION_MAJOR	1
//#define	HARDWARE_VERSION_MINOR	1
//#define	HARDWARE_VERSION_MINOR	2
#define	HARDWARE_VERSION_MINOR	3


#define	SOFTWARE_VERSION_MAJOR	1
#define	SOFTWARE_VERSION_MINOR	3

// ----------------------------------------------------------------------------
// use can-interface with support for extended identifiers (29-bit)

#define	SUPPORT_AT90CAN			1

#define	SUPPORT_EXTENDED_CANID	1
#define	SUPPORT_TIMESTAMPS		1

#define	CAN_FORCE_TX_ORDER		1

#define	CAN_RX_BUFFER_SIZE		32
#define	CAN_TX_BUFFER_SIZE		64

// ----------------------------------------------------------------------------
extern void debugger_indicate_tx_traffic(void);
extern void debugger_indicate_rx_traffic(void);

#define	CAN_INDICATE_TX_TRAFFIC_FUNCTION	debugger_indicate_tx_traffic()
#define	CAN_INDICATE_RX_TRAFFIC_FUNCTION	debugger_indicate_rx_traffic()

// ----------------------------------------------------------------------------
// pin defintion

#if  HARDWARE_VERSION_MINOR == 1

	#define	LED_GREEN		B,7
	#define	LED_YELLOW		B,5
	#define	LED_RED			B,6
	
	#define	SW0				B,2
	#define	SW1				B,3

#elif  HARDWARE_VERSION_MINOR == 2

	#define	LED_PWR			D,7
	#define	LED_TX			B,7
	#define	LED_RX			B,6
	
	#define	LED_DUO1_1		F,7
	#define	LED_DUO1_2		F,4
	
	#define	LED_DUO2_1		F,6
	#define	LED_DUO2_2		F,3
		
	#define	CAN_TERM		C,0
	
	#define	SW2				B,5
	#define	SW1				B,4
	
#elif HARDWARE_VERSION_MINOR == 3
	
	#define	LED_PWR			D,7
	
	#define	LED_RX			G,4
	#define	LED_RX_2		G,3
	
	#define	LED_TX			B,7
	#define	LED_TX_2		B,6
	
	#define	LED_DUO1_1		F,7
	#define	LED_DUO1_2		F,4
	
	#define	LED_DUO2_1		F,6
	#define	LED_DUO2_2		F,3
		
	#define	CAN_TERM		C,0
	
	#define	SW2				B,5
	#define	SW1				B,4

#endif

#define	RD				G,1
#define	WR				G,0

#define	USB_DATA		A

#define	USB_RXF			E,7
#define	USB_TXE			E,6


#endif	// CONFIG_H
