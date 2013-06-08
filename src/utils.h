/**
 * Copyright (C) 2001 Billy Biggs.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

// For the record, I hate using 'string'.
#include <string>

/**
 * Expands ~'s in input to the user's home directory etc.
 */
std::string expand_path( const std::string &path );

/**
 * Returns -1 if realtime (SCHED_FIFO) priority cannot be set, 0 otherwise.
 */
int set_realtime_priority( void );

/**
 * Reads settings from the .bttrkrc file.  The .ttrkrc file is in a name=value
 * pair format, one per line.  Comment character is '#'.  All spaces are
 * ignored.
 */
void read_settings( void );

#endif // UTILS_H_INCLUDED
