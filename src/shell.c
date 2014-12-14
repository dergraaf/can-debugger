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

#include <avr/pgmspace.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#include "shell.h"
#include "shell_programs.h"

#define CENTER		'\r'
#define CNEWLINE	'\n'
#define CBS			'\b'

#define	CL_HISTORY_LINES	10
#define CMAXCL				50

#define	ERROR_UNKNOWN			0x01

// ----------------------------------------------------------------------------
typedef struct tShell
{
	struct tShell *left;
	struct tShell *right;
	
	char cl[CMAXCL];
	uint8_t length;
} tShell;

// ----------------------------------------------------------------------------
typedef struct
{
	const ShellProgram *program_list;
	uint8_t program_list_length;
	
	struct {
		tShell history[CL_HISTORY_LINES];
		tShell *active;
		tShell *head;
	} shell;
	
	uint8_t (*pt_func)(char *, char);
	enum { COMMANDSHELL_MODE_INIT,
		COMMANDSHELL_MODE_RUN,
		COMMANDSHELL_MODE_BREAK
	} shell_state;
	
	struct {
		int8_t pos; // current Position
		uint8_t escape_state;
		
		enum { GET_TERMSTRING_STATE_INIT,
			GET_TERMSTRING_STATE_SAMPLE,
			GET_TERMSTRING_STATE_RETURN
		} state;
	} get;
} tShellState;

tShellState virtual_shell;

// ----------------------------------------------------------------------------
void insert_right(tShell *node, tShell *new)
{
	new->left = node;
	new->right = node->right;
	node->right = new;
	new->right->left = new;
}

void delete(tShell *node)
{
	node->right->left = node->left;
	node->left->right = node->right;
}

// ----------------------------------------------------------------------------
// Gibt einen Fehlercode aus

void print_error(uint8_t error_code)
{
	switch (error_code) {
		case ERROR_UNKNOWN:
			term_puts_P("shell: command not found\n");
			break;
		default:
			term_puts_P("shell: invalid error-code\n");
			break;
	}
}

// ----------------------------------------------------------------------------
// Initialisiert die Kommandozeile

extern const ShellProgram shell_program_list[];
extern uint8_t shell_program_list_length;

void init_command_shell(void)
{
	virtual_shell.shell_state = COMMANDSHELL_MODE_INIT;
	virtual_shell.get.state = GET_TERMSTRING_STATE_INIT;
	virtual_shell.get.pos = 0;
	virtual_shell.get.escape_state = 0;
	
	// TODO
	virtual_shell.program_list = shell_program_list;
	virtual_shell.program_list_length = shell_program_list_length;
	
	virtual_shell.pt_func = NULL;
	
	// erzeuge eine verkettete Liste
	for (uint8_t i=0;i<CL_HISTORY_LINES;i++) {
		insert_right(&virtual_shell.shell.history[0], &virtual_shell.shell.history[i]);
		
		virtual_shell.shell.history[i].cl[0] = '\0';
		virtual_shell.shell.history[i].length = 0;
	}
	
	virtual_shell.shell.head = &virtual_shell.shell.history[0];
	virtual_shell.shell.active = &virtual_shell.shell.history[0];
}

// ----------------------------------------------------------------------------
// liest einen durch <Enter> abgeschlossenen String von der seriellen 
// Schnittstelle

