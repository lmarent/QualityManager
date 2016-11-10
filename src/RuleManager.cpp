
/*! \file   RuleManager.cpp

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
	rule database
    Code based on Netmate Implementation

    $Id: RuleManager.h 748 2015-03-03 13:54:03 amarentes $

*/

#include "RuleManager.h"
#include "CtrlComm.h"
#include "constants.h"

/* ------------------------- RuleManager ------------------------- */

RuleManager::RuleManager( string fdname, string fvname) 
    : tasks(0), filterDefFileName(fdname), filterValFileName(fvname), 
	  idSource(1)
{
    log = Logger::getInstance();
    ch = log->createChannel("RuleManager");
#ifdef DEBUG
    log->dlog(ch,"Starting");
#endif

}


/* ------------------------- ~RuleManager ------------------------- */

RuleManager::~RuleManager()
{
    ruleDBIter_t iter;

#ifdef DEBUG
    log->dlog(ch,"Shutdown");
#endif

    for (iter = ruleDB.begin(); iter != ruleDB.end(); iter++) {
        if (*iter != NULL) {
            // delete rule
            saveDelete(*iter);
        } 
    }

    for (ruleDoneIter_t i = ruleDone.begin(); i != ruleDone.end(); i++) {
        saveDelete(*i);
    }
}


/* -------------------- isReadableFile -------------------- */

static int isReadableFile( string fileName ) {

    FILE *fp = fopen(fileName.c_str(), "r");

    if (fp != NULL) {
        fclose(fp);
        return 1;
    } else {
        return 0;
    }
}


/* -------------------- loadFilterDefs -------------------- */

void RuleManager::loadFilterDefs(string fname)
{
    if (filterDefFileName.empty()) {
        if (fname.empty()) {
            fname = FILTERDEF_FILE;
		}
    } else {
        fname = filterDefFileName;
    }

#ifdef DEBUG
    log->dlog(ch, "filename %s", fname.c_str());
#endif

    if (isReadableFile(fname)) {
        if (filterDefs.empty() && !fname.empty()) {
            FilterDefParser f = FilterDefParser(fname.c_str());
            f.parse(&filterDefs);
        }
    
    }else{
#ifdef DEBUG
    log->dlog(ch, "filename %s is not readable", fname.c_str());
#endif    
    }
    
}


/* -------------------- loadFilterVals -------------------- */

void RuleManager::loadFilterVals( string fname )
{
    if (filterValFileName.empty()) {
        if (fname.empty()) {
            fname = FILTERVAL_FILE;
        }
    } else {
        fname = filterValFileName;
    }

    if (isReadableFile(fname)) {
        if (filterVals.empty() && !fname.empty()) {
            FilterValParser f = FilterValParser(fname.c_str());
            f.parse(&filterVals);
        }
    }
}


/* -------------------------- getRule ----------------------------- */

Rule *RuleManager::getRule(int uid)
{
    if ((uid >= 0) && ((unsigned int)uid <= ruleDB.size())) {
        return ruleDB[uid];
    } else {
        return NULL;
    }
}


/* -------------------- getRule -------------------- */

Rule *RuleManager::getRule(string sname, string rname)
{
    ruleSetIndexIter_t iter;
    ruleIndexIter_t iter2;

    iter = ruleSetIndex.find(sname);
    if (iter != ruleSetIndex.end()) {		
        iter2 = iter->second.find(rname);
        if (iter2 != iter->second.end()) {
            return getRule(iter2->second);
        }
        else
        {
#ifdef DEBUG
    log->dlog(ch,"RuleId not found");
#endif		
			
		}
    }
    else
    {
#ifdef DEBUG
    log->dlog(ch,"Ruleset not found");
#endif		
	}

    return NULL;
}


/* -------------------- getRules -------------------- */

ruleIndex_t *RuleManager::getRules(string sname)
{
    ruleSetIndexIter_t iter;

    iter = ruleSetIndex.find(sname);
    if (iter != ruleSetIndex.end()) {
        return &(iter->second);
    }

    return NULL;
}

ruleDB_t RuleManager::getRules()
{
    ruleDB_t ret;

    for (ruleSetIndexIter_t r = ruleSetIndex.begin(); r != ruleSetIndex.end(); r++) {
        for (ruleIndexIter_t i = r->second.begin(); i != r->second.end(); i++) {
            ret.push_back(getRule(i->second));
        }
    }

    return ret;
}

/* ----------------------------- parseRules --------------------------- */

