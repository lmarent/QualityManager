
/*!\file   QOSProcessor.cpp

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
    manages and applies packet processing modules

    $Id: PacketProcessor.cpp 748 2009-09-10 02:54:03Z szander $
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "ProcError.h"
#include "QOSProcessor.h"
#include "Module.h"
#include "ParserFcts.h"
#include "QualityManager.h"


ppaction_t& ppaction_t::operator=( ppaction_t const& rhs)
{

	cout << "ppaction Operator=" << endl;

	module = rhs.module;
	mapi = rhs.mapi;
	flowData = rhs.flowData;
	params = rhs.params;

	return *this;
}


ruleActions_t& ruleActions_t::operator=( ruleActions_t const& rhs)
{

	cout << "ruleActions Operator=" << endl;

	lastPkt = rhs.lastPkt;
	packets = rhs.packets;
	auto_flows = rhs.auto_flows;
	bidir = rhs.bidir;
	seppaths = rhs.seppaths;
	rule = rhs.rule;
	actions = rhs.actions;
	return *this;
}


/* ------------------------- QoSProcessor ------------------------- */

QOSProcessor::QOSProcessor(ConfigManager *cnf, int threaded, string moduleDir )
    : QualityManagerComponent(cnf, "QOS_PROCESSOR", threaded),
      numRules(0)
{
    string txt;

#ifdef DEBUG
    log->dlog(ch,"Starting");
#endif

    if (moduleDir.empty()) {
		txt = cnf->getValue("ModuleDir", "QOS_PROCESSOR");
        if ( txt != "") {
            moduleDir = txt;
        }
    }

    try {
        loader = new ModuleLoader(cnf, moduleDir.c_str() /*module (lib) basedir*/,
                                  cnf->getValue("Modules", "QOS_PROCESSOR"),/*modlist*/
                                  "Proc" /*channel name prefix*/,
                                  getConfigGroup() /* Configuration group */);

#ifdef DEBUG
    log->dlog(ch,"End starting");
#endif

    } catch (Error &e) {
        throw e;
    }
}


/* ------------------------- ~QoSProcessor ------------------------- */

QOSProcessor::~QOSProcessor()
{

    log->dlog(ch,"Shutdown");

    // destroy Flow setups for all rules

    log->dlog(ch, "Active rules:%d", (int) rules.size());

    ruleActionListIter_t it = rules.begin();
    while ( it != rules.end())
    {
        Rule *r = NULL;
        r = (it->second).rule;

        if (r == NULL){
			log->dlog(ch, "ruleId:%d with Null rule", it->first ) ;
            it++;
        }
		else{
            // To avoid making the iterator invalid, we increase it before deleting.
            ++it;
			delRule(r);
        }
    }

    // discard the Module Loader
    saveDelete(loader);

    log->dlog(ch,"End Shutdown");

}


/* ------------------------- addEvent ------------------------- */

void QOSProcessor::addEvent(Event *ev)
{

#ifdef DEBUG
    log->dlog(ch,"new event %s", eventNames[ev->getType()].c_str());
#endif

    AUTOLOCK(threaded, &maccess);
    in_events.push_back(ev);
    log->log(ch, "Adding event NumEventsIn:%d", (int) in_events.size() );
}


/*! deleting is costly but it should occur much less than exporting
    data or flow timeouts assuming the lifetimes of rules are reasonably large
*/
void QOSProcessor::delRuleEvents(int uid)
{
    int ret = 0;
    eventVecIter_t iter, tmp;

	AUTOLOCK(threaded, &maccess);

    // search linearly through list for rule with given ID and delete entries
    iter = in_events.begin();
    while (iter != in_events.end()) {
        tmp = iter;
        iter++;

        ret = (*tmp)->deleteRule(uid);
        if (ret == 1) {
            // ret = 1 means rule was present in event but other rules are still in
            // the event
#ifdef DEBUG
            log->dlog(ch,"remove rule %d from event %s", uid,
                      eventNames[(*tmp)->getType()].c_str());
#endif
        } else if (ret == 2) {
            // ret=2 means the event is now empty and therefore can be deleted
#ifdef DEBUG
            log->dlog(ch,"remove event %s", eventNames[(*tmp)->getType()].c_str());
#endif

            saveDelete(*tmp);
            in_events.erase(tmp);
        }
    }
}


