
/* \file constants.cc

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
  here all string and numeric constants for the netmate toolset are stored

  $Id: constants.cc 748 2009-09-10 02:54:03Z szander $

*/

#include "stdincpp.h"
#include "constants.h"

using namespace std;


// Logger.h
const string DEFAULT_LOG_FILE = DEF_STATEDIR "/log/scalTest.log";

#ifdef USE_SSL
// certificate file location (SSL)
const string CERT_FILE = DEF_SYSCONFDIR "/netmate.pem";
#endif

// scal_test
const string DEFAULT_RULE_FILE = DEF_STATEDIR "/etc/rule.xml";


// help text in interactive shell
const string HELP =  "" \
"interactive commands: \n" \
"help                            displays this help text \n" \
"quit, exit, bye                 end telly program \n" \
"any other text is sent to the server and the reply from the server is displayed.";

