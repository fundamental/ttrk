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

#include <song.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

int PAT_BLANK  = 0;
int PAT_NOTEOFF = -1;
int PAT_FULL   = 1;
int INDEX_DONE = -99;
char blank_name[] = "";
int VERSION = 1;
// Increment VERSION whenever a change is made to
// the song file format.  In loadSong, you can refer 
// to the version number saved in the file to handle
// a song saved by an earlier version.
// 02-AUG-2006: Version increased from 0 to 1 when Sam added a velocity attribute to each channel.  That attribute used used to set the velocity as notes are entered in pattern edit mode.

Song ttrkSong;
MidiControl midiControl;

SongPattern::SongPattern( int index )
	: id( index )
{
	note = new int[ SongPatLen ];
	slide = new bool[ SongPatLen ];
	vol = new int[ SongPatLen ];
	name = blank_name;
}

void SongPattern::clear( void )
{
	for( int i = 0; i < SongPatLen; ++i ) {
		note[ i ] = Song::NoteEmpty;
		slide[ i ] = false;
		vol[ i ] = 64;
		setName( blank_name );
	}
}

SongPattern::~SongPattern( void )
{
	delete[] note;
	delete[] slide;
	delete[] vol;
	setName( blank_name );
}

bool SongPattern::isBlank( void ) const
{
	for( int i = 0; i < SongPatLen; ++i ) {
		if( note[ i ] != Song::NoteEmpty || slide[ i ] != false
				|| vol[ i ] != 64 || name != blank_name ) {
			return false;
		}
	}
	return true;
}

bool SongPattern::isNoteOff( void ) const
{
    return id == Song::NoteOffPatternId;
}

void SongPattern::copyTo( SongPattern *pat ) const
{
	for( int i = 0; i < SongPatLen; ++i ) {
		pat->note[ i ] = note[ i ];
		pat->slide[ i ] = slide[ i ];
		pat->vol[ i ] = vol[ i ];
	}
	pat->setName(getName());
}

void SongPattern::setVolume( int beat, int value )
{
	if( beat < SongPatLen ) {
		if( value < 0 ) value = 0;
		else if( value > 127 ) value = 127;
		vol[ beat ] = value;
	}
}

void SongPattern::setName( const char *newname )
{
	if( !strcmp( newname, blank_name ) ) {
		name = blank_name;
	} else {
		if( name != blank_name ) {
			free( name );
		}
		name = strdup( newname );
	}
}

void SongPattern::transposeNotes( int trans )
{
	for( int i = 0; i < SongPatLen; ++i ) {
		if( note[ i ] != Song::NoteEmpty
				&& note[ i ] != Song::NoteOff ) {

			note[ i ] += trans;
			if( note[ i ] > 127 ) {
				note[ i ] = 127;
			} else if( note[ i ] < 0 ) {
				note[ i ] = 0;
			}
		}
	}
}

void SongPattern::shiftNotes(int from, bool direction)
{
	if(direction) {
        for( int i = SongPatLen; i >= from; i-- ) {
            if(i == from) {
                note[i] = Song::NoteEmpty;
                slide[i] = false;
                vol[i] = 64;
            } else {
                note[i] = note[i-1];
                slide[i] = slide[i-1];
                vol[i] = vol[i-1];
            }
        }	
    } else {
        for( int i = from; i < SongPatLen; i++ ) {
            if(i == SongPatLen-1) {
                note[i] = Song::NoteEmpty;
                slide[i] = false;
                vol[i] = 64;
            } else {
                note[i] = note[i+1];
                slide[i] = slide[i+1];
                vol[i] = vol[i+1];
            }
        }
    }
}

SongChannel::SongChannel( void )
{
	patterns = new SongPattern*[ SongMaxPat ];
	for( int i = 0; i < SongMaxPat; ++i )
		patterns[ i ] = new SongPattern( i + 1 );
	songdata = new int[ SongLen ];
	name = blank_name;
    velocity = 64;
    // One special "NoteOff" pattern.
    noteOffPattern = new SongPattern(-1);
	noteOffPattern->clear();
	noteOffPattern->setNote(0 , Song::NoteOff);
}

void SongChannel::clear( void )
{
	int i;

	midichannel = 0;
	volume = 75;
	devnum = 1;
	lastnote = 0;
	mute = false;
	setName( blank_name );

	for( i = 0; i < SongLen; ++i ) songdata[ i ] = 0;
	for( i = 0; i < SongMaxPat; ++i ) patterns[ i ]->clear();
}

void SongChannel::copyTo( SongChannel *chan ) const
{
	int i;
	for( i = 0; i < SongLen; ++i ) {
		chan->songdata[ i ] = songdata[ i ];
	}
	for( i = 0; i < SongMaxPat; ++i ) {
		patterns[ i ]->copyTo(chan->patterns[ i ]);
	}
	chan->setName(getName());
	chan->setVolume(getVolume());
	chan->setMidiChannel(getMidiChannel());
	chan->setVelocity(getVelocity());
}

