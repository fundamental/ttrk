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

#ifndef THREAD_H_INCLUDED
#define THREAD_H_INCLUDED

#include <pthread.h>

/**
 * This is my happy thread class.  Thank you too Andrew Lewycky
 * <amplewyc@student.math.uwaterloo.ca> who originally wrote this code.
 */
class Thread
{
public:
	/**
	 * Creates a new thread object.  Does not start it.
	 */
	Thread( void )
		: running( false ) {}

	/**
	 * Deletes the thread, join'ing with it if necessary.
	 */
	virtual ~Thread( void ) { stop(); }

	/**
	 * Returns true if the thread is currently active.
	 */
	bool isRunning( void ) const { return running; }

	/**
	 * Returns true iff the thread was started.  If you call this function
	 * and it succeeds, then you can't call it again for the same object.
	 */
	bool run( void );

protected:
	/**
	 * Kills the thread. You can only call this if the thread is running.
	 */
	void stop( void );

	/**
	 * Override this function and put your thread's code here.
	 */
	virtual void thread_main( void ) = 0;

	/**
	 * Check to see if the thread has been cancelled, and stop the thread
	 * if it has.
	 */
	void testcancel( void ) { pthread_testcancel(); }

private:
	pthread_t thread_handle;
	bool running;
	static void *thread_thunk( void *pthis );
};

#endif // THREAD_H_INCLUDED
