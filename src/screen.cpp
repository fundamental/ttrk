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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <screen.h>
#include <signal.h>
#include <slang.h>
#include <assert.h>
#include <ctype.h>
#include <song.h>
#include <fcntl.h>
#include <unistd.h>
#include <utils.h>
#include <patbar.h>
#include <songbar.h>
#include <sys/timeb.h>

using namespace std;

#define INP_BUF_LEN 80
static volatile int Screen_Size_Changed = 0;
static const int numbars = 8;
static char inputbuf[ INP_BUF_LEN ];
static const int SL_KEY_F10 = 0x1000;
static const int SL_KEY_F11 = 0x1001;
static const int SL_KEY_F12 = 0x1002;
static const int SL_KEY_SHIFTLEFT = 0x1003;
static const int SL_KEY_SHIFTRIGHT = 0x1004;
static const int END_OF_ROWS_MARKER = -99;

static void inputBuffer( char *prompt, char *value )
{
	int i = 0;
	unsigned int c;
	strcpy(inputbuf, value);
	SLsmg_gotorc( SLtt_Screen_Rows - 1, 0 );
	SLsmg_erase_eol();
	SLsmg_write_string( prompt );
	SLsmg_write_string( value );
	i = strlen(value);
	SLsmg_refresh();

	for(;;) {
		if( SLang_input_pending( 1 ) ) {
			c = SLkp_getkey();
			if( c == '\r' ) { inputbuf[ i ] = '\0'; break; }
			else if( c == SL_KEY_BACKSPACE ||  c == SL_KEY_DELETE ||  c == SL_KEY_LEFT ) {
				if(i > 0) {
					SLsmg_gotorc( SLtt_Screen_Rows - 1, SLsmg_get_column() - 1 );
					SLsmg_write_char( ' ' );
					SLsmg_gotorc( SLtt_Screen_Rows - 1, SLsmg_get_column() - 1 );
					SLsmg_refresh();
					i--;
				}
			}
			else if( c == SL_KEY_IC || c == SL_KEY_RIGHT ||  c == SL_KEY_HOME ||  c == SL_KEY_END || c == SL_KEY_PPAGE || c == SL_KEY_NPAGE ) {
				// ignore these keys (seems that isprint() does'nt filters them out on some systems)
			}
			else if(isprint(c))
			{
				inputbuf[ i++ ] = c;
				SLsmg_write_char( c );
				SLsmg_refresh();
			}
		}
	}
	SLsmg_gotorc( SLtt_Screen_Rows - 1, 0 );
		SLsmg_erase_eol();
}

static bool confirm( char *prompt )
{
	unsigned int c;
	SLsmg_gotorc( SLtt_Screen_Rows - 1, 0 );
	SLsmg_erase_eol();
	SLsmg_write_string( prompt );
	SLsmg_write_string( " (y/n) " );
	SLsmg_refresh();
	while(! SLang_input_pending(1)) {
		// do nothing; wait for a key press
	}
	c = SLkp_getkey();
	SLsmg_gotorc( SLtt_Screen_Rows - 1, 0 );
	SLsmg_erase_eol();
    return c == 'y';
}

static void sigwinch_handler( int sig )
{
	Screen_Size_Changed = 1;
	SLsignal( SIGWINCH, sigwinch_handler );
}

