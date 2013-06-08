/**
 * Copyright (C) 2001 Paul Davis, Billy Biggs.
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

#include <config.h>
#ifdef HAVE_LINUX_RTC_H
// This is a 2.4'ism.
#include <linux/rtc.h>
#else
// Fucking mc146..rtc.h needs me to include asm/splinlock.
#include <asm/spinlock.h>
#include <linux/mc146818rtc.h>
#endif

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <rtctimer.h>

RealTimeClock::RealTimeClock( void )
{
	init_ok=true;
	
	if( ( rtc_fd = open( "/dev/rtc", O_RDONLY ) ) < 0 ) {
		printf( "\nRealTimeClock: Cannot open /dev/rtc: %s\n",
			strerror( errno ) );
		printf( "You cant do internal sync without the real "
			"time clock, so I'm shutting down.\n" );
		printf( "I will make this more graceful in future.\n" );
		init_ok=false;
		return;
	}

	if( fcntl( rtc_fd, F_SETOWN, getpid() ) < 0 ) {
		printf( "\nRealTimeClock: cannot set ownership of "
			"/dev/rtc: %s\n", strerror( errno ) );
		printf( "You cant do internal sync without the real "
			"time clock, so I'm shutting down.\n" );
		printf( "I will make this more graceful in future.\n" );
		init_ok=false;
		return;
	}

	rtc_running = false;
	current_hz = 0;
}

RealTimeClock::~RealTimeClock( void )
{
	stopClock();
	close( rtc_fd );
}

int RealTimeClock::nextTick( void )
{
	unsigned long rtc_data;
	struct pollfd pfd;
	pfd.fd = rtc_fd;
	pfd.events = POLLIN | POLLERR;

again:
	if( poll( &pfd, 1, 100000 ) < 0 ) {
		if (errno == EINTR) {
			// This happens mostly when run under gdb, or when
			// exiting due to a signal.
			goto again;
		}

		printf( "RealTimeClock: poll call failed: %s\n",
			strerror( errno ) );
		return 0;
	}

	read( rtc_fd, &rtc_data, sizeof( rtc_data ) );
	return 1;
}

bool RealTimeClock::setInterval( int hz )
{
	bool restart;

	if( hz == current_hz ) {
		return true;
	}
	
	restart = stopClock();

	if( ioctl( rtc_fd, RTC_IRQP_SET, hz ) < 0 ) {
		printf( "RealTimeClock: cannot set periodic interval: %s\n",
			strerror( errno ) );
		return false;
	}

	current_hz = hz;

	if( restart ) {
		startClock();
	}

	return true;
}

bool RealTimeClock::startClock( void )
{
	if( !rtc_running ) {
		if( ioctl( rtc_fd, RTC_PIE_ON, 0 ) < 0 ) {
			printf( "RealTimeClock: cannot start periodic "
				"interrupts: %s\n", strerror( errno ) );
			return false;
		}
		rtc_running = true;
	}
	return rtc_running;
}
	
bool RealTimeClock::stopClock( void )
{
	bool was_running = rtc_running;

	if( rtc_running ) {
		if( ioctl( rtc_fd, RTC_PIE_OFF, 0 ) < 0 ) {
			printf( "RealTimeClock: cannot stop periodic "
				"interrupts: %s\n", strerror( errno ) );
			return rtc_running;
		}
		rtc_running = false;
	}

	return was_running;
}

