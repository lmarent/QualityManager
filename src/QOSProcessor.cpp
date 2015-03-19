
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

#include "QOSProcessor.h"
#include "Module.h"
#include "ParserFcts.h"
#include <stdio.h>
#include <stdlib.h>



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
    } catch (Error &e) {
        throw e;
    }
}


/* ------------------------- ~QoSProcessor ------------------------- */

QOSProcessor::~QOSProcessor()
{

#ifdef DEBUG
    log->dlog(ch,"Shutdown");
#endif

#ifdef ENABLE_THREADS
    if (threaded) {
        mutexLock(&maccess);
        stop();
        mutexUnlock(&maccess);
        mutexDestroy(&maccess);
    }
#endif
	
	
	
    // destroy Flow setups for all rules
    for (ruleActionListIter_t r = rules.begin(); r != rules.end(); r++) 
    {
		ppactionListIter_t i;
        for ( i = r->actions.begin(); i != r->actions.end(); i++) 
        {

             // Define the Flow id to be used.
             char buffer [50];
             configItemList_t itmConf;
             configItem_t flowId;
             flowId.group = getConfigGroup();
             flowId.name = "FlowId";
             int ruleId = (r->rule)->getUId();
             sprintf (buffer, "%d", ruleId);
             flowId.value = string(buffer);
             flowId.type = "UInt16"; 
             itmConf.push_front(flowId);
             
  			 i->mapi->destroyFlowSetup(
					ConfigManager::getParamList(itmConf), i->flowData);
            saveDeleteArr(i->params);
        }
        
    }

    // discard the Module Loader
    saveDelete(loader);

}


// check a ruleset (the filter part)
void QOSProcessor::checkRules(ruleDB_t *rules)
{
    ruleDBIter_t iter;
    
    for (iter = rules->begin(); iter != rules->end(); iter++) {
        checkRule(*iter);
    }
}


// add rules
void QOSProcessor::addRules( ruleDB_t *rules, EventScheduler *e )
{
    ruleDBIter_t iter;
   
    for (iter = rules->begin(); iter != rules->end(); iter++) {
        addRule(*iter, e);
    }
}


// delete rules
void QOSProcessor::delRules(ruleDB_t *rules)
{
    ruleDBIter_t iter;

    for (iter = rules->begin(); iter != rules->end(); iter++) {
        delRule(*iter);
    }
}


int QOSProcessor::checkRule(Rule *r)
{
    int ruleId;
    actionList_t *actions;
    ppaction_t a;

    ruleId  = r->getUId();
    actions = r->getActions();

#ifdef DEBUG
    log->dlog(ch, "checking Rule %s.%s - Id:%d", r->getSetName().c_str(), r->getRuleName().c_str(), ruleId);
#endif  

    try {
        AUTOLOCK(threaded, &maccess);

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
				sprintf (buffer1, "%d", ruleId);
				flowId.value = string(buffer1);
                flowId.type = "UInt16"; 
                itmConf.push_front(flowId);
                a.params = ConfigManager::getParamList(itmConf);
                               
                int ret = (a.mapi)->initFlowSetup(a.params, r->getFilter(), &a.flowData);

                if (ret < 0) {
                    throw Error("Invalid parameters for module %s", mname.c_str());
                }

                saveDeleteArr(a.params);
                a.params = NULL;

                // free memory
                configItemList_t itmConf2;
				char buffer2 [50];
                configItem_t flowId2;
                flowId2.group = getConfigGroup();
                flowId2.module = mname;
                flowId2.name = "FlowId";
				sprintf (buffer2, "%d", ruleId);
				flowId2.value = string(buffer2);
                flowId2.type = "UInt16"; 
                itmConf2.push_front(flowId2);
                a.params = ConfigManager::getParamList(itmConf2);
                
                (a.mapi)->destroyFlowSetup(a.params, a.flowData);
                saveDeleteArr(a.params);
                a.params = NULL;


                a.flowData = NULL;

                //release packet processing modules already loaded for this rule
                loader->releaseModule(a.module);
                a.module = NULL;
            }
        }
    	
    } catch (Error &e) { 
        log->elog(ch, e);

        if (a.params != NULL) {
			saveDeleteArr(a.params);
        }

        // free memory
        if (a.flowData != NULL) {

            configItemList_t itmConf3;
			char buffer3 [50];
            configItem_t flowId3;
            flowId3.group = getConfigGroup();
            flowId3.name = "FlowId";
			sprintf (buffer3, "%d", ruleId);
			flowId3.value = string(buffer3);
            flowId3.type = "UInt16"; 
            itmConf3.push_front(flowId3);
            a.params = ConfigManager::getParamList(itmConf3);
                
            (a.mapi)->destroyFlowSetup(a.params, a.flowData);

			if (a.params != NULL) {
				saveDeleteArr(a.params);
			}
			
        }
            
        //release packet processing modules already loaded for this rule
        if (a.module) {
            loader->releaseModule(a.module);
        }
       
        throw e;
    }
    return 0;
}