Screen::Screen( void )
	: pos( 0 ), octave( 3 ), patmode( false ), curcol( 0 ), copyPattern( 0 )
{
	int ret, i;

	// Initialize s-lang stuff.
	SLsignal( SIGWINCH, sigwinch_handler );
	SLtt_get_terminfo();
	SLang_init_tty( 0, 1, 0 );
	ret = SLkp_init();
	assert( ret != -1 );
	ret = SLsmg_init_smg();
	assert( ret != -1 );

	// Define all the extra keys.
	SLkp_define_keysym( "[21~", SL_KEY_F10 );
	SLkp_define_keysym( "[23~", SL_KEY_F11 );
	SLkp_define_keysym( "[24~", SL_KEY_F12 );
	SLkp_define_keysym( "[2D", SL_KEY_SHIFTLEFT );
	SLkp_define_keysym( "[2C", SL_KEY_SHIFTRIGHT );

	// Cursor colour.
	SLtt_set_color( Screen::CursorColour, 0, "white", "blue" );

	// Hilight colour.
	SLtt_set_color( Screen::HilightColour, 0, "green", "black" );

	// Hilight + cursor colour.
	SLtt_set_color( Screen::HilightCursorColour, 0, "green", "blue" );

	// Muted channel colour.
	SLtt_set_color( Screen::MutedChannelColour, 0, "blue", "black" );

	// Realtime Light colour.
	SLtt_set_color( Screen::RealtimeLightColour, 0, "white", "red" );

	// Play position colour.
	SLtt_set_color( Screen::PlayPositionColour, 0, "red", "black" );

	// Loop hilight colour.
	SLtt_set_color( Screen::LoopHilightColour, 0, "black", "green" );

	// Loop inactive hilight colour.
	SLtt_set_color( Screen::LoopInactiveColour, 0, "black", "red" );
	
	// key help colour.
	SLtt_set_color( Screen::HelpColour, 0, "lightgray", "black" );    
	
	// statusbar colour.
	SLtt_set_color( Screen::StatusColour, 0, "green", "black" );    

	// color for current row.
	SLtt_set_color( Screen::CurrRowColour, 0, "black", "lightgray" );
	
	// color for current row + highlight.
	SLtt_set_color( Screen::HilightCurrRowColour, 0, "black", "green" );

	// Default colour.
	SLtt_set_color( 0, 0, "white", "black" );

	// Remember to stay at the default colour from now on.
	SLsmg_set_color( Screen::DefaultColour );

	// Create the bars array.
	bars = new ChanBar*[ numbars ];

	// Create the pattern bars.
	patbars = new PatBar*[ numbars ];
	for( i = 0; i < numbars; i++ ) {
		patbars[ i ] = new PatBar( i, 3, 5 + ( i * 9 ) );
	}

	// Create the channel bars.
	songbars = new SongBar*[ numbars ];
	for( i = 0; i < numbars; i++ ) {
		songbars[ i ] = new SongBar( i, 3, 5 + ( i * 9 ) );
	}

	// Clear the virtual screen.
	SLsmg_cls();

	// Switch the mode to setup everything initially.
	switchMode();

	// Reset the cursor.
	patbars[ 0 ]->setCursor( 0 );
	songbars[ 0 ]->setCursor( 0 );
	moveCursorPosition( 0, 0 );

	markedRow = -1;
	// TODO: Instead of allocating the max possible copy buffer
	// and using a marker, allocate and free only the memory needed
	// as the user marks and copies.
	copySongRows = new int*[SongLen];
	for(int i=0; i<SongLen; i++) {
		copySongRows[i] = new int[SongMaxChan];
		for(int j=0; j<SongMaxChan; j++) {
			copySongRows[i][j] = END_OF_ROWS_MARKER;
		}
	}

	showStatusMessage( "Welcome to b-tektracker. Press '?' to get help." );
	
	// Refresh the screen.
	refresh();
}

Screen::~Screen( void )
{
	SLsmg_reset_smg();
	SLang_reset_tty();
}

void Screen::switchMode( void )
{
	patmode = !patmode;
	refresh_bars();
	moveCursorPosition( curstate.cursorrow, curcol );
}

void Screen::refresh_bars( void )
{
	int i;
	
	if( patmode ) {
		for( i = 0; i < numbars; ++i ) {
			bars[ i ] = patbars[ i ];
			bars[ i ]->refresh();
		}
		curstate = patstate;
	} else {
		for( i = 0; i < numbars; ++i ) {
			bars[ i ] = songbars[ i ];
			bars[ i ]->refresh();
		}
		curstate = songstate;
	}    
	printCurrRow();
}

void Screen::printCurrRow( void )
{
	int i;
	for( i = 0; i < numbars; ++i ) {
		if(i != curcol) {
			if( patmode ) {
			    patbars[ i ]->printCurrRow(curstate.cursorrow);
			} else {
			    songbars[ i ]->printCurrRow(curstate.cursorrow);
			}
		}
	}
}

void Screen::refresh( void ) const
{
	// Need to refresh the header.
	printHeader();

	// Need to refresh the sidebar too.
	printSidebar();

	// Print beat pulse.
	if( midiControl.beatPulse() ) {
		SLsmg_gotorc( 0, SLtt_Screen_Cols - 2 );
		SLsmg_printf( "*" );
	} else {
		SLsmg_gotorc( 0, SLtt_Screen_Cols - 2 );
		SLsmg_printf( " " );
	}

	// Announce if we got leet root access.
	SLsmg_gotorc( SLtt_Screen_Rows - 2, 0 );
	SLsmg_printf( "============================================================================" );
	if( midiControl.isRealtime() ) {
		//SLsmg_forward( SLtt_Screen_Cols - 2 );
		SLsmg_gotorc( SLtt_Screen_Rows - 2, SLtt_Screen_Cols - 2 );
		SLsmg_set_color( Screen::RealtimeLightColour );
		SLsmg_printf( "RT" );
		SLsmg_set_color( Screen::DefaultColour );
	}

	// Always leave the cursor in the bottom-right corner.
	SLsmg_gotorc( SLtt_Screen_Rows - 1, SLtt_Screen_Cols - 1 );
	SLsmg_refresh();
}

