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

#ifndef PATBAR_H_INCLUDED
#define PATBAR_H_INCLUDED

#include <song.h>
#include <chanbar.h>

class PatBar : public ChanBar
{
public:
	PatBar( int initchan, int startrow, int startcol );
	~PatBar( void );

	// Cursor movement.
	int setCursor( int newpos );
	int getCursor() {return cursor;}

	// Screen management.
	void setPosition( int newpos );
	void refresh( void ) const;
	void printCurrRow( int row );

	// Song editing.
	bool patternSet( void ) const
		{ return ( getChannel()->getSongPattern( pos ) != 0 ); }
	void setController (int cnum);	
	void setNote( int note );
	void toggleCursorSlide( void );
	void setPatternName( const char *newname ) const;
	void setCursorVolume( int vol ) const;
	void editCursorVolume( int mod ) const;
	void setPattern( int pat ) const { return; /*TODO*/}	
	void editPattern( int mod ) const;
	void transposeNotes( int trans ) const;
	void shiftRows( int from, bool direction) const;
	void ctlLinearFill(int cnum, int beats, int fromValue, int toValue);
	void setVelocity( int vel ) const;

private:
	int pos;

	void printPatLine( int i, bool currRow ) const;
	void noteToString( char *str, int note ) const;
	void controllerToString( char *str, int cnum ) const;
	SongPattern *autoPattern ( int setpos );
};

#endif // PATBAR_H_INCLUDED
