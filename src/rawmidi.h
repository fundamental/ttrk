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

#ifndef RAWMIDI_H_INCLUDED
#define RAWMIDI_H_INCLUDED

#include <mididev.h>

/**
 * This class provides an interface to the /dev/midi* style devices, which
 * allows a raw interface to a midi port.  MIDI commands are sent and received
 * as they appear on the wire, without timestamping.  This means that our
 * application must handle events as quick as possible, requiring root access
 * and the realtime clock device.
 */
class RawMidi : public MidiDev
{
public:
	/**
	 * Creates a raw midi device, opening the given filename with the
	 * specified mode.
	 */
	RawMidi( const char *mididev, MidiDev::OpenMode mode );

	/**
	 * Close the device.
	 */
	~RawMidi( void );

	/**
	 * Returns true if the device was successfully opened.
	 */
	bool isOpen( void ) const { return (midi_fd > 0); }

	// Basic stuff.
	void noteOn( unsigned int chan, unsigned int note, unsigned int vel );
	void noteOff( unsigned int chan, unsigned int note, unsigned int vel );
	void polyphonicAftertouch( unsigned int chan, unsigned int note,
		unsigned int amt );
	void programChange( unsigned int chan, unsigned int prog );
	void channelAftertouch( unsigned int chan, unsigned int amt );
	void pitchWheel( unsigned int chan, unsigned int amt );
	void controlChange( unsigned int chan, unsigned int controller, unsigned int amt);

	// Channel stuff.
	void allSoundOff( int chan );
	void resetAllControllers( int chan );
	void setLocal( int chan, bool on );
	void allNotesOff( int chan );

	// Sync stuff.
	void syncStart( void );
	void syncStop( void );
	void syncContinue( void );
	void syncTick( void );

	// Read stuff.
	MidiDev::InMessage readMessage( void );

	// Other stuff.
	void flush( void );

private:
	int midi_fd;
	int buffpos;
	unsigned char buff[ 1024 ];
};

#endif // RAWMIDI_H_INCLUDED
