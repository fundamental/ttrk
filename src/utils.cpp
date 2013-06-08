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

#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <ctype.h>
#include <stdio.h>
#include <fcntl.h>
#include <song.h>
#include <utils.h>
#include <patbar.h>

using namespace std;

// Modified from the UNIX FAQ.  Google gave me this URL:
// http://www.erlenstar.demon.co.uk/unix/faq_3.html

string expand_path( const string &path )
{
	if( path.length() == 0 || path[ 0 ] != '~' )
		return path;

	const char *pfx = NULL;
	string::size_type pos = path.find_first_of( '/' );

	if( path.length() == 1 || pos == 1 ) {
		pfx = getenv( "HOME" );

		if( !pfx ) {
			// Punt. We're trying to expand ~/, but HOME
			// isn't set.
			struct passwd *pw = getpwuid( getuid() );
			if( pw ) pfx = pw->pw_dir;
		}
	} else {
		string user( path, 1, ( pos == string::npos ) ? string::npos : pos - 1 );
		struct passwd *pw = getpwnam( user.c_str() );
		if( pw ) pfx = pw->pw_dir;
	}

	// If we failed to find an expansion, return the path unchanged.
	if( !pfx ) return path;

	string result( pfx );

	if( pos == string::npos ) return result;

	if( result.length() == 0 || result[ result.length() - 1 ] != '/' )
		result += '/';

	result += path.substr(pos+1);

	return result;
}

int set_realtime_priority( void )
{
	struct sched_param schp;

	memset( &schp, 0, sizeof( schp ) );
	schp.sched_priority = sched_get_priority_max( SCHED_FIFO );

	if( sched_setscheduler( 0, SCHED_FIFO, &schp ) != 0 ) {
		return -1;
	}

	return 0;
}

void add_setting( const char *name, const char *value )
{
	if( strcmp( name, "initial_tempo" ) == 0 ) {
		ttrkSong.setTempo( atoi( value ) );
	} else if( strcmp( name, "start_external_sync" ) == 0 ) {
		if( strcmp( value, "true" ) == 0 ) {
			midiControl.useExternalClock();
		}
	} else if( strcmp( name, "rtc_frequency" ) == 0 ) {
		if( strcmp( value, "high" ) == 0 ) {
			midiControl.setRTCFrequency( 8192 );
		}
	} else if( strcmp( name, "mididev" ) == 0 ) {
		midiControl.openMidiDevice( value );
	} else if( strcmp( name, "highlight_curr_row" ) == 0 ) {
		PatBar::hilightCurrRow = (atoi(value) != 0);
	} else {
		fprintf( stderr, "Unknown .bttrkrc file option '%s'\n", name );
	}
}

void read_settings( void )
{
	// Accept name=value pairs with names and values up to 128 characters.
	const int maxlen = 128;

	char curname[ maxlen ];
	char curval[ maxlen ];
	char cur;
	bool inname;
	bool invalue;
	bool iscomment;
	int fd, npos, vpos;

	// Open $HOME/.bttrkrc
	int fnamelen = strlen( "/.bttrkrc" ) + strlen( getenv( "HOME" ) );
	char *filename = new char[ fnamelen + 1 ];
	sprintf( filename, "%s/.bttrkrc", getenv( "HOME" ) );
	fd = open( filename, O_RDONLY );

	// If there's no file, then no bother.
	if( fd < 0 ) {
		delete[] filename;
		return;
	}

	fprintf( stderr, "Reading options from %s...\n", filename );
	delete[] filename;

	npos = 0;
	curname[ npos ] = '\0';
	vpos = 0;
	curval[ vpos ] = '\0';
	inname = false;
	invalue = false;
	iscomment = false;

	// Read a character at a time.
	while( read( fd, &cur, 1 ) > 0 ) {

		// Done a line.
		if( cur == '\n' ) {
			iscomment = false;
			inname = false;
			invalue = false;
			if( npos ) {
				add_setting( curname, curval );
			}
			npos = 0;
			curname[ npos ] = '\0';
			vpos = 0;
			curval[ vpos ] = '\0';
		} else if( iscomment ) {
			continue;
		} else if( isspace( cur ) ) {
			continue;
		} else if( !inname && !invalue && cur == '#' ) {
			iscomment = true;
		} else if( inname && cur == '=' ) {
			inname = false;
			invalue = true;
		} else if( invalue ) {
			curval[ vpos++ ] = tolower( cur );
			curval[ vpos ] = '\0';
			if( vpos == maxlen - 1 ) vpos--;
		} else {
			inname = true;
			curname[ npos++ ] = tolower( cur );
			curname[ npos ] = '\0';
			if( npos == maxlen - 1 ) npos--;
		}
	}

	close( fd );
}

