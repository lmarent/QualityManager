
/*!\file   ProcError.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogota, Colombia.

    This file is part of Network Measurement and Accounting System (NETQoS).

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
    Error class used for exceptions in the procedure module
 
    $Id: Error.cpp 748 2015-03-11 16:12:00 amarentes $

*/

#include "ProcError.h"
#include <stdio.h>
#include <stdarg.h>


const int ERROR_MSG_BUFSIZE = 8192;


/* ------------------------- Error ------------------------- */

ProcError::ProcError(const char *fmt, ...)
{
    va_list argp;
    char buf[ERROR_MSG_BUFSIZE];

    va_start(argp, fmt);

    if (vsprintf( buf, fmt, argp ) < 0) {
        throw (ProcError("error constructing error string"));
    }
    
    va_end(argp);

    error = buf;
    errorNo = 0;
}


ProcError::ProcError(const int error_no, const char *fmt, ...)
{
    va_list argp;
    char buf[ERROR_MSG_BUFSIZE];

    va_start(argp, fmt);

    if (vsprintf( buf, fmt, argp ) < 0) {
        throw (ProcError("error constructing error string"));
    }
    
    va_end(argp);

    error = buf;
    errorNo = error_no;
}


std::ostream& operator<<(std::ostream &os, ProcError &e)
{
    e.dump(os);
    return os;
}


void ProcError::dump( std::ostream &os )
{ 
    int e = getErrorNo();
    
    if (e) {
        os << e << ": " << getError();
    } else {
        os << getError();
    }
}
