
/*! \file constants_qos.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogotá, Colombia.

    This file is part of Network Measurement and Accounting System (NETMATE).

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
	here all string and numeric constants for the netqos toolset are declared

  $Id: constants_qos.cpp 748 2015-03-05 15:29:00 amarentes $

*/

#include "stdincpp.h"
#include "constants_qos.h"

using namespace std;

// QualityManager.h
const string NETQOS_DEFAULT_CONFIG_FILE = DEF_SYSCONFDIR "/netqos_conf.xml";
const string NETQOS_LOCK_FILE   = DEF_STATEDIR "/run/netqos.pid";

// Logger.h
extern const string QOS_DEFAULT_LOG_FILE = DEF_STATEDIR "/log/netqos.log";


// ConfigParser.h
const string QOS_CONFIGFILE_DTD  = DEF_SYSCONFDIR "/netqos_conf.dtd";


#ifdef USE_SSL
// certificate file location (SSL)
const string QOS_CERT_FILE = DEF_SYSCONFDIR "/netqos.pem";
#endif

