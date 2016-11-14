
/*!  \file   Timeval.h

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
    Code based on Netmate Implementation

    $Id: Event.h 748 2015-03-03 15:44:00 amarentes $
*/

#ifndef _TIMEVAL_H_
#define _TIMEVAL_H_


#include <iostream>


class Timeval
{
  private:
    static struct timeval g_time;

  public:

    /*! \short compare t1 and t2
        \returns -1 if t1<t2, 0 if t1=t2 or +1 if t1>t2
    */
    static int cmp(struct timeval t1, struct timeval t2);

    //! add timval a and b and return the result
    static struct timeval add(struct timeval a, struct timeval b);

    //! subtract timval sub from timeval num and return result; if result < 0 return 0
    static struct timeval sub0(struct timeval num, struct timeval sub);

    //! function for reading the current time
    static int gettimeofdayown(struct timeval *tv, struct timezone *tz);

    static time_t time(time_t *t);

    //! set the time (used when reading the time from a pcap file)
    // return 0 if ok and -1 if time is in the past (reordering)
    static int settimeofday(const struct timeval *tv);

};

#endif // _TIMEVAL_H_
