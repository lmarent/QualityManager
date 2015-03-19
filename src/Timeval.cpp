
/*!  \file   Timeval.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogotá, Colombia

    This file is part of Network Quality Manager System (NETQoS).

    NETQoS is free software; you can redistribute it and/or modify 
    it under the terms of the GNU General Public License as published by 
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NETQoS is distributed in the hope that it will be useful, 
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this software; if not, write to the Free Software 
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Description:
    functions for comparing, adding and subtracting timevals
    Code based on Netmate

    $Id: Timeval.cpp 748 2015-03-03 15:46:00 amarentes $

*/

#include "Timeval.h"
#include "Error.h"


// global time.
struct timeval Timeval::g_time = {0, 0};


int Timeval::cmp(struct timeval t1, struct timeval t2)
{
    
    if (t1.tv_sec < t2.tv_sec) {
	return -1;
    } else if (t1.tv_sec > t2.tv_sec) {
	return +1;
    } else {
	if (t1.tv_usec < t2.tv_usec) {
	    return -1;
	} else if (t1.tv_usec > t2.tv_usec) {
	    return +1;
	} else {
	    return 0;
	}
    }
}

struct timeval Timeval::add(struct timeval a, struct timeval b)
{
    struct timeval rv;

    rv.tv_sec = a.tv_sec + b.tv_sec;
    rv.tv_usec = a.tv_usec + b.tv_usec;
    if (rv.tv_usec >= 1000000) { 
	rv.tv_usec -= 1000000;
	rv.tv_sec++;
    }

    return rv;
}

struct timeval Timeval::sub0(struct timeval num, struct timeval sub)
{
    struct timeval rv;

    rv.tv_sec = num.tv_sec - sub.tv_sec;
    rv.tv_usec = num.tv_usec - sub.tv_usec;
    if (rv.tv_usec < 0) { 
	rv.tv_usec += 1000000;
	rv.tv_sec--;
    }
    if (rv.tv_sec < 0) { 
	rv.tv_sec = 0;
	rv.tv_usec = 0;
    }

    return rv;
}

int Timeval::gettimeofday(struct timeval *tv, struct timezone *tz)
{
    // use timestamps from the trace
    tv->tv_sec = g_time.tv_sec;
    tv->tv_usec = g_time.tv_usec;
    return 0;

}
 
time_t Timeval::time(time_t *t)
{
    if (t != NULL) {
      *t = g_time.tv_sec;
    }
    return g_time.tv_sec;
}


// set the time
int Timeval::settimeofday(const struct timeval *tv)
{
  if ( (tv->tv_sec*1e6 + tv->tv_usec) < (g_time.tv_sec*1e6 + g_time.tv_usec) ) {
    std::cerr << "Encountered a packet from the past" << std::endl;
    return -1;
  } 

  g_time.tv_sec = tv->tv_sec;
  g_time.tv_usec = tv->tv_usec;

  return 0;
}
