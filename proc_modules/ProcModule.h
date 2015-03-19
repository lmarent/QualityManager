
/*! \file  proc_modules/ProcModule.h

    Copyright 2003-2004 Fraunhofer Institute for Open Communication Systems (FOKUS),
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
    declaration of interface for Classifier Action Modules

    $Id: ProcModule.h 748 2009-09-10 02:54:03Z szander $

*/


#ifndef __PROCMODULE_H
#define __PROCMODULE_H


#include "ProcModuleInterface.h"
#include "stdincpp.h"

/*! Functions for reading parameters and filters, 
 *  It is assumed that data is already verified. */
 
 
long parseLong( string s );
uint8_t parseUInt8(unsigned char *val);

unsigned long parseULong( string s );
uint16_t parseUInt16( unsigned char *val );

long long parseLLong( string s );
uint32_t parseUInt32( unsigned char *val );

unsigned long long parseULLong( string s );

int parseInt( string s );

inline int isNumericIPv4( string s);

inline struct in_addr parseIPAddr( string s);
inline uint32_t parseIPAddr( unsigned char *val );

inline int isNumericIPv6( string s);
inline struct in6_addr parseIP6Addr( string s);

inline int parseBool( string s);

inline float parseFloat( string s);

inline double parseDouble( string s);


/*! \short   declaration of struct containing all function pointers of a module */
extern ProcModuleInterface_t func;


#endif /* __PROCMODULE_H */

