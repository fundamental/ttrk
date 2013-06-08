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

#ifndef SONGBAR_H_INCLUDED
#define SONGBAR_H_INCLUDED

#include <song.h>
#include <chanbar.h>

class SongBar : public ChanBar
{
public:
	SongBar( int initchan, int startrow, int startcol );
	~SongBar( void );

	// Cursor movement.
	int setCursor( int newpos );

	// Screen management.
	void refresh( void ) const;
	void printCurrRow( int row );

	// Song editing.
	void setChannelName( const char *newname ) const;
	void editChannelVolume( int mod ) const;
	void editPattern( int mod ) const;
	void setPattern( int pat ) const;
	bool patternSet( void ) const;
	void setPatternName( const char *newname ) const;
	void shiftRows( int from, bool direction) const;

	void setVelocity( int vel ) const;

private:
	void printLine( int i, bool currRow ) const;
};

#endif // SONGBAR_H_INCLUDED
