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
#include "song.h"
#include <stdio.h>
#include "utils.h"
#include "../config.h"
//#include <rtctimer.h>
#include "mididev.h"
#include "midicontrol.h"

MidiControl::MidiControl( void )
	: mididev( 0 ), useintclock( true ), isrt( false ),
		bp( false ), bcount( 0 ), oldTime( 0 ), dt( 0 ),
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
	delete mididev;
    mididev = new JackMidi();

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

void MidiControl::jackTimeCheck( void )
{
    mididev->updateStatus();
    const size_t newTime = mididev->time;
    const int tt = ttrkSong.getUsecpb() / 3;
    dt += (newTime-oldTime)*1e6/mididev->Fs;
    oldTime = newTime;
    usleep(1000);
    //on the off chance that something stupid produces NaN again...
    assert(dt == dt);
	
    while( dt > tt ) {

        dt -= tt;
        ++midiclockcounter;
        midiclockcounter %= 3;
        //if(mididev)
        //    mididev->syncTick();
        bcount++;
        bcount %= 24;
        bp = bcount == 0;

        if( midiclockcounter == 0 && ttrkSong.isPlaying() ) {
            //if(!wasplaying)
            //    mididev->syncStart();
            wasplaying = true;
            doCurrentNotes();
        } else if(wasplaying && !ttrkSong.isPlaying() ) {
            wasplaying = false;
            //mididev->syncStop();
            shutUp();
        }
    }
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
	// If we're root, get realtime priority.
	set_realtime_priority();

	for(;;) {
		testcancel();
        jackTimeCheck();
	}
}