bool Screen::pollScreen( void )
{
	unsigned int c;

	// Handle a screen size change (annoying).
	if( Screen_Size_Changed ) {
		SLtt_get_screen_size();
#ifdef HAVE_SLSMG_REINIT_SMG
		SLsmg_reinit_smg();
#else
		SLsmg_reset_smg();
		SLsmg_init_smg();
#endif // HAVE_SLSMG_REINIT_SMG
		Screen_Size_Changed = 0;
		refresh();
		
		for( int i = 0; i < numbars; i++ ) {
			bars[ i ]->refresh();
		}
	}

	if(!msg_visible) clearStatusMessage();
	
	// This input call will throttle the main loop.
	if( SLang_input_pending( 1 ) ) {
		if(msg_visible) --msg_visible;
		
		c = SLkp_getkey();
		switch( c ) {

		case 17: if(confirm("Really quit?")) return true; break; // Ctrl-Q is quit

		// Channel mutes.
		case SL_KEY_F(1):    bars[ 0 ]->toggleChannelMute(); printCurrRow(); break;
		case SL_KEY_F(2):    bars[ 1 ]->toggleChannelMute(); printCurrRow(); break;
		case SL_KEY_F(3):    bars[ 2 ]->toggleChannelMute(); printCurrRow(); break;
		case SL_KEY_F(4):    bars[ 3 ]->toggleChannelMute(); printCurrRow(); break;
		case SL_KEY_F(5):    bars[ 4 ]->toggleChannelMute(); printCurrRow(); break;
		case SL_KEY_F(6):    bars[ 5 ]->toggleChannelMute(); printCurrRow(); break;
		case SL_KEY_F(7):    bars[ 6 ]->toggleChannelMute(); printCurrRow(); break;
		case SL_KEY_F(8):    bars[ 7 ]->toggleChannelMute(); printCurrRow(); break;

		// Song Control.
		case SL_KEY_F(9):  
			if (ttrkSong.isPlaying()) {
                ttrkSong.stop();
            } else {
			    ttrkSong.setPatternPlayFlag(false);
			    ttrkSong.start(); 
            }
			break;
		case SL_KEY_F10:
			ttrkSong.setPatternPlayFlag(false);
            ttrkSong.rewind();
            ttrkSong.start();
            break;
		case SL_KEY_F11:   
			ttrkSong.setPatternPlayFlag(false);
			if(patmode) ttrkSong.rewindTo(pos, patbars[ curcol ]->getCursor()); 
			else ttrkSong.rewindTo(pos, 0);
			ttrkSong.start(); 
			break;
		//case SL_KEY_F12:   ttrkSong.rewind(); break;
		case SL_KEY_F12:
			ttrkSong.setPatternPlayFlag(true);
			ttrkSong.setPatternPlayNum(pos);
			ttrkSong.rewindTo(pos, 0);
			ttrkSong.start(); 
			break;

		case SL_KEY_DOWN:
			moveCursorPosition( curstate.cursorrow + 1, curcol ); break;
		case SL_KEY_UP:
			moveCursorPosition( curstate.cursorrow - 1, curcol ); break;
		case SL_KEY_LEFT:
			moveCursorPosition( curstate.cursorrow, curcol - 1 ); break;
		case SL_KEY_RIGHT:
			moveCursorPosition( curstate.cursorrow, curcol + 1 ); break;

		case SL_KEY_SHIFTLEFT:
			moveCursorPosition( curstate.cursorrow, curcol - numbars ); break;
		case SL_KEY_SHIFTRIGHT:
			moveCursorPosition( curstate.cursorrow, curcol + numbars ); break;

		case SL_KEY_IC:     bars[ curcol ]->editMidiChannel( 1 ); break;
		case SL_KEY_DELETE: bars[ curcol ]->editMidiChannel( -1 ); break;

		case '/': bars[ curcol ]->editChannelVolume( -1 ); break;
		case '*': bars[ curcol ]->editChannelVolume( 1 ); break;

		case SL_KEY_HOME:
			if( patmode ) patbars[ curcol ]->editCursorVolume( 1 ); break;
		case SL_KEY_END:
			if( patmode ) patbars[ curcol ]->editCursorVolume( -1 ); break;

		case '[': bars[ curcol ]->editPattern( -1 ); break;
		case ']': bars[ curcol ]->editPattern( 1 ); break;

		case '+': ++octave; break;
		case '-': --octave; break;

		case '(': if( patmode ) patbars[ curcol ]->transposeNotes( -1 ); break;
		case ')': if( patmode ) patbars[ curcol ]->transposeNotes( 1 ); break;

		case SL_KEY_PPAGE:
			if( patmode ) {
				setPosition( pos - 1 );
				printCurrRow();
			} else {
				moveCursorPosition( curstate.cursorrow - 8, curcol );
			}
			break;
		case SL_KEY_NPAGE:
			if( patmode ) {
				setPosition( pos + 1 );
				printCurrRow();
			} else {
				moveCursorPosition( curstate.cursorrow + 8, curcol );
			}
			break;

		case '`':
			if( patmode ) {
				 patbars[ curcol ]->toggleCursorSlide();
				moveCursorPosition( patstate.cursorrow + 1, curcol );
			}
			break;

		case ' ':
			if( patmode ) {
				setNote( Song::NoteEmpty );
			} else {
				//bars[ curcol ]->editPattern( 0 - ( SongMaxPat + 1 ) );
				bars[ curcol ]->setPattern( 0 );
				moveCursorPosition( curstate.cursorrow + 1, curcol );
			}
			break;

		case 'a': break;
		case 'z': setNote( (octave + 1) * 12 + 0 ); break;
		case 's': setNote( (octave + 1) * 12 + 1 ); break;
		case 'x': setNote( (octave + 1) * 12 + 2 ); break;
		case 'd': setNote( (octave + 1) * 12 + 3 ); break;
		case 'c': setNote( (octave + 1) * 12 + 4 ); break;
		case 'f': break;
		case 'v': setNote( (octave + 1) * 12 + 5 ); break;
		case 'g': setNote( (octave + 1) * 12 + 6 ); break;
		case 'b': setNote( (octave + 1) * 12 + 7 ); break;
		case 'h': setNote( (octave + 1) * 12 + 8 ); break;
		case 'n': setNote( (octave + 1) * 12 + 9 ); break;
		case 'j': setNote( (octave + 1) * 12 + 10 ); break;
		case 'm': setNote( (octave + 1) * 12 + 11 ); break;
		case 'k': break;
		case 'l': break;

		case '1': break;
		case 'q': setNote( (octave + 1) * 12 + 12 ); break;
		case '2': setNote( (octave + 1) * 12 + 13 ); break;
		case 'w': setNote( (octave + 1) * 12 + 14 ); break;
		case '3': setNote( (octave + 1) * 12 + 15 ); break;
		case 'e': setNote( (octave + 1) * 12 + 16 ); break;
		case '4': break;
		case 'r': setNote( (octave + 1) * 12 + 17 ); break;
		case '5': setNote( (octave + 1) * 12 + 18 ); break;
		case 't': setNote( (octave + 1) * 12 + 19 ); break;
		case '6': setNote( (octave + 1) * 12 + 20 ); break;
		case 'y': setNote( (octave + 1) * 12 + 21 ); break;
		case '7': setNote( (octave + 1) * 12 + 22 ); break;
		case 'u': setNote( (octave + 1) * 12 + 23 ); break;
		case '8': break;

		case 'i': setNote( (octave + 1) * 12 + 24 ); break;
		case '9': setNote( (octave + 1) * 12 + 25 ); break;
		case 'o': setNote( (octave + 1) * 12 + 26 ); break;
		case '0': setNote( (octave + 1) * 12 + 27 ); break;
		case 'p': setNote( (octave + 1) * 12 + 28 ); break;

		//case SL_KEY_BACKSPACE: setNote( Song::NoteOff ); break;
		case SL_KEY_BACKSPACE: 
			if( patmode ) {
				setNote( Song::NoteOff );
			} else {
				bars[ curcol ]->setPattern( Song::NoteOffPatternId );
				moveCursorPosition( curstate.cursorrow + 1, curcol );
			}
			break;

		case '#': setController(); break;
		case '@': enterVelocity(); break;
		case '=': inputValue(); break;
		case '': ctlLinearFill(); break;
		case '?': printHelp(); break;

		case '': inputTempo(); break;
		case '': ttrkSong.setLoopPosition( pos + 1 ); break;
		case '': ttrkSong.setLoopDestination( pos ); break;
		case '': ttrkSong.setStartPos( pos ); break;
		case  19 : saveSong(); break; // Ctrl-S
		case '': loadSong(); break; // Ctrl-S
		case '': toggleSync(); break;
		case '': saveCopy(); break;
		case '': pasteCopy(); break;
		case 'C':  saveSongRowCopy(); break;
		case 'V':  pasteSongRowCopy(); break;
		case 'M':  markSongRow(); break;
		case '': enterChanName(); break;
		case '': enterPatName(); break;
		case  '!': ttrkSong.toggleLoop(); break;
		case '\t': switchMode(); break;
		case '': shiftDel();break; //Ctrl-D
		case '': shiftIns();break; //Ctrl-B
		case 'D': shiftDelRows();break; // Shift-D
		case 'B': shiftInsRows();break; // Shift-B
		default: SLtt_beep(); break;
		}
	} else {
		if(jam_note) {
			// Don't stop playing the jam note unless
			// a certain number of milliseconds has passed without 
			// keyboard input detected.
			// The problem is that with s-lang, we only know when a 
			// key is pressed; there are no "key up" events.
			// So in general we assume that the user is holding a key
			// down if we get the same note more than once in a row.
			// However, we sample the keyboard more frequently than
			// the typical keyboard repeat delay, so even though
			// the user may be holding a key down, s-lang will timeout
			// waiting for input before the keyboard starts to repeat.
			// The default Linux console repeat delay is 250 ms (see
			// "man kbdrate").
			// To use the same repeat delay in X, "xset r rate 250".
			int JAM_TIMEOUT = 250;
			struct timeb now;
			ftime(&now);
			if(diff_timeb(now, jam_time) >= JAM_TIMEOUT) {
				midiControl.stopNote(jam_chan, jam_note, jam_vel);
				jam_note = 0;
				jam_chan = 0;
				jam_vel = 0;
				//fprintf(stderr, "stopping jam - no key pending\n");
			}
		}
	}

	refresh();
	return false;
}