Event *QOSProcessor::getNextEvent()
{

    Event *ev;


#ifdef ENABLE_THREADS
    mutexLock(&maccess);
#endif

    if (in_events.begin() != in_events.end()) {
        ev = in_events.front();
        // dequeue event
        in_events.erase(in_events.begin());
        // the receiver is responsible for
        // returning or freeing the event
#ifdef ENABLE_THREADS
        mutexUnlock(&maccess);
#endif
        return ev;
    }
    else
    {
#ifdef ENABLE_THREADS
        mutexUnlock(&maccess);
#endif
        return NULL;
    }
}


// check a ruleset (the filter part) - single Thread.
void QOSProcessor::checkRules(ruleDB_t *_rules, EventScheduler *e)
{
    ruleDBIter_t iter;

    for (iter = _rules->begin(); iter != _rules->end(); iter++) {
		Rule *rule = *iter;
		if ((rule->getState() == RS_NEW ) ||
			  (rule->getState() == RS_SCHEDULED ) ||
			    (rule->getState() == RS_ERROR )){
			checkRule(rule);
        }
    }
}


// check a ruleset (the filter part) - By the QoS processor interface
void QOSProcessor::checkRules(ruleDB_t *_rules, Event *evt)
{

    ruleDBIter_t iter;
    ruleDB_t response;

    log->log(ch, "starting checking rules");

    for (iter = _rules->begin(); iter != _rules->end(); iter++) {
		Rule *rule = *iter;
		if ((rule->getState() == RS_NEW ) ||
			  (rule->getState() == RS_SCHEDULED ) ||
			    (rule->getState() == RS_ERROR )){
			checkRule(rule);
        }

        response.push_back(rule);
    }

    AUTOLOCK(threaded, &maccess);
    Event * newEvt = new respCheckRulesQoSProcessorEvent(response);
    newEvt->setParent( evt->getParent());
    out_events.push_back( newEvt );
    const char c = 'P';
    write(QualityManager::s_sigpipe[1], &c, 1);

    log->log(ch, "ending checking rules");
}


// add rules single thread
void QOSProcessor::addRules( ruleDB_t *_rules, EventScheduler *e )
{

    log->dlog(ch, "starting add rules");

    ruleDBIter_t iter;
    for (iter = _rules->begin(); iter != _rules->end(); iter++)
    {
		Rule *rule = *iter;

        if ((rule->getState() == RS_VALID) ||  (rule->getState() == RS_SCHEDULED))
        {
			log->dlog(ch, "it is going to add  Rule %s.%s - Status:%d", rule->getSetName().c_str(), rule->getRuleName().c_str(), (int) rule->getState());
			addRule(rule, e);
		}
		else
		{
			log->dlog(ch, "Rule is not going to be added %s.%s because in Status:%d", rule->getSetName().c_str(), rule->getRuleName().c_str(), (int) rule->getState());
		}

    }

    log->dlog(ch, "ending add rules");
}


// add rules by the Qos Processor Event Interface.
void QOSProcessor::addRules( ruleDB_t *rules, Event *evt )
{

    log->log(ch, "starting add rules");

    ruleDBIter_t iter;
    ruleDB_t response;

    for (iter = rules->begin(); iter != rules->end(); iter++) {
		Rule *rule = *iter;

        if ((rule->getState() == RS_VALID) ||
			 (rule->getState() == RS_SCHEDULED)){
			addRule(rule);
		}

        response.push_back(rule);
    }

    AUTOLOCK(threaded, &maccess);
    Event *newEvt = new respAddRulesQoSProcesorEvent(response);
    newEvt->setParent(evt->getParent());
    out_events.push_back( newEvt );
    const char c = 'P';
    write(QualityManager::s_sigpipe[1], &c, 1);

    log->log(ch, "ending add rules NumEvents:%d", (int) out_events.size() );
}



// delete rules single thread.
void QOSProcessor::delRules(ruleDB_t *_rules, EventScheduler *e)
{
    ruleDBIter_t iter;

    for (iter = _rules->begin(); iter != _rules->end(); iter++) {
        delRule(*iter);
    }
}

