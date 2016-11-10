/*
 * Test the QosProcessor class.
 *
 * $Id: QoSProcessor_test.cpp 2016-09-02 8:10:00 amarentes $
 *      This tests are for the non-threaded methods.
 * $HeadURL: https://./test/QoSProcessor_test.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>

#include "RuleManager.h"
#include "EventScheduler.h"
#include "QOSProcessor.h"
#include "Logger.h"
#include "ParserFcts.h"




class QosProcessor_Test : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE( QosProcessor_Test );

	CPPUNIT_TEST( testCheckOkRules );
    CPPUNIT_TEST( testCheckNonOkRules );
    CPPUNIT_TEST( testAddDeleteRules );

	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void tearDown();
    
    void testCheckOkRules();
    void testCheckNonOkRules();    
    void testAddDeleteRules();

  private:

	  ConfigManager *conf = NULL;
      RuleManager *rulem = NULL;
      EventScheduler *evnt = NULL;
	  QOSProcessor *qosProcessorPtr = NULL;
      Logger *log;

      //! logging channel number used by objects of this class
      int ch;
	  
    
};

#ifndef ENABLE_THREADS

CPPUNIT_TEST_SUITE_REGISTRATION( QosProcessor_Test );

#endif


void QosProcessor_Test::setUp() 
{

	try
	{
		string commandLine = "qualityManager";
		char *cstr = new char[commandLine.length() + 1];
		strcpy(cstr, commandLine.c_str());
		
		enum { kMaxArgs = 64 };
		int argc = 0;
		char *argv[kMaxArgs];

		char *p2 = strtok(cstr, " ");
		while (p2 && argc < kMaxArgs-1)
		{
			argv[argc++] = p2;
			p2 = strtok(0, " ");
		}
		argv[argc] = 0;
					
		const string configDTD = DEF_SYSCONFDIR "/netqos_conf.dtd";
		const string configFileName = NETQOS_DEFAULT_CONFIG_FILE;
		conf = new ConfigManager(configDTD, configFileName, argv[0]);

        log = Logger::getInstance(); 	
        ch = log->createChannel("QoSProcessor_test");

        // set logging vebosity level if configured
        string verbosity = conf->getValue("VerboseLevel", "MAIN");
        if (!verbosity.empty()) {
            log->setLogLevel( ParserFcts::parseInt( verbosity, -1, 4 ) );
        }
				

		qosProcessorPtr = new QOSProcessor( conf , 1);


        rulem =  new RuleManager(conf->getValue("FilterDefFile", "MAIN"), 
                            conf->getValue("FilterConstFile", "MAIN"));
  
        evnt = new EventScheduler();
				        
	}
	catch(Error &e){
		cout << "Error:" << e.getError() << endl << flush;
		throw e;
	}
		
}

void QosProcessor_Test::tearDown() 
{

    log->log(ch, "starting teardown");

	// wait for packet processor to handle all remaining packets (if threaded)
	qosProcessorPtr->waitUntilDone();

	// This deletes the config Manager ptr previously allocated.
	if (qosProcessorPtr != NULL)
        delete(qosProcessorPtr);

    log->log(ch, "after deleting qosProcessor");

    // delete the rule manager.
    if (rulem != NULL)
        delete(rulem);
    
    log->log(ch, "after deleting ruleManager");
            
    // delete the event schedule.
    if (evnt != NULL)
        delete(evnt);
        
    log->log(ch, "after deleting evnt scheduler");
}

void QosProcessor_Test::testCheckOkRules()
{
    ruleDB_t *new_rules = NULL;
    
    try
    {
        
        log->log(ch, "starting testCheckOkRules");
        
        const string ruleFile = DEF_SYSCONFDIR "/example_rules1.xml";
        new_rules = rulem->parseRules(ruleFile);
    
        log->log(ch, "after loading the rules");
        qosProcessorPtr->checkRules(new_rules, evnt);
    
    
        ruleDBIter_t it;
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;
            log->log(ch, "Rule %s.%s - Status:%d", rule->getSetName().c_str(), rule->getRuleName().c_str(), (int) rule->getState());        
            CPPUNIT_ASSERT( rule->getState() == RS_VALID );
        }

        // Release the memory created. 
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;
            saveDelete(rule);
        }

        saveDelete(new_rules);
        
        log->log(ch, "ending testCheckOkRules");
        
    }
    catch (Error &e)
    {
        cout << "Error:" << e.getError() << endl;
    }

}


void QosProcessor_Test::testCheckNonOkRules()
{
    ruleDB_t *new_rules = NULL;
    
    try
    {
        
        log->log(ch, "starting testCheckNonOkRules");
        
        const string ruleFile = DEF_SYSCONFDIR "/example_rules2.xml";
        new_rules = rulem->parseRules(ruleFile);
    
        log->log(ch, "numRules %d", (int) new_rules->size() );
    
        qosProcessorPtr->checkRules(new_rules, evnt);
    
        ruleDBIter_t it;
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;      
            log->log(ch, "Rule %s.%s - Status:%d", rule->getSetName().c_str(), rule->getRuleName().c_str(), (int) rule->getState());
            CPPUNIT_ASSERT( rule->getState() == RS_ERROR );
        }


        // Release the memory created. 
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;
            saveDelete(rule);
        }

        saveDelete(new_rules);

        log->log(ch, "ending testCheckNonOkRules");
        
    }
    catch (Error &e)
    {
        cout << "Error:" << e.getError() << endl;
    }

}


void QosProcessor_Test::testAddDeleteRules()
{


    ruleDB_t *new_rules = NULL;
    
    try
    {
        
        log->log(ch, "starting testAddRules");
        
        const string ruleFile = DEF_SYSCONFDIR "/example_rules1.xml";
        new_rules = rulem->parseRules(ruleFile);
    
        log->log(ch, "numRules %d", (int) new_rules->size() );
    
        qosProcessorPtr->checkRules(new_rules, evnt);
        
        log->log(ch, "numRules %d", (int) new_rules->size() );

        ruleDBIter_t it;
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;      
            log->log(ch, "Rule %s.%s - Status:%d", rule->getSetName().c_str(), rule->getRuleName().c_str(), (int) rule->getState());
        }

        
        qosProcessorPtr->addRules(new_rules, evnt);

        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;      
            log->log(ch, "Rule %s.%s - Status:%d", rule->getSetName().c_str(), rule->getRuleName().c_str(), (int) rule->getState());
            CPPUNIT_ASSERT( rule->getState() == RS_ACTIVE );
        }


        qosProcessorPtr->delRules(new_rules, evnt);

        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;      
            log->log(ch, "Rule %s.%s - Status:%d", rule->getSetName().c_str(), rule->getRuleName().c_str(), (int) rule->getState());
            CPPUNIT_ASSERT( rule->getState() == RS_DONE );
        }


        // Release the memory created. 
        for (it = new_rules->begin(); it != new_rules->end(); ++it)
        {
            Rule *rule = *it;
            saveDelete(rule);
        }

        saveDelete(new_rules);

        log->log(ch, "ending testAddRules");
        
    }
    catch (Error &e)
    {
        cout << "Error:" << e.getError() << endl;
    }
    

}

