
/*! \file   Rule.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogotá, Colombia.

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
    container class for rule
    Code based on Netmate

    $Id: Rule.h 748 2015-03-03 17:23:00 amarentes $

*/

#include "Rule.h"
#include "Error.h"
#include "ParserFcts.h"
#include "constants.h"

/* ------------------------- Rule ------------------------- */

Rule::Rule(int _uid, time_t now, string sname, string rname, filterList_t &f, 
           actionList_t &a, miscList_t &m)
  : uid(_uid), state(RS_NEW), ruleName(rname), setName(sname), flags(0), bidir(0), seppaths(0),
      filterList(f), actionList(a), miscList(m)
{
    unsigned long duration;

    log = Logger::getInstance();
    ch = log->createChannel("Rule");

    log->dlog(ch, "Rule constructor");

    try {
	
        parseRuleName(rname);
	
        if (rname.empty()) {
            // we tolerate an empty sname but not an empty rname
            throw Error("missing rule identifier value in rule description");
        }
        if (sname.empty()) {
            sname = DEFAULT_SETNAME;
        }

        if (filterList.empty()) {
            throw Error("no filters specified", sname.c_str(), rname.c_str());
        }

        if (actionList.empty()) {
            throw Error("no actions specified", sname.c_str(), rname.c_str());
        }

        /* time stuff */
        start = now;
        // stop = 0 indicates infinite running time
        stop = 0;
        // duration = 0 indicates no duration set
        duration = 0;
	    
        // get the configured values
        string sstart = getMiscVal("Start");
        string sstop = getMiscVal("Stop");
        string sduration = getMiscVal("Duration");
        string sinterval = getMiscVal("Interval");
        string salign = getMiscVal("Align");
        string sftimeout = getMiscVal("FlowTimeout");
	    
        if (!sstart.empty() && !sstop.empty() && !sduration.empty()) {
            throw Error(409, "illegal to specify: start+stop+duration time");
        }
	
        if (!sstart.empty()) {
            start = parseTime(sstart);
            if(start == 0) {
                throw Error(410, "invalid start time %s", sstart.c_str());
            }
        }

        if (!sstop.empty()) {
            stop = parseTime(sstop);
            if(stop == 0) {
                throw Error(411, "invalid stop time %s", sstop.c_str());
            }
        }
	
        if (!sduration.empty()) {
            duration = ParserFcts::parseULong(sduration);
        }

        if (duration) {
            if (stop) {
                // stop + duration specified
                start = stop - duration;
            } else {
                // stop [+ start] specified
                stop = start + duration;
            }
        }
	
        // now start has a defined value, while stop may still be zero 
        // indicating an infinite rule
	    
        // do we have a stop time defined that is in the past ?
        if ((stop != 0) && (stop <= now)) {
            throw Error(300, "task running time is already over");
        }
	
        if (start < now) {
            // start late tasks immediately
            start = now;
        }

        /* TODO AM: Verify this code.
        // parse flow timeout if set
        if (!sftimeout.empty()) {
            if ((sftimeout != "no") && (sftimeout != "false") && (sftimeout != "0")) {
                flags |= RULE_FLOW_TIMEOUT;
                if ((sftimeout != "yes") && (sftimeout != "true")) {
                    flowTimeout = ParserFcts::parseInt(sftimeout);
                }
            } else {
                flowTimeout = 0;
            }
        }
        */

        string bidir_str = getMiscVal("bidir");
        if (!bidir_str.empty()) {
            bidir = 1;
        }

		string seppaths_str = getMiscVal("sep_paths");
		if (!seppaths_str.empty()) {
			seppaths = 1;
		}

        if (!getMiscVal("auto").empty()) {
            flags |=  RULE_AUTO_FLOWS;
        }

    } catch (Error &e) {    
        state = RS_ERROR;
        throw Error("rule %s.%s: %s", sname.c_str(), rname.c_str(), e.getError().c_str());
    }
}


/* ------------------------- ~Rule ------------------------- */

