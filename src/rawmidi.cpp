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

#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <rawmidi.h>
#include <assert.h>

RawMidi::RawMidi( const char *mididev, MidiDev::OpenMode mode )
{
	mode_t openmode = 0;

	// Le Kernel-Bug Workaround.
	switch( mode ) {
		case MidiDev::ReadMode:      openmode = O_RDONLY | O_NONBLOCK; break;
		case MidiDev::WriteMode:     openmode = O_WRONLY | O_NONBLOCK; break;
		default:
		case MidiDev::ReadWriteMode: openmode = O_RDWR | O_NONBLOCK; break;
	}
	if( ( midi_fd = open( mididev, openmode ) ) == -1 ) {
		fprintf( stderr, "Failed to open %s: %s.\n",
				mididev, strerror( errno ) );
		return;
	}
	close( midi_fd );

	// We now know we can successfully open it in blocking mode.
	switch( mode ) {
		case MidiDev::ReadMode:      openmode = O_RDONLY; break;
		case MidiDev::WriteMode:     openmode = O_WRONLY; break;
		default:
		case MidiDev::ReadWriteMode: openmode = O_RDWR; break;
	}
	if( ( midi_fd = open( mididev, openmode ) ) == -1 ) {
		fprintf( stderr, "Failed to re-open %s for blocking I/O: %s.\n",
				mididev, strerror( errno ) );
		return;
	}

	buffpos = 0;
}

RawMidi::~RawMidi( void )
{
	if( midi_fd > 0 ) close( midi_fd );
}

void RawMidi::noteOn( unsigned int chan, unsigned int note, unsigned int vel )
{
	buff[ buffpos++ ] = 0x90 | chan;
	buff[ buffpos++ ] = note;
	buff[ buffpos++ ] = vel;
}

void RawMidi::noteOff( unsigned int chan, unsigned int note, unsigned int vel )
{
	buff[ buffpos++ ] = 0x80 | chan;
	buff[ buffpos++ ] = note;
	buff[ buffpos++ ] = vel;
}

void RawMidi::polyphonicAftertouch( unsigned int chan, unsigned int note,
	unsigned int amt )
{
	buff[ buffpos++ ] = 0xA0 | chan;
	buff[ buffpos++ ] = note;
	buff[ buffpos++ ] = amt;
}

void RawMidi::programChange( unsigned int chan, unsigned int prog )
{
	buff[ buffpos++ ] = 0xC0 | chan;
	buff[ buffpos++ ] = prog;
	buff[ buffpos++ ] = 0;
}

void RawMidi::channelAftertouch( unsigned int chan, unsigned int amt )
{
	buff[ buffpos++ ] = 0xD0 | chan;
	buff[ buffpos++ ] = amt;
	buff[ buffpos++ ] = 0;
}

void RawMidi::pitchWheel( unsigned int chan, unsigned int amt )
{
	buff[ buffpos++ ] = 0xE0 | chan;
	buff[ buffpos++ ] = 0x7F & amt; //[boo] was: buff[ buffpos++ ] = 0xF | amt;
	buff[ buffpos++ ] = amt >> 7; //[boo] was: amt >> 8
}

void RawMidi::controlChange( unsigned int chan, unsigned int controller, unsigned int amt)
{
	buff[ buffpos++ ] = 0xB0 | chan;
	buff[ buffpos++ ] = controller;
	buff[ buffpos++ ] = amt;
}

void RawMidi::allSoundOff( int chan )
{
	buff[ buffpos++ ] = 0xB0 | chan;
	buff[ buffpos++ ] = 0x78;
	buff[ buffpos++ ] = 0;
}

void RawMidi::resetAllControllers( int chan )
{
	buff[ buffpos++ ] = 0xB0 | chan;
	buff[ buffpos++ ] = 0x79;
	buff[ buffpos++ ] = 0;
}

void RawMidi::setLocal( int chan, bool on )
{
	buff[ buffpos++ ] = 0xB0 | chan;
	buff[ buffpos++ ] = 0x7A;

	if( on == true ) {
		buff[ buffpos++ ] = 127;
	} else {
		buff[ buffpos++ ] = 0;
	}
}

void RawMidi::allNotesOff( int chan )
{
	buff[ buffpos++ ] = 0xB0 | chan;
	buff[ buffpos++ ] = 0x7B;
	buff[ buffpos++ ] = 0;
}

void RawMidi::syncStart( void )
{
	buff[ buffpos++ ] = 0xFA;
}

void RawMidi::syncStop( void )
{
	buff[ buffpos++ ] = 0xFC;
}

void RawMidi::syncContinue( void )
{
	buff[ buffpos++ ] = 0xFB;
}

void RawMidi::syncTick( void )
{
	buff[ buffpos++ ] = 0xF8;
}

MidiDev::InMessage RawMidi::readMessage( void )
{
	unsigned char buff;
	int bytesread;


	struct pollfd pfd;
	int ret;
	pfd.fd = midi_fd;
	pfd.events = POLLIN | POLLERR;

again:
	ret = poll( &pfd, 1, 1000 );
	if( ret < 0 ) {
		if (errno == EINTR) {
			// This happens mostly when run under gdb, or when
			// exiting due to a signal.
			goto again;
		}
		return NoMessage;
	}

	if( ret == 0 ) return NoMessage;

	buff = 0;
	bytesread = read( midi_fd, &buff, 1 );

	if( bytesread < 1 ) {
		return NoMessage;
	}

	if( buff == 0xFA ) {
		return MIDIStart;
	}

	if( buff == 0xFB ) {
		return MIDIContinue;
	}

	if( buff == 0xFC ) {
		return MIDIStop;
	}

	if( buff == 0xF8 ) {
		return MIDIClockTick;
	}

	return NoMessage;
}

void RawMidi::flush( void )
{
	write( midi_fd, buff, buffpos );
	buffpos = 0;
}