void Screen::printHeader( void ) const
{
	char *status;
	char *loopstat;
	char tempo[ 4 ];

	if( ttrkSong.isPlaying() ) {
	    if( ttrkSong.getPatternPlayFlag() ) {
		    status = "Pattern";
        } else {
		    status = "Playing";
        }
	} else {
		status = "Stopped";
	}

	if( ttrkSong.isLoopActive() ) {
		loopstat = "  Active";
	} else {
		loopstat = "Inactive";
	}

	if( midiControl.usingInternalClock() ) {
		sprintf( tempo, "%03d", ttrkSong.getTempo() );
	} else {
		strcpy( tempo, "---" );
	}

	SLsmg_gotorc( 0, 0 );
	SLsmg_printf( " %03x:%02d %s          Speed: %sbpm            Loop: %s, %03x -> %03x",
		ttrkSong.getCurPat(), ttrkSong.getCurBeat(), status, tempo, loopstat,
		ttrkSong.getLoopPosition(), ttrkSong.getLoopDestination() );
	SLsmg_gotorc( 1, 0 );
	SLsmg_printf( "[%03x]                  Viewing: %03x             Octave: %02d",
		ttrkSong.getStartPos(), pos, octave );
	SLsmg_gotorc( 2, 0 );
	//SLsmg_printf( "       <F1>     <F2>     <F3>     <F4>     <F5>     <F6>     <F7>     <F8>");
	SLsmg_printf( "       [F1]     [F2]     [F3]     [F4]     [F5]     [F6]     [F7]     [F8]");
}

