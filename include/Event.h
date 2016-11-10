
/*! \file   Event.h

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
    event classes for all events handled by meter
    Code based on Netmate Implementation

    $Id: Event.h 748 2015-03-03 13:00:03 amarentes $
*/

#ifndef _EVENT_H_
#define _EVENT_H_


#include "stdincpp.h"
#include "RuleFileParser.h"
#include "QualityManagerInfo.h"


//! event numbers
typedef enum 
{
      ADD_RULES = 0,
      REMOVE_RULES,
      ACTIVATE_RULES,
      ADD_RULES_QOS_PROCESSOR,
      RESP_ADD_RULES_QOS_PROCESSOR,
      CHECK_RULES_QOS_PROCESSOR,
      RESP_CHECK_RULES_QOS_PROCESSOR,
      DEL_RULES_QOS_PROCESSOR,
      RESP_DEL_RULES_QOS_PROCESSOR,
      GET_INFO,
      GET_MODINFO,
      TEST,
      REMOVE_RULES_CTRLCOMM,
      ADD_RULES_CTRLCOMM,
      PROC_MODULE_TIMER,
      CTRLCOMM_TIMER,      
} event_t;


//! rule states during lifecycle
typedef enum
{
    EV_NEW = 0,
    EV_PROCESSING, 
    EV_DONE,
} eventState_t;


//! event names for dump method
const string eventNames[] = 
{
      "Add-rules",
      "Remove-rules",
      "Activate-rules",
      "Add Rule to QOS Processor",
      "Response to add rule event in the QOS Processor",
      "Check Rule to QOS Processor",
      "Response to check rule event in the QOS Processor",
      "Del rule in the QOS Processor",
      "Response to delete rule in the QOS Processor",
      "Get-info",
      "Get-module-info",
      "Test",
      "Remove-rules-ctrlcomm",
      "Add-rules-ctrlcomm",
      "Proc-module-timer",
      "Ctrlcomm-timer",      
};

/* ------------------------- Event class ------------------------- */

/*! \short   basic event element that is the base class of all 
             other events that can be stored in the event queue 
             of EventScheduler
*/

class Event {
      
  private:

    //! type of the event   
    event_t type;    
    
    //!< time for next processing of event [sec since Epoch]
    struct timeval when;
    
    //!< interval between two event processings [msec]
    unsigned long interval;
    
    //! align events on time boundaries
    void doAlign();

    //! Event State 
    eventState_t state;

    //! parent event that created this event. 
    Event *parent;

    
  public:
  
    /*! \short  create an event at an absolute time
        \arg \c type  type of the event
        \arg \c time  absolute timestamp when the event is due
        \arg \c ival  interval (in ms) if the event is recurrent
        \arg \c align  align the event on the next ival
    */
    Event(event_t type, struct timeval time, unsigned long ival=0, 
	  int align=0, eventState_t state=EV_NEW, Event *parent=NULL);
    
    /*! \short  create an event relative to current time
        \arg \c type  type of the event
        \arg \c offs_sec  sec offset from now when the event is due
        \arg \c offs_used  usec offset from now when the event is due
        \arg \c ival  interval (in ms) if the event is recurrent
        \arg \c align  align the event on the next ival
    */
    Event(event_t type, time_t offs_sec, time_t offs_usec = 0, 
    	  unsigned long ival=0, int align=0, eventState_t state=EV_NEW, Event *parent=NULL);
    
    /*! \short  create an event at the current time (now)
        \arg \c type  type of the event
        \arg \c ival  interval (in ms) if the event is recurrent
        \arg \c align  align the event on the next ival
    */
    Event(event_t type, unsigned long ival=0, int align=0,eventState_t state=EV_NEW, Event *parent=NULL);
    
    virtual ~Event() {}
    
    //! get event type
    event_t getType() 
    { 
        return type;
    }

    int isType( event_t atype )
    { 
        return (type == atype);
    }

    //! get expiry time
    struct timeval getTime()                        
    {
        return when;
    }
    
    //! get interval
    unsigned long getIval()                        
    {
        return interval;
    }
    
    //! set interval
    void setInterval( unsigned long ival )
    {
        interval = ival;
    }
    
