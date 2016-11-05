
/*! \file   QualityManager.cpp

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
    
    Code based on Netmate

    $Id: QualityManager.cpp 748 2015-03-05 11:01:01 amarentes $
*/


#include "QualityManager.h"
#include "ParserFcts.h"
#include "constants_qos.h"


// globals in QualityManager class
int QualityManager::s_sigpipe[2];
int QualityManager::enableCtrl = 0;

// global timeout flag
int g_timeout = 0;

// remove newline from end of C string
static inline char *noNewline( char* s )
{
    char *p = strchr( s, '\n' );
    if (p != NULL) {
        *p = '\0';
    }
    return s;
}


/* ------------------------- QualityManager ------------------------- */

QualityManager::QualityManager( int argc, char *argv[])
    :  pprocThread(0)
{

    // record meter start time for later output
  startTime = ::time(NULL);

    try {

        if (pipe((int *)&s_sigpipe) < 0) {
            throw Error("failed to create signal pipe");
        }
				
        fdList[make_fd(s_sigpipe[0], FD_RD)] = NULL;

        // the read fd must not block
        fcntl(s_sigpipe[0], F_SETFL, O_NONBLOCK);
	
        // install signal handlers
        signal(SIGINT, sigint_handler);
        signal(SIGTERM, sigint_handler);
        signal(SIGUSR1, sigusr1_handler);
        signal(SIGALRM, sigalarm_handler);
        // FIXME sighup for log file rotation
				
        auto_ptr<CommandLineArgs> _args(new CommandLineArgs());
        args = _args;

        // basic args
        args->add('c', "ConfigFile", "<file>", "use alternative config file",
                  "MAIN", "configfile");
        args->add('l', "LogFile", "<file>", "use alternative log file",
                  "MAIN", "logfile");
        args->add('r', "RuleFile", "<file>", "use specified rule file",
                  "MAIN", "rulefile");
        args->addFlag('V', "RelVersion", "show version info and exit",
                      "MAIN", "version");
        args->add('D', "FilterDefFile", "<file>", "use alternative file for filter definitions",
                  "MAIN", "fdeffile");
        args->add('C', "FilterConstFile", "<file>", "use alternative file for filter constants",
                  "MAIN", "fcontfile");
#ifdef USE_SSL
        args->addFlag('x', "UseSSL", "use SSL for control communication",
                      "CONTROL", "usessl");
#endif
        args->add('P', "ControlPort", "<portnumber>", "use alternative control port",
                  "CONTROL", "cport");
#ifndef ENABLE_NF
#ifndef HAVE_LIBIPULOG_LIBIPULOG_H
        args->add('i', "NetInterface", "<iface>[,<iface2>,...]", "select network interface(s)"
                  " to capture from", "MAIN", "interface");
        args->addFlag('p', "NoPromiscInt", "don't put interface in "
                      "promiscuous mode", "MAIN", "nopromisc");
#endif
#endif


        // get command line arguments from components via static method
        // TODO AM: Check if this part is required
        // ClassifierSimple::add_args(args.get());

        // parse command line args
        if (args->parseArgs(argc, argv)) {
            // user wanted help
            exit(0);
        }

        if (args->getArgValue('V') == "yes") {
            cout << getHelloMsg();
            exit(0);
        }

        // test before registering the exit function
        if (alreadyRunning()) {
            throw Error("already running on this machine - terminating" );
        }

        // register exit function
        atexit(exit_fct);
				
        auto_ptr<Logger> _log(Logger::getInstance()); 	
        log = _log;
        ch = log->createChannel("QualityManager");

        log->log(ch,"Initializing quality manager system");
        log->log(ch,"Program executable = %s", argv[0]);
        log->log(ch,"Started at %s", noNewline(ctime(&startTime)));
				
        // parse config file
        configFileName = args->getArgValue('c');
        if (configFileName.empty()) { 
            // is no config file is given then use the default
            // file located in a relative location to the binary
            configFileName = NETQOS_DEFAULT_CONFIG_FILE;
        }

        auto_ptr<ConfigManager> _conf(new ConfigManager(QOS_CONFIGFILE_DTD, configFileName, argv[0]));
        conf = _conf;

        // merge into config
        conf->mergeArgs(args->getArgList(), args->getArgVals());

        // dont need this anymore
        CommandLineArgs *a = args.release();
        saveDelete(a);

        // use logfilename (in order of precedence):
        // from command line / from config file / hardcoded default

        // query command line for log file name
        logFileName = conf->getValue("LogFile", "MAIN");

        if (logFileName.empty()) {
            logFileName = QOS_DEFAULT_LOG_FILE;
        }

        log->setDefaultLogfile(logFileName);

        // set logging vebosity level if configured
        string verbosity = conf->getValue("VerboseLevel", "MAIN");
        if (!verbosity.empty()) {
            log->setLogLevel( ParserFcts::parseInt( verbosity, -1, 4 ) );
        }

        log->log(ch,"configfilename used is: '%s'", configFileName.c_str());
#ifdef DEBUG
        log->dlog(ch,"------- startup -------" );
#endif

        // startup other core classes
        auto_ptr<PerfTimer> _perf(PerfTimer::getInstance());
        perf = _perf;
        auto_ptr<RuleManager> _rulm(new RuleManager(conf->getValue("FilterDefFile", "MAIN"),
                                                    conf->getValue("FilterConstFile", "MAIN")));
        rulm = _rulm;
        auto_ptr<EventScheduler> _evnt(new EventScheduler());
        evnt = _evnt;

        // startup Quality components

#ifdef ENABLE_THREADS

        auto_ptr<QOSProcessor> _proc(new QOSProcessor(conf.get(),
							    conf->isTrue("Thread",
									 "QOS_PROCESSOR")));
        pprocThread = conf->isTrue("Thread", "QOS_PROCESSOR");
#else
        
        auto_ptr<QOSProcessor> _proc(new QOSProcessor(conf.get(), 0));
        pprocThread = 0;
				
        if (conf->isTrue("Thread", "QOS_PROCESSOR") ) {
            log->wlog(ch, "Threads enabled in config file but executable is compiled without thread support");
        }
#endif
        proc = _proc;
        proc->mergeFDs(&fdList);
		
		// setup initial rules
		string rfn = conf->getValue("RuleFile", "MAIN");
        if (!rfn.empty()) {
			evnt->addEvent(new AddRulesEvent(rfn));
        }

        // disable logger threading if not needed
        if (!pprocThread ) {
            log->setThreaded(0);
        }
		enableCtrl = conf->isTrue("Enable", "CONTROL");

		if (enableCtrl) {
			// ctrlcomm can never be a separate thread
			auto_ptr<CtrlComm> _comm(new CtrlComm(conf.get(), 0));
			comm = _comm;
			comm->mergeFDs(&fdList);
		}


    } catch (Error &e) {
        if (log.get()) {
            log->elog(ch, e);
        }  
        throw e;
    }
}


