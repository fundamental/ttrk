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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <screen.h>
#include <midicontrol.h>
#include <utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <rtctimer.h>

int main( int argc, char **argv )
{
	// Announcements.
	fprintf( stderr, "b-ttrk version %s\n", VERSION );
	fprintf( stderr, "by boo_boo(^at^)inbox.ru\n");
	fprintf( stderr, "with enhancements by sampbrauer(^at^)yahoo.com\n");
	fprintf( stderr, "based on ttrk by billy biggs (http://vektor.ca/audio/ttrk/)\n" );
	fprintf( stderr, "This is free software under the GNU GPL, "
			"for details see http://www.gnu.org.\n" );
	//fprintf( stderr, "For help and updates see http://div8.net/ttrk/\n\n" );

	// Reads in the settings from the .bttrkrc file.
	read_settings();

	// If no device was specified in the config file, open the default MIDI
	// device.
	if( !midiControl.isMidiDeviceOpen() ) {
		midiControl.openMidiDevice();
	}

	// Exit the app if we failed up to this point.
	if( !midiControl.isMidiDeviceOpen() ) {
		fprintf( stderr, "\nNo MIDI device available, exiting.\n"
			"Try setting a different device in your ~/.bttrkrc file.\n" );
		return 1;
	}
	
	//test timer availability
	RealTimeClock *rtc = new RealTimeClock();
	if(!rtc->isInitialized()) {
		fprintf( stderr, "\nTimer error, exiting.\n");
		exit(EXIT_FAILURE);
	}
	delete rtc;
	
	// Load a file if one is specified.
	if( argc > 1 ) {
		ttrkSong.loadSong( argv[ 1 ] );
	}

	// Start up the MIDI playing thread.
	midiControl.run();

	// Now create the 'gui' hee hee hee...
	Screen scr;

	// Main loop!
	for(;;) { 			
		if( scr.pollScreen() ) break;
	}

	return 0;
}