    //! set expiry time
    void setTime( struct timeval newTime )         
    {
        when = newTime;
    }
    
    //! set expiry time (full sec)
    void setTime(time_t newTimeSec)
    { 
        when.tv_sec = newTimeSec;
        when.tv_usec = 0;
    }
    
    //! get next expiry time (recurrent events)
    void advance();
    
    //! Get the parent event from this event
    Event * getParent()
    {
       return parent;
    }
    
    //! set the parent event for this event.
    void setParent(Event * _parent)
    {
        parent = _parent;
    }

    eventState_t getState()
    {
        return state;
    }
    
    void setState(eventState_t _state)
    {
    
        state = _state;
    }
    
    virtual void dump( ostream &os );
    
    //! delete rules stored in this event
    virtual int deleteRule(int uid) 
    {
	return 0;
    }
};

//! base class for all ctrlcomm events, contains pointer to request
class CtrlCommEvent: public Event
{
  private:  
    struct REQUEST *req;

  public:
    //! ctrlcomm events always expire now
    CtrlCommEvent(event_t type, unsigned long ival=0, int align=0)
      : Event(type, ival, align) {}

    virtual ~CtrlCommEvent() {}

    //! get request pointer
    struct REQUEST *getReq()
    { 
	return req;
    }
    
    //! set request pointer
    void setReq(struct REQUEST *r)
    {
	req = r;
    }
};


//! test event
class TestEvent : public Event
{
  private:
   
  public:

    TestEvent(struct timeval time, unsigned long ival=0)
      : Event(TEST, time, ival) {}

    TestEvent(time_t offs_sec, time_t offs_usec, unsigned long ival=0)
      : Event(TEST, offs_sec, offs_usec, ival) {}

    TestEvent(unsigned long ival=0)
      : Event(TEST, ival) {}
};



//! base class for all QosProcessor events, contains a reference to rules
class QoSProcessorEvent : public Event
{

   private:
      ruleDB_t rules;
   
   public:
	  
      QoSProcessorEvent(event_t type, struct timeval time, ruleDB_t &r) 
		: Event(type, time), rules(r) {}

      QoSProcessorEvent(event_t type, time_t offs_sec, ruleDB_t &r) 
        : Event(type, offs_sec), rules(r) {}
    
      QoSProcessorEvent(event_t type, ruleDB_t &r) 
        : Event(type), rules(r) {}
	  
	  
	  ruleDB_t * getRules()
	  {
		  return  &rules;
	  }


      int deleteRule(int uid)
      {
         int ret = 0;
         ruleDBIter_t iter;
        
         for (iter=rules.begin(); iter != rules.end(); iter++) 
         {
            if ((*iter)->getUId() == uid) {
                rules.erase(iter);
                ret++;
                break;
            }   
         }
          
         if (rules.empty()) {
            return ++ret;
         }
          
         return ret;
      }
};


/* --------------------------------- events ------------------------------ */


class AddRulesEvent : public Event
{
  private:
    string fileName;

  public:

    AddRulesEvent(string fname, int mapi=0) 
      : Event(ADD_RULES), fileName(fname) 
    {
        
    }

    string getFileName()
    {
        return fileName;
    }
};




class ActivateRulesEvent : public Event
{
  private:
    ruleDB_t rules;

  public:

    ActivateRulesEvent(struct timeval time, ruleDB_t &r) 
      : Event(ACTIVATE_RULES, time), rules(r) {}

     ActivateRulesEvent(time_t offs_sec, ruleDB_t &r) 
      : Event(ACTIVATE_RULES, offs_sec), rules(r) {}

     ActivateRulesEvent(ruleDB_t &r) 
      : Event(ACTIVATE_RULES), rules(r) {}

     ruleDB_t *getRules()
     {
         return &rules;
     }

     int deleteRule(int uid)
     {
         int ret = 0;
         ruleDBIter_t iter;
           
         for (iter=rules.begin(); iter != rules.end(); iter++) {
             if ((*iter)->getUId() == uid) {
                 rules.erase(iter);
                 ret++;
                 break;
             }   
         }
         
         if (rules.empty()) {
             return ++ret;
         }
         
         return ret;
     }
};