/* ------------------------- ~QualityManager ------------------------- */

QualityManager::~QualityManager()
{
    // objects are destroyed by their auto ptrs
}


/* -------------------- getHelloMsg -------------------- */

string QualityManager::getHelloMsg()
{
    ostringstream s;
    
    static char name[128] = "\0";

    if (name[0] == '\0') { // first time
        gethostname(name, sizeof(name));
    }

    s << "QualityManager build " << BUILD_TIME 
      << ", running at host \"" << name << "\"," << endl
      << "compile options: "
#ifndef ENABLE_THREADS
      << "_no_ "
#endif
      << "multi-threading support, "
#ifndef USE_SSL
      << "_no_ "
#endif
      << "secure sockets (SSL) support"
      << endl;
    
    return s.str();
}


/* -------------------- getInfo -------------------- */

string QualityManager::getInfo(infoType_t what, string param)
{  
    time_t uptime;
    ostringstream s;
    
    s << "<info name=\"" << QualityManagerInfo::getInfoString(what) << "\" >";

    switch (what) {
    case I_QUALITYMANAGER_VERSION:
        s << getHelloMsg();
        break;
    case I_UPTIME:
      uptime = ::time(NULL) - startTime;
        s << uptime << " s, since " << noNewline(ctime(&startTime));
        break;
    case I_TASKS_STORED:
        s << rulm->getNumTasks();
        break;
    case I_CONFIGFILE:
        s << configFileName;
        break;
    case I_USE_SSL:
        s << (httpd_uses_ssl() ? "yes" : "no");
        break;
    case I_HELLO:
        s << getHelloMsg();
        break;
    case I_TASKLIST:
        s << CtrlComm::xmlQuote(rulm->getInfo());
        break;
    case I_TASK:
        if (param.empty()) {
            throw Error("get_info: missing parameter for rule = <rulename>" );
        } else {
            int n = param.find(".");
            if (n > 0) {
                s << CtrlComm::xmlQuote(rulm->getInfo(param.substr(0,n), param.substr(n+1, param.length())));
            } else {
                s << CtrlComm::xmlQuote(rulm->getInfo(param));
            }
        }
        break;
    case I_NUMQUALITYMANAGERINFOS:
    default:
        return string();
    }

    s << "</info>" << endl;
    
    return s.str();
}


string QualityManager::getQualityManagerInfo(infoList_t *i)
{
    ostringstream s;
    infoListIter_t iter;
   
    s << "<QualityManagerInfos>\n";

    for (iter = i->begin(); iter != i->end(); iter++) {
        s << getInfo(iter->type, iter->param);
    }

    s << "</QualityManagerInfos>\n";

    return s.str();
}


