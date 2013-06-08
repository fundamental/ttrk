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

#include <slang.h>
#include <screen.h>
#include <songbar.h>

SongBar::SongBar( int initchan, int startrow, int startcol )
{
	chan = initchan;
	row = startrow;
	col = startcol;
	cursor = -1;
}

SongBar::~SongBar( void )
{
}

int SongBar::setCursor( int newpos )
{
	cursor = newpos;
	int scrsize = SLtt_Screen_Rows - row - Screen::HeaderSize - Screen::BottomSize;
	if( scrsize < SongLen ) {
		if( cursor < (scrsize / 2) ) {
			return 0;
		} else if( cursor >= ( SongLen - ( scrsize / 2 ) ) ) {
			return SongLen - scrsize;
		} else {
			return cursor - ( scrsize / 2 );
		}
	} else {
		return 0;
	}
}

void SongBar::refresh( void ) const
{
	SLsmg_gotorc( row, col );
	SLsmg_printf( "%-8s", getChannel()->getName() );

	SLsmg_gotorc( row + 1, col );
	SLsmg_printf( "%02x     ", chan );

	SLsmg_gotorc( row + 2, col );
	SLsmg_printf( "Vol: %02d ", getChannel()->getVolume() );

	SLsmg_gotorc( row + 3, col );
	SLsmg_printf( "Ch : %02d ", getChannel()->getMidiChannel() + 1 );

	SLsmg_gotorc( row + 4, col );
	SLsmg_printf( "Vel: %02x ", getChannel()->getVelocity() );

	SLsmg_gotorc( row + 5, col );
	if( cursor != -1 ) {
		SongPattern *pat = getChannel()->getSongPattern( cursor );
		if( pat ) {
			SLsmg_printf( "%-8s", pat->getName() );
		} else {
			SLsmg_printf( "        " );
		}
	} else {
		SLsmg_printf( "        " );
	}

	SLsmg_gotorc( row + 6, col );
	SLsmg_printf( "========" );

	for( int i = 0; i < SongLen; i++ ) {
		printLine( i, false );
	}
}

void SongBar::setPattern( int pat ) const
{
	getChannel()->setSongPattern( cursor, pat );
	refresh();
}

void SongBar::editPattern( int mod ) const
{
	SongPattern *pat = getChannel()->getSongPattern( cursor );
	if( pat ) {
		int val = getChannel()->getSongPattern( cursor )->getIndex();
		getChannel()->setSongPattern( cursor, val + mod );
	} else if( mod == 1 ) {
		getChannel()->setSongPattern( cursor, 1 );
	}
	refresh();
}

void SongBar::printLine( int i, bool currRow ) const
{
	int printrow;

	int scrsize = SLtt_Screen_Rows - row - Screen::HeaderSize - Screen::BottomSize;
	if( i < scrolltop || i >= ( scrolltop + scrsize ) ) return;
	printrow = row + Screen::HeaderSize + i - scrolltop;

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

	SongPattern *pat = getChannel()->getSongPattern( i );
	if( pat ) {
        int idx = pat->getIndex();
        if(idx == Song::NoteOffPatternId) {
		    SLsmg_printf( "   ^^   " );
        } else {
		    SLsmg_printf( "   %02x   ", idx );
        }
	} else {
		SLsmg_printf( "   --   " );
	}
	SLsmg_set_color( Screen::DefaultColour );
}

void SongBar::printCurrRow( int row ) {
	printLine( row, true );	
}

bool SongBar::patternSet( void ) const
{
	if( cursor == -1 ) return false;
	return ( getChannel()->getSongPattern( cursor ) != 0 );
}

void SongBar::setPatternName( const char *newname ) const
{
	SongPattern *pat = getChannel()->getSongPattern( cursor );
	if( !pat ) return;
	pat->setName( newname );
	refresh();
}

void SongBar::setVelocity( int vel ) const
{
	getChannel()->setVelocity(vel);
	refresh();
}

void SongBar::shiftRows( int from, bool direction) const
{
	int i,val;

	if(!direction) {
		for(i=from;i < SongLen;i++) {
			SongPattern *pat = getChannel()->getSongPattern( i+1 );
			if(pat) val = pat->getIndex();
			else val = 0;
			getChannel()->setSongPattern( i, val );
		}
		
		getChannel()->setSongPattern( SongLen, 0 );
	}
	else {
		for(i=SongLen;i > from;i--) {
			SongPattern *pat = getChannel()->getSongPattern( i-1 );
			if(pat) val = pat->getIndex();
			else val = 0;
			getChannel()->setSongPattern( i, val );
		}
		
		getChannel()->setSongPattern( from, 0 );
	}

	
	refresh();
}
