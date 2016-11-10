
/*!  \file   QualityManagerComponent.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogotá, Colombia

    This file is part of Network Measurement and Accounting System (NETQoS).

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
    meter component base class, provides common functionality
    for all meter components (e.g. threads, fd handling, ...)

    $Id: QualityManagerComponent.cpp 748 2015-03-05 18:25:00 amarentes $
*/

#include "QualityManagerComponent.h"

QualityManagerComponent::QualityManagerComponent(ConfigManager *_cnf, string name, int thread )
    :   running(0), cname(name), threaded(thread),  cnf(_cnf)
{
    log  = Logger::getInstance();
    ch   = log->createChannel( name );
    perf = PerfTimer::getInstance();

#ifdef ENABLE_THREADS
    if (threaded) {
        mutexInit(&maccess);
		threadCondInit(&doneCond);

    log->log(ch, "Created the component as another thread");
    }
#endif

}


QualityManagerComponent::~QualityManagerComponent()
{
    log->dlog(ch, "finishing QualityManagerComponent");
    
#ifdef ENABLE_THREADS
    if (threaded) {
        mutexLock(&maccess);
        stop();
        mutexUnlock(&maccess);
        log->dlog(ch, "waiting done work for thread destroy");
		threadCondDestroy(&doneCond);
        log->dlog(ch,"to destroy mutex");
        mutexDestroy(&maccess);
    }
#endif
}


void QualityManagerComponent::run(void)
{
#ifdef ENABLE_THREADS
    if (threaded && !running) {
		int res = threadCreate(&thread, thread_func, this);
		if (res != 0) {
			throw Error("Cannot create thread within %s: %s",
				cname.c_str(), strerror(res));
		}
		running = 1;
    }
#endif
}


void QualityManagerComponent::stop(void)
{
#ifdef ENABLE_THREADS
    if (threaded && running) {
		threadCancel(thread);
		threadJoin(thread);
		running = 0;
        log->log(ch, "Thread stopped");
    }
#endif
}


void* QualityManagerComponent::thread_func(void *arg)
{
#ifdef ENABLE_THREADS
    // asynch cancel
    threadSetCancelType(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    ((QualityManagerComponent *)arg)->main();
#endif
    return NULL;
}


void QualityManagerComponent::main(void)
{
    // nothing here
}


