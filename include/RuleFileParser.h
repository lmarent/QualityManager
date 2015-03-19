
/*!  \file   RuleFileParser.h

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
    parse rule files
    Code based on Netmate Implementation

    $Id: RuleFileParser.h 748 2015-03-03 18:05:00 amarentes $
*/

#ifndef _RULEFILEPARSER_H_
#define _RULEFILEPARSER_H_


#include "stdincpp.h"
#include "libxml/parser.h"
#include "Logger.h"
#include "XMLParser.h"
#include "Rule.h"
#include "ConfigParser.h"
#include "FilterDefParser.h"
#include "FilterValParser.h"
#include "RuleIdSource.h"


//! rule list
typedef vector<Rule*>            ruleDB_t;
typedef vector<Rule*>::iterator  ruleDBIter_t;

class RuleFileParser : public XMLParser
{
  private:

    Logger *log;
    int ch;

    //! parse a config item
    configItem_t parsePref(xmlNodePtr cur);

    //! lookup filter value
    string lookup(filterValList_t *filterVals, string fvalue, filter_t *f);
    //! parse a filter value
    void parseFilterValue(filterValList_t *filterVals, string value, filter_t *f);

  public:

    RuleFileParser( string fname );

    RuleFileParser( char *buf, int len );

    virtual ~RuleFileParser() {}

    //! parse rules and add parsed rules to the list of rules
    virtual void parse(filterDefList_t *filters, 
					   filterValList_t *filterVals, 
					   ruleDB_t *rules,
					   RuleIdSource *idSource );
};

#endif // _RULEFILEPARSER_H_