void QualityManager::handlerGetInfo(Event *e, fd_sets_t *fds)
{

    // get info types from event
    try 
    {

#ifdef DEBUG
        log->dlog(ch,"processing event Get info" );
#endif

        infoList_t *i = ((GetInfoEvent *)e)->getInfos(); 
        
        // send meter info
        comm->sendMsg(getQualityManagerInfo(i), ((GetInfoEvent *)e)->getReq(), fds, 0 /* do not html quote */ );
    } 
    catch(Error &err) 
    {
        comm->sendErrMsg(err.getError(), ((GetInfoEvent *)e)->getReq(), fds);
    }
    
    e->setState(EV_DONE);
    
}


void QualityManager::handlerModInfo(Event *e, fd_sets_t *fds)
{

    // get module information from loaded module (proc mods only now)
    try 
    {

#ifdef DEBUG
        log->dlog(ch,"processing event Get modinfo" );
#endif
        string s = proc->getModuleInfoXML(((GetModInfoEvent *)e)->getModName());
        // send module info
        comm->sendMsg(s, ((GetModInfoEvent *)e)->getReq(), fds, 0);
    } 
    catch(Error &err) 
    {
        comm->sendErrMsg(err.getError(), ((GetModInfoEvent *)e)->getReq(), fds);
    }
    
    e->setState(EV_DONE);
}


void QualityManager::handlerAddRulesNonThreaded(Event *e, fd_sets_t *fds)
{

    ruleDB_t *new_rules = NULL;

    try 
    {

#ifdef DEBUG
        log->dlog(ch,"processing event adding rules" );
#endif
        // support only XML rules from file
        new_rules = rulm->parseRules(((AddRulesEvent *)e)->getFileName());

#ifdef DEBUG
        log->dlog(ch,"Rules sucessfully parsed " );
#endif
             
        // test rule spec 
        proc->checkRules(new_rules);

#ifdef DEBUG
        log->dlog(ch,"Rules sucessfully checked " );
#endif

        // no error so lets add the rules and schedule for activation
        // and removal
        rulm->addRules(new_rules, evnt.get());

#ifdef DEBUG
        log->dlog(ch,"Rules sucessfully added " );
#endif


        saveDelete(new_rules);

    } 
    catch (Error &e) 
    {
        // error in rule(s)
        if (new_rules) {
            saveDelete(new_rules);
        }
        
        log->elog(ch,(string("error processing ADD_RULES") + e.getError()).c_str() );
    }
    
    e->setState(EV_DONE);
}


void QualityManager::handlerAddRulesThreaded(Event *e, fd_sets_t *fds)
{

    ruleDB_t *new_rules = NULL;

    try {

#ifdef DEBUG
        log->dlog(ch,"processing event adding rules" );
#endif
        // support only XML rules from file
        new_rules = rulm->parseRules(((AddRulesEvent *)e)->getFileName());
             
        ruleDB_t rules;
        ruleDBIter_t it;
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            rules.push_back(*it);
        }

        // test rule spec 
        Event * evt = new checkRulesQoSProcessorEvent(rules);
        evt->setParent(e);
        e->setState(EV_PROCESSING);
        unsigned long ival = 1000; // Next revision interval 
        e->setInterval(ival);
        e->advance();
        proc->addEvent(evt);

        // Delete the container created by the parseRules function.
        saveDelete(new_rules);

    } 
    catch (Error &err) 
    {
        // error in rule(s)
        if (new_rules) {

            // Delete the memory for the rules objects created.
            ruleDBIter_t it;
            for (it = new_rules->begin(); it != new_rules->end(); ++it)
            {
                Rule *rule = *it;
                saveDelete(rule);
            }

            saveDelete(new_rules);
        }
        e->setState(EV_DONE);
        log->elog(ch,(string("error processing ADD_RULES") + err.getError()).c_str() );
    }
}


void QualityManager::handlerAddRulesCntrlCommNonThreads(Event *e, fd_sets_t *fds)
{
    
    ruleDB_t *new_rules = NULL;

    try {

#ifdef DEBUG
        log->dlog(ch,"processing event add rules by controlcomm" );
#endif
              
        new_rules = rulm->parseRulesBuffer( 
                ((AddRulesCtrlEvent *)e)->getBuf(),
                ((AddRulesCtrlEvent *)e)->getLen(), ((AddRulesCtrlEvent *)e)->isMAPI());

        // test rule spec 
        proc->checkRules(new_rules);
	  
        // no error so let's add the rules and 
        // schedule for activation and removal
        rulm->addRules(new_rules, evnt.get());
        
        // Response to the entity triggering the event.
        comm->sendMsg("rule(s) added", ((AddRulesCtrlEvent *)e)->getReq(), fds);
        saveDelete(new_rules);

    } 
    catch (Error &err) 
    {
        // error in rule(s)
        if (new_rules) {
            saveDelete(new_rules);
        }
        comm->sendErrMsg(err.getError(), ((AddRulesCtrlEvent *)e)->getReq(), fds); 
    }

    e->setState(EV_DONE);
}


