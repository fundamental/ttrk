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

#include "jackmidi.h"
#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <rtosc/rtosc.h>

//TODO make these class specific or whatever
static char pending_events[100][128];
static void *midi_output_queue;
static size_t current_time = 0;
static size_t last_time = 0;
FILE *f;
int process(unsigned nframes, void *v);

JackMidi::JackMidi( void )
    :Fs(0), time(0), client(0),
    port(0), bToU(0), uToB(0)
{
    for(int i=0; i<100; ++i)
        for(int j=0; j<128; ++j)
            pending_events[i][j] = 0;
    bToU = jack_ringbuffer_create(4096);
    uToB = jack_ringbuffer_create(4096);


    jack_status_t jackstatus;
    client = jack_client_open("ttrk", JackNoStartServer, &jackstatus);
    if(client && !(port = jack_port_register(client, "midi",
                                     JACK_DEFAULT_MIDI_TYPE,
                                     JackPortIsOutput | JackPortIsTerminal, 0)))
        client = NULL;
    if(client && jack_set_process_callback(client, process, this))
        client = NULL;
    if(client && jack_activate(client))
        client = NULL;
    if(client)
        Fs = jack_get_sample_rate(client);

    assert(client);
    assert(port);
    //f = fopen("log.txt", "w");
}

JackMidi::~JackMidi( void )
{
    if(port)
        jack_port_unregister(client, port);
    if(client) {
        jack_deactivate(client);
        jack_client_close(client);
    }
    port   = NULL;
    client = NULL;
    jack_ringbuffer_free(bToU);
    jack_ringbuffer_free(uToB);
    fclose(f);
}

void JackMidi::noteOn(char chan, char note, char vel)
{
    int ev[3] = {0x90 | chan, note, vel};
}

void JackMidi::noteOff(char chan, char note, char vel)
{
    int ev[3] = {0x80 | chan, note, vel};
}

void JackMidi::polyphonicAftertouch(char chan, char note, char amt)
{
    int ev[3] = {0xA0 | chan, note, amt};
}

void JackMidi::programChange( char chan, char prog )
{
    int ev[3] = {0xC0 | chan, prog, 0};
}

void JackMidi::channelAftertouch( char chan, char amt )
{
    int ev[3] = {0xD0 | chan, amt, 0};
}

void JackMidi::pitchWheel( char chan, char amt )
{
    int ev[3] = {0xE0 | chan, 0x7F & amt, amt >> 7};
}

void JackMidi::controlChange( char chan, char controller, char amt)
{
    int ev[3] = { 0xB0 | chan, controller, amt};
}

void JackMidi::allSoundOff( char chan )
{
    int ev[3] = { 0xB0 | chan, 0x78, 0};
}

void JackMidi::resetAllControllers( char chan )
{
	int ev[3] = {0xB0 | chan, 0x79, 0};
}

void JackMidi::setLocal( char chan, bool on )
{
	int ev[3] = {0xB0 | chan, 0x7A, on ? 127 : 0};
}

void JackMidi::allNotesOff( char chan )
{
	int ev[3] = {0xB0 | chan, 0x7B, 0};
}

void JackMidi::syncStart( void )
{
	int ev = 0xFA;
}

void JackMidi::syncStop( void )
{
	int ev = 0xFC;
}

void JackMidi::syncContinue( void )
{
	int ev = 0xFB;
}

void JackMidi::syncTick( void )
{
	int ev = 0xF8;
}
    
void JackMidi::updateStatus( void )
{
    if(jack_ringbuffer_read_space(bToU) >= 8)
        jack_ringbuffer_read(bToU, (char*)&time, sizeof(time));

}

MidiDev::InMessage JackMidi::readMessage( void )
{
//	unsigned char buff;
//	int bytesread;
//
//
//	struct pollfd pfd;
//	int ret;
//	pfd.fd = midi_fd;
//	pfd.events = POLLIN | POLLERR;
//
//again:
//	ret = poll( &pfd, 1, 1000 );
//	if( ret < 0 ) {
//		if (errno == EINTR) {
//			// This happens mostly when run under gdb, or when
//			// exiting due to a signal.
//			goto again;
//		}
//		return NoMessage;
//	}
//
//	if( ret == 0 ) return NoMessage;
//
//	buff = 0;
//	bytesread = read( midi_fd, &buff, 1 );
//
//	if( bytesread < 1 ) {
//		return NoMessage;
//	}
//
//	if( buff == 0xFA ) {
//		return MIDIStart;
//	}
//
//	if( buff == 0xFB ) {
//		return MIDIContinue;
//	}
//
//	if( buff == 0xFC ) {
//		return MIDIStop;
//	}
//
//	if( buff == 0xF8 ) {
//		return MIDIClockTick;
//	}
//
	return MidiDev::NoMessage;
}

void JackMidi::flush( void )
{
	//write( midi_fd, buff, buffpos );
	//buffpos = 0;
}

void insert_pending(const char *msg,
                    size_t msg_size)
{
    assert(rtosc_bundle_p(msg));
    printf("Inserting Pending Event\n");
}

void dispatch(const char *msg,
              size_t msg_size)
{
    if(rtosc_bundle_p(msg)) {
        if(rtosc_bundle_timetag(msg) <= current_time) {
            unsigned elements = rtosc_bundle_elements(msg, msg_size);
        } else
            insert_pending(msg, msg_size);
    } else { //normal message dispatch
        const char *args = rtosc_argument_string(msg);
        if(!strcmp(msg, "/write_midi") && !strcmp(args, "b")) {
            printf("Writing midi event\n");
        }
    }
}


void dispatch_events(size_t time)
{
    for(int i=0; i<100; ++i) {
        char *bundle = pending_events[i];
        if(!rtosc_bundle_p(bundle))
            continue;
        if(rtosc_bundle_timetag(bundle) >= current_time) {
            dispatch(bundle, rtosc_message_length(bundle, 128));
            for(int j=0; j<128; ++j)
                bundle[j] = 0;
        }
        //pending_events
    }
}

int process(unsigned nframes, void *v)
{
    JackMidi &jm = *(JackMidi*)v;
    //Read incomming events
    
    //for(int i=0; i<nframes; ++i)
    //    dispatch_events(current_time++);
    current_time += nframes;
    
    //If it has been over 20ms since last update tick, sync the UI
    if((current_time-last_time)*1.0/jack_get_sample_rate(jm.client) > 20e-3) {
        //char buffer[1024];
        //rtosc_message(buffer, 1024, "/update_time", "h", current_time);
        if(jack_ringbuffer_write_space(jm.bToU) > 8)
            jack_ringbuffer_write(jm.bToU, (char*)&current_time, sizeof(current_time));
        
        //fprintf(f, "Sending update tick '%ulld' '%ulld' '%f'\n", current_time, current_time-last_time,
        //        1.0*jack_get_sample_rate(jm.client));
        //fflush(f);
        last_time = current_time;
    } 

    return 0;
}