void Screen::printSidebar( void ) const
{
	int i;

	SLsmg_gotorc( 9, 0 );
	SLsmg_printf( "====" );

	int scrsize = SLtt_Screen_Rows - 3 - Screen::HeaderSize - Screen::BottomSize;
	if( patmode ) {
		for( i = patstate.scrolltop; i < SongPatLen; i++ ) {
			if( i >= ( patstate.scrolltop + scrsize ) ) break;
			SLsmg_gotorc( 10 + i - patstate.scrolltop, 0 );
			if( ttrkSong.getCurPat() == pos && ttrkSong.getCurBeat() == i ) {
				SLsmg_set_color( Screen::PlayPositionColour );
			} else if( i % 4 == 0 ) SLsmg_set_color( Screen::HilightColour );
			SLsmg_printf( " %02d: ", i );
			SLsmg_set_color( Screen::DefaultColour );
		}
	} else {
		for( i = songstate.scrolltop; i < SongLen; i++ ) {
			if( i >= ( songstate.scrolltop + scrsize ) ) break;
			SLsmg_gotorc( 10 + i - songstate.scrolltop, 0 );
			if( ttrkSong.getCurPat() == i ) {
				SLsmg_set_color( Screen::PlayPositionColour );
			} else if( i % 4 == 0 ) SLsmg_set_color( Screen::HilightColour );
			SLsmg_printf( "%03x:", i );
			if( i < ttrkSong.getLoopPosition() && i >= ttrkSong.getLoopDestination() ) {
				if( ttrkSong.isLoopActive() ) {
					SLsmg_set_color( Screen::LoopHilightColour );
				} else {
					SLsmg_set_color( Screen::LoopInactiveColour );
				}
			}
			SLsmg_printf( " " );
			SLsmg_set_color( Screen::DefaultColour );
		}
	}
}

void Screen::printHelp( void )
{
	int currow=0;
	char kformat[]="%30s  %30s";
	
	SLsmg_cls();
	SLsmg_set_color( Screen::HelpColour );

	SLsmg_gotorc( currow++, 35 );
	SLsmg_printf( "Global:");
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Load song:   ^A","Save song:   ^S" );
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Quit:   ^Q","MIDI Sync Source:   ^E" );
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Play/Stop:   F9","Rewind+Play:  F10" );
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Play from cursor:  F11","Play Pattern:  F12" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "View position: PgUp Dn","Toggle view:  TAB" );        
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Mute channel:   F1-F8","+/- pattern:  [ ]" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Set start position:   ^F","Toggle loop mode:    !" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Set loop start:   ^L","Set loop destination:   ^K" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Set tempo:   ^T","+/- octave:  + -" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Set channel name:   ^N","Set pattern name:   ^P" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "+/- MIDI channel: Ins Del","+/- channel volume:  * /" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Clear row:  SPC", "Set NoteOff:   ^H" );    
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Insert blank row:   ^B","Delete row:   ^D" );
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Insert blank rows:   ~B","Delete rows:   ~D" );
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Copy:   ^C", "Paste:   ^V" );
	currow++;
	SLsmg_gotorc( currow++, 27 );
	SLsmg_printf( "Pattern view specific:");
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "+/- vel/Ctl value: Hm End", "Set vel/Ctl value:  =");
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Set controller:  #", "Ctl linear fill:   ^W");
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Set channel velocity:  @", "Transpose notes:  ( )" );    
	currow++;
	SLsmg_gotorc( currow++, 27 );
	SLsmg_printf( "Sequence view specific:");
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Copy row(s):   ~C", "Paste row(s):   ~V" );
	SLsmg_gotorc( currow++, 2 );
	SLsmg_printf( kformat, "Mark row:   ~M", "Set pattern: =" );    
	currow++;
	SLsmg_gotorc( currow++, 13 );
	SLsmg_printf( "Notes:     ^ = Ctrl     ~ = Shift     ^H = Backspace");

	SLsmg_set_color( Screen::DefaultColour );
	SLsmg_gotorc( SLtt_Screen_Rows - 1, SLtt_Screen_Cols - 1 );
	SLsmg_refresh();
		
	for(;;) {
		if( SLang_input_pending( 1 ) ) {SLkp_getkey(); break;}
	}
	
	SLsmg_cls();
	refresh();
	refresh_bars();
}

