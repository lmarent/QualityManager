
/*  \file   MAPIRuleParser.h

    Copyright 2014-2015 Universidad de los Andes,
                        Bogota, Colombia

    This file is part of Network Quality Managing System (NETQoS).

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
    parser for API text rule syntax

    $Id: MAPIRuleParser.h 748 2015-04-06 08:21:00 amarentes $
*/

#ifndef _MAPI_RULE_PARSER_H_
#define _MAPI_RULE_PARSER_H_


#include "stdincpp.h"
#include "RuleFileParser.h"

//! parser for API text rule syntax

class MAPIRuleParser
{

  private:

    Logger *log;
    int ch;
    char *buf;
    int len;
    string fileName;

    //! FIXME document!
    void parseFilterValue(filterValList_t *filterVals, string value, filter_t *f);
    
    string lookup(filterValList_t *filterVals, string fvalue, filter_t *f);

  public:

    MAPIRuleParser(string fname);

    MAPIRuleParser(char *b, int l);

    virtual ~MAPIRuleParser() {}

    //! parse given rules and add parsed rules to rules
    virtual void parse(filterDefList_t *filters, 
					   filterValList_t *filterVals, 
					   ruleDB_t *rules,
					   RuleIdSource *idSource );

};


#endif // _MAPI_RULE_PARSER_H_
