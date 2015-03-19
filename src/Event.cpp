
/*! \file   Event.cpp

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
    event classes for all events handled by meter
    Code based on Netmate Implementation

    $Id: Event.h 748 2015-03-03 13:00:03 amarentes $

*/

#include "Error.h"
#include "Event.h"
#include "Timeval.h"
#include "QualityManager.h"


Event::Event(event_t typ, unsigned long ival, int align)
    : type(typ), interval(ival)
{
    Timeval::gettimeofday(&when, NULL);

    if (align) {
        doAlign();
    }
}


Event::Event(event_t typ, time_t offs_sec, time_t offs_usec, unsigned long ival, int align)
    : type(typ), interval(ival)
{
    Timeval::gettimeofday(&when, NULL);
    
    when.tv_sec += offs_sec;
    when.tv_usec += offs_usec;
    
    if (align) {
        doAlign();
    }
}


Event::Event(event_t typ, struct timeval time, unsigned long ival, 
	     int align) 
    : type(typ), when(time), interval(ival)
{ 
    if (align) {
        doAlign();
    }
}


void Event::doAlign()
{
    if (interval > 0) {
        double _when = when.tv_sec*1e6+when.tv_usec;
        double _ival = interval*1000;
        
        _when = floor(_when/_ival)*_ival + _ival;
        
        when.tv_sec = (unsigned long) floor(_when/1e6);
        when.tv_usec = (unsigned long) (_when-when.tv_sec*1e6);
    }
}


void Event::advance()                         
{ 
    struct timeval ad;

    ad.tv_sec = (unsigned long) interval/1000;
    ad.tv_usec = (unsigned long) ( (interval % 1000)*1000);
    
    when = Timeval::add(when, ad);
}


void Event::dump( ostream &os )       
{ 
    os << eventNames[type] << " event: "
       << when.tv_sec*1e6+when.tv_usec << " (" << interval << ")";
}


ostream& operator<< (ostream &os, Event &ev )
{
    ev.dump(os);
    return os;
}
