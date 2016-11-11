
/* \file constants.h

    Copyright 2015-2016 Fraunhofer Institute for Open Communication Systems (FOKUS),
                        Berlin, Germany

    This file is part of Network Measurement and Accounting System (NETMATE).

    NETMATE is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NETMATE is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this software; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Description:
  here all string and numeric constants for the netmate toolset are declared

  $Id: constants.h 748 2009-09-10 02:54:03Z szander $

*/


#include "config.h"


#ifndef _CONSTANTS_H_
#define _CONSTANTS_H_


// Logger.h
extern const string DEFAULT_LOG_FILE;

#ifdef USE_SSL
// certificate file location (SSL)
extern const string CERT_FILE;
#endif

// scal_test
extern const string DEFAULT_RULE_FILE;

#endif // _CONSTANTS_H_