ruleDB_t *RuleManager::parseRules(string fname)
{

#ifdef DEBUG
    log->dlog(ch,"ParseRules");
#endif

    ruleDB_t *new_rules = new ruleDB_t();

    // load the filter def list
    loadFilterDefs(fname);
	
    // load the filter val list
    loadFilterVals(fname);
	
    RuleFileParser rfp = RuleFileParser(fname);
    
    
    try
    {

        rfp.parse(&filterDefs, &filterVals, new_rules, &idSource);

#ifdef DEBUG
    log->dlog(ch, "rules parsed");
#endif

        return new_rules;

    } catch (Error &e) {

        for(ruleDBIter_t i=new_rules->begin(); i != new_rules->end(); i++) {
           saveDelete(*i);
        }
        saveDelete(new_rules);
        throw e;
    }
}


/* -------------------- parseRulesBuffer -------------------- */

ruleDB_t *RuleManager::parseRulesBuffer(char *buf, int len, int mapi)
{
    ruleDB_t *new_rules = new ruleDB_t();

    try {
        // load the filter def list
        loadFilterDefs("");
	
        // load the filter val list
        loadFilterVals("");
	
        if (mapi) {
             MAPIRuleParser rfp = MAPIRuleParser(buf, len);
             rfp.parse(&filterDefs, &filterVals, new_rules, &idSource);
        } else {
            RuleFileParser rfp = RuleFileParser(buf, len);
            rfp.parse(&filterDefs, &filterVals, new_rules, &idSource);
        }

        return new_rules;
	
    } catch (Error &e) {

        for(ruleDBIter_t i=new_rules->begin(); i != new_rules->end(); i++) {
            saveDelete(*i);
        }
        saveDelete(new_rules);
        throw e;
    }
}


/* ---------------------------------- addRules ----------------------------- */

void RuleManager::addRules(ruleDB_t *rules, EventScheduler *e)
{
    ruleDBIter_t        iter;
    ruleTimeIndex_t     start;
    ruleTimeIndex_t     stop;
    ruleTimeIndexIter_t iter2;
    time_t              now = time(NULL);
    
    // add valid rules.
    for (iter = rules->begin(); iter != rules->end(); iter++) {
        Rule *r = (*iter);
        
        if (r->getState()  == RS_VALID) {
        
            try {
                addRule(r);

                start[r->getStart()].push_back(r);
                if (r->getStop()) {
                    stop[r->getStop()].push_back(r);
                }
            } catch (Error &e ) {
                saveDelete(r);
                // if only one rule, then return error
                if (rules->size() == 1) {
                    throw e;
                }
                // FIXME else return number of successively installed rules
            }
        }
    }
    
#ifdef DEBUG    
    log->dlog(ch, "Start all rules - it is going to activate them");
#endif      

    // group rules with same start time
    for (iter2 = start.begin(); iter2 != start.end(); iter2++) {
        e->addEvent(new ActivateRulesEvent(iter2->first-now, iter2->second));
    }
    
    // group rules with same stop time
    for (iter2 = stop.begin(); iter2 != stop.end(); iter2++) {
        e->addEvent(new RemoveRulesEvent(iter2->first-now, iter2->second));
    }

#ifdef DEBUG    
    log->dlog(ch, "Finished adding rules");
#endif      

}


/* -------------------- addRule -------------------- */

void RuleManager::addRule(Rule *r)
{
  
#ifdef DEBUG    
    log->dlog(ch, "adding new rule with name = '%s'",
              r->getRuleName().c_str());
#endif  

    // test for presence of ruleSource/ruleName combination
    // in ruleDatabase in particular set
    if (getRule(r->getSetName(), r->getRuleName())) {
        log->elog(ch, "task %s.%s already installed",
                  r->getSetName().c_str(), r->getRuleName().c_str());
                  
        r->setState(RS_ERROR);
        throw Error(408, "task with this name is already installed");
    }

    try {

#ifdef DEBUG    
    log->dlog(ch, "Rule Id = '%d'", r->getUId());
#endif 

        // resize vector if necessary
        if ((unsigned int)r->getUId() >= ruleDB.size()) {
            ruleDB.reserve(r->getUId() * 2 + 1);
            ruleDB.resize(r->getUId() + 1);
        }

        // insert rule
        ruleDB[r->getUId()] = r; 	

        // add new entry in index
        ruleSetIndex[r->getSetName()][r->getRuleName()] = r->getUId();
	
        tasks++;

#ifdef DEBUG    
    log->dlog(ch, "finish adding new rule with name = '%s'",
              r->getRuleName().c_str());
#endif  

    } catch (Error &e) { 

        // adding new task failed in some component
        // something failed -> remove rule from database
        delRule(r->getSetName(), r->getRuleName(), NULL);
	
        throw e;
    }
}

/* ------------------------- getInfo ------------------------- */

string RuleManager::getInfo(Rule *r)
{
    ostringstream s;

#ifdef DEBUG
    log->dlog(ch, "looking up rule with uid = %d", r->getUId());
#endif

    s << r->getInfo() << endl;
    
    return s.str();
}


