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

#ifndef SCREEN_H_INCLUDED
#define SCREEN_H_INCLUDED

#include <song.h>
#include <sys/timeb.h>

class PatBar;
class ChanBar;
class SongBar;

class ModeState
{
public:
	ModeState( void ) : cursorrow( 0 ), scrolltop( 0 ) {}
	int cursorrow;
	int scrolltop;
};

class Screen
{
public:
	Screen( void );
	~Screen( void );

	bool pollScreen( void );
	void refresh( void ) const;
	void refresh_bars( void );
	void showStatusMessage( const char *newmsg );
	void clearStatusMessage( void ) const;

	enum ScreenSizes {
        //[sam]: added a header row for velocity
		//HeaderSize = 6,
		HeaderSize = 7,
		BottomSize = 2
	};

	enum Colours {
		DefaultColour = 0,
		CursorColour = 1,
		HilightColour = 2,
		HilightCursorColour = 3,
		MutedChannelColour = 4,
		RealtimeLightColour = 5,
		PlayPositionColour = 6,
		LoopHilightColour = 7,
		LoopInactiveColour = 8,
		HelpColour = 9,
		StatusColour = 10,
		CurrRowColour = 11,
		HilightCurrRowColour = 12
	};

private:
	ChanBar **bars;
	PatBar **patbars;
	SongBar **songbars;
	int pos;
	int octave;
	bool patmode;
	int curcol;
	ModeState curstate;
	ModeState patstate;
	ModeState songstate;
	SongPattern copyPattern;
	SongChannel copyChannel;
	int **copySongRows;
	int markedRow;
	int msg_visible;

	// Support for "jamming".  While in song mode, you 
	// can use the note keys to play the current instrument in realtime.
	int jam_note;
	int jam_chan;
	int jam_vel;
	struct timeb jam_time;  // tracks the last time that the user pressed
                            // or held down a key to "jam"

	void switchMode( void );
	void printHeader( void ) const;
	void printSidebar( void ) const;
	void printHelp( void );

	void setNote( int newnote );
	void setController( void );
	void moveCursorPosition( int newrow, int newcol );
	void setPosition( int newpos );
	void inputTempo( void ) const;
	void saveSong( void );
	void loadSong( void );
	void toggleSync( void );
	void saveCopy( void );
	void pasteCopy( void );
	void saveSongRowCopy( void );
	void pasteSongRowCopy( void );
	void markSongRow( void );
	void enterPatName( void );
	void enterChanName( void );
	void inputValue( void );
	void ctlLinearFill( void );
	void shiftDel( void );
	void shiftIns( void );
	void shiftDelRows( void );
	void shiftInsRows( void );
	void enterVelocity( void );
	void printCurrRow( void );

    int diff_timeb(struct timeb time1, struct timeb time2) {
        // Subtract time2 from time2.  Returns the difference in milliseconds.
        if(time1.time == time2.time) {
            return time1.millitm - time2.millitm;
        } else {
            return ((1000 * time1.time) + time1.millitm) - ((1000 * time2.time) + time2.millitm);
        }
    }
};

#endif // SCREEN_H_INCLUDED
