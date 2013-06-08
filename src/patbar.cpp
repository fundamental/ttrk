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

#include <patbar.h>
#include <slang.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <screen.h>

PatBar::PatBar( int initchan, int startrow, int startcol )
{
	chan = initchan;
	pos = 0;
	row = startrow;
	col = startcol;
	cursor = -1;
	scrolltop = 0;
}

PatBar::~PatBar( void )
{
}

void PatBar::setPosition( int newpos )
{
	if( newpos < 0 ) newpos = 0;
	if( newpos > SongLen ) newpos = SongLen;
	pos = newpos;
}

SongPattern * PatBar::autoPattern ( int setpos )
{
	SongPattern *pat=NULL;
	
	for( int i = 0; i < SongMaxPat; i++ ) {
		if( getChannel()->getChannelPattern( i )->isBlank() ) {
			getChannel()->setSongPattern( setpos, i + 1 );
			pat = getChannel()->getSongPattern( setpos );
			break;
		}
	}

	assert(pat!=NULL);
	return(pat);
}

void PatBar::setController (int cnum)
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if (!pat) {
        pat=autoPattern(pos);
    } else if (pat->isNoteOff()) {
        pat=autoPattern(pos);
        pat->setNote(0 , Song::NoteOff);
    }
	assert( cursor != -1 );
	pat->setController( cursor, cnum );
	printPatLine( cursor, false );	
}

void PatBar::ctlLinearFill(int cnum, int beats, int fromValue, int toValue)
{
	int i,nbeat,cval,cvallast=-1;
	int cpos=pos;
	double val;
	double delta=((float)(toValue-fromValue)/beats);
	SongPattern *pat = getChannel()->getSongPattern( cpos );
	if (!pat) {
        pat=autoPattern(cpos);
    } else if (pat->isNoteOff()) {
        pat=autoPattern(cpos);
        pat->setNote(0 , Song::NoteOff);
    }
	
	for(i=0, nbeat=cursor, val=fromValue; pat && i < beats; nbeat++, val+=delta, i++) {
		if(nbeat >= SongPatLen) {
			pat=getChannel()->getSongPattern( ++cpos );
			nbeat=0;
			
            if (!pat) {
                pat=autoPattern(cpos);
            } else if (pat->isNoteOff()) {
                pat=autoPattern(cpos);
                pat->setNote(0 , Song::NoteOff);
            }
		}		
		cval=(int)(val+0.5);
		if(i+1 == beats) cval=toValue;
		if(cval!=cvallast) {
			pat->setController( nbeat,cnum );
			pat->setVolume(nbeat,cval);
			cvallast=cval;
		}
	}
	
	refresh();
}

void PatBar::setNote( int note )
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
    if (!pat) {
        pat=autoPattern(pos);
    } else if (pat->isNoteOff()) {
        pat=autoPattern(pos);
        pat->setNote(0 , Song::NoteOff);
    }

	assert( cursor != -1 );
    int old_note = pat->getNote(cursor);
	pat->setNote( cursor, note );
    if(note == Song::NoteEmpty) {
        // reset the velocity if the row is now empty
	    pat->setVolume( cursor, 64 );
    } else {
        // only set the velocity if the row was empty
        if(old_note == Song::NoteEmpty) {
	        pat->setVolume( cursor, getChannel()->getVelocity() );
        }
    }
	printPatLine( cursor, false );
}


void PatBar::refresh( void ) const
{
	bool muted = getChannel()->isMuted();
	SongPattern *pat = getChannel()->getSongPattern( pos );

	SLsmg_gotorc( row, col );
	SLsmg_printf( "%-8s", getChannel()->getName() );

	SLsmg_gotorc( row + 1, col );
	SLsmg_printf( "%02x ", chan );
	if( pat ) {
        int idx = pat->getIndex();
        if(idx == Song::NoteOffPatternId) {
		    SLsmg_printf( "^^ " );
        } else {
		    SLsmg_printf( "%02x ", idx );
        }
	} else {
		SLsmg_printf( "-- " );
	}
	if( muted ) {
		SLsmg_printf( "M " );
	} else {
		SLsmg_printf( "  " );
	}

	SLsmg_gotorc( row + 2, col );
	SLsmg_printf( "Vol: %02d ", getChannel()->getVolume() );

	SLsmg_gotorc( row + 3, col );
	SLsmg_printf( "Ch : %02d ", getChannel()->getMidiChannel() + 1 );

	SLsmg_gotorc( row + 4, col );
	SLsmg_printf( "Vel: %02x ", getChannel()->getVelocity() );

	SLsmg_gotorc( row + 5, col );
	if( pat ) {
		SLsmg_printf( "%-8s", pat->getName() );
	} else {
		SLsmg_printf( "        " );
	}

	SLsmg_gotorc( row + 6, col );
	SLsmg_printf( "========" );

	for( int i = 0; i < SongPatLen; i++ ) {
		printPatLine( i, false );
	}
	int scrsize = SLtt_Screen_Rows - row - Screen::HeaderSize - Screen::BottomSize;
	if( scrsize > SongPatLen ) {
		for( int i = SongPatLen; i < scrsize; ++i ) {
			SLsmg_gotorc( row + Screen::HeaderSize + i, 0 );
			SLsmg_erase_eol();
		}
	}
}