/* ------------------------- addRule ------------------------- */

int QOSProcessor::addRule( Rule *r, EventScheduler *e )
{
    int ruleId;
    ruleActions_t entry;
    actionList_t *actions;

    ruleId  = r->getUId();
    actions = r->getActions();

#ifdef DEBUG
    log->dlog(ch, "adding Rule #%d", ruleId);
#endif  

    AUTOLOCK(threaded, &maccess);  

    entry.lastPkt = 0;
    entry.packets = 0;
    entry.bytes = 0;
    entry.flist = r->getFilter();
    entry.bidir = r->isBidir();
    entry.seppaths = r->sepPaths();
    entry.rule = r;

    try {

        int cnt = 0;
        for (actionListIter_t iter = actions->begin(); iter != actions->end(); iter++) {
            ppaction_t a;
            Module *mod;
            string mname = iter->name;

#ifdef DEBUG
    log->dlog(ch, "it is going to load module %s", mname.c_str());
#endif 

            	    
            // load Action Module used by this rule
            mod = loader->getModule(mname.c_str());
            a.module = dynamic_cast<ProcModule*> (mod);



            if (a.module != NULL) { // is it a processing kind of module

#ifdef DEBUG
    log->dlog(ch, "module %s loaded", mname.c_str());
#endif 
                a.mapi = a.module->getAPI();

                // init module
                configItemList_t itmConf = iter->conf;
                
                // Define the Flow id to be used.
                configItem_t flowId;
				char buffer1 [50];
                flowId.group = getConfigGroup();
                flowId.module = mname;
                flowId.name = "FlowId";
				sprintf (buffer1, "%d", ruleId);
				flowId.value = string(buffer1);
                flowId.type = "UInt16"; 
                itmConf.push_front(flowId);
                a.params = ConfigManager::getParamList(itmConf);
                                                
                a.flowData = NULL;
                int ret = (a.mapi)->initFlowSetup(a.params, r->getFilter(), &a.flowData);

                // if packet proc modules requires bidir matching
                // then set rule to bidir
                // FIXME not a well defined method...
                if (ret == 1) {
                    r->setBidir();
                }
								
                // init timers
                addTimerEvents(ruleId, cnt, a, *e);
	
                entry.actions.push_back(a);

            }

            cnt++;
        }
    
        // make sure the vector of rules is large enough
        if ((unsigned int)ruleId + 1 > rules.size()) {
            rules.reserve(ruleId*2 + 1);
            rules.resize(ruleId + 1 );
        }
        // success ->enter struct into internal table
        rules[ruleId] = entry;

		cout << "voy aqui 3" << endl;

	
    } catch (Error &e) { 
        log->elog(ch, e);
	
        for (ppactionListIter_t i = entry.actions.begin(); 
             i != entry.actions.end(); i++) {

            saveDelete(i->params);
            
			// Define the Flow id to be used.
			configItemList_t itmConf2;
			char buffer2 [50];
			configItem_t flowId2;
			flowId2.group = getConfigGroup();
			flowId2.name = "FlowId";
			sprintf (buffer2, "%d", ruleId);
			flowId2.value = string(buffer2);
			flowId2.type = "UInt16"; 
			itmConf2.push_front(flowId2);
			configParam_t * params = ConfigManager::getParamList(itmConf2);

            (i->mapi)->destroyFlowSetup( params, i->flowData );

			if (params != NULL) {
				saveDeleteArr(params);
			}
	    
            //release packet processing modules already loaded for this rule
            if (i->module) {
                loader->releaseModule(i->module);
            }
        }
        // empty the list itself
        entry.actions.clear();

        throw e;
    }
    return 0;
}