void QualityManager::handlerAddRulesCntrlCommThreaded(Event *e, fd_sets_t *fds)
{
    
    ruleDB_t *new_rules = NULL;


    try {

#ifdef DEBUG
        log->dlog(ch,"processing event adding rules" );
#endif
        // support only XML rules from file
        new_rules = rulm->parseRulesBuffer( 
                        ((AddRulesCtrlEvent *)e)->getBuf(),
                        ((AddRulesCtrlEvent *)e)->getLen(), ((AddRulesCtrlEvent *)e)->isMAPI());
             
        ruleDB_t rules;
        ruleDBIter_t it;
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            rules.push_back(*it);
        }

        // test rule spec 
        Event * evt = new checkRulesQoSProcessorEvent(rules);
        evt->setParent(e);
        e->setState(EV_PROCESSING);
        unsigned long ival = 1000; // Next revision interval 
        e->setInterval(ival);
        e->advance();  
              
        proc->addEvent(evt);

        // Delete the container created by the parseRules function.
        saveDelete(new_rules);

    } 
    catch (Error &err) 
    {
        // error in rule(s)
        if (new_rules) {

            // Delete the memory for the rules objects created.
            ruleDBIter_t it;
            for (it = new_rules->begin(); it != new_rules->end(); ++it)
            {
                Rule *rule = *it;
                saveDelete(rule);
            }

            saveDelete(new_rules);
        }
        e->setState(EV_DONE);
        comm->sendErrMsg(err.getError(), ((AddRulesCtrlEvent *)e)->getReq(), fds); 
    }
}


void QualityManager::handlerActivateRulesNonThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
        log->dlog(ch,"processing event activate rules" );
#endif

    try
    {

        // get the rules from the event.
        ruleDB_t *rules = ((ActivateRulesEvent *)e)->getRules();
        
        // execute rule activation on the Qos processor.
        proc->addRules(rules, evnt.get());
        
        // activate
        rulm->activateRules(rules, evnt.get());
    }
    catch(Error &err)
    {        
        log->elog(ch, (string("error processing ACTIVATE_RULES") + err.getError()).c_str() );
    }
    
    e->setState(EV_DONE);
}


void QualityManager::handlerActivateRulesThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
        log->dlog(ch,"processing event activate rules" );
#endif

    try
    {
        ruleDB_t *rules = ((ActivateRulesEvent *)e)->getRules();

        Event * evt = new addRulesQoSProcesorEvent(*rules);
        evt->setParent(e);
        e->setState(EV_PROCESSING);
        unsigned long ival = 1000; // Next revision interval 
        e->setInterval(ival);
        e->advance();
        
        proc->addEvent(evt);
    }
    catch(Error &err)
    {        
        e->setState(EV_DONE);
        log->elog(ch, (string("error processing ACTIVATE_RULES") + err.getError()).c_str() );
    }
}


void QualityManager::handlerRemoveRulesNonThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"processing event remove rules" );
#endif
    
    try
    {
        ruleDB_t *rules = ((RemoveRulesEvent *)e)->getRules();
	  	  
        // now get rid of the expired rule
        proc->delRules(rules);
        rulm->delRules(rules, evnt.get());    
    }

    catch(Error &err)
    {        
        log->elog(ch, (string("error processing REMOVE_RULES") + err.getError()).c_str() );
    }
    
    e->setState(EV_DONE);
}


void QualityManager::handlerRemoveRulesThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"processing event remove rules" );
#endif
    
    try
    {
        ruleDB_t *rules = ((RemoveRulesEvent *)e)->getRules();
	  	  

        Event *evt = new delRulesQoSProcesorEvent(*rules);
        evt->setParent(e);
        e->setState(EV_PROCESSING);
        unsigned long ival = 1000; // Next revision interval 
        e->setInterval(ival);
        e->advance();        
        
        proc->addEvent(evt);

    }

    catch(Error &err)
    {        
        e->setState(EV_DONE);
        log->elog(ch, (string("error processing REMOVE_RULES") + err.getError()).c_str() );
    }
}


