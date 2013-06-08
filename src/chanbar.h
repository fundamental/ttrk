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

#ifndef CHANBAR_H_INCLUDED
#define CHANBAR_H_INCLUDED

#include <song.h>

class ChanBar
{
public:
	ChanBar( void ) {}
	virtual ~ChanBar( void ) {}

	// Cursor movement.
	virtual int setCursor( int newpos ) = 0;
	virtual void refresh( void ) const = 0;
	void setScrollTop( int newpos ) { scrolltop = newpos; refresh(); }

	// Channel edit functions.
	int getIndex( void ) const { return chan; }
	virtual void editPattern( int mod ) const = 0;
	virtual void setPattern( int pat ) const = 0;
	void toggleChannelMute( void ) const
		{ getChannel()->toggleMute(); refresh(); }
	void setChannelName( const char *newname ) const;
	void editChannelVolume( int mod ) const;
	void editMidiChannel( int mod ) const;
	virtual bool patternSet( void ) const = 0;
	virtual void setPatternName( const char *newname ) const = 0;
	virtual void shiftRows( int from, bool direction) const = 0;

	// Screen management.
	void setChannel( int newchan );
	SongChannel *getChannel( void ) const
		{ return ttrkSong.getChannel( chan ); }
		
	static bool hilightCurrRow;	

protected:
	int chan;
	int row;
	int col;
	int cursor;
	int scrolltop;
};

#endif // CHANBAR_H_INCLUDED