// delete rules by the Qos Processor Event Interface.
void QOSProcessor::delRules( ruleDB_t *_rules, Event *evt )
{
    ruleDBIter_t iter;
    ruleDB_t response;

    for (iter = _rules->begin(); iter != _rules->end(); iter++) {
        delRule(*iter);
        response.push_back(*iter);
    }

    AUTOLOCK(threaded, &maccess);
    Event *newEvt = new respDelRulesQoSProcesorEvent(response);
    newEvt->setParent(evt->getParent());
    out_events.push_back( newEvt );

    const char c = 'P';
    write(QualityManager::s_sigpipe[1], &c, 1);

    log->log(ch, "ending add rules NumEvents:%d", (int) out_events.size() );

}


int QOSProcessor::checkRule(Rule *r)
{
    int ruleId;
    actionList_t *actions;
    ppaction_t a;
    int errNo;
    string errStr;
    bool exThrown = false;
    int cnt = 1;

	AUTOLOCK(threaded, &maccess);

    ruleId  = r->getUId();
    actions = r->getActions();

    log->log(ch, "checking Rule %s.%s - Id:%d", r->getSetName().c_str(), r->getRuleName().c_str(), ruleId);

    try {


        for (actionListIter_t iter = actions->begin(); iter != actions->end(); iter++) {
            Module *mod;
            string mname = iter->name;

            a.module = NULL;
            a.params = NULL;
            a.flowData = NULL;

            // load Action Module used by this rule
            mod = loader->getModule(mname.c_str());
            a.module = dynamic_cast<ProcModule*> (mod);

            if (a.module != NULL) { // is it a processing kind of module

                a.mapi = a.module->getAPI();

                // init module
                configItemList_t itmConf = iter->conf;
				char buffer1 [50];
                configItem_t flowId;
                flowId.group = getConfigGroup();
                flowId.module = mname;
                flowId.name = "FlowId";

			    // The flowid is made of the rule id and the action id.
			    uint16_t first_number = ruleId;
				uint16_t second_number = cnt;
				uint32_t uint32flowid = (uint32_t) first_number << 16;
				uint32flowid |= second_number;
				sprintf (buffer1, "%lu", (unsigned long) uint32flowid);
				flowId.value = string(buffer1);
                flowId.type = "UInt32";

                itmConf.push_front(flowId);
                a.params = ConfigManager::getParamList(itmConf);

				errNo = (a.mapi)->checkBandWidth(a.params);

				log->log(ch, "bandwidth checking %s.%s - return:%d", r->getSetName().c_str(), r->getRuleName().c_str(), errNo);

				if ( errNo < 0 ){
					 errStr = "Not available bandwidth";
					 exThrown = true;
					 break;
				}

                (a.mapi)->initFlowSetup(ruleId, cnt, a.params, r->getFilter(), &a.flowData);

                (a.mapi)->destroyFlowSetup(ruleId, cnt,  a.params, r->getFilter(), a.flowData);
                saveDeleteArr(a.params);
                a.params = NULL;


                a.flowData = NULL;

                //release packet processing modules already loaded for this rule
                loader->releaseModule(a.module);
                a.module = NULL;
                cnt = cnt+ 1;
            }
        }

    	log->log(ch, "checking Rule %s.%s - Id:%d - Pass the check", r->getSetName().c_str(), r->getRuleName().c_str(), ruleId);
    	r->setState(RS_VALID);
    }
    catch (Error &e) {
        log->elog(ch, e);
        errNo = e.getErrorNo();
        errStr = e.getError();
        log->elog(ch, e);
		exThrown = true;
    }
	catch (ProcError &proce){
        errNo = proce.getErrorNo();
        errStr = proce.getError();
        log->elog(ch, proce);
		exThrown = true;
	}

	if (exThrown)
	{

		log->elog(ch, "Rule %s.%s - Id:%d has errors", r->getSetName().c_str(), r->getRuleName().c_str(), ruleId);


        // free memory
        if (a.flowData != NULL) {

            (a.mapi)->destroyFlowSetup(ruleId, cnt, a.params, r->getFilter(), a.flowData);

			if (a.params != NULL) {
				saveDeleteArr(a.params);
			}

        }

        //release packet processing modules already loaded for this rule
        if (a.module) {
            loader->releaseModule(a.module);
        }

        // Something on the rule is not correct, so it goes to not valid.
        r->setState(RS_ERROR);

	}

    return 0;
}


