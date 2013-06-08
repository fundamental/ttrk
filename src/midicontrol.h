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

#ifndef MIDICONTROL_H_INCLUDED
#define MIDICONTROL_H_INCLUDED

#include <thread.h>

class SongChannel;
class SongPattern;
class RealTimeClock;
class MidiDev;

class MidiControl : public Thread
{
public:
	MidiControl( void );
	~MidiControl( void );

	/**
	 * Opens the midi device.  By default, we load /dev/midi00.
	 */
	void openMidiDevice( const char *filename = 0 );

	/**
	 * Returns true if the MIDI device has been opened.
	 */
	bool isMidiDeviceOpen( void ) const { return (mididev != 0); }

	/**
	 * Returns true if we're running at realtime priority.
	 */
	bool isRealtime( void ) const { return isrt; }

	/**
	 * Switch to using an internal clock.
	 */
	void useInternalClock( void ) { useintclock = true; }

	/**
	 * Switch to syncing to an external clock.
	 */
	void useExternalClock( void ) { useintclock = false; }

	/**
	 * Returns true if we're using the internal clock for sync.
	 */
	bool usingInternalClock( void ) const { return useintclock; }

	/**
	 * Return true if the beat pulse is on.  The beat pulse occurs every
	 * quarter so you can turn a happy light on.  I love blinky lights.
	 */
	bool beatPulse( void ) const { return bp; }

	/**
	 * Set the RTC frequency for root access.  This can be either 1024 or
	 * 8192 depending on how beefy your system is.  The default is 1024, to
	 * be nice to slower machines.
	 */
	void setRTCFrequency( int newfreq ) { rtcfreq = newfreq; }

	
	// Play a note out a channel.
	void doNote( SongChannel *chan, SongPattern *pat, unsigned int beat );
	
	// Change controller value for a channel.
	void doController( SongChannel *chan, SongPattern *pat, unsigned int beat );	

	// Sent an allNotesOff message on every MIDI channel.
	void shutUp( void );	

    // for jam mode
    void playNote( int chan, int note, int vel );

    // for jam mode
    void stopNote( int chan, int note, int vel );
		
private:
	MidiDev *mididev;
	bool useintclock;
	bool isrt;
	bool bp;
	int bcount;
	int rtcfreq;

	// Last sec/usec we sent a sync signal.
	int last_sec;
	int last_usec;
	int last_diff;

	// RTC device.
	RealTimeClock *rtc;

	// Our cheezy MIDI tick counter (24ppq for now).
	int midiclockcounter;

	// Ugly hacked state information.
	bool wasplaying;

	// Do the next beat using RTC sync.
	void rtcTimeCheck( void );

	// Check if we're into the next beat when using external midi sync.
	void midiTimeCheck( void );

	// This function acutally plays the current noteset.
	void doCurrentNotes( void );

	// Thread loop.
	void thread_main( void );
};

#endif // MIDICONTROL_H_INCLUDED
