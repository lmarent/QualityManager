
/*! \file   Rule.h

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


#ifndef _RULE_H_
#define _RULE_H_


#include "stdincpp.h"
#include "Logger.h"
#include "PerfTimer.h"
#include "ConfigParser.h"
#include "FilterValue.h"
#include "FilterDefParser.h"
#include "ProcModuleInterface.h"


// list of defined flags for use within a Rule object
typedef enum
{
  // auto creation of flows (based on wildcard attributes)
  RULE_AUTO_FLOWS     =  0x04
} ruleFlags_t;

//! rule states during lifecycle
typedef enum
{
    RS_NEW = 0,
    RS_VALID,
    RS_SCHEDULED,
    RS_ACTIVE,
    RS_DONE,
    RS_ERROR
} ruleState_t;


//! FIXME document!
typedef struct
{
    string name;
    configItemList_t conf;
} action_t;


//! action list (only push_back & sequential access)
typedef list<action_t>            actionList_t;
typedef list<action_t>::iterator  actionListIter_t;

//! misc list (random access based on name required)
typedef map<string,configItem_t>            miscList_t;
typedef map<string,configItem_t>::iterator  miscListIter_t;


//! parse and store a complete rule description

class Rule
{
  private:

    Logger *log; //!< link to global logger object
    int ch;      //!< logging channel number used by objects of this class
    
    //! define the rules running time properties
    time_t start;
    time_t stop;

    //! define the flow timeout
    unsigned long flowTimeout;

    //! unique ruleID of this Rule instance (has to be provided)
    int uid;

    //! state of this rule
    ruleState_t state;

    //! name of the rule by convention this must be either: <name> or <source>.<id>
    string ruleName;

    //! parts of rule name for efficiency
    string source;
    string id;

    //! name of the rule set this rule belongs to
    string setName;

    //! stores flags that indicate if a certain yes/no option is enabled
    unsigned long flags;

    //! rule bidirectional or unidirectional
    int bidir;

    //! separate forward and backward paths (only useful for auto+bidir rules)
    int seppaths;

    //! list of filters
    filterList_t filterList;

    //! list of actions
    actionList_t actionList;

    //! list of misc stuff (start, stop, duration etc.)
    miscList_t miscList;

    /*! \short   parse identifier format 'sourcename.rulename'

        recognizes dor (.) in task identifier and saves sourcename and 
        rulename to the new malloced strings source and rname
    */
    void parseRuleName(string rname);

    //! parse time string
    time_t parseTime(string timestr);

    //! get a value by name from the misc rule attributes
    string getMiscVal(string name);


  public:
    
    void setState(ruleState_t s) 
    { 
        state = s;
    }

    ruleState_t getState()
    {
        return state;
    }

    int isFlagEnabled(ruleFlags_t f) 
    { 
        return (flags & f);
    }

    int getUId() 
    { 
        return uid;
    }
    
    void setUId(int nuid)
    {
        uid = nuid;
    }
    
    string getSetName()
    {
        return setName;
    }

    string getRuleName()
    {
        return ruleName;
    }
    
    string getRuleSource()
    {
        return source;
    }
    
    string getRuleId()
    {
        return id;
    }

    time_t getStart()
    {
        return start;
    }
    
    time_t getStop()
    {
        return stop;
    }
        
    int isBidir()
    {
        return bidir;
    }
    
    int sepPaths()
    {
      return seppaths;
    }
    
    void setBidir()
    {
        bidir = 1;
    }
    
    unsigned long getFlowTimeout()
    {
        return flowTimeout;
    }
    
    /*! \short   construct and initialize a Rule object
        \arg \c now   current timestamp
        \arg \c sname   rule set name
        \arg \c s  rname  rule name
        \arg \c f  list of filters
        \arg \c a  list of actions
        \arg \c e  list of exports
        \arg \c m  list of misc parameters
    */
    Rule(int _uid, time_t now, string sname, string rname, filterList_t &f, actionList_t &a,
    	  miscList_t &m);

    //! destroy a Rule object
    ~Rule();
   
    /*! \short   get names and values (parameters) of configured actions
        \returns a pointer (link) to a list that contains the configured actions for this rule
    */
    actionList_t *getActions();

    /*! \short   get names and values (parameters) of configured filter rule

        \returns a pointer (link) to a ParameterSet object that contains the 
                 configured filter rule description for this rule
    */
    filterList_t *getFilter();

    /*! \short   get names and values (parameters) of misc. attributes

        \returns a pointer (link) to a ParameterSet object that contains the 
                 miscanellenous attributes of a configured filter rule
    */
    miscList_t *getMisc();
    
    //! dump a Rule object
    void dump( ostream &os );

    //! get rule info string
    string getInfo(void);
};


//! overload for <<, so that a Rule object can be thrown into an iostream
ostream& operator<< ( ostream &os, Rule &ri );


#endif // _RULEINFO_H_