void get_console_string(char c)
{
	if (virtual_shell.get.state == GET_TERMSTRING_STATE_INIT )
	{
		virtual_shell.get.state = GET_TERMSTRING_STATE_SAMPLE;
		virtual_shell.get.pos = 0;
		virtual_shell.get.escape_state = 0;
		
		if (virtual_shell.shell.head == virtual_shell.shell.active) {
			virtual_shell.shell.head = virtual_shell.shell.head->left;
		}
		else {
			delete(virtual_shell.shell.active);
			insert_right(virtual_shell.shell.head, virtual_shell.shell.active);
		}
		
		// Falls die Zeile leer ist wieder sie wieder hinten anstellt
		if (virtual_shell.shell.head->right->length == 0) {
			virtual_shell.shell.head = virtual_shell.shell.head->right;
		}
		
		virtual_shell.shell.active = virtual_shell.shell.head;
		virtual_shell.shell.active->cl[0] = '\0';
		virtual_shell.shell.active->length = 0;
	}
	
	if (virtual_shell.get.escape_state == 1) {
		if (c == '[') // 0x56
			virtual_shell.get.escape_state = 2;
		else
			virtual_shell.get.escape_state = 0;
		return;
	}
	else if (virtual_shell.get.escape_state == 2) {
		if (c == 'A') {
			// Pfeil Oben
			if (virtual_shell.shell.active->right != virtual_shell.shell.head) {
				virtual_shell.shell.active = virtual_shell.shell.active->right;
				virtual_shell.get.pos = virtual_shell.shell.active->length;
				
				term_puts_P("\x1b[2K" "\x1b[50D" "$ ");
				term_puts(virtual_shell.shell.active->cl);
			}
		}
		else if (c == 'B') {
			// Pfeil Unten
			if (virtual_shell.shell.active != virtual_shell.shell.head) {
				virtual_shell.shell.active = virtual_shell.shell.active->left;
				virtual_shell.get.pos = virtual_shell.shell.active->length;
				
				term_puts_P("\x1b[2K" "\x1b[50D" "$ ");
				term_puts(virtual_shell.shell.active->cl);
			}
		}
		else if (c == 'C') {
			// Pfeil Rechts
			if (virtual_shell.get.pos < virtual_shell.shell.active->length) {
				virtual_shell.get.pos++;
				term_puts_P("\x1b[C");
			}
		}
		else if (c == 'D') {
			// Pfeil Links
			if (virtual_shell.get.pos > 0) {
				virtual_shell.get.pos--;
				term_puts_P("\x1b[D");
			}
		}
		
		virtual_shell.get.escape_state = 0;
		return;
	}
	
	switch (c)
	{
		// Backspace
		case 0x7f:
		case CBS:
			if (virtual_shell.get.pos > 0) {
				virtual_shell.shell.active->length--;
				virtual_shell.get.pos--;
				
				uint8_t length = virtual_shell.shell.active->length;
				char *p = &virtual_shell.shell.active->cl[virtual_shell.get.pos];
				
				if (virtual_shell.get.pos != length) {
					// Zeichen in die Mitte des Strings loeschen
					memmove(p, p + 1, length - virtual_shell.get.pos + 1);
					
					// Zeile ab eins vor dem Cursor loeschen und neu zeichnen
					term_puts_P("\x1b[D" "\x1b[s" "\x1b[K");
					term_puts(virtual_shell.shell.active->cl + virtual_shell.get.pos);
					term_puts_P("\x1b[u");
				}
				else {
					// Zeichen am Ende des Strings entfernen
					*p = '\0';
					
					term_putc('\b');
					term_putc(' ');
					term_putc('\b');
				}
			}
			break;
		
		case 0x1B: // <ESC>
			virtual_shell.get.escape_state = 1;
			break;
		
		case CENTER:
			virtual_shell.shell.active->cl[virtual_shell.shell.active->length] = '\0';
			virtual_shell.get.state = GET_TERMSTRING_STATE_RETURN;
			break;
		
		default:
			if (virtual_shell.get.pos < (CMAXCL-1))
			{
				uint8_t length = virtual_shell.shell.active->length;
				char *p = &virtual_shell.shell.active->cl[virtual_shell.get.pos];
				
				if (virtual_shell.get.pos != length) {
					// Zeichen in die Mitte des Strings einfuegen
					memmove(p + 1, p, length - virtual_shell.get.pos);
					virtual_shell.shell.active->cl[virtual_shell.get.pos] = c;
					
					// Zeile ab Cursor loeschen und neu zeichnen
					term_puts_P("\x1b[s" "\x1b[K");
					term_puts(virtual_shell.shell.active->cl + virtual_shell.get.pos);
					term_puts_P("\x1b[u" "\x1b[C");
				}
				else {
					*p++ = c;
					*p = '\0';	// String wieder mit '\0' abschliessen
					term_putc(c);
				}
				
				virtual_shell.get.pos++;
				virtual_shell.shell.active->length++;
			}
			break;
	}
}

