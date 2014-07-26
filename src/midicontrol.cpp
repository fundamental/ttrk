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

#include <sys/time.h>
#include <unistd.h>
#include <rawmidi.h>
#include <song.h>
#include <stdio.h>
#include <utils.h>
#include <config.h>
#include <rtctimer.h>
#include <mididev.h>
#include <midicontrol.h>

MidiControl::MidiControl( void )
	: mididev( 0 ), useintclock( true ), isrt( false ),
		bp( false ), bcount( 0 ), rtcfreq( 1024 ),
		midiclockcounter( 0 ), wasplaying( false )
{
}

MidiControl::~MidiControl( void )
{
	stop();
	shutUp();
    if(mididev)
        mididev->flush();
}

void MidiControl::openMidiDevice( const char *filename )
{
	if( mididev != 0 ) delete mididev;

	// Now open the device.  I'm scared this might block sometimes, so we
	// had better announce what's going on to give a clue..
	if( filename ) {
		fprintf( stderr, "Using raw MIDI device %s "
				"[from ~/.bttrkrc].\n", filename );
		mididev = new RawMidi( filename, MidiDev::ReadWriteMode );
	} else {
		fprintf( stderr, "Using raw MIDI device /dev/midi00.\n" );
		mididev = new RawMidi( "/dev/midi00", MidiDev::ReadWriteMode );
	}

	if( !mididev->isOpen() ) {
		delete mididev;
		mididev = 0;
	}
}

void MidiControl::doNote( SongChannel *chan, SongPattern *pat,
	unsigned int beat )
{
	int data = chan->getVolume();
	if( pat->getVolume( beat ) > 0 )
		data = pat->getVolume( beat ) * chan->getVolume() / 100;
	int last_note = chan->getLastNote();
	int mchan = chan->getMidiChannel();
	int note = pat->getNote( beat );
	bool slide = pat->getSlide( beat );
	bool mute = chan->isMuted();

    if(!mididev)
        return;

	// Shut up if we're in mute mode or a NoteOff is reached.
	if( mute || note == Song::NoteOff ) {
		if( last_note ) mididev->noteOff( mchan, last_note, data );
		chan->setLastNote( 0 );
		return;
	}

	// If we have no note to play, we're done here.
	if( note < 0 || note > 127) return;

	if( slide ) {
		// Play legato.
		mididev->noteOn( mchan, note, data );
		if( last_note ) mididev->noteOff( mchan, last_note, data );
	} else {
		// No legato.
		if( last_note ) mididev->noteOff( mchan, last_note, data );
		mididev->noteOn( mchan, note, data );
	}

	// Remember to set the last note so we can shut it up later.
	chan->setLastNote( note );
}

// for jam mode
void MidiControl::playNote( int chan, int note, int vel )
{
    if(mididev)
        mididev->noteOn( chan, note, vel );
}

// for jam mode
void MidiControl::stopNote( int chan, int note, int vel )
{
    if(mididev)
        mididev->noteOff( chan, note, vel );
}

void MidiControl::doController( SongChannel *chan, SongPattern *pat,
	unsigned int beat )
{
	int data = pat->getVolume( beat );
	int cnum = pat->getController( beat );
	int mchan = chan->getMidiChannel();
		
	if(cnum >= 0 && mididev) mididev->controlChange( mchan, cnum, data );
}

void MidiControl::shutUp( void )
{
    if(!mididev)
        return;
	for( int i = 0; i < 16; i++ ) mididev->allNotesOff( i );
}

static void diffTimes( struct timeval *result, struct timeval *big,
	struct timeval *little )
{
	result->tv_sec = big->tv_sec - little->tv_sec;
	if( big->tv_usec - little->tv_usec < 0 ) {
		result->tv_sec -= 1;
		result->tv_usec = ( 1000 * 1000 ) + big->tv_usec - little->tv_usec;
	} else {
		result->tv_usec = big->tv_usec - little->tv_usec;
	}
}