/* ------------------------- addRule ------------------------- */

int QOSProcessor::addRule( Rule *r, EventScheduler *e )
{
    int ruleId;
    ruleActions_t entry;
    actionList_t *actions;
    int errNo;
    string errStr;
    bool exThrown = false;

    ruleId  = r->getUId();
    actions = r->getActions();

    log->log(ch, "adding Rule %d", ruleId);


    entry.lastPkt = 0;
    entry.packets = 0;
    entry.bytes = 0;
    entry.bidir = r->isBidir();
    entry.seppaths = r->sepPaths();
    entry.rule = r;

    try {

        int cnt = 1;
        for (actionListIter_t iter = actions->begin(); iter != actions->end(); iter++)
        {
            ppaction_t a;

            a.module = NULL;
            a.params = NULL;
            a.flowData = NULL;

            Module *mod;
            string mname = iter->name;

			log->log(ch, "it is going to load module %s", mname.c_str());


            // load Action Module used by this rule
            mod = loader->getModule(mname.c_str());
            a.module = dynamic_cast<ProcModule*> (mod);

            if (a.module != NULL) { // is it a processing kind of module

				log->log(ch, "module %s loaded", mname.c_str());

                a.mapi = a.module->getAPI();

                // init module
                configItemList_t itmConf = iter->conf;

                // Define the Flow id to be used.
                configItem_t flowId;
				char buffer1 [50];
                flowId.group = getConfigGroup();
                flowId.module = mname;
                flowId.name = "FlowId";

			    // The flowid is made of the rule id and the action id.
			    uint16_t first_number = ruleId;
				uint16_t second_number = cnt;
				uint32_t uint32flowid = (uint32_t) first_number << 16;
				uint32flowid |= second_number;
				sprintf (buffer1, "%lu", (unsigned long) uint32flowid);
				flowId.value = string(buffer1);
                flowId.type = "UInt32";

                itmConf.push_front(flowId);
                a.params = ConfigManager::getParamList(itmConf);

                a.flowData = NULL;
                (a.mapi)->initFlowSetup(ruleId, cnt, a.params, r->getFilter(), &a.flowData);
				if (a.params != NULL) {
					saveDeleteArr(a.params);
					a.params = ConfigManager::getParamList(itmConf);
				}

                // init timers
                addTimerEvents(ruleId, cnt, a, *e);

                (entry.actions)[cnt] = a;
	            cnt++;

            }

        }

        // success ->enter struct into internal table
        rules[ruleId] = entry;

        // Set the rule as active.
        r->setState(RS_ACTIVE);


		filterListIter_t iter;
		for ( iter = r->getFilter()->begin() ; iter != r->getFilter()->end() ; iter++ )
		{
			log->log(ch, "filter name%s - type: %s \n",  (iter->name).c_str(), (iter->type).c_str() );

		}

    }
    catch (Error &e)
    {
        log->elog(ch, e);
        errNo = e.getErrorNo();
        errStr = e.getError();
		exThrown = true;
    }

	catch (ProcError &e)
	{
        log->elog(ch, e);
        errNo = e.getErrorNo();
        errStr = e.getError();
		exThrown = true;
	}

	if (exThrown)
	{
        for (ppactionListIter_t i = entry.actions.begin(); i != entry.actions.end(); i++) {
			ppaction_t a = i->second;
			int action_id = i->first;

            (a.mapi)->destroyFlowSetup( ruleId, action_id, a.params, r->getFilter(), a.flowData );

			if (a.params != NULL) {
				saveDeleteArr(a.params);
			}

            //release packet processing modules already loaded for this rule
            if (a.module) {
                loader->releaseModule(a.module);
            }
        }
        // empty the map itself
        entry.actions.clear();

        throw Error(errNo, errStr);
	}

    return 0;
}