/* ------------------------- delRule ------------------------- */

int QOSProcessor::delRule( Rule *r )
{
    ruleActions_t *ra;
    int ruleId = r->getUId();

#ifdef DEBUG
    log->dlog(ch, "deleting Rule #%d", ruleId);
#endif

    AUTOLOCK(threaded, &maccess);

    ra = &rules[ruleId];


    // now free flow data and release used Modules
    for (ppactionListIter_t i = ra->actions.begin(); i != ra->actions.end(); i++) {

         // Define the Flow id to be used.
         configItemList_t itmConf;
         char buffer [50];
         configItem_t flowId;
         flowId.group = getConfigGroup();
         flowId.name = "FlowId";
		 sprintf (buffer, "%d", ruleId);
		 flowId.value = string(buffer);
         flowId.type = "UInt16"; 
         itmConf.push_front(flowId);
         configParam_t * params = ConfigManager::getParamList(itmConf);

        // dismantle flow data structure with module function
        i->mapi->destroyFlowSetup( params, i->flowData );
        
        if (params != NULL) {
            saveDeleteArr(params);
            params = NULL;
        }

        // release modules loaded for this rule
        loader->releaseModule(i->module);
        
        // FIXME disable timers
    }
    
    ra->actions.clear();
    ra->lastPkt = 0;
    ra->auto_flows = 0;
    ra->bidir = 0;
    ra->seppaths = 0;
    ra->rule = NULL;

    return 0;
}


int QOSProcessor::handleFDEvent(eventVec_t *e, fd_set *rset, fd_set *wset, fd_sets_t *fds)
{

    return 0;
}

void QOSProcessor::main()
{

    // this function will be run as a single thread inside the QOS processor
    log->log(ch, "QoS Processor thread running");
    
    for (;;) {
        handleFDEvent(NULL, NULL,NULL, NULL);
    }
}       

void QOSProcessor::waitUntilDone(void)
{
#ifdef ENABLE_THREADS
    AUTOLOCK(threaded, &maccess);

    if (threaded) {
      while (queue->getUsedBuffers() > 0) {
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

void QOSProcessor::addTimerEvents( int ruleID, int actID,
                                      ppaction_t &act, EventScheduler &es )
{
    timers_t *timers = (act.mapi)->getTimers(act.flowData);

    if (timers != NULL) {
        while (timers->flags != TM_END) {
            es.addEvent(new ProcTimerEvent(ruleID, actID, timers++));
        }
    }
}

// handle module timeouts
void QOSProcessor::timeout(int rid, int actid, unsigned int tmID)
{
    ppaction_t *a;
    ruleActions_t *ra;

    AUTOLOCK(threaded, &maccess);

    ra = &rules[rid];

    a = &ra->actions[actid];
    a->mapi->timeout(tmID, a->flowData);
}

/* -------------------- getModuleInfoXML -------------------- */

string QOSProcessor::getModuleInfoXML( string modname )
{
    AUTOLOCK(threaded, &maccess);
    return loader->getModuleInfoXML( modname );
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