class RemoveRulesEvent : public Event
{
  private:
    ruleDB_t rules;

  public:

    RemoveRulesEvent(struct timeval time, ruleDB_t &r) 
      : Event(REMOVE_RULES, time), rules(r) {}

    RemoveRulesEvent(time_t offs_sec, ruleDB_t &r) 
      : Event(REMOVE_RULES, offs_sec), rules(r) {}
    
    RemoveRulesEvent(ruleDB_t &r) 
      : Event(REMOVE_RULES), rules(r) {}

    ruleDB_t *getRules()
    {
        return &rules;
    }
    
    int deleteRule(int uid)
    {
        int ret = 0;
        ruleDBIter_t iter;
        
        for (iter=rules.begin(); iter != rules.end(); iter++) {
            if ((*iter)->getUId() == uid) {
                rules.erase(iter);
                ret++;
                break;
            }   
        }
          
        if (rules.empty()) {
            return ++ret;
        }
          
        return ret;
    }
};

class ProcTimerEvent : public Event
{
private:

 public:
    int rid;
    int actid;
    unsigned int tmID;
    
public:

    ProcTimerEvent( int ruleID, int actID, timers_t *timer ) :
      Event( PROC_MODULE_TIMER, timer->ival_msec/1000, timer->ival_msec%1000,
             ((timer->flags & TM_RECURRING) ? timer->ival_msec : 0),
             timer->flags & TM_ALIGNED),
      rid(ruleID), 
      actid(actID),
      tmID(timer->id)
      {}
    
    int getRID() 
      { 
          return rid;
      }

    int getAID()
      {
          return actid;
      }

    unsigned int getTID()
      {
          return tmID;
      }
    
    int deleteRule(int uid)
    {
        int ret = 0;
        
        if (uid == rid) {
            ret = 2;
        }
        return ret;
    }
};


class CtrlCommTimerEvent : public Event
{
    
  public:
    
    CtrlCommTimerEvent(time_t offs_sec, unsigned long ival=0, int align=0) 
      : Event(CTRLCOMM_TIMER, offs_sec,0,ival,align) {}
};

/* ------------------------------- ctrlcomm events ------------------------ */

class GetInfoEvent : public CtrlCommEvent
{
  private:
    infoList_t *infos;
   
  public:

    GetInfoEvent(infoList_t *i)
      : CtrlCommEvent(GET_INFO), infos(i) {}

    ~GetInfoEvent()
    {
        saveDelete(infos);
    }

    infoList_t *getInfos()
    {
        return infos;
    }
};


class GetModInfoEvent : public CtrlCommEvent
{
  private:

    string modname;
    
  public:

    GetModInfoEvent( string modulename )
      : CtrlCommEvent(GET_MODINFO), modname(modulename) {}

    ~GetModInfoEvent() {}

    string getModName()
    {
        return modname;
    }
};


//! add rules flags
const int ADD_RULES_MAPI   = 0x1;

class AddRulesCtrlEvent : public CtrlCommEvent
{
  private:
    int type;
    char *buf;
    int len;

  public:

    AddRulesCtrlEvent(char *b, int l, int mapi=0)
      : CtrlCommEvent(ADD_RULES_CTRLCOMM), type(mapi), len(l) 
    {
        buf = new char[len+1];
        memcpy(buf, b, len+1);          
    }

    ~AddRulesCtrlEvent()
    {
        saveDeleteArr(buf);
    }

    int isMAPI()
    {
        return type;
    }

    char *getBuf()
    {
        return buf;
    }

    int getLen()
    {
        return len;
    }
};


class RemoveRulesCtrlEvent : public CtrlCommEvent
{
  private:
    string rule;

  public:
    
    RemoveRulesCtrlEvent(string r) 
      : CtrlCommEvent(REMOVE_RULES_CTRLCOMM), rule(r) {}

    string getRule()
    {
        return rule;
    }
};


class addRulesQoSProcesorEvent : public QoSProcessorEvent
{

   public:
	  
      addRulesQoSProcesorEvent(struct timeval time, ruleDB_t &r) 
		: QoSProcessorEvent(ADD_RULES_QOS_PROCESSOR, time, r) {}

