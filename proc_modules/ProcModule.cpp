
/*! \file ProcModule.cpp

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
    implementation of helper functions for Packet Processing Modules

    $Id: ProcModule.cpp 748 2009-09-10 02:54:03Z szander $

*/

#include "ProcModule.h"
#include "ProcError.h"

/*! \short   embed magic cookie number in every packet processing module
    _N_et_M_ate _P_rocessing Module */
int magic = PROC_MAGIC;


/*! \short   declaration of struct containing all function pointers of a module */
ProcModuleInterface_t func = 
{ 
    3, 
    initModule, 
    destroyModule, 
    initFlowSetup, 
    getTimers, 
    destroyFlowSetup,
    resetFlowSetup, 
    checkBandWidth,
    timeout, 
    getModuleInfo, 
    getErrorMsg };


/*! \short   global state variable used within data export macros */
void *_dest;
int   _pos;
int   _listlen;
int   _listpos;


/* align macro */
#define ALIGN( var, type ) var = ( var + sizeof(type)-1 )/sizeof(type) * sizeof(type)

/*! Parse parameter functions */

long parseLong( string s )
{
    char *errp = NULL;
    long n;
    n = strtol(s.c_str(), &errp, 0);

    if (s.empty() || (*errp != '\0')) {
        throw ProcError("Not a long number: %s", errp);
    }
    
    return n;
	
}

uint8_t parseUInt8(unsigned char *val)
{
	uint8_t val_return = val[0];
	return val_return;
}



unsigned long parseULong( string s )
{
    char *errp = NULL;
    unsigned long n;
    n = strtoul(s.c_str(), &errp, 0);

    if (s.empty() || (*errp != '\0')) {
        throw ProcError("Not an unsigned long number: %s", errp);
    }
    
    return n;	
}

uint16_t parseUInt16( unsigned char *val )
{
	uint16_t val_return;
	val_return = (uint16_t)val[0] << 8 | (uint16_t)val[1];
	return val_return;
}

long long parseLLong( string s )
{
    char *errp = NULL;
    long long n;
    n = strtoll(s.c_str(), &errp, 0);

    if (s.empty() || (*errp != '\0')) {
        throw ProcError("Not a long long number: %s", errp);
    }
    
    return n;	
}

uint32_t parseUInt32( unsigned char *val )
{
	uint32_t val_return;
	val_return = (uint32_t)val[0] << 24 |
				 (uint32_t)val[1] << 16 |
				 (uint32_t)val[2] << 8  |
				 (uint32_t)val[3];
	return val_return;
}

unsigned long long parseULLong( string s )
{
    char *errp = NULL;
    unsigned long long n;
    n = strtoull(s.c_str(), &errp, 0);

    if (s.empty() || (*errp != '\0')) {
        throw ProcError("Not an unsigned long long number: %s", errp);
    }
    
    return n;

}

int parseInt( string s )
{
    char *errp = NULL;
    int n;
    
    n = (int) strtol(s.c_str(), &errp, 0);
    if (s.empty() || (*errp != '\0'))
        throw ProcError("Not an int number: %s", errp);
    
    return n;
	
}

inline int isNumericIPv4(string s)
{
    return (s.find_first_not_of("0123456789.", 0) >= s.length());  
}

inline struct in_addr parseIPAddr(string s)
{
    int rc;
    struct in_addr a;
    struct addrinfo ask, *res = NULL;
   
    memset(&ask,0,sizeof(ask));
    ask.ai_socktype = SOCK_STREAM;
    ask.ai_flags = 0;
    if (isNumericIPv4(s)) {
        ask.ai_flags |= AI_NUMERICHOST;
    }
    ask.ai_family = PF_INET;

    // set timeout
    g_timeout = 0;
    alarm(2);

    rc = getaddrinfo(s.c_str(), NULL, &ask, &res);

    alarm(0);

    try {
        if (g_timeout) {
            throw Error("DNS timeout: %s", s.c_str());
        }

        if (rc == 0) {
            // take first address only, in case of multiple addresses fill addresses
            // FIXME set match
            a = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
            freeaddrinfo(res);
        } else {
            throw ProcError("Invalid or unresolvable ip address: %s", s.c_str());
        }
    } catch (ProcError &e) {
        freeaddrinfo(res);
        throw e;
    }

    return a;

}

uint32_t parseIPAddr( unsigned char *val )
{
	return parseUInt32( val );
}

inline int isNumericIPv6(string s)
{
    return (s.find_first_not_of("0123456789abcdefABCDEF:.", 0) >= s.length());	
}

inline struct in6_addr parseIP6Addr(string s)
{
    int rc;
    struct in6_addr a;
    struct addrinfo ask, *res = NULL;
   
    memset(&ask,0,sizeof(ask));
    ask.ai_socktype = SOCK_STREAM;
    ask.ai_flags = 0;
    if (isNumericIPv6(s)) {
        ask.ai_flags |= AI_NUMERICHOST;
    }
    ask.ai_family = PF_INET6;

    // set timeout
    g_timeout = 0;
    alarm(2);

    rc = getaddrinfo(s.c_str(), NULL, &ask, &res);

    alarm(0);

    try {
        if (g_timeout) {
            throw ProcError("DNS timeout: %s", s.c_str());
        }

        if (rc == 0) {  
            // take first address only, in case of multiple addresses fill addresses
            // FIXME set match
            a = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
            freeaddrinfo(res);
        } else {
            throw ProcError("Invalid or unresolvable ip6 address: %s", s.c_str());
        }
    } catch (ProcError &e) {
        freeaddrinfo(res);
        throw e;
    }
    
    return a;	
}

int parseBool(string s)
{
    if ((s == "yes") || (s == "1") || (s == "true")) {
        return 1;
    } else if ((s == "no") || (s == "0") || (s == "false")) {
        return 0;
    } else {
        throw ProcError("Invalid bool value: %s", s.c_str());
    }
	
}
inline float parseFloat(string s)
{
    char *errp = NULL;
    float n;

    n = strtof(s.c_str(), &errp);

    if (s.empty() || (*errp != '\0')) {
        throw ProcError("Not a float number: %s", errp);
    }

    return n;	
}

inline double parseDouble(string s)
{
    char *errp = NULL;
    double n;

    n = strtod(s.c_str(), &errp);

    if (s.empty() || (*errp != '\0')) {
        throw ProcError("Not a double number: %s", errp);
    }

    return n;

}