void MidiControl::rtcTimeCheck( void )
{
	struct timeval tv;
	struct timeval tvdiff;
	struct timeval result;
	int diff;
	int tt = ttrkSong.getUsecpb() / 3;

	// Throttle the box.
	rtc->nextTick();

	gettimeofday( &tv, 0 );
	diff = (tv.tv_sec - last_sec) * 1000 * 1000 + tv.tv_usec - last_usec;

	if( diff > tt ) {
		while( diff > tt ) {
			diff -= tt;
			++midiclockcounter;
			midiclockcounter %= 3;
            if(mididev)
                mididev->syncTick();
			bcount++;
			bcount %= 24;
			if( bcount == 0 ) bp = true; else bp = false;

			if( midiclockcounter == 0 && ttrkSong.isPlaying() ) {
				if( wasplaying == false ) mididev->syncStart();
				wasplaying = true;
				doCurrentNotes();
			} else if( wasplaying == true && !ttrkSong.isPlaying() ) {
				wasplaying = false;
				mididev->syncStop();
				shutUp();
			}
		}

		// Now write all the MIDI data.
        if(mididev)
            mididev->flush();

		tvdiff.tv_sec = 0;
		tvdiff.tv_usec = diff;
		diffTimes( &result, &tv, &tvdiff );

		last_sec = result.tv_sec;
		last_usec = result.tv_usec;
	}
}

void MidiControl::midiTimeCheck( void )
{
	int curmessage;
    if(!mididev)
        return;

	curmessage = mididev->readMessage();

	switch( curmessage ) {
		case MidiDev::NoMessage: return;

		case MidiDev::MIDIClockTick:
			mididev->syncTick();

			++midiclockcounter;
			midiclockcounter %= 3;
			bcount++;
			bcount %= 24;
			if( bcount == 0 ) bp = true; else bp = false;

			if( midiclockcounter == 0 && ttrkSong.isPlaying() )
				doCurrentNotes();
			break;

		case MidiDev::MIDIStart:
			mididev->syncStart();

			// Reset everything to the start
			midiclockcounter = 0;
			ttrkSong.rewind();
			ttrkSong.start();

			// Better send out the current notes now.
			doCurrentNotes();
			break;

		case MidiDev::MIDIStop:
			mididev->syncStop();
			ttrkSong.stop();
			shutUp();
			break;

		case MidiDev::MIDIContinue:
			mididev->syncContinue();
			ttrkSong.start();
			break;
	}

	mididev->flush();
}

void MidiControl::doCurrentNotes( void )
{
	// Play notes.
	for( int i = 0; i < SongMaxChan; i++ ) {
		SongChannel *curchan = ttrkSong.getChannel( i );
		SongPattern *curpat;

		curpat = curchan->getSongPattern( ttrkSong.getCurPat() );
		if( curpat ) {
			doNote( curchan, curpat, ttrkSong.getCurBeat() );
			doController( curchan, curpat, ttrkSong.getCurBeat() );
		}
	}

	// Increment the beat counter.
	ttrkSong.incrBeat();
}

void MidiControl::thread_main( void )
{
	struct timeval tv;
	int rt;

	// If we're root, get realtime priority.
	rt = set_realtime_priority();

	// Initialize for internalTimeCheck().
	gettimeofday( &tv, 0 );
	last_sec = tv.tv_sec;
	last_usec = tv.tv_usec;

	// Initialize for rtcTimeCheck();
	rtc = new RealTimeClock();
	
	if( rt < 0 ) {
		// We're not root.  Setup for shitty rtc resolution.
		rtc->setInterval( 64 );
		isrt = false;
	} else {
		rtc->setInterval( rtcfreq );
		isrt = true;
	}
	rtc->startClock();
	last_diff = 0;

	for(;;) {
		testcancel();

		if( useintclock ) {
			rtcTimeCheck();
		} else {
			midiTimeCheck();
		}
	}
}