void Screen::showStatusMessage( char *newmsg )
{
	SLsmg_set_color( Screen::StatusColour );
	
	SLsmg_gotorc( SLtt_Screen_Rows - 1, 0 );
		SLsmg_erase_eol();
		SLsmg_write_string( newmsg );
	SLsmg_gotorc( SLtt_Screen_Rows - 1, SLtt_Screen_Cols - 1 );
	SLsmg_refresh();
	
	SLsmg_set_color( Screen::DefaultColour );
	
	msg_visible=15; /*for how long message will be shown*/
}

void Screen::clearStatusMessage( void ) const
{
	SLsmg_gotorc( SLtt_Screen_Rows - 1, 0 );
	SLsmg_erase_eol();
}
	
void Screen::moveCursorPosition( int newrow, int newcol )
{
	int i;

	// Wrap up-down.
	if( patmode ) {
		if( newrow < 0 ) newrow = 31;
		newrow %= SongPatLen;
	} else {
		if( newrow < 0 ) newrow = 0;
		if( newrow >= SongLen ) newrow = SongLen - 1;
	}

	// Wrap left-right.
	newcol += bars[ 0 ]->getIndex();
	if( newcol < 0 ) newcol = 0;
	if( newcol >= SongMaxChan ) newcol = SongMaxChan - 1;

	int range = ( newcol / numbars );
	for( i = 0; i < numbars; i++ ) {
		patbars[ i ]->setChannel( (numbars * range ) + i );
		songbars[ i ]->setChannel( (numbars * range ) + i );
	}
	newcol %= numbars;

	patbars[ curcol ]->setCursor( -1 );
	songbars[ curcol ]->setCursor( -1 );
	curstate.scrolltop = bars[ newcol ]->setCursor( newrow );
	for( i = 0; i < numbars; i++ ) {
		bars[ i ]->setScrollTop( curstate.scrolltop );
	}
	printSidebar();
	
	curstate.cursorrow = newrow;
	curcol = newcol;
	if( patmode ) {
		patstate = curstate;
	} else {
		songstate = curstate;
		setPosition( songstate.cursorrow );
	}
	printSidebar();
	printCurrRow();
}

void Screen::setPosition( int newpos )
{
	if( newpos < 0 ) newpos = 0;
	if( newpos > SongLen ) newpos = SongLen;
	pos = newpos;

	for( int i = 0; i < numbars; i++ ) {
		patbars[ i ]->setPosition( pos );
		if( patmode ) patbars[ i ]->refresh();
	}
	songstate.cursorrow = pos;
	ttrkSong.setPatternPlayNum(pos);

	refresh();
}

void Screen::inputTempo( void ) const
{
	inputBuffer( "Enter Tempo: ", "" );
	ttrkSong.setTempo( atoi( inputbuf ) );
	refresh();
}

void Screen::saveSong( void )
{
	inputBuffer( "Enter Save Filename: ", ttrkSong.getFilename() );
	if( inputbuf[ 0 ] == '\0' ) {
		showStatusMessage( "Save cancelled." );
		return;
	}
	string result = expand_path( string( inputbuf ) );

	const char *filename = result.c_str();
	if(strcmp(filename, ttrkSong.getFilename())) {
        int fd = open( filename, O_RDONLY );
        if( fd > 0 ) {
            close(fd);
            if(! confirm("File exists. Overwrite?")) {
                showStatusMessage( "Save aborted." );
                refresh();
                return;
            }
        }
	}
    
	if( !ttrkSong.saveSong( filename ) ) {
		showStatusMessage( "Save failed." );
	}
	else {
		showStatusMessage( "Saved." );
	}
	refresh();
}

void Screen::loadSong( void )
{
	inputBuffer( "Enter Load Filename: ", ttrkSong.getFilename() );
	if( inputbuf[ 0 ] == '\0' ) {
		showStatusMessage( "Load cancelled." );
		return;
	}
	string result = expand_path( string( inputbuf ) );
	if( !ttrkSong.loadSong( result.c_str() ) ) {
		showStatusMessage( "Load failed." );
	}
	else {
		for( int i = 0; i < numbars; i++ ) {
			bars[ i ]->refresh();
		}
		showStatusMessage( "Song loaded." );
	}
	refresh();
}