/* ------------------------- addRule ------------------------- */

int QOSProcessor::addRule( Rule *r )
{
    int ruleId;
    ruleActions_t entry;
    actionList_t *actions;
    int errNo;
    string errStr;
    bool exThrown = false;

    AUTOLOCK(threaded, &maccess);

    ruleId  = r->getUId();
    actions = r->getActions();

    log->dlog(ch, "adding Rule #%d", ruleId);


    entry.lastPkt = 0;
    entry.packets = 0;
    entry.bytes = 0;
    entry.bidir = r->isBidir();
    entry.seppaths = r->sepPaths();
    entry.rule = r;

    try {

        int cnt = 1;
        for (actionListIter_t iter = actions->begin(); iter != actions->end(); iter++) {
            ppaction_t a;

            a.module = NULL;
            a.params = NULL;
            a.flowData = NULL;

            Module *mod;
            string mname = iter->name;

			log->dlog(ch, "it is going to load module %s", mname.c_str());


            // load Action Module used by this rule
            mod = loader->getModule(mname.c_str());
            a.module = dynamic_cast<ProcModule*> (mod);

            if (a.module != NULL) { // is it a processing kind of module

				log->dlog(ch, "module %s loaded", mname.c_str());
                a.mapi = a.module->getAPI();

                // init module
                configItemList_t itmConf = iter->conf;

                // Define the Flow id to be used.
                configItem_t flowId;
				char buffer1 [50];
                flowId.group = getConfigGroup();
                flowId.module = mname;
                flowId.name = "FlowId";

			    // The flowid is made of the rule id and the action id.
			    uint16_t first_number = ruleId;
				uint16_t second_number = cnt;
				uint32_t uint32flowid = (uint32_t) first_number << 16;
				uint32flowid |= second_number;
				sprintf (buffer1, "%lu", (unsigned long) uint32flowid);
				flowId.value = string(buffer1);
                flowId.type = "UInt32";

                itmConf.push_front(flowId);
                a.params = ConfigManager::getParamList(itmConf);

                a.flowData = NULL;
                (a.mapi)->initFlowSetup(ruleId, cnt, a.params, r->getFilter(), &a.flowData);

                // Set the whole parameters, which includes the filter, for the action.
				if (a.params != NULL) {
					saveDeleteArr(a.params);
					a.params = ConfigManager::getParamList(itmConf);
				}

                // init timers
                addTimerEvents(ruleId, cnt, a);

                (entry.actions)[cnt] = a;
	            cnt++;

            }

        }

        // success ->enter struct into internal table
        rules[ruleId] = entry;

        // Set the rule as active.
        r->setState(RS_ACTIVE);

    }
    catch (Error &e)
    {
        log->elog(ch, e);
        errNo = e.getErrorNo();
        errStr = e.getError();
		exThrown = true;
    }

	catch (ProcError &e)
	{
        log->elog(ch, e);
        errNo = e.getErrorNo();
        errStr = e.getError();
		exThrown = true;
	}

	if (exThrown)
	{
        for (ppactionListIter_t i = entry.actions.begin(); i != entry.actions.end(); i++)
        {
			ppaction_t a = i->second;
			int action_id = i->first;

            (a.mapi)->destroyFlowSetup( ruleId, action_id, a.params, r->getFilter(), a.flowData );

			if (a.params != NULL) {
				saveDeleteArr(a.params);
			}

            //release packet processing modules already loaded for this rule
            if (a.module) {
                loader->releaseModule(a.module);
            }
        }
        // empty the list itself
        entry.actions.clear();

        throw Error(errNo, errStr);;
	}
    return 0;
}



/* ------------------------- delRule ------------------------- */

