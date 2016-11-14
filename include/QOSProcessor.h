
/*! \file   QOSProcessor.h

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
	here all string and numeric constants for the netqos toolset are declared

    Description:
    manages and applies QoS processing modules

    $Id: QosProcessor.h 748 2015-12-01 amentes $
*/

#ifndef _QOS_PROCESSOR_H_
#define _QOS_PROCESSOR_H_


#include "stdincpp.h"
#include "ProcModule.h"
#include "ModuleLoader.h"
#include "Rule.h"
#include "QualityManagerComponent.h"
#include "Error.h"
#include "Logger.h"
#include "EventScheduler.h"


struct ppaction_t
{
    ProcModule *module;
    ProcModuleInterface_t *mapi; // module API
    void *flowData;
    // config params for module
    configParam_t *params;


    ppaction_t& operator=( ppaction_t const& rhs);

};

//! actions for each rule
typedef std::map<int, ppaction_t>            ppactionList_t;
typedef std::map<int, ppaction_t>::iterator  ppactionListIter_t;


struct ruleActions_t
{
    /*! time stamp of last packet seen for the packet flow of this task
         =0 indicates the flow was set to idle previously
     */
    time_t lastPkt;

    //! number of packets and bytes seen by this rule/task
    unsigned long long packets, bytes;

    //! master list of action module data
    ppactionList_t actions;

    //! 1 if flow autocreation enabled for rule
    int auto_flows;

    //! 1 if this flow uses bidir matching (bidir autocreation flows)
    int bidir;
    int seppaths;

    Rule *rule;

    ruleActions_t& operator=( ruleActions_t const& rhs);

};

//! action list for each rule
typedef map<int, ruleActions_t>            ruleActionList_t;
typedef map<int, ruleActions_t>::iterator  ruleActionListIter_t;


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

    //! add timer events to the output queue
    void addTimerEvents( int ruleID, int actID, ppaction_t &act );

    void createFlowKey(unsigned char *mvalues, unsigned short len, ruleActions_t *ra);

  protected:

	eventVec_t in_events;  //!< Pending input event list (to be processes)
	eventVec_t out_events;  //!< Pending output event list (response to events processed)

  public:

    /*! \short   construct and initialize a PacketProcessor object

        \arg \c cnf        config manager
        \arg \c threaded   run as separate thread
        \arg \c moduleDir  action module directory
    */
    QOSProcessor(ConfigManager *cnf, int threaded, string moduleDir = "" );

    //!   destroy a PacketProcessor object, to be overloaded
    virtual ~QOSProcessor();


    /*! \short   add an Event to the event queue

        \arg \c ev - an event (or an object derived from Event) that is
                     schdeuled for a particular time (possibly repeatedly
                     at a given interval)
    */
    void addEvent( Event *ev );


    /*! \short   delete all events for a given rule

        delete all Events related to the specified rule from the list of events

        \arg \c uid  - the unique identification number of the rule
    */
    void delRuleEvents( int uid );

    /*! \short   return the next event to be executed.

    */
	Event * getNextEvent();

    //! check a ruleset (the action part) - to be used in the scenario of no threads.
    virtual void checkRules( ruleDB_t *rules, EventScheduler *e  );

    //! check a ruleset (the action part) - to be used in the scenario of threads.
    virtual void checkRules( ruleDB_t *rules, Event *evt );

    //! add rules - to be used in the scenario of no threads.
    virtual void addRules( ruleDB_t *rules, EventScheduler *e );

	//! add rules - to be used in the scenario of thread enable.
	virtual void addRules( ruleDB_t *rules, Event *evt );

    //! delete rules - to be used in the scenario of no threads.
    virtual void delRules( ruleDB_t *rules, EventScheduler *e );

    //! delete rules - to be used in the scenario of no threads.
    virtual void delRules( ruleDB_t *rules, Event *evt );

    //! check a single rule
    int checkRule(Rule *r);

    /*! \short   add a Rule and its associated actions to rule list (no Threads)
        \arg \c r   pointer to rule
        \arg \c e   pointer to event scheduler (timer events)
        \returns 0 - on success, <0 - else
    */
    int addRule( Rule *r, EventScheduler *e );


    /*! \short   add a Rule and its associated actions to rule list (Thread Enable)
        \arg \c r   pointer to rule
        \arg \c e   pointer to event scheduler (timer events)
        \returns 0 - on success, <0 - else
    */
    int addRule( Rule *r );



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

    int getNumRules()
    {
        return (int) rules.size();
    }

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


    //! handle the events
    void handleEvent( Event *e );

};


//! overload for <<, so that a PacketProcessor object can be thrown into an iostream
ostream& operator<< ( ostream &os, QOSProcessor &pe );


#endif // _QOS_PROCESSOR_H_