SongChannel::~SongChannel( void )
{
	for( int i = 0; i < SongMaxPat; ++i )
		delete patterns[ i ];

	delete[] patterns;
	delete[] songdata;
}

void SongChannel::setVolume( int newvol )
{
	if( newvol < 0 ) volume = 0;
	else if( newvol > 99 ) volume = 99;
	else volume = newvol;
}

void SongChannel::setMidiChannel( int newchan )
{
	if( newchan < 0 ) midichannel = 0;
	else if( newchan > 15 ) midichannel = 15;
	else midichannel = newchan;
}

void SongChannel::setSongPattern( int index, int patid )
{
	//if( patid < 0 ) patid = 0;
	if( patid < -1 ) patid = -1;
	if( patid > SongMaxPat ) patid = SongMaxPat;
	songdata[ index ] = patid;
}

void SongChannel::setName( const char *newname )
{
	if( !strcmp( newname, blank_name ) ) {
		name = blank_name;
	} else {
		if( name != blank_name ) {
			free( name );
		}
		name = strdup( newname );
	}
}

void SongChannel::setVelocity( int newvel ) {
	velocity = newvel;
}

void SongChannel::shiftRows( int from, bool direction)
{
        int i,val;
        if(!direction) {
                for(i=from;i < SongLen;i++) {
                        SongPattern *pat = getSongPattern( i+1 );
                        if(pat) val = pat->getIndex();
                        else val = 0;
                        setSongPattern( i, val );
                }
                setSongPattern( SongLen, 0 );
        }
        else {
                for(i=SongLen;i > from;i--) {
                        SongPattern *pat = getSongPattern( i-1 );
                        if(pat) val = pat->getIndex();
                        else val = 0;
                        setSongPattern( i, val );
                }
                setSongPattern( from, 0 );
        }
}

Song::Song( void )
{
	channels = new SongChannel*[ SongMaxChan ];
	for( int i = 0; i < SongMaxChan; ++i )
		channels[ i ] = new SongChannel();
	clear();
    curfilename[0] = '\0';
    patternPlayFlag = false;
}

Song::~Song( void )
{
	delete [] channels;
}

void Song::clear( void )
{
	for( int i = 0; i < SongMaxChan; ++i ) channels[ i ]->clear();
	startpat = 0;
	loopactive = true;
	looppos = 1;
	loopdest = 0;
	setTempo( 140 );
}

void Song::setTempo( int newtempo )
{
	tempo = newtempo;
	usecpb = 60 * 1000 * 1000 / newtempo / 8; // 32nd notes, divisor is 8.
}

void Song::incrBeat( void )
{
	++curbeat;

	if( curbeat == SongPatLen ) {
		curbeat = 0;
        if(patternPlayFlag) {
            curpat = patternPlayNum;
        } else {
		    ++curpat;
		    if( curpat == looppos && loopactive ) curpat = loopdest;
        }
	}
}

bool Song::saveSong( const char *filename )
{
	int i, j, k, len;

	gzFile fd = gzopen( filename, "w" );
	if( fd == 0 ) return false;

	// Write version.
	gzwrite( fd, &VERSION, sizeof( int ) );

	// Write basic song information.

	// Starting position.
	gzwrite( fd, &startpat, sizeof( int ) );

	// Current tempo.
	gzwrite( fd, &tempo, sizeof( int ) );

	// Is looping active.
	gzwrite( fd, &loopactive, sizeof( bool ) );

	// Loop position.
	gzwrite( fd, &looppos, sizeof( int ) );

	// Loop destination.
	gzwrite( fd, &loopdest, sizeof( int ) );

	// Write each channel.
	for( i = 0; i < SongMaxChan; ++i ) {

		// Write the channel data.
		gzwrite( fd, &(channels[ i ]->midichannel), sizeof( int ) );
		gzwrite( fd, &(channels[ i ]->volume), sizeof( int ) );
		gzwrite( fd, &(channels[ i ]->devnum), sizeof( int ) );
		gzwrite( fd, &(channels[ i ]->mute), sizeof( bool ) );
		gzwrite( fd, &(channels[ i ]->velocity), sizeof( int ) );

		// Write the channel name: length, string.
		len = strlen( channels[ i ]->name ) + 1;
		gzwrite( fd, &len, sizeof( int ) );
		gzwrite( fd, channels[ i ]->name, len );

		// Write song pattern indexes.
		for( j = 0; j < SongLen; ++j ) {
			if( channels[ i ]->songdata[ j ] ) {
				gzwrite( fd, &j, sizeof( int ) );
				gzwrite( fd, &(channels[ i ]->songdata[ j ]), sizeof( int ) );
			}
		}
		gzwrite( fd, &INDEX_DONE, sizeof( int ) );

		// Now write all the patterns.
		for( j = 0; j < SongMaxPat; ++j ) {
			if( channels[ i ]->patterns[ j ]->isBlank() ) {
				gzwrite( fd, &PAT_BLANK, sizeof( int ) );
				continue;
			}

			if( channels[ i ]->patterns[ j ]->isNoteOff() ) {
				gzwrite( fd, &PAT_NOTEOFF, sizeof( int ) );
				continue;
			}

			gzwrite( fd, &PAT_FULL, sizeof( int ) );

			// Write the pattern name: length, string.
			len = strlen( channels[ i ]->patterns[ j ]->name ) + 1;
			gzwrite( fd, &len, sizeof( int ) );
			gzwrite( fd, channels[ i ]->patterns[ j ]->name, len );

			for( k = 0; k < SongPatLen; ++k ) {
				gzwrite( fd, &(channels[ i ]->patterns[ j ]->note[ k ]), sizeof( int ) );
				gzwrite( fd, &(channels[ i ]->patterns[ j ]->slide[ k ]), sizeof( bool ) );
				gzwrite( fd, &(channels[ i ]->patterns[ j ]->vol[ k ]), sizeof( int ) );
			}
		}
	}

	// Close the file.
	gzclose( fd );

    strcpy(curfilename, filename);

	// Success!
	return true;
}

