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

#ifndef	PROGRAMS_H
#define	PROGRAMS_H

// ----------------------------------------------------------------------------
/**
 * \brief	user programs for use in terminal emulation
 *
 * \author	Fabian Greif <fabian.greif@rwth-aachen.de>
 * \version	$Id: shell_programs.h 6802 2008-11-12 10:25:05Z fabian $
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdint.h>

#include "shell.h"

// ----------------------------------------------------------------------------
/**
 * \brief	Liste der Befehle mit Callback-Funktionen
 */
extern const ShellProgram terminal_command_list[];

// ----------------------------------------------------------------------------
/**
 * \brief	Laenge der Befehls-Liste
 */
extern uint8_t terminal_command_list_length;


// ----------------------------------------------------------------------------

extern uint8_t flag_stop_output;

#endif	// PROGRAMS_H