void QualityManager::handlerRemoveRulesCntrlCommNonThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"processing event remove rules cntrlcomm" );
#endif

    ruleDB_t rules;

    try 
    {

         string r = ((RemoveRulesCtrlEvent *)e)->getRule();
         int n = r.find(".");
         if (n > 0) 
         {
             string sname = r.substr(0,n); 
			 string rname = r.substr(n+1, r.length()-n);
#ifdef DEBUG
             log->dlog(ch,"Deleting rule set=%s ruleId=%s", sname.c_str(), rname.c_str() );
#endif

             // delete 1 rule
             Rule *rptr = rulm->getRule(sname, rname);
             if (rptr == NULL) {
                 throw Error("no such rule");
             }
             rules.push_back(rptr);

          } 
          else 
          {

#ifdef DEBUG
              log->dlog(ch,"Deleting rule set=%s ", r.c_str() );
#endif				  
              // delete rule set
              ruleIndex_t *ruleset = rulm->getRules(r);
              if (ruleset == NULL) {
                  throw Error("no such rule set");
              }

              for (ruleIndexIter_t i = ruleset->begin(); i != ruleset->end(); i++) {
                  Rule *rptr = rulm->getRule(i->second);
                  rules.push_back(rptr);
              }
           }
        
           proc->delRules(&rules, evnt.get());
           rulm->delRules(&rules, evnt.get());

           comm->sendMsg("rule(s) deleted", ((RemoveRulesCtrlEvent *)e)->getReq(), fds);
    } 
    catch (Error &err) 
    {
        comm->sendErrMsg(err.getError(), ((RemoveRulesCtrlEvent *)e)->getReq(), fds);
    }
    
    e->setState(EV_DONE);
}


void QualityManager::handlerRemoveRulesCntrlCommThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"processing event remove rules cntrlcomm" );
#endif

    ruleDB_t rules;

    try 
    {

         string r = ((RemoveRulesCtrlEvent *)e)->getRule();
         int n = r.find(".");
         if (n > 0) 
         {
             string sname = r.substr(0,n); 
			 string rname = r.substr(n+1, r.length()-n);
#ifdef DEBUG
             log->dlog(ch,"Deleting rule set=%s ruleId=%s", sname.c_str(), rname.c_str() );
#endif

             // delete 1 rule
             Rule *rptr = rulm->getRule(sname, rname);
             if (rptr == NULL) {
                 throw Error("no such rule");
             }
             rules.push_back(rptr);

         } 
         else 
         {

#ifdef DEBUG
              log->dlog(ch,"Deleting rule set=%s ", r.c_str() );
#endif				  
              // delete rule set
              ruleIndex_t *ruleset = rulm->getRules(r);
              if (ruleset == NULL) {
                  throw Error("no such rule set");
              }

              for (ruleIndexIter_t i = ruleset->begin(); i != ruleset->end(); i++) {
                  Rule *rptr = rulm->getRule(i->second);
                  rules.push_back(rptr);
              }
         }
        
        Event * evt = new delRulesQoSProcesorEvent(rules) ;
        evt->setParent(e);
        e->setState(EV_PROCESSING);
        unsigned long ival = 1000; // Next revision interval 
        e->setInterval(ival);
        e->advance();        
        
        proc->addEvent(evt);

    } 
    catch (Error &err) 
    {
        e->setState(EV_DONE);
        comm->sendErrMsg(err.getError(), ((RemoveRulesCtrlEvent *)e)->getReq(), fds);
    }
}


void QualityManager::handlerProcTimerNonThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"processing event proc module timer" );
#endif
    
    try
    {
        proc->timeout(((ProcTimerEvent *)e)->getRID(), ((ProcTimerEvent *)e)->getAID(),
                      ((ProcTimerEvent *)e)->getTID());
    
    }
    catch (Error &err)
    {
        log->elog(ch,(string("error processing PROC TIMER") + err.getError()).c_str() );
    }
    
    e->setState(EV_DONE);
}


void QualityManager::handlerProcTimerThreaded(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"processing event proc module timer" );
#endif
    
    // FIXME: Code not implemented yet.
    
    e->setState(EV_DONE);
}


void QualityManager::handlerResponseAddRulesQoSProcessor(Event *e, fd_sets_t *fds)
{

    respAddRulesQoSProcesorEvent *evt = static_cast<respAddRulesQoSProcesorEvent *>(e);

    try
    {
        // get the parent event and put it on done
        ActivateRulesEvent *evtParent = dynamic_cast<ActivateRulesEvent *>(evt->getParent());
        if (evtParent != NULL)
            evtParent->setState(EV_DONE);

        // activate
        rulm->activateRules(evt->getRules(), evnt.get());
        
        
    }
    catch (Error &err)
    {
        ActivateRulesEvent *evtParent = dynamic_cast<ActivateRulesEvent *>(evt->getParent());
        if (evtParent != NULL)
            evtParent->setState(EV_DONE);
        
        log->elog(ch, (string("error processing ACTIVATE_RULES") + err.getError()).c_str() );
    }

    // Put this event in done.
    e->setState(EV_DONE);

}