      addRulesQoSProcesorEvent(time_t offs_sec, ruleDB_t &r) 
        : QoSProcessorEvent(ADD_RULES_QOS_PROCESSOR, offs_sec, r) {}
    
      addRulesQoSProcesorEvent(ruleDB_t &r) 
        : QoSProcessorEvent(ADD_RULES_QOS_PROCESSOR, r) {}	
	
};

class respAddRulesQoSProcesorEvent : public QoSProcessorEvent
{

   public:
	  
      respAddRulesQoSProcesorEvent(struct timeval time, ruleDB_t &r) 
		: QoSProcessorEvent(RESP_ADD_RULES_QOS_PROCESSOR, time, r) {}

      respAddRulesQoSProcesorEvent(time_t offs_sec, ruleDB_t &r) 
        : QoSProcessorEvent(RESP_ADD_RULES_QOS_PROCESSOR, offs_sec, r) {}
    
      respAddRulesQoSProcesorEvent(ruleDB_t &r) 
        : QoSProcessorEvent(RESP_ADD_RULES_QOS_PROCESSOR, r) {}	
		
};

class checkRulesQoSProcessorEvent : public QoSProcessorEvent
{

   
   public:
	  
      checkRulesQoSProcessorEvent(struct timeval time, ruleDB_t &r) 
		: QoSProcessorEvent(CHECK_RULES_QOS_PROCESSOR, time, r) {}

      checkRulesQoSProcessorEvent(time_t offs_sec, ruleDB_t &r) 
        : QoSProcessorEvent(CHECK_RULES_QOS_PROCESSOR, offs_sec, r) {}
    
      checkRulesQoSProcessorEvent(ruleDB_t &r) 
        : QoSProcessorEvent(CHECK_RULES_QOS_PROCESSOR, r) {}
	  	  
};


class respCheckRulesQoSProcessorEvent : public QoSProcessorEvent
{

   public:
      
      respCheckRulesQoSProcessorEvent(struct timeval time, ruleDB_t &r) 
		: QoSProcessorEvent(RESP_CHECK_RULES_QOS_PROCESSOR, time, r) {}

      respCheckRulesQoSProcessorEvent(time_t offs_sec, ruleDB_t &r) 
        : QoSProcessorEvent(RESP_CHECK_RULES_QOS_PROCESSOR, offs_sec, r) {}
    
      respCheckRulesQoSProcessorEvent(ruleDB_t &r) 
        : QoSProcessorEvent(RESP_CHECK_RULES_QOS_PROCESSOR, r) {}
	    
};


class delRulesQoSProcesorEvent : public QoSProcessorEvent
{

   public:
	  
      delRulesQoSProcesorEvent(struct timeval time, ruleDB_t &r) 
		: QoSProcessorEvent(DEL_RULES_QOS_PROCESSOR, time, r) {}

      delRulesQoSProcesorEvent(time_t offs_sec, ruleDB_t &r) 
        : QoSProcessorEvent(DEL_RULES_QOS_PROCESSOR, offs_sec, r) {}
    
      delRulesQoSProcesorEvent(ruleDB_t &r) 
        : QoSProcessorEvent(DEL_RULES_QOS_PROCESSOR, r) {}	
	
};

class respDelRulesQoSProcesorEvent : public QoSProcessorEvent
{

   public:
	  
      respDelRulesQoSProcesorEvent(struct timeval time, ruleDB_t &r) 
		: QoSProcessorEvent(RESP_DEL_RULES_QOS_PROCESSOR, time, r) {}

      respDelRulesQoSProcesorEvent(time_t offs_sec, ruleDB_t &r) 
        : QoSProcessorEvent(RESP_DEL_RULES_QOS_PROCESSOR, offs_sec, r) {}
    
      respDelRulesQoSProcesorEvent(ruleDB_t &r) 
        : QoSProcessorEvent(RESP_DEL_RULES_QOS_PROCESSOR, r) {}	
		
};



//! overload for << so that an Event object can be thrown into an ostream
ostream& operator<< ( ostream &os, Event &ev );


#endif // _EVENT_H_
