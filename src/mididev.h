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

#ifndef MIDIDEV_H_INCLUDED
#define MIDIDEV_H_INCLUDED

/**
 * Abstract class for a MIDI device.  An example would be the sequencer
 * device, or a /dev/midi-capable device, or whatever.
 */
class MidiDev
{
public:
	enum OpenMode { ReadMode, WriteMode, ReadWriteMode };

	MidiDev( void ) {}
	virtual ~MidiDev( void ) {}

	/**
	 * Returns true if the device was successfully opened.
	 */
	virtual bool isOpen( void ) const = 0;

	// Note stuff.
	virtual void noteOn( unsigned int chan, unsigned int note,
		unsigned int vel ) = 0;
	virtual void noteOff( unsigned int chan, unsigned int note,
		unsigned int vel ) = 0;
	virtual void polyphonicAftertouch( unsigned int chan, unsigned int note,
		unsigned int amt ) = 0;
		
	// Channel stuff.
	virtual void programChange( unsigned int chan, unsigned int prog ) = 0;
	virtual void channelAftertouch( unsigned int chan, unsigned int amt ) = 0;
	virtual void pitchWheel( unsigned int chan, unsigned int amt ) = 0;
	virtual void controlChange( unsigned int chan, unsigned int controller,
		unsigned int amt) = 0;

	// Bigger stuff.
	virtual void allSoundOff( int chan ) = 0;
	virtual void resetAllControllers( int chan ) = 0;
	virtual void setLocal( int chan, bool on ) = 0;
	virtual void allNotesOff( int chan ) = 0;

	// Sync stuff.
	virtual void syncStart( void ) = 0;
	virtual void syncStop( void ) = 0;
	virtual void syncContinue( void ) = 0;
	virtual void syncTick( void ) = 0;

	// Stuff we can read.
	enum InMessage {
		NoMessage,
		MIDIStart,
		MIDIStop,
		MIDIContinue,
		MIDIClockTick
	};

	// Read call.
	virtual InMessage readMessage() = 0;

	// Other stuff.
	virtual void flush() = 0;
};

#endif // MIDIDEV_H_INCLUDED
