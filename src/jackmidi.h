/**
 * Copyright (C) 2014 Mark McCurry
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

#ifndef JACKMIDI_H_INCLUDED
#define JACKMIDI_H_INCLUDED

#include "mididev.h"
#include <jack/jack.h>
#include <jack/ringbuffer.h>

/**
 * This class provides an interface to jack MIDI.
 * MIDI commands are sent and received as they appear
 * on the wire, with additional timestamping.
 */
class JackMidi// : public MidiDev
{
public:
	/**
	 * Creates a raw midi device, opening the given filename with the
	 * specified mode.
	 */
	JackMidi( void );

	/**
	 * Close the device.
	 */
	~JackMidi( void );

	/**
	 * Returns true if the device was successfully opened.
	 */
	bool isOpen( void ) const { return client; }

	// Basic stuff.
	void noteOn( char chan, char note, char vel );
	void noteOff( char chan, char note, char vel );
	void polyphonicAftertouch( char chan, char note, char amt );
	void programChange( char chan, char prog );
	void channelAftertouch( char chan, char amt );
	void pitchWheel( char chan, char amt );
	void controlChange( char chan, char controller, char amt);

	// Channel stuff.
	void allSoundOff( char chan );
	void resetAllControllers( char chan );
	void setLocal( char chan, bool on );
	void allNotesOff( char chan );

	// Sync stuff.
	void syncStart( void );
	void syncStop( void );
	void syncContinue( void );
	void syncTick( void );

    // rtosc handler
    void updateStatus( void );

	// Read stuff.
	MidiDev::InMessage readMessage( void );

	// Other stuff.
	void flush( void );

    void event(size_t time, const char *dest, const char *args, ...);

    double Fs;
    size_t time;

    jack_client_t     *client;
    jack_port_t       *port;
    jack_ringbuffer_t *bToU;
    jack_ringbuffer_t *uToB;
};

#endif // JACKMIDI_H_INCLUDED
