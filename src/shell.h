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

#ifndef TERMINAL_H
#define TERMINAL_H

// ----------------------------------------------------------------------------
/**
 * \brief	VT100-compatible terminal emulation
 *
 * \author	Fabian Greif <fabian.greif@rwth-aachen.de>
 * \version	$Id: shell.h 6802 2008-11-12 10:25:05Z fabian $
 */

#include "termio.h"
#include <stdint.h>

// ----------------------------------------------------------------------------
/**
 * \brief	Datenstrucktur zur Aufnahme eines Terminal Befehls
 */
typedef struct {
	const PGM_P pstr;				//!< Befehls-String
	uint8_t length;					//!< Laenge des Befehls-String
	uint8_t (*fp)(char *, char);	//!< Pointer zur Funktion die aufgerufen wird
} ShellProgram;

// ----------------------------------------------------------------------------
/**
 * \brief	Liefert einen Pointer auf das n-te Argument im String.
 *
 * Argumente muessen durch (beliebig viele) Leerzeichen getrennt sein.
 */
extern char* get_parameter(char *cl, uint8_t n);

// ----------------------------------------------------------------------------
/**
 * \brief	Liefert die Laenge des Parameters
 */
extern uint8_t get_parameter_length(char *cl);

// ----------------------------------------------------------------------------
/**
 * \brief	Liefert einen Pointer auf das naechste Argument im String.
 *
 * Argumente muessen durch (beliebig viele) Leerzeichen getrennt sein.
 */
extern char* get_next_parameter(char *cl);

// ----------------------------------------------------------------------------
extern void init_command_shell(void);

// ----------------------------------------------------------------------------
/**
 * \brief	
 */
extern void command_shell(void);

#endif	// TERMINAL_H
