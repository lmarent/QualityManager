/*
 * Test the Quality Manager class.
 *
 * $Id: QualityManager_test.cpp 2016-11-07 9:35:00 amarentes $
 * $HeadURL: https://./test/QualityManager_test.cpp $
 */
#include <cppunit/TestCase.h>
#include <cppunit/extensions/HelperMacros.h>
#include <string>
#include <fstream>
#include <streambuf>

#include "ParserFcts.h"
#include "Event.h"
#include "QualityManager.h"

class QualityManager_Test;


/*
 * We use a subclass for testing and make the test case a friend. This
 * way the test cases have access to protected methods. 
 */
class qualitymanager_test : public QualityManager {
  public:
	qualitymanager_test( int _argc, char *_argv[] )
		: QualityManager( _argc, _argv)  { }

	friend class QualityManager_test;
};


class QualityManager_test : public CppUnit::TestFixture {

	CPPUNIT_TEST_SUITE( QualityManager_test );

    CPPUNIT_TEST( test );
	CPPUNIT_TEST_SUITE_END();

  public:
	void setUp();
	void tearDown();
	void test();
	

  private:
    
	qualitymanager_test *qualitymanagerPtr;
    
};

#ifndef ENABLE_THREADS

CPPUNIT_TEST_SUITE_REGISTRATION( QualityManager_test );

#endif

void QualityManager_test::setUp() 
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
        // start up the netqos (this blocks until Ctrl-C !)
        qualitymanagerPtr = new qualitymanager_test(argc, argv);
        
    } catch (Error &e) {
        cout << "Terminating Quality Manager on error: " << e.getError() << endl;
    }
				
}

void QualityManager_test::tearDown() 
{
	try
    {
        if (qualitymanagerPtr != NULL)
            saveDelete(qualitymanagerPtr);
    } catch (Error &e) {
        cout << "Terminating Quality Manager on error: " << e.getError() << endl;
    }
}

void QualityManager_test::test() 
{
    
    Event * evt = NULL;
    
	try
	{

        // going into main loop
         if (qualitymanagerPtr != NULL){
             
            cout << "Log level configured is:" << qualitymanagerPtr->log->getLogLevel() << endl; 
             
			evt = qualitymanagerPtr->evnt.get()->getNextEvent();
			
			// Verifies that a new add auctions event was generated			
			AddRulesEvent *aae = dynamic_cast<AddRulesEvent *>(evt);
			CPPUNIT_ASSERT( aae != NULL );
			
			// Process the add rules event.
			qualitymanagerPtr->handleEvent(evt, NULL);
			
            // delete the memory allocated to this event.
            qualitymanagerPtr->evnt.get()->reschedNextEvent(evt);
            
			// Verifies the number of rules
			CPPUNIT_ASSERT( qualitymanagerPtr->rulm->getNumRules() == 1);
			
			evt = qualitymanagerPtr->evnt.get()->getNextEvent();

			// Verifies that a new active rules event was generated			
			ActivateRulesEvent *are = dynamic_cast<ActivateRulesEvent *>(evt);
			CPPUNIT_ASSERT( are != NULL );
            
			// Process the active rules event.
			qualitymanagerPtr->handleEvent(evt, NULL);

            // delete the memory allocated to this event.
            qualitymanagerPtr->evnt.get()->reschedNextEvent(evt);
            
			// Verifies the number of rules
			CPPUNIT_ASSERT( qualitymanagerPtr->proc->getNumRules() == 1);

			RemoveRulesEvent *rre = NULL;
            evt = qualitymanagerPtr->evnt.get()->getNextEvent();
			while ( evt != NULL ){
                
                rre = dynamic_cast<RemoveRulesEvent *>(evt);
                if (rre != NULL)
                {
                    break;
                }
                else
                {
                    cout << eventNames[evt->getType()].c_str() << endl;
                    // delete the memory allocated to this event.
                    qualitymanagerPtr->evnt.get()->reschedNextEvent(evt);
                    evt = qualitymanagerPtr->evnt.get()->getNextEvent();                     
                }
			}			

            CPPUNIT_ASSERT( rre != NULL );
            
            qualitymanagerPtr->handleEvent(evt, NULL);
            
            CPPUNIT_ASSERT( qualitymanagerPtr->proc->getNumRules() == 0);
            CPPUNIT_ASSERT( qualitymanagerPtr->rulm->getNumRules() == 0);
            
		 }
		
	} catch(Error &e){
		std::cout << "Error:" << e.getError() << std::endl << std::flush;
	}


}