void QualityManager::handlerResponseCheckRulesQoSProcessor(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"starting handler response check rules QoS Processor" );
#endif
                
    respCheckRulesQoSProcessorEvent *evt = static_cast<respCheckRulesQoSProcessorEvent *>(e);
        
        
    if (dynamic_cast<AddRulesEvent*>(evt->getParent()))
    {
        
        AddRulesEvent *evtParent= dynamic_cast<AddRulesEvent*>(evt->getParent());
        
        try
        {

            // no error so lets add the rules and schedule for activation
            // and removal
            rulm->addRules(evt->getRules(), evnt.get());
            
            evtParent->setState(EV_DONE);
        }
        catch( Error &err)
        {
            evtParent->setState(EV_DONE);
            log->elog(ch, (string("error processing ADD_RULES") + err.getError()).c_str() );
        }
    }
    
    else if (dynamic_cast<AddRulesCtrlEvent*>(evt->getParent()))
    {
        
        AddRulesCtrlEvent *evtParent = dynamic_cast<AddRulesCtrlEvent*>(evt->getParent());
        
        try
        {

            // no error so lets add the rules and schedule for activation
            // and removal
            rulm->addRules(evt->getRules(), evnt.get());

            // Response to the entity triggering the event.
            comm->sendMsg("rule(s) added", evtParent->getReq(), fds);
            
            evtParent->setState(EV_DONE);
        }
        catch( Error &err)
        {
            evtParent->setState(EV_DONE);
            comm->sendErrMsg(err.getError(), evtParent->getReq(), fds); 
        }
    }
            
    e->setState(EV_DONE);
    
#ifdef DEBUG
    log->dlog(ch,"ending handler response check rules QoS Processor" );
#endif

}


void QualityManager::handlerResponseDelRulesQoSProcessor(Event *e, fd_sets_t *fds)
{

#ifdef DEBUG
    log->dlog(ch,"starting handler response delete rules QoS Processor" );
#endif
                
    respDelRulesQoSProcesorEvent *evt = static_cast<respDelRulesQoSProcesorEvent *>(e);
        
        
    if (dynamic_cast<RemoveRulesEvent*>(evt->getParent()))
    {
        
        RemoveRulesEvent *evtParent= dynamic_cast<RemoveRulesEvent*>(evt->getParent());
        
        try
        {
            // no error so lets add the rules and schedule for activation
            // and removal
            rulm->delRules(evt->getRules(), evnt.get());
            
            evtParent->setState(EV_DONE);
        }
        catch( Error &e)
        {
            evtParent->setState(EV_DONE);
            log->elog(ch,(string("error processing DELETE_RULES") + e.getError()).c_str() );
        }
    }
    
    else if (dynamic_cast<RemoveRulesCtrlEvent*>(evt->getParent()))
    {
        
        RemoveRulesCtrlEvent *evtParent = dynamic_cast<RemoveRulesCtrlEvent*>(evt->getParent());
        
        try
        {

            // no error so lets add the rules and schedule for activation
            // and removal
            rulm->delRules(evt->getRules(), evnt.get());

            // Response to the entity triggering the event.
            comm->sendMsg("rule(s) deleted", evtParent->getReq(), fds);
            
            evtParent->setState(EV_DONE);
        }
        catch( Error &e)
        {
            evtParent->setState(EV_DONE);
            comm->sendErrMsg(e.getError(), evtParent->getReq(), fds); 
        }
    }
            
    e->setState(EV_DONE);
    
#ifdef DEBUG
    log->dlog(ch,"ending handler response delete rules QoS Processor" );
#endif

}