Rule::~Rule()
{
    log->dlog(ch, "Rule destructor");

}

/* functions for accessing the templates */

string Rule::getMiscVal(string name)
{
    miscListIter_t iter;

    iter = miscList.find(name);
    if (iter != miscList.end()) {
        return iter->second.value;
    } else {
        return "";
    }
}


void Rule::parseRuleName(string rname)
{
    int n;

    if (rname.empty()) {
        throw Error("malformed rule identifier %s, "
                    "use <identifier> or <source>.<identifier> ",
                    rname.c_str());
    }

    if ((n = rname.find(".")) > 0) {
        source = rname.substr(0,n);
        id = rname.substr(n+1, rname.length()-n);
    } else {
        // no dot so everything is recognized as id
        id = rname;
    }

}


/* ------------------------- parseTime ------------------------- */

time_t Rule::parseTime(string timestr)
{
    struct tm  t;
  
    if (timestr[0] == '+') {
        // relative time in secs to start
        try {
	    struct tm tm;
            int secs = ParserFcts::parseInt(timestr.substr(1,timestr.length()));
            time_t start = time(NULL) + secs;
            return mktime(localtime_r(&start,&tm));
        } catch (Error &e) {
            throw Error("Incorrect relative time value '%s'", timestr.c_str());
        }
    } else {
        // absolute time
        if (timestr.empty() || (strptime(timestr.c_str(), TIME_FORMAT.c_str(), &t) == NULL)) {
            return 0;
        }
    }
    return mktime(&t);
}


/* ------------------------- getActions ------------------------- */

actionList_t *Rule::getActions()
{
    return &actionList;
}


/* ------------------------- getFilter ------------------------- */

filterList_t *Rule::getFilter()
{
    log->log(ch, "getFilterList");
    return &filterList;
}

/* ------------------------- getMisc ------------------------- */

miscList_t *Rule::getMisc()
{
    return &miscList;
}


/* ------------------------- dump ------------------------- */

void Rule::dump( ostream &os )
{
    os << "Rule dump :" << endl;
    os << getInfo() << endl;
  
}


/* ------------------------- getInfo ------------------------- */

string Rule::getInfo(void)
{
    ostringstream s;

    s << getSetName() << "." << getRuleName() << " ";

    switch (getState()) {
    case RS_NEW:
        s << "new";
        break;
    case RS_VALID:
        s << "validated";
        break;
    case RS_SCHEDULED:
        s << "scheduled";
        break;
    case RS_ACTIVE:
        s << "active";
        break;
    case RS_DONE:
        s << "done";
        break;
    case RS_ERROR:
        s << "error";
        break;
    default:
        s << "unknown";
    }

    s << ": ";

    filterListIter_t i = filterList.begin();
    while (i != filterList.end()) {
        s << i->name << "&" << i->mask.getString() << " = ";
        switch (i->mtype) {
        case FT_WILD:
            s << "*";
            break;
        case FT_EXACT:
            s << i->value[0].getString();
            break;
        case FT_RANGE:
           s << i->value[0].getString();
           s << "-";
           s << i->value[1].getString();
           break;
        case FT_SET:
            for (int j=0; j < i->cnt; j++) {
                s << i->value[j].getString();
                if (j < (i->cnt-1)) {
                    s << ", ";
                }
            }
            break;
        }

        i++;
        
        if (i != filterList.end()) {
            s << ", ";
        }
    }

    s << " | ";

    actionListIter_t ai = actionList.begin();
    while (ai != actionList.end()) {
        s << ai->name;

        ai++;

        if (ai != actionList.end()) {
            s << ", ";
        }
    }

    s << " | ";

    miscListIter_t mi = miscList.begin();
    while (mi != miscList.end()) {
        s << mi->second.name << " = " << mi->second.value;

        mi++;

        if (mi != miscList.end()) {
            s << ", ";
        }
    }

    s << endl;

    return s.str();
}


/* ------------------------- operator<< ------------------------- */

ostream& operator<< ( ostream &os, Rule &ri )
{
    ri.dump(os);
    return os;
}





