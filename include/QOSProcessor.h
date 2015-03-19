
/*! \file   QOSProcessor.h

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

    $Id: PacketProcessor.h 748 2009-09-10 02:54:03Z szander $
*/

#ifndef _PACKETPROCESSOR_H_
#define _PACKETPROCESSOR_H_


#include "stdincpp.h"
#include "ProcModule.h"
#include "ModuleLoader.h"
#include "Rule.h"
#include "QualityManagerComponent.h"
#include "Error.h"
#include "Logger.h"
#include "EventScheduler.h"


typedef struct
{
    ProcModule *module;
    ProcModuleInterface_t *mapi; // module API
    void *flowData;
    // config params for module
    configParam_t *params;
} ppaction_t;

//! actions for each rule
typedef vector<ppaction_t>            ppactionList_t;
typedef vector<ppaction_t>::iterator  ppactionListIter_t;


typedef struct {
    /*! time stamp of last packet seen for the packet flow of this task
         =0 indicates the flow was set to idle previously
     */
    time_t lastPkt;

    //! number of packets and bytes seen by this rule/task
    unsigned long long packets, bytes;

    // master list of action module data
    ppactionList_t actions;

     // 1 if flow autocreation enabled for rule
    int auto_flows;
     // 1 if this flow uses bidir matching (bidir autocreation flows)
    int bidir;
    int seppaths;

    Rule *rule;

    // pointer to filter list from rule description
    filterList_t *flist;

} ruleActions_t;

//! action list for each rule
typedef vector<ruleActions_t>            ruleActionList_t;
typedef vector<ruleActions_t>::iterator  ruleActionListIter_t;


/*! \short   manage and apply Action Modules, retrieve flow data

    the PacketProcessor class allows to manage filter rules and their
    associated actions and apply those actions to incoming packets, 
    manage and retrieve flow data
*/

class QOSProcessor : public QualityManagerComponent
{
  private:

    //! number of rules
    int numRules;

    //! associated module loader
    ModuleLoader *loader;

    //! action list for rules
    ruleActionList_t  rules;

    //! add timer events to scheduler
    void addTimerEvents( int ruleID, int actID, ppaction_t &act, EventScheduler &es );

    void createFlowKey(unsigned char *mvalues, unsigned short len, ruleActions_t *ra);

  public:

    /*! \short   construct and initialize a PacketProcessor object

        \arg \c cnf        config manager
        \arg \c threaded   run as separate thread
        \arg \c moduleDir  action module directory
    */
    QOSProcessor(ConfigManager *cnf, int threaded, string moduleDir = "" );

    //!   destroy a PacketProcessor object, to be overloaded
    virtual ~QOSProcessor();

    //! check a ruleset (the action part)
    virtual void checkRules( ruleDB_t *rules );

    //! add rules
    virtual void addRules( ruleDB_t *rules, EventScheduler *e );

    //! delete rules
    virtual void delRules( ruleDB_t *rules );

    //! check a single rule
    int checkRule(Rule *r);

    /*! \short   add a Rule and its associated actions to rule list
        \arg \c r   pointer to rule
        \arg \c e   pointer to event scheduler (timer events)
        \returns 0 - on success, <0 - else
    */
    int addRule( Rule *r, EventScheduler *e );

    /*! \short   delete a Rule from the rule list
        \arg \c r  pointer to rule
        \returns 0 - on success, <0 - else
    */
    int delRule( Rule *r );

    //! handle file descriptor event
    virtual int handleFDEvent(eventVec_t *e, fd_set *rset, fd_set *wset, fd_sets_t *fds);

    //! thread main function
    void main();

    /*! \short return -1 (no packet seen), 0 (timeout), >0 (no timeout; adjust last time)

        \arg \c ruleId  - number indicating matching rule for packet
    */
    unsigned long ruleTimeout(int ruleID, unsigned long ival, time_t now);
    
    //! get information about loaded modules
    string getInfo();

    //! dump a PacketProcessor object
    void dump( ostream &os );

    //! get the number of action modules currently in use
    int numModules() 
    { 
        return loader->numModules(); 
    }

    // handle module timeouts
    void timeout(int rid, int actid, unsigned int tmID);

    //! get xml info for a specific module
    string getModuleInfoXML( string modname );

    virtual string getConfigGroup() 
    { 
        return "QOS_PROCESSOR"; 
    }

    virtual void waitUntilDone();

};


//! overload for <<, so that a PacketProcessor object can be thrown into an iostream
ostream& operator<< ( ostream &os, QOSProcessor &pe );


#endif // _PACKETPROCESSOR_H_