int QOSProcessor::delRule( Rule *r )
{
    int ruleId = r->getUId();
    ruleActions_t *ra =NULL;

    log->log(ch, "deleting Rule: %d", ruleId);

    AUTOLOCK(threaded, &maccess);

    ra = &rules[ruleId];

	log->log(ch, "Num filters for rule: %d - %d", ruleId, (int) r->getFilter()->size());

	filterListIter_t iter;
	for ( iter = r->getFilter()->begin() ; iter != r->getFilter()->end() ; iter++ )
	{
        log->log(ch, "filter name%s - type: %s \n",  (iter->name).c_str(), (iter->type).c_str() );

	}

    // now free flow data and release used Modules
    for (ppactionListIter_t i = ra->actions.begin(); i != ra->actions.end(); i++)
    {
		int action_id = i->first;
		ppaction_t a = i->second;

		try
		{
			// dismantle flow data structure with module function
			a.mapi->destroyFlowSetup( ruleId, action_id, a.params, r->getFilter(), a.flowData );

			log->log(ch, "Sucessfully destroy the flow setup");

			if (a.params != NULL) {
				saveDeleteArr(a.params);
				a.params = NULL;
			}
		} catch (ProcError &err){
			log->elog(ch, err);
		}


        // release modules loaded for this rule
        loader->releaseModule(a.module);


        log->log(ch, "After sucessfully release the module");

        // FIXME disable timers
    }

    ra->actions.clear();
    ra->lastPkt = 0;
    ra->auto_flows = 0;
    ra->bidir = 0;
    ra->seppaths = 0;
    ra->rule = NULL;

    // Remove the rule from the container
    rules.erase(ruleId);

	// Set the rule as done.
    r->setState(RS_DONE);

    return 0;
}


int QOSProcessor::handleFDEvent(eventVec_t *e, fd_set *rset, fd_set *wset, fd_sets_t *fds)
{

    Event *evn;

    // get next entry from event queue
    while ((evn = getNextEvent()) != NULL)
    {
        handleEvent(evn);
        saveDelete(evn);

	}

	// This part returns the events created during execution.
	if (e != NULL){
        AUTOLOCK(threaded, &maccess);
		eventVecIter_t it;
		for (it = out_events.begin(); it != out_events.end(); it++){
			evn = *it;
			e->push_back(evn);
		}

		out_events.clear();
	}

#ifdef ENABLE_THREADS
	if (threaded && ((out_events.size() == 0) && (in_events.size() == 0))) {
	  threadCondSignal(&doneCond);
	}
#endif

    return 0;
}

void QOSProcessor::main()
{

    // this function will be run as a single thread inside the QOS processor
    log->log(ch, "QoS Processor thread running");

    // Sleeps 10 milliseconds.
    unsigned int microseconds = 10000;
    for (;;) {
        handleFDEvent(NULL, NULL,NULL, NULL);
        usleep(microseconds);
    }
}

void QOSProcessor::waitUntilDone(void)
{
#ifdef ENABLE_THREADS
    AUTOLOCK(threaded, &maccess);

    if (threaded) {
      while ((out_events.size() > 0) || (in_events.size() > 0) ) {
        threadCondWait(&doneCond, &maccess);
      }
    }
#endif
}


/* ------------------------- ruleTimeout ------------------------- */

// return 0 (if timeout), 1 (stays idle), >1 (active and no timeout yet)
unsigned long QOSProcessor::ruleTimeout(int ruleID, unsigned long ival, time_t now)
{
    AUTOLOCK(threaded, &maccess);

    time_t last = rules[ruleID].lastPkt;

    if (last > 0) {
        ruleActions_t *ra = &rules[ruleID];
		 log->dlog(ch,"auto flow idle, export: YES");
         return 0;
    }

    return 1;
}

string QOSProcessor::getInfo()
{
    ostringstream s;

    AUTOLOCK(threaded, &maccess);

    /* FIXME delete?
     *
     * uncomment to print out which task uses what modules

    for (unsigned int j = 0; j<rules.size(); j++) {

	if (rules[ j ].actions.size()>0) {

    s << "rule with id #" << j << " uses "
    << rules[ j ].actions.size() << " actions :";

    s << " ";
    for (ppactionListIter_t i = rules[j].actions.begin(); i != rules[j].actions.end(); i++) {
    s << (i->module)->getModName();
    s << ",";
    }
    s << endl;
	}
    }
    */

    s << loader->getInfo();  // get the list of loaded modules

    return s.str();
}

/* -------------------- addTimerEvents -------------------- */

