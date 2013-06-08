/**
 * Copyright (C) 1999, 2000, 2001 Billy Biggs and Andrew Lewycky.
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

#include <assert.h>
#include <thread.h>

void *Thread::thread_thunk( void *pthis )
{
	// Make sure we don't get killed early.
	pthread_setcanceltype( PTHREAD_CANCEL_DEFERRED, 0 );
	( (Thread *) pthis )->thread_main();
	return NULL;
}

bool Thread::run( void )
{
	assert( !running );
	if( pthread_create( &thread_handle, NULL, thread_thunk, this ) != 0 ) {
		return false;
	}
	running = true;
	return true;
}

void Thread::stop( void )
{
	if( !running ) return;
	pthread_cancel( thread_handle );
	pthread_join( thread_handle, NULL );
	running = false;
}