/* ------------------------- getInfo ------------------------- */

string RuleManager::getInfo(string sname, string rname)
{
    ostringstream s;
    string info;
    Rule *r;
  
    r = getRule(sname, rname);

    if (r == NULL) {
        // check done tasks
        for (ruleDoneIter_t i = ruleDone.begin(); i != ruleDone.end(); i++) {
            if (((*i)->getRuleName() == rname) && ((*i)->getSetName() == sname)) {
                info = (*i)->getInfo();
            }
        }
        
        if (info.empty()) {
            throw Error("no rule with rule name '%s.%s'", sname.c_str(), rname.c_str());
        }
    } else {
        // rule with given identification is in database
        info = r->getInfo();
    }
    
    s << info;

    return s.str();
}


/* ------------------------- getInfo ------------------------- */

string RuleManager::getInfo(string sname)
{
    ostringstream s;
    ruleSetIndexIter_t r;

    r = ruleSetIndex.find(sname);

    if (r != ruleSetIndex.end()) {
        for (ruleIndexIter_t i = r->second.begin(); i != r->second.end(); i++) {
            s << getInfo(sname, i->first);
        }
    } else {
        s << "No such ruleset" << endl;
    }
    
    return s.str();
}


/* ------------------------- getInfo ------------------------- */

string RuleManager::getInfo()
{
    ostringstream s;
    ruleSetIndexIter_t iter;

    for (iter = ruleSetIndex.begin(); iter != ruleSetIndex.end(); iter++) {
        s << getInfo(iter->first);
    }
    
    return s.str();
}


/* ------------------------- delRule ------------------------- */

void RuleManager::delRule(string sname, string rname, EventScheduler *e)
{
    Rule *r;

#ifdef DEBUG    
    log->dlog(ch, "Deleting rule set= %s name = '%s'",
              sname.c_str(), rname.c_str());
#endif  


    if (sname.empty() && rname.empty()) {
        throw Error("incomplete rule set or name specified");
    }

    r = getRule(sname, rname);

    if (r != NULL) {
        delRule(r, e);
    } else {
        throw Error("rule %s.%s does not exist", sname.c_str(),rname.c_str());
    }
}


/* ------------------------- delRule ------------------------- */

void RuleManager::delRule(int uid, EventScheduler *e)
{
    Rule *r;

    r = getRule(uid);

    if (r != NULL) {
        delRule(r, e);
    } else {
        throw Error("rule uid %d does not exist", uid);
    }
}


/* ------------------------- delRules ------------------------- */

void RuleManager::delRules(string sname, EventScheduler *e)
{
    if (ruleSetIndex.find(sname) != ruleSetIndex.end()) {
        for (ruleSetIndexIter_t i = ruleSetIndex.begin(); i != ruleSetIndex.end(); i++) {
            delRule(getRule(sname, i->first),e);
        }
    }
}


/* ------------------------- delRule ------------------------- */

void RuleManager::delRule(Rule *r, EventScheduler *e)
{
#ifdef DEBUG    
    log->dlog(ch, "removing rule with name = '%s'", r->getRuleName().c_str());
#endif

    // remove rule from database and from index
    storeRuleAsDone(r);
    ruleDB[r->getUId()] = NULL;
    ruleSetIndex[r->getSetName()].erase(r->getRuleName());

    // delete rule set if empty
    if (ruleSetIndex[r->getSetName()].empty()) {
        ruleSetIndex.erase(r->getSetName());
    }
    
    if (e != NULL) {
        e->delRuleEvents(r->getUId());
    }

    tasks--;
}


/* ------------------------- delRules ------------------------- */

void RuleManager::delRules(ruleDB_t *rules, EventScheduler *e)
{
    ruleDBIter_t iter;

    for (iter = rules->begin(); iter != rules->end(); iter++) {
        delRule(*iter, e);
    }
}


/* -------------------- storeRuleAsDone -------------------- */

void RuleManager::storeRuleAsDone(Rule *r)
{
    
    r->setState(RS_DONE);
    ruleDone.push_back(r);

    if (ruleDone.size() > DONE_LIST_SIZE) {
        // release id
        idSource.freeId(ruleDone.front()->getUId());
        // remove rule
        saveDelete(ruleDone.front());
        ruleDone.pop_front();
    }
}


/* ------------------------- dump ------------------------- */

void RuleManager::dump( ostream &os )
{
    
    os << "RuleManager dump :" << endl;
    os << getInfo() << endl;
    
}


/* ------------------------- operator<< ------------------------- */

ostream& operator<< ( ostream &os, RuleManager &rm )
{
    rm.dump(os);
    return os;
}