void Screen::enterPatName( void )
{
	if( !bars[ curcol ]->patternSet() ) {
		showStatusMessage( "No pattern active on current channel." );
		return;
	}
	inputBuffer( "Enter Pattern Name: ", "" );
	// For now, trim all names to 8 characters.
	inputbuf[ 8 ] = '\0';
	bars[ curcol ]->setPatternName( inputbuf );
	showStatusMessage( "Name set." );
}

void Screen::enterChanName( void )
{
	inputBuffer( "Enter Channel Name: ", "" );
	// For now, trim all names to 8 characters.
	inputbuf[ 8 ] = '\0';
	bars[ curcol ]->setChannelName( inputbuf );
	showStatusMessage( "Name set." );
}

void Screen::enterVelocity( void )
{
	int cvel;
	inputBuffer( "Enter Channel Velocity [0-7f]: ", "" );
	sscanf(inputbuf,"%x",&cvel);
		
	if(cvel > 0x7F || cvel < 0 || strlen(inputbuf) > 2) {
		showStatusMessage( "Invalid velocity." );
	}
	else {
		if( patmode ) {
			patbars[ curcol ]->setVelocity( cvel );
		} else {
			songbars[ curcol ]->setVelocity( cvel );
		}
		showStatusMessage( "Velocity set." );
	}
}

void Screen::toggleSync( void )
{
	if( !midiControl.usingInternalClock() ) {
		midiControl.useInternalClock();
		showStatusMessage( "Using internal sync." );
	} else {
		midiControl.useExternalClock();
		showStatusMessage( "Using external sync." );
	}
}

void Screen::saveCopy( void )
{
    if( patmode ) {
        SongPattern *pat = ttrkSong.getChannel(
            patbars[ curcol ]->getIndex() )->getSongPattern( pos );
        if( !pat ) return;
        pat->copyTo( &copyPattern );
    } else {
        SongChannel *chan = ttrkSong.getChannel(
            patbars[ curcol ]->getIndex() );
        if( !chan ) return;
        chan->copyTo( &copyChannel );
    }
    showStatusMessage( "Copied." );
}

void Screen::pasteCopy( void )
{
    if(patmode ) {
        SongChannel *chan = ttrkSong.getChannel(
            patbars[ curcol ]->getIndex() );
        SongPattern *pat = chan->getSongPattern( pos );
        if( (!pat) || pat->isNoteOff() ) {
            for( int i = 0; i < SongMaxPat; i++ ) {
                if( chan->getChannelPattern( i )->isBlank() ) {
                    chan->setSongPattern( pos, i + 1 );
                    pat = chan->getSongPattern( pos );
                    break;
                }
            }
        }
        pat = chan->getSongPattern( pos );
        if( !pat ) return;
        copyPattern.copyTo( pat );
        patbars[ curcol ]->refresh();
    } else {
        SongChannel *chan = ttrkSong.getChannel(
            patbars[ curcol ]->getIndex() );
        if( !chan ) return;
        copyChannel.copyTo( chan );
        songbars[ curcol ]->refresh();
    }
    showStatusMessage( "Pasted." );
}

void Screen::markSongRow( void )
{
	if( patmode ) {
		SLtt_beep();
		return;
	}
	markedRow = pos;
	showStatusMessage( "Marked." );
}

void Screen::saveSongRowCopy( void )
{
	if( patmode ) {
		SLtt_beep();
		return;
	}
	int start;
	int end;
	start = end = pos;
	if(markedRow != -1) {
		if(markedRow > pos) {
			end = markedRow;
		} else {
			start = markedRow;
		}
	}
	for(int row=0; row<SongLen; row++) {
		copySongRows[row][0] = END_OF_ROWS_MARKER;
		if(row > (end - start)) break;
		for(int i = 0; i < SongMaxChan; i++) {
			copySongRows[row][i] = ttrkSong.getChannel(i)->getSongPatternId(row+start);
		}
	}
	markedRow = -1;
	showStatusMessage( "Copied." );
}

void Screen::pasteSongRowCopy( void )
{
	if( patmode ) {
		SLtt_beep();
		return;
	}
	for(int row = 0; row < SongLen; row++) {
		if( copySongRows[row][0] == END_OF_ROWS_MARKER) {
			break;
		}
		for(int i = 0; i < SongMaxChan; i++) {
			ttrkSong.getChannel(i)->setSongPattern(pos+row, copySongRows[row][i]);
		}
	}
	refresh_bars();
	showStatusMessage( "Pasted." );
}