// ----------------------------------------------------------------------------
// Liefert einen Pointer auf das n-te Argument im String.
// Argumente koennen durch beliebig viele Leerzeichen getrennt sein.

char* get_parameter(char *cl, uint8_t n)
{
	char *s = cl;
	
	// skip leading whitepaces if any
	while( (*s != '\0') && isblank(*s) ) { s++; }
	
	// go to n-th arg (0 is the command)
	for(uint8_t i = 0; i<n; i++)
	{
		// find first character of argument
		while( (*s != '\0') && !isblank(*s) )
			s++;
		
		// skip whitespaces separating arguments
		while( (*s != '\0') && isblank(*s) )
			s++;
	}
	
	// return pointer to first character in n-th argument
	return s;
}

// ----------------------------------------------------------------------------
// Liefert einen Pointer auf das naechste Argument im String.
// Argumente koennen durch beliebig viele Leerzeichen getrennt sein.

char* get_next_parameter(char *cl)
{
	// return pointer to first character in next argument
	return get_parameter(cl, 1);
}

// ----------------------------------------------------------------------------
// Liefert die Laenge des Parameters

uint8_t get_parameter_length(char *cl)
{
	char *s = cl;
	uint8_t i = 0;
	
	while( (*s != '\0') && !isblank(*s) ) {
		s++;
		i++;
	}
	
	return i;
}

// ----------------------------------------------------------------------------

void command_shell(void)
{
	if (virtual_shell.shell_state == COMMANDSHELL_MODE_INIT) {
		term_puts_P("$ ");
		virtual_shell.get.state = GET_TERMSTRING_STATE_INIT;
		virtual_shell.shell_state = COMMANDSHELL_MODE_RUN;
	}
	
	if (virtual_shell.shell_state == COMMANDSHELL_MODE_BREAK) {
		if (term_data_available()) {
			if (virtual_shell.pt_func(NULL, term_getc())) {
				virtual_shell.shell_state = COMMANDSHELL_MODE_INIT;
			}
		}
	}
	else {
		if (term_data_available()) {
			get_console_string(term_getc());
		}
	}
	
	if (virtual_shell.get.state == GET_TERMSTRING_STATE_RETURN) 
	{
		virtual_shell.get.state = GET_TERMSTRING_STATE_INIT;
		virtual_shell.shell_state = COMMANDSHELL_MODE_INIT;
		
		term_putc_cr('\n');
		
		// leere Eingabezeilen werden nicht behandelt
		if (virtual_shell.shell.active->length == 0)
			return;
		
		bool found = false;
		for (uint8_t i=0; i < virtual_shell.program_list_length; i++)
		{
			// Die Befehle muessen mindestens durch ein Leereichen von
			// den Parameter getrennt sein.
			// => ueberpruefen ob an der entsprechenden Stelle im String
			// ein Leerzeichen steht bzw. der String zu Ende ist.
			
			uint8_t tmp = virtual_shell.shell.active->cl[virtual_shell.program_list[i].length];
			if (isblank(tmp) || (tmp == '\0'))
			{
				if (!strncmp_P(virtual_shell.shell.active->cl, virtual_shell.program_list[i].pstr, virtual_shell.program_list[i].length))
				{
					virtual_shell.pt_func = virtual_shell.program_list[i].fp;
					
					// Funktion ausfuehren.
					// => Wenn Sie Null zurueckliefert werden alle weiteren
					// Zeicheneingaben an die Funktion weitergeleitet.
					if (!virtual_shell.pt_func(virtual_shell.shell.active->cl, 0)) {
						virtual_shell.shell_state = COMMANDSHELL_MODE_BREAK;
					}
					
					found = true;
					break;
				}
			}
		}
		
		if (!found) {
			print_error(ERROR_UNKNOWN);
		}
	}
}