void QOSProcessor::addTimerEvents( int ruleID, int actID, ppaction_t &act, EventScheduler &es )
{
    timers_t *timers = (act.mapi)->getTimers(act.flowData);

    if (timers != NULL) {
        while (timers->flags != TM_END)
        {
			log->log(ch, "ival:%d", (int) timers->ival_msec);
			unsigned int duration = (timers->ival_msec / 1000);
			unsigned int duration_msec = (timers->ival_msec % 1000);
			Event *evt = new ProcTimerEvent(ruleID, actID, timers->id, (time_t) duration, (time_t) duration_msec, timers->flags);
			struct timeval tv = evt->getTime();
			log->log(ch, "Creating a new timer event %s", ctime((const time_t *) &tv.tv_sec) );
            es.addEvent(evt);
            timers++;
        }
    }
}


void QOSProcessor::addTimerEvents( int ruleID, int actID, ppaction_t &act )
{
    timers_t *timers = (act.mapi)->getTimers(act.flowData);

    if (timers != NULL) {
        while (timers->flags != TM_END)
        {
			unsigned int duration = (timers->ival_msec / 1000);
			unsigned int duration_msec = (timers->ival_msec % 1000);
			Event *evt = new ProcTimerEvent(ruleID, actID, timers->id, (time_t) duration, (time_t) duration_msec, timers->flags);
			struct timeval tv = evt->getTime();
			log->log(ch, "Creating a new timer event %s", ctime((const time_t *) &tv.tv_sec) );
            out_events.push_back(evt);
            timers++;
        }
    }
}



// handle module timeouts
void QOSProcessor::timeout(int rid, int actid, unsigned int tmID)
{
    ppaction_t *a;
    ruleActions_t *ra;

    log->log(ch, "starting timeout");


    AUTOLOCK(threaded, &maccess);

    ra = &rules[rid];
	Rule *r = ra->rule;
    a = &(ra->actions[actid]);

	if ((r != NULL) and (a != NULL))
	{

		try
		{
			// dismantle flow data structure with module function
			a->mapi->destroyFlowSetup(rid, actid, a->params, r->getFilter(), a->flowData );

			log->log(ch, "Sucessfully destroy the flow setup");

			if (a->params != NULL) {
				saveDeleteArr(a->params);
				a->params = NULL;
			}
		} catch (ProcError &err){
			log->elog(ch, err);
		}

		// release modules loaded for this rule
		loader->releaseModule(a->module);

		log->log(ch, "After sucessfully release the module");

		(ra->actions).erase(actid);
	}

	log->log(ch, "Ending timeout");

}

/* -------------------- getModuleInfoXML -------------------- */

string QOSProcessor::getModuleInfoXML( string modname )
{
    AUTOLOCK(threaded, &maccess);
    return loader->getModuleInfoXML( modname );
}


void QOSProcessor::handleEvent(Event *e)
{

    switch (e->getType()) {
    case TEST:
      {

          log->dlog(ch,"processing event test" );

      }
      break;
    case ADD_RULES_QOS_PROCESSOR:
      {

        log->log(ch,"Add rules Qos Processor" );

        ruleDB_t *rules = ((addRulesQoSProcesorEvent *)e)->getRules();

        // now check rules.
        addRules(rules, e);

      }
      break;
	case CHECK_RULES_QOS_PROCESSOR:
	  {

        log->log(ch,"Check rules Qos Processor" );

        ruleDB_t *rules = ((checkRulesQoSProcessorEvent *)e)->getRules();

        // now check rules.
        checkRules(rules, e);


	  }
	  break;
    case DEL_RULES_QOS_PROCESSOR:
      {

          log->log(ch,"Delete rules QoS Processor" );

          ruleDB_t *rules = ((delRulesQoSProcesorEvent *)e)->getRules();

          // now get rid of the expired rule
          delRules(rules, e);

      }
      break;

    default:
        throw Error("unknown event");
    }
}


/* ------------------------- dump ------------------------- */

void QOSProcessor::dump( ostream &os )
{

    os << "QoS Processor dump :" << endl;
    os << getInfo() << endl;

}


/* ------------------------- operator<< ------------------------- */

ostream& operator<< ( ostream &os, QOSProcessor &pe )
{
    pe.dump(os);
    return os;
}