/* -------------------- handleEvent -------------------- */
void QualityManager::handleEvent(Event *e, fd_sets_t *fds)
{
   
    unsigned long ival = 0;
   
    switch (e->getType()) {
    case TEST:
      {

#ifdef DEBUG
        log->dlog(ch,"processing event test" );
#endif

      }
      break;
    case GET_INFO:
      {

          if (e->getState() == EV_NEW)
          {
            handlerGetInfo(e, fds);
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          { 
              e->setInterval(ival);
          }
      }
      break;
    case GET_MODINFO:
      {

          if (e->getState() == EV_NEW)
          {
              handlerModInfo(e, fds);
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          {
              e->setInterval(ival);
          }  

      }
      break;
    case ADD_RULES:
      {

          if (e->getState() == EV_NEW)
          {
              if (pprocThread)
              {
                  handlerAddRulesThreaded(e, fds);
              }    
              else
              {
                  handlerAddRulesNonThreaded(e, fds);
              }
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          { 
              e->setInterval(ival);
          }
      }
      break;
    case ADD_RULES_CTRLCOMM:
      {

          if (e->getState() == EV_NEW)
          {
              if (pprocThread)
              {
                  handlerAddRulesCntrlCommThreaded(e, fds);
              }    
              else
              {
                  handlerAddRulesCntrlCommNonThreads(e, fds);
              }
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          { 
              e->setInterval(ival);
          }
            
      }
      break; 	

    case ACTIVATE_RULES:
      {

          if (e->getState() == EV_NEW)
          {
              if (pprocThread)
              {
                  handlerAddRulesCntrlCommThreaded(e, fds);
              }    
              else
              {
                  handlerActivateRulesNonThreaded(e, fds);
              }
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          { 
              e->setInterval(ival);
          }

      }
      break;
    case REMOVE_RULES:
      {

          if (e->getState() == EV_NEW)
          {
              if (pprocThread)
              {
                  handlerRemoveRulesCntrlCommThreaded(e, fds);
              }    
              else
              {
                  handlerRemoveRulesNonThreaded(e, fds);
              }
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          { 
              e->setInterval(ival);
          }
          
      }
      break;

    case REMOVE_RULES_CTRLCOMM:
      {

          if (e->getState() == EV_NEW)
          {
          
              if (pprocThread)
              {
                  handlerRemoveRulesCntrlCommThreaded(e, fds);
              }
              else
              {
                  handlerRemoveRulesCntrlCommNonThreaded(e, fds);
              }
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          {  
              e->setInterval(ival);
          } 

      }
      break;

    case PROC_MODULE_TIMER:
      {

          if (e->getState() == EV_NEW)
          {
              if (pprocThread)
              {
                  handlerProcTimerThreaded(e, fds);
              }
              else
              {
                  handlerProcTimerNonThreaded(e, fds);
              }  
          }
          else if (e->getState() == EV_PROCESSING)
          {
              // EV_PROCESSING - Nothing to do.
          }
          else
          {
              e->setInterval(ival);
          }  

      }
      break;

    case RESP_ADD_RULES_QOS_PROCESSOR:
      {
          handlerResponseAddRulesQoSProcessor(e,fds);
      }
      break;
    
    case RESP_CHECK_RULES_QOS_PROCESSOR:
      {
          handlerResponseCheckRulesQoSProcessor(e,fds);
      }    
    case RESP_DEL_RULES_QOS_PROCESSOR:
      {
          handlerResponseDelRulesQoSProcessor(e,fds);
      }
      break;
      
    default:
        log->elog(ch,"Error Unknow event" );
    }
}


/* ----------------------- run ----------------------------- */

void QualityManager::run()
{
    fdListIter_t   iter;
    fd_set         rset, wset;
    fd_sets_t      fds;
    struct timeval tv;
    int            cnt = 0;
    int            stop = 0;
    eventVec_t     retEvents;
    Event         *e = NULL;

    try {
        // fill the fd set
        FD_ZERO(&fds.rset);
        FD_ZERO(&fds.wset);
        for (iter = fdList.begin(); iter != fdList.end(); iter++) {
            if ((iter->first.mode == FD_RD) || (iter->first.mode == FD_RW)) {
                FD_SET(iter->first.fd, &fds.rset);
            }
            if ((iter->first.mode == FD_WT) || (iter->first.mode == FD_RW)) {
                FD_SET(iter->first.fd, &fds.wset);
            }
        }
        fds.max = fdList.begin()->first.fd;
		

        // register a timer for ctrlcomm (only online capturing)
		if (enableCtrl) {
		  int t = comm->getTimeout();
		  if (t > 0) {
				evnt->addEvent(new CtrlCommTimerEvent(t, t * 1000));
		  }
		}
		
		
        // start threads (if threading is configured)
        proc->run();

#ifdef DEBUG
        log->dlog(ch,"------- Quality Manager is running -------");
#endif

        do {

			// select
            rset = fds.rset;
            wset = fds.wset;
	    
			tv = evnt->getNextEventTime();

            //cerr << "timeout: " << tv.tv_sec*1e6+tv.tv_usec << endl;

            // note: under most unix the minimal sleep time of select is
            // 10ms which means an event may be executed 10ms after expiration!
            if ((cnt = select(fds.max+1, &rset, &wset, NULL, &tv)) < 0) {
                 if (errno != EINTR) {
					throw Error("select error: %s", strerror(errno));
                 }
            }

            // check FD events
            if (cnt > 0)  {
                if (FD_ISSET( s_sigpipe[0], &rset)) {
                    // handle sig action
                    char c;
                    if (read(s_sigpipe[0], &c, 1) > 0) {
                        switch (c) {
                        case 'S':
                            stop = 1;
                            break;
                        case 'D':
                            cerr << *this;
                            break;
                        case 'A':
                            // next event
                            
                            // check Event Scheduler events
                            e = evnt->getNextEvent();
                            if (e != NULL) {
                                // FIXME hack
                                if (e->getType() == CTRLCOMM_TIMER) {
                                    comm->handleFDEvent(&retEvents, NULL, 
                                                        NULL, &fds);
                                } else {
                                    handleEvent(e, &fds);
                                }
                                // reschedule the event
                                evnt->reschedNextEvent(e);
                                e = NULL;
                            }		    
                            break;
                        default:
                            throw Error("unknown signal");
                        } 
                        //} else {
                        //throw Error("sigpipe read error");
                    }
                } else {
                    if (enableCtrl) {
                      comm->handleFDEvent(&retEvents, &rset, &wset, &fds);
                    }
                }
	        }	

            // execute all due events
            evnt->getNextEventTime();
            char c;
            while (read(s_sigpipe[0], &c, 1) > 0) {
                switch (c) {
                   case 'S':
                       stop = 1;
                       break;
                   case 'D':
                       cerr << *this;
                       break;
                   case 'A':
                       // check Event Scheduler events
                       e = evnt->getNextEvent();
                       if (e != NULL) {
						   handleEvent(e, &fds);
						   // reschedule the event
						   evnt->reschedNextEvent(e);
						   e = NULL;
                       }
                       break;

                   default:
                       throw Error("unknown signal");
				}
                evnt->getNextEventTime();
            }

            if (!pprocThread) {
				proc->handleFDEvent(&retEvents, NULL,NULL, NULL);
            }

            // schedule events
            if (retEvents.size() > 0) {
                for (eventVecIter_t iter = retEvents.begin();
                     iter != retEvents.end(); iter++) {

                    evnt->addEvent(*iter);
                }
                retEvents.clear(); 
            }
        } while (!stop);

		// wait for packet processor to handle all remaining packets (if threaded)
		proc->waitUntilDone();

		log->log(ch,"NetQoS going down on Ctrl-C" );

#ifdef DEBUG
		log->dlog(ch,"------- shutdown -------" );
#endif


    } catch (Error &err) {
        
        cout << "error in run() method" << err << endl;
        if (log.get()) { // Logger might not be available yet
            log->elog(ch, err);
        }	   

        // in case an exception happens between get and reschedule event
        if (e != NULL) {
            saveDelete(e);
        }

        throw err;
    }
    catch (...){
		cout << "error in run() method" << endl;
	}
}

/* ------------------------- dump ------------------------- */

void QualityManager::dump(ostream &os)
{
    /* FIXME to be implemented */
    os << "dump" << endl;
}


/* ------------------------- operator<< ------------------------- */

ostream& operator<<(ostream &os, QualityManager &obj)
{
    obj.dump(os);
    return os;
}

/* ------------------------ signal handler ---------------------- */

void QualityManager::sigint_handler(int i)
{
    char c = 'S';

    write(s_sigpipe[1], &c,1);
}

void QualityManager::sigusr1_handler(int i)
{
    char c = 'D';
    
    write(s_sigpipe[1], &c,1);
}

void QualityManager::exit_fct(void)
{
    unlink(NETMATE_LOCK_FILE.c_str());
}

void QualityManager::sigalarm_handler(int i)
{
    g_timeout = 1;
}

/* -------------------- alreadyRunning -------------------- */

int QualityManager::alreadyRunning()
{
    FILE *file;
    char cmd[128];
    struct stat stats;
    int status, oldPid;

	cout << NETQOS_LOCK_FILE.c_str() << endl;

    // do we have a lock file ?
    if (stat(NETQOS_LOCK_FILE.c_str(), &stats ) == 0) { 
		
		
		
        // read process ID from lock file
        file = fopen(NETQOS_LOCK_FILE.c_str(), "rt" );
        if (file == NULL) {
            throw Error("cannot open old pidfile '%s' for reading: %s\n",
                        NETQOS_LOCK_FILE.c_str(), strerror(errno));
        }
        fscanf(file, "%d\n", &oldPid);
        fclose(file);

        // check if process still exists
        sprintf( cmd, "ps %d > /dev/null", oldPid );
        status = system(cmd);

        // if yes, do not start a new meter
        if (status == 0) {
            return 1;
        }

        // pid file but no meter process ->meter must have crashed
        // remove (old) pid file and proceed
        unlink(NETQOS_LOCK_FILE.c_str());
    }
	
	cout << NETQOS_LOCK_FILE.c_str() << endl;
	
    // no lock file and no running meter process
    // write new lock file and continue
    file = fopen(NETQOS_LOCK_FILE.c_str(), "wt" );
    if (file == NULL) {
        throw Error("cannot open pidfile '%s' for writing: %s\n",
                    NETQOS_LOCK_FILE.c_str(), strerror(errno));
    }
    
    cout << NETQOS_LOCK_FILE.c_str() << endl;
    
    fprintf(file, "%d\n", getpid());
    fclose(file);

    return 0;
}