void Screen::setNote( int newnote )
{
	if( patmode ) {
		patbars[ curcol ]->setNote( newnote );
		moveCursorPosition( patstate.cursorrow + 1, curcol );
		// refresh the screen
		SLsmg_refresh();
		// preview the note
		if((newnote != Song::NoteEmpty) && (newnote != Song::NoteOff) && (!ttrkSong.isPlaying()))
		{
			int chan = patbars[curcol]->getChannel()->getMidiChannel();
			int vel = patbars[curcol]->getChannel()->getVelocity();
			midiControl.playNote(chan, newnote, vel);
			while(! SLang_input_pending(1)) {
				// do nothing; wait for a key press
			}
			midiControl.stopNote(chan, newnote, vel);
		}
	} else {
		if(jam_note != newnote) {
			if(jam_note) {
				// a note is playing that we need to stop
				midiControl.stopNote(jam_chan, jam_note, jam_vel);
				//fprintf(stderr, "stopping jam - new note detected\n");
			}
			jam_note = newnote;
			jam_chan = bars[curcol]->getChannel()->getMidiChannel();
			jam_vel =  bars[curcol]->getChannel()->getVelocity();
			midiControl.playNote(jam_chan, jam_note,jam_vel);
			//fprintf(stderr, "starting jam - chan:%i note:%i\n", jam_chan+1, jam_note);
		}
		ftime(&jam_time);  // still jammin'
	}
}

void Screen::setController( void )
{
	int cnum;
	
	if( patmode ) {
		inputBuffer( "Controller number [0-7f]: ", "" );
		sscanf(inputbuf,"%x",&cnum);
		
		if(cnum > 0x7F || cnum < 0 || strlen(inputbuf) > 2) {
			showStatusMessage( "Invalid controller number." );
		}
		else {
			patbars[ curcol ]->setController( cnum );
			//moveCursorPosition( patstate.cursorrow + 1, curcol );
		}
	}    
}


void Screen::inputValue( void )
{
	int val;
	
	if( patmode ) {
		inputBuffer( "Value [0-7f]: ", "" );
		sscanf(inputbuf,"%x",&val);
		if(val > 0x7F || val < 0 || strlen(inputbuf) > 2) {
			showStatusMessage( "Invalid value." );
			return;
		}
		patbars[ curcol ]->setCursorVolume(val);
	}
	else {
		inputBuffer( "Pattern [0-ff]: ", "" );
		sscanf(inputbuf,"%x",&val);
		if(val > 0xFF || val < 0 || strlen(inputbuf) > 2) {
			showStatusMessage( "Invalid pattern number." );
			return;
		}
		bars[ curcol ]->setPattern(val);
	}
}

void Screen::ctlLinearFill( void )
{
	int cnum, beats, vfrom, vto;
	
	if(!patmode) return;
		
	inputBuffer( "Controller number [0-7f]: ", "" );
	sscanf(inputbuf,"%x",&cnum);
	
	if(cnum > 0x7F || cnum < 0 || strlen(inputbuf) > 2) {
		showStatusMessage( "Invalid controller number." );
		return;
	}
	
	inputBuffer( "beats: ", "" );
	beats=atoi(inputbuf);
	if(!beats || beats < 0) {
		showStatusMessage( "Invalid value." );
		return;        
	}
	
	inputBuffer( "From controller value [0-7f]: ", "" );
	sscanf(inputbuf,"%x",&vfrom);
	if(vfrom > 0x7F || vfrom < 0 || strlen(inputbuf) > 2) {
		showStatusMessage( "Invalid value." );
		return;
	}

	inputBuffer( "To controller value [0-7f]: ", "" );
	sscanf(inputbuf,"%x",&vto);
	if(vto > 0x7F || vto < 0 || strlen(inputbuf) > 2) {
		showStatusMessage( "Invalid value." );
		return;
	}

	patbars[ curcol ]->ctlLinearFill(cnum,beats,vfrom,vto);
}

void Screen::shiftDel( void )
{
	if( patmode ) {
		patbars[ curcol ]->shiftRows(patbars[ curcol ]->getCursor(),false);
	} else {
		bars[ curcol ]->shiftRows(pos,false);
	}
}

void Screen::shiftIns( void )
{
	if( patmode ) {
		patbars[ curcol ]->shiftRows(patbars[ curcol ]->getCursor(),true);
	} else {
		bars[ curcol ]->shiftRows(pos,true);
	}
}

void Screen::shiftDelRows( void )
{
	int i;
	if( patmode ) {
		for(i = 0; i < SongMaxChan; i++) {
			SongPattern *pat = ttrkSong.getChannel(i)->getSongPattern(pos);
			if(pat) {
				pat->shiftNotes(patbars[ curcol ]->getCursor(), false);
			}
		}
	} else {
		for(i = 0; i < SongMaxChan; i++) {
			ttrkSong.getChannel(i)->shiftRows(pos, false);
		}
	}
	refresh_bars();
}

void Screen::shiftInsRows( void )
{
	int i;
	if( patmode ) {
		for(i = 0; i < SongMaxChan; i++) {
			SongPattern *pat = ttrkSong.getChannel(i)->getSongPattern(pos);
			if(pat) {
				pat->shiftNotes(patbars[ curcol ]->getCursor(), true);
			}
		}
	} else {
		for(i = 0; i < SongMaxChan; i++) {
			ttrkSong.getChannel(i)->shiftRows(pos, true);
		}
	}
	refresh_bars();
}

