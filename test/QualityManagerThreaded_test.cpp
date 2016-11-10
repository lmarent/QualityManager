/*
 * Test the Quality Manager Threaded class.
 *
 * $Id: QualityManagerThreaded_test.cpp 2016-11-07 9:35:00 amarentes $
 * $HeadURL: https://./test/QualityManagerThreaded.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <fstream>
#include <streambuf>

#include "ParserFcts.h"
#include "Event.h"
#include "QualityManager.h"

class QualityManagerThreaded_test;


/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods. 
 */
class qualitymanagerthreaded_test : public QualityManager {
  public:
	qualitymanagerthreaded_test( int _argc, char *_argv[] )
		: QualityManager( _argc, _argv)  { }

	friend class QualityManagerThreaded_test;
};


class QualityManagerThreaded_test : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE( QualityManagerThreaded_test );

    CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void tearDown();
	void test();
	

  private:
    
	qualitymanagerthreaded_test *qualitymanagerPtr;
    Logger *log;

    //! logging channel number used by objects of this class
    int ch;
    
};

#ifdef ENABLE_THREADS

CPPUNIT_TEST_SUITE_REGISTRATION( QualityManagerThreaded_test );

#endif

void QualityManagerThreaded_test::setUp() 
{


	string commandLine = "qualityManager -c " DEF_SYSCONFDIR "/netqos_conf.xml";
	
	cout << "commandLine:" << commandLine << endl;
	
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
		
	try
	{
        log = Logger::getInstance(); 	
        ch = log->createChannel("QualityManagerThreaded_test");

        // start up the netqos (this blocks until Ctrl-C !)
        qualitymanagerPtr = new qualitymanagerthreaded_test(argc, argv);
        
    } catch (Error &e) {
        cout << "Terminating Quality Manager on error: " << e.getError() << endl;
    }
				
}

void QualityManagerThreaded_test::tearDown() 
{
	try
    {
        if (qualitymanagerPtr != NULL)
            saveDelete(qualitymanagerPtr);
    } catch (Error &e) {
        cout << "Terminating Quality Manager on error: " << e.getError() << endl;
    }
}

void QualityManagerThreaded_test::test() 
{
    
    Event * evt = NULL;
    // sleep waiting to execute the check method.
    unsigned int microseconds = 1000000;
    eventVec_t eVec;    
    
	try
	{

        // going into main loop
         if (qualitymanagerPtr != NULL){
             
            cout << "Log level configured is:" << qualitymanagerPtr->log->getLogLevel() << endl; 
             
			evt = qualitymanagerPtr->evnt.get()->getNextEvent();
			
			// Verifies that a new add rule event was generated			
			AddRulesEvent *aae = dynamic_cast<AddRulesEvent *>(evt);
			CPPUNIT_ASSERT( aae != NULL );
			
			// Process the add rules event.
			qualitymanagerPtr->handleEvent(evt, NULL);

            // delete the memory allocated to this event.
            qualitymanagerPtr->evnt.get()->reschedNextEvent(evt);

            // Wait to bring back again the response event. 
            usleep(microseconds);
            
            qualitymanagerPtr->proc->handleFDEvent(&eVec, NULL, NULL, NULL);
            
            qualitymanagerPtr->scheduleEvents(&eVec);
			
            CPPUNIT_ASSERT( qualitymanagerPtr->evnt.get()->getNumEvents() == 2 );
            
			evt = qualitymanagerPtr->evnt.get()->getNextEvent();
			
			// Verifies that a new response add event was generated			
			respCheckRulesQoSProcessorEvent *rcrq = dynamic_cast<respCheckRulesQoSProcessorEvent *>(evt);
			CPPUNIT_ASSERT( rcrq != NULL );
            
			// Process the response add rules event.
			qualitymanagerPtr->handleEvent(evt, NULL);

            // Verifies the number of rules
			CPPUNIT_ASSERT( qualitymanagerPtr->rulm->getNumRules() == 1);
            
			evt = qualitymanagerPtr->evnt.get()->getNextEvent();

			// Verifies that a new active rules event was generated			
			ActivateRulesEvent *are = dynamic_cast<ActivateRulesEvent *>(evt);
			CPPUNIT_ASSERT( are != NULL );
            
			// Process the active rules event.
			qualitymanagerPtr->handleEvent(evt, NULL);

            // Wait to bring back again the response event. 
            usleep(microseconds);

            // Bring the response from QoS processor.
            qualitymanagerPtr->proc->handleFDEvent(&eVec, NULL, NULL, NULL);
            
            // Put the response in the event scheduler.
            qualitymanagerPtr->scheduleEvents(&eVec);
            
            // Verifies the number of rules
			CPPUNIT_ASSERT( qualitymanagerPtr->proc->getNumRules() == 1);
            
            // Obtains the next event, which is a response to add rule Qos Processor event

			evt = qualitymanagerPtr->evnt.get()->getNextEvent();

			// Verifies that a new active rules event was generated			
			respAddRulesQoSProcesorEvent *rarq = dynamic_cast<respAddRulesQoSProcesorEvent *>(evt);
			CPPUNIT_ASSERT( rarq != NULL );

			// Process the response add rules event.
			qualitymanagerPtr->handleEvent(evt, NULL);

            evt = qualitymanagerPtr->evnt.get()->getNextEvent();
			while ( evt != NULL )
            {               
                log->log(ch, "EventName: %s", eventNames[evt->getType()].c_str());
                // delete the memory allocated to this event.
                if (evt->getType() == PROC_MODULE_TIMER)
                {
                    evt->setInterval(0);
                }
                else if (evt->getType() == REMOVE_RULES)
                {
                    break;
                }
                
                qualitymanagerPtr->evnt.get()->reschedNextEvent(evt);
                evt = qualitymanagerPtr->evnt.get()->getNextEvent();
			}			
            
            RemoveRulesEvent *rre = dynamic_cast<RemoveRulesEvent *>(evt);
            CPPUNIT_ASSERT( rre != NULL );
            
            qualitymanagerPtr->handleEvent(evt, NULL);

            // Wait to bring back again the response event. 
            usleep(microseconds);

            // Bring the response from QoS processor.
            qualitymanagerPtr->proc->handleFDEvent(&eVec, NULL, NULL, NULL);
            
            // Put the response in the event scheduler.
            qualitymanagerPtr->scheduleEvents(&eVec);
            
            CPPUNIT_ASSERT( qualitymanagerPtr->proc->getNumRules() == 0);

			evt = qualitymanagerPtr->evnt.get()->getNextEvent();

			// Verifies that a new active rules event was generated			
			respDelRulesQoSProcesorEvent *rdrq = dynamic_cast<respDelRulesQoSProcesorEvent *>(evt);
			CPPUNIT_ASSERT( rdrq != NULL );

			// Process the response add rules event.
			qualitymanagerPtr->handleEvent(evt, NULL);

            CPPUNIT_ASSERT( qualitymanagerPtr->rulm->getNumRules() == 0);
               
		 }
		
	} catch(Error &e){
		std::cout << "Error:" << e.getError() << std::endl << std::flush;
	}
}