void PatBar::noteToString( char *str, int note ) const
{
	switch( note ) {
		case Song::NoteEmpty:  strcpy( str, "---" ); break;
		case Song::NoteOff: strcpy( str, "^^^" ); break;
		default:
			if ( note > 127) {
				note = 127;
			}
			
			switch( note % 12 ) {
				case 0: strcpy( str, "C-" ); break;
				case 1: strcpy( str, "C#" ); break;
				case 2: strcpy( str, "D-" ); break;
				case 3: strcpy( str, "D#" ); break;
				case 4: strcpy( str, "E-" ); break;
				case 5: strcpy( str, "F-" ); break;
				case 6: strcpy( str, "F#" ); break;
				case 7: strcpy( str, "G-" ); break;
				case 8: strcpy( str, "G#" ); break;
				case 9: strcpy( str, "A-" ); break;
				case 10: strcpy( str, "A#" ); break;
				case 11: strcpy( str, "B-" ); break;
			}
			str[ 2 ] = (( note / 12 ) - 1) + '0';
			str[ 3 ] = '\0';
			break;
	}
}

void PatBar::controllerToString( char *str, int cnum ) const
{
	sprintf(str,"#%02x",cnum);
}

void PatBar::printPatLine( int i, bool currRow ) const
{
	char note[ 4 ] = "---";
	const char *slide = "-";
	int vol = 64;
	int printrow;

	int scrsize = SLtt_Screen_Rows - row - Screen::HeaderSize - Screen::BottomSize;
	if( i < scrolltop || i >= ( scrolltop + scrsize ) ) return;
	printrow = row + Screen::HeaderSize + i - scrolltop;

	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( pat ) {
		if(pat->getNote( i ) >= 0) noteToString( note, pat->getNote( i ) );
		else if(pat->getController( i ) >= 0) controllerToString( note, pat->getController( i ) );
		else noteToString( note, Song::NoteEmpty );
			
		if( pat->getSlide( i ) ) slide = "S";
		vol = pat->getVolume( i );
	}

	SLsmg_gotorc( printrow, col );
	if(currRow && hilightCurrRow) {
		if( i % 4 == 0 ) SLsmg_set_color( Screen::HilightCurrRowColour );
		else SLsmg_set_color( Screen::CurrRowColour );	
	} else {
		if( i == cursor && i % 4 == 0 ) SLsmg_set_color( Screen::HilightCursorColour );
		else if( i == cursor ) SLsmg_set_color( Screen::CursorColour );
		else if( i % 4 == 0 ) SLsmg_set_color( Screen::HilightColour );
		else if( getChannel()->isMuted() ) SLsmg_set_color( Screen::MutedChannelColour );
	}
	SLsmg_printf( "%s %02x %s", note, vol, slide );
	SLsmg_set_color( Screen::DefaultColour );
}

int PatBar::setCursor( int newpos )
{
	cursor = newpos;
	int scrsize = SLtt_Screen_Rows - row - Screen::HeaderSize - Screen::BottomSize;
	if( scrsize < SongPatLen ) {
		if( cursor < (scrsize / 2) ) {
			return 0;
		} else if( cursor >= ( SongPatLen - ( scrsize / 2 ) ) ) {
			return SongPatLen - scrsize;
		} else {
			return cursor - ( scrsize / 2 );
		}
	} else {
		return 0;
	}
}

void PatBar::printCurrRow( int row ) {
	printPatLine( row, true );	
}

void PatBar::setCursorVolume( int vol ) const
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( pat ) pat->setVolume( cursor, vol );
	refresh();
}

void PatBar::setVelocity( int vel ) const
{
	getChannel()->setVelocity(vel);
	refresh();
}

void PatBar::editCursorVolume( int mod ) const
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( pat ) pat->setVolume( cursor, pat->getVolume( cursor ) + mod );
	refresh();
}

void PatBar::toggleCursorSlide( void )
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( pat ) pat->toggleSlide( cursor );
}

void PatBar::editPattern( int mod ) const
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( pat ) {
		int val = getChannel()->getSongPattern( pos )->getIndex();
		getChannel()->setSongPattern( pos, val + mod );
	} else if( mod == 1 ) {
		getChannel()->setSongPattern( pos, 1 );
	}
	refresh();
}

void PatBar::transposeNotes( int trans ) const
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( pat ) {
		pat->transposeNotes( trans );
		refresh();
	}
}

void PatBar::setPatternName( const char *newname ) const
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( !pat ) return;
	pat->setName( newname );
	refresh();
}

void PatBar::shiftRows( int from, bool direction) const
{
	SongPattern *pat = getChannel()->getSongPattern( pos );
	if( pat ) {
		pat->shiftNotes(from,direction);
		refresh();
	}	
}
