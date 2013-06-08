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

#include <chanbar.h>

bool ChanBar::hilightCurrRow = true;

void ChanBar::setChannel( int newchan )
{
	chan = newchan;
	refresh();
}

void ChanBar::setChannelName( const char *newname ) const
{
	getChannel()->setName( newname );
	refresh();
}

void ChanBar::editChannelVolume( int mod ) const
{
	getChannel()->setVolume( getChannel()->getVolume() + mod );
	refresh();
}

void ChanBar::editMidiChannel( int mod ) const
{
	getChannel()->setMidiChannel( getChannel()->getMidiChannel() + mod );
	refresh();
}