bool Song::loadSong( const char *filename )
{
	int i, j, k;
	int curi, val, len;
	char *tempstr;
	int version;

	gzFile fd = gzopen( filename, "r" );
	if( fd == 0 ) return false;

	// Read version.
	gzread( fd, &version, sizeof( int ) );
	//if( curi != version ) return false;
    if(version > VERSION) return false;

	// Now we can clear.
	clear();

	// Starting position.
	gzread( fd, &startpat, sizeof( int ) );

	// Current tempo.
	gzread( fd, &tempo, sizeof( int ) );
	setTempo(tempo);

	// Is looping active.
	gzread( fd, &loopactive, sizeof( bool ) );

	// Loop position.
	gzread( fd, &looppos, sizeof( int ) );

	// Loop destination.
	gzread( fd, &loopdest, sizeof( int ) );

	// Read each channel.
	for( i = 0; i < SongMaxChan; ++i ) {

		// Read the channel data.
		gzread( fd, &(channels[ i ]->midichannel), sizeof( int ) );
		gzread( fd, &(channels[ i ]->volume), sizeof( int ) );
		gzread( fd, &(channels[ i ]->devnum), sizeof( int ) );
		gzread( fd, &(channels[ i ]->mute), sizeof( bool ) );
        if(version > 0) {
		    gzread( fd, &(channels[ i ]->velocity), sizeof( int ) );
        }

		// Read the channel name: length, string.
		gzread( fd, &len, sizeof( int ) );
		tempstr = new char[ len ];
		gzread( fd, tempstr, len );
		channels[ i ]->setName( tempstr );
		delete[] tempstr;

		// Read song pattern indexes.
		for(;;) {
			gzread( fd, &curi, sizeof( int ) );
            if(version == 0) {
                // Version 0 didn't have NoteOff patterns, so -1 was INDEX_DONE.
			    if( curi == -1 ) break;
            } else {
			    if( curi == INDEX_DONE ) break;
            }

			gzread( fd, &val, sizeof( int ) );
			channels[ i ]->songdata[ curi ] = val;
		}

		// Now read all the patterns.
		for( j = 0; j < SongMaxPat; ++j ) {
			gzread( fd, &curi, sizeof( int ) );
			if( curi == PAT_BLANK ) continue;
			if( curi == PAT_NOTEOFF ) continue;

			// Read the channel name: length, string.
			gzread( fd, &len, sizeof( int ) );
			tempstr = new char[ len ];
			gzread( fd, tempstr, len );
			channels[ i ]->patterns[ j ]->setName( tempstr );
			delete[] tempstr;

			for( k = 0; k < SongPatLen; ++k ) {
				gzread( fd, &(channels[ i ]->patterns[ j ]->note[ k ]), sizeof( int ) );
				gzread( fd, &(channels[ i ]->patterns[ j ]->slide[ k ]), sizeof( bool ) );
				gzread( fd, &(channels[ i ]->patterns[ j ]->vol[ k ]), sizeof( int ) );
			}
		}
	}

	// Close the file.
	gzclose( fd );

    strcpy(curfilename, filename);

	// Success!
	return true;
}

void Song::setLoopPosition( int newpos )
{
	if( newpos < 0 || newpos >= SongLen ) return;
	looppos = newpos;
}

void Song::setLoopDestination( int newdest )
{
	if( newdest < 0 || newdest >= SongLen ) return;
	loopdest = newdest;
}

