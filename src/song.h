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

#ifndef SONG_H_INCLUDED
#define SONG_H_INCLUDED

#include <assert.h>
#include <midicontrol.h>

const int SongPatLen = 32;
const int SongMaxChan = 256;
const int SongMaxPat = 255;	// This needs to be less then 256 because 0
				// means blank pattern.
const int SongLen = 4096;

class SongPattern
{
	friend class Song;
public:
	SongPattern( int index );
	~SongPattern( void );

	void clear( void );

	int getIndex( void ) const { return id; }
	int getNote( int beat ) const
		{ if( beat < SongPatLen ) return ((note[ beat ] <= 128)? note[ beat ]: -1); else return -1; }
	int getController( int beat ) const
		{ if( beat < SongPatLen ) return ((note[ beat ] >= 0x100)? note[ beat ] - 0x100 : -1); else return -1; }
	void setNote( int beat, int value )
		{ if( beat < SongPatLen ) note[ beat ] = value; }
	void setController( int beat, int value )
		{ if( beat < SongPatLen ) note[ beat ] = 0x100+value; /*'notes' 0x100-0x17f are controllers*/}		
	bool getSlide( int beat ) const
		{ if( beat < SongPatLen ) return slide[ beat ]; else return 0; }
	void toggleSlide( int beat )
		{ if( beat < SongPatLen ) slide[ beat ] = !slide[ beat ]; }
	int getVolume( int beat ) const
		{ if( beat < SongPatLen ) return vol[ beat ]; else return 0; }
	void setVolume( int beat, int value );
	const char *getName( void ) const { return name; }
	void setName( const char *newname );
	bool isBlank( void ) const;
	bool isNoteOff( void ) const;
	void transposeNotes( int trans );
	void shiftNotes(int from, bool direction);

	void copyTo( SongPattern *pat ) const;

private:
	int id;
	int *note;
	bool *slide;
	int *vol;
	char *name;
};

class SongChannel
{
	friend class Song;
public:
	SongChannel( void );
	~SongChannel( void );

	void clear( void );

	int getMidiChannel( void ) const { return midichannel; }
	void setMidiChannel( int newchan );

	int getVolume( void ) const { return volume; }
	void setVolume( int newvol );

	int getDevice( void ) const { return devnum; }
	void setDevice( int newdev ) { devnum = newdev; }

	int getLastNote( void ) const { return lastnote; }
	void setLastNote( int last ) { lastnote = last; }

	const char *getName( void ) const { return name; }
	void setName( const char *newname );

	bool isMuted( void ) const { return mute; }
	void toggleMute( void ) { mute = !mute; }

	inline SongPattern *getSongPattern( int index );
	inline SongPattern *getChannelPattern( int index ) const;

	void setSongPattern( int index, int patid );
	inline int getSongPatternId( int index );

	void setVelocity( int newvel );
	int getVelocity( void ) const { return velocity; }

	void shiftRows( int from, bool direction);

	void copyTo( SongChannel *chan ) const;

private:
	int midichannel;
	int volume;
	int devnum;
	int lastnote;
	bool mute;
	int *songdata;
	char *name;
	SongPattern **patterns;
	int velocity;
	SongPattern *noteOffPattern;
};

class Song
{
public:
	Song( void );
	~Song( void );

	void clear( void );

	void start( void ) { playing = true; }
	void stop( void ) { playing = false; }
	void rewind( void ) { curpat = startpat; curbeat = 0; }
	void rewindTo( int pos, int beat ) { curpat = pos; curbeat = beat; }
	void incrBeat( void );

	bool isPlaying( void ) const { return playing; }
	int getCurPat( void ) const { return curpat; }
	int getCurBeat( void ) const { return curbeat; }

	int getStartPos( void ) const { return startpat; }
	void setStartPos( int newstartpat ) { startpat = newstartpat; }

	SongChannel *getChannel( int chan ) const { return channels[ chan ]; }

	void setTempo( int newtempo );
	int getTempo( void ) const { return tempo; }
	int getUsecpb( void ) const { return usecpb; }

	bool isLoopActive( void ) const { return loopactive; }
	void toggleLoop( void ) { loopactive = !loopactive; }

	int getLoopPosition( void ) const { return looppos; }
	int getLoopDestination( void ) const { return loopdest; }

	void setLoopPosition( int newpos );
	void setLoopDestination( int newdest );

	bool saveSong( const char *filename );
	bool loadSong( const char *filename );

	void setPatternPlayFlag( bool flag ) { patternPlayFlag = flag; }
	int getPatternPlayFlag( void ) const { return patternPlayFlag; }
	void setPatternPlayNum( int pattern ) { patternPlayNum = pattern; }

	enum Note {
		NoteEmpty = -1,
		NoteOff = 128
	};
    enum PatternIds {
	    NoteOffPatternId = -1  // Used to identify a special pattern
    };                         // that is filled with NoteOff notes.

    char* getFilename( void ) { return curfilename; }

private:
	bool playing;
	int curpat;
	int curbeat;
	int startpat;
	bool loopactive;
	int tempo;
	int usecpb;
	int looppos;
	int loopdest;
	SongChannel **channels;
    char curfilename[80];
	bool patternPlayFlag;
    int patternPlayNum;
};

// Globals.
extern Song ttrkSong;
extern MidiControl midiControl;

SongPattern *SongChannel::getSongPattern( int index )
{
	int patid = songdata[ index ];
	if( patid == Song::NoteOffPatternId ) {
        return noteOffPattern;
    }
	if( patid ) {
		return patterns[ patid - 1 ];
    }
	return 0;
}

SongPattern *SongChannel::getChannelPattern( int index ) const
{
	assert( index >= 0 );
	assert( index < SongMaxPat );
	return patterns[ index ];
}
int SongChannel::getSongPatternId( int index )
{
	int patid = songdata[ index ];
	if(patid) return patid;
	return 0;
}

#endif // SONG_H_INCLUDED
