
/*! \file  tbf.c

    Copyright 2014-2015 Universidad de los Andes, Bogota, Colombia.

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
    action module for setting  up classes and queue disciplines

    $Id: tbf.c 748 2015-03-09 08:35:00 amarentes $
*/

#include "config.h"
#include <stdio.h>
#include "arpa/inet.h"
#include <sys/types.h>
#include <time.h>
#include <iostream>

#include "ProcError.h"
#include "ProcModule.h"
#include "TcNetqosErrorCode.h"



const int COUNTCHUNK = 20;    /* new entries per realloc */

/*
  defines timer for making 'counter snapshot'
*/

timers_t timers[] = {
    /* handle, ival_msec, flags */
    {       1,  1000 * 0, TM_RECURRING },
    /*         ival == 0 means no timeout between collections by default */
    TIMER_END
};


/* per task flow record information */

typedef struct {
	int priority;
    timers_t currTimers[ sizeof(timers) / sizeof(timers[0]) ];
} accData_t;


struct timeval zerotime = {0,0};


void initModule(configParam_t *params)
{
    std::cout << "priority init module" << std::endl;
}


void destroyModule( configParam_t *params )
{
    std::cout << "priority destroy module" << std::endl;
}


void initFlowSetup( int rule_id, int action_id, configParam_t *params, filterList_t *filters, void **flowdata )
{
	accData_t *data;

    data = (accData_t *) malloc( sizeof(accData_t) );

    if (data == NULL )
        throw ProcError(NET_TC_PARAMETER_ERROR,
							"TBF Flow init - allocation flow data error");


    /* copy default timers to current timers array for a specific task */
    memcpy(data->currTimers, timers, sizeof(timers));

    while (params[0].name != NULL) {
        if (!strcmp(params[0].name, "Priority")) {
            int _priority= atoi(params[0].value);
            if (_priority > 0) {
				data->priority = _priority;
#ifdef DEBUG
		fprintf( stderr, "priority module: setting priority to %d \n", _priority );
#endif
            }
        }

        if (!strcmp(params[0].name, "Duration")) {
            int _interval= atoi(params[0].value);
            if (_interval > 0) {
                data->currTimers[0].ival_msec = 1000 * _interval;
#ifdef DEBUG
		fprintf( stderr, "priority module: setting rate to %d in secs\n", _interval );
#endif
            }
        }

        params++;
    }

    *flowdata = data;

#ifdef DEBUG
	std::cout << "priority init flow setup" << std::endl;
#endif

}


void resetFlowSetup( configParam_t *params )
{
	std::cout << "priority reset flow setup" << std::endl;
}


/*! \short  check if bandwidth available for the rule is enought.

    \arg \c params - rule parameters
    \returns 0 - on success (bandwidth is valid), <0 - else
*/
int checkBandWidth( configParam_t *params )
{

    uint64_t rate;
    int numparams = 0;

    while (params[0].name != NULL) {

        if (!strcmp(params[0].name, "Rate")) {
            rate = parseLong(params[0].value);
			numparams++;
			break;
        }
        params++;
     }

	if (numparams == 1){
		if (( bandwidth_available - rate ) >= 0)
			return 0;
		else
			return NET_TC_RATE_AVAILABLE_ERROR;
	}

	return NET_TC_RATE_AVAILABLE_ERROR;
}

void destroyFlowSetup( int rule_id, int action_id, configParam_t *params, filterList_t *filters, void *flowdata )
{
	accData_t *data = (accData_t *)flowdata;
	free( data );

#ifdef DEBUG
	std::cout << "priority destroy flow setup" << std::endl;
#endif

}


const char* getModuleInfo(int i)
{
    /* fprintf( stderr, "count : getModuleInfo(%d)\n",i ); */

    switch(i) {
    case I_MODNAME:    return "bandwidth";
    case I_VERSION:    return "0.1";
    case I_CREATED:    return "2015/03/09";
    case I_MODIFIED:   return "2015/03/09";
    case I_BRIEF:      return "rules to setup bandwidth ";
    case I_VERBOSE:    return "rules to setup bandwidth ";
    case I_HTMLDOCS:   return "http://www.uniandes.edu.co/... ";
    case I_PARAMS:     return "Priority[int] : priority to setup";
    case I_RESULTS:    return "results description = ...";
    case I_AUTHOR:     return "Andres Marentes";
    case I_AFFILI:     return "Universidad de los Andes, Colombia";
    case I_EMAIL:      return "la.marentes455@uniandes.edu.co";
    case I_HOMEPAGE:   return "http://homepage";
    default: return NULL;
    }
}


char* getErrorMsg( int code )
{
    std::cout << "priority get error message" << std::endl;
    return NULL;
}


void timeout( int timerID, void *flowdata )
{
	std::cout << "priority time out" << std::endl;
}


timers_t* getTimers( void *flowdata )
{
	std::cout << "priority get timers" << std::endl;
	accData_t *data = (accData_t *)flowdata;

    /* return timers record if ival > 0, else return "no timers available" (NULL) */
    return (data->currTimers[0].ival_msec > 0) ? data->currTimers : NULL;
}
