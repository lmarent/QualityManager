
/*! \file  htb.cpp

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
    action module for setting up bandwidth on a interface for a flow.

    $Id: htb.cpp 748 2015-03-09 11:22:00 amarentes $
*/

#include "config.h"
#include <stdio.h>
#include "ProcModule.h"
#include "arpa/inet.h"
#include "htb_functions.h"
#include <sys/types.h>
#include <time.h>     
#include <iostream>

const int COUNTCHUNK = 20;    /* new entries per realloc */
const int MOD_INIT_REQUIRED_PARAMS = 3;
const int MOD_INI_FLOW_REQUIRED_PARAMS = 5;
const int MOD_DEL_FLOW_REQUIRED_PARAMS = 1;


struct nl_sock *sk;
struct nl_cache *link_cache;
struct rtnl_link *nllink;
struct rtnl_cls *cls;

enum def_parameters {
    defp_srcipmask,
    defp_dstipmask,
    defp_prot,
    defp_iniface,
    defp_outiface,
    defp_nlgroup,
    defp_cprange,
    defp_prefix,
    defp_qthreshold,
    defp_match,
    defp_bidir
};

static const char *param_names[] = {
    ( "srcip" ),
    ( "dstip" ),
    ( "proto" ),
    ( "iiface" ),
    ( "oiface" ),
    ( "ulog-nlgroup" ),
    ( "ulog-cprange" ),
    ( "ulog-prefix" ),
    ( "ulog-qthreshold" ),
    ( "match" ),
    ( "bidir" ),
    ( 0 )
};

/* mapping table, map mcl parameter to netfilter modules paramter */

struct opt_map {
    const char *mcl;
    const char *iptables;
};

static const struct opt_map option_map[] = {
    { "srcport", "sport" },
    { "dstport", "dport" },
    { "snaplen", "ulog-cprange" },
    { "qthresh", "ulog-qthreshold" },
    { 0 }
};

/* A few hardcoded protocols for 'all' and in case the user has no
   /etc/protocols */
struct pprot {
    const char *name;
    u_int8_t num;
};

static const struct pprot chain_protos[] = {
    { "tcp", IPPROTO_TCP },
    { "udp", IPPROTO_UDP },
    { "icmp", IPPROTO_ICMP },
    { "esp", IPPROTO_ESP },
    { "ah", IPPROTO_AH },
    { "all", 0 },
};

/*
  defines timer for making 'rule remove'
*/

timers_t timers[] = {
    /* handle, ival_msec, flags */
    {       1,  1000 * 0, TM_RECURRING },
    /*         ival == 0 means no timeout by default */
    TIMER_END
};


/* per task flow record information */

typedef struct {
	long double rate;
    timers_t currTimers[ sizeof(timers) / sizeof(timers[0]) ];
} accData_t;


struct timeval zerotime = {0,0};


int initModule( configParam_t *params )
{
    
     sk = NULL;
     link_cache = NULL;
     nllink = NULL;
     int err;
     double rate;
     std::string infc;
     uint32_t burst;
     int numparams = 0;


     while (params[0].name != NULL) {
		fprintf( stdout, "Evaluating parameter %s \n", params[0].name ); 
		
        if (!strcmp(params[0].name, "Rate")) {
            double _rate= atol(params[0].value);
            if (_rate > 0) {
				rate = _rate;
				numparams++;
#ifdef DEBUG
		fprintf( stdout, "htb module: setting rate to   %lf \n", _rate );
#endif
            }
        }

        if (!strcmp(params[0].name, "NetInterface")) {
            std::string intfcName= params[0].value;
            if (intfcName != "") {
                infc = intfcName;
                numparams++;
#ifdef DEBUG
		fprintf( stdout, "htb module: Interface name %s \n", infc.c_str() );
#endif
            }
        }

        if (!strcmp(params[0].name, "Burst")) {
            uint32_t _burst = (uint32_t) atoi(params[0].value);
            if ( _burst > 0 ) {
                burst = _burst;
                numparams++;
#ifdef DEBUG
		fprintf( stdout, "htb module: Burst %u \n", _burst );
#endif
            }
        }

        params++;
     }

#ifdef DEBUG
		fprintf( stdout, "htb module: number of parameters given: %d \n", numparams );
#endif

	 if ( numparams == MOD_INIT_REQUIRED_PARAMS ){
		 /* 1. Establish the socket and a cache to list interfaces and other data */
		 sk = nl_socket_alloc();
		 if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0) {
			nl_perror(err, "Unable to connect socket");
			return err;
		 }

		 if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache))< 0)
		 { 
			nl_perror(err, "Unable to allocate cache");
			return err;
		 }
		 
		 nl_cache_mngt_provide(link_cache);

		 int link_int = rtnl_link_name2i(link_cache, infc.c_str());
		 nllink = rtnl_link_get(link_cache, link_int);
		 if (nllink == NULL){
			   
			fprintf(stdout,"The interface does not exist");       
		 }
		
		 err = qdisc_add_root_HTB(sk, nllink);
		 if (err == 0){
			err = class_add_HTB_root(sk, nllink, rate, rate, burst, burst);
		 }
		 else
		 {
			 std::cout << "htb init module: error:" << err<< std::endl;
		 }
		 std::cout << "htb init module" << std::endl;
		 return err;
     } 
     else{
		 std::cout << "htb init module - not enought parameters" << std::endl;
		 return NET_TC_PARAMETER_ERROR;
	 }
     
}


int destroyModule()
{
    if ((sk != NULL) and (nllink != NULL))
		qdisc_delete_root_HTB(sk, nllink);
    
    if (link_cache != NULL)
		nl_cache_free(link_cache);
	
	if (sk != NULL) 
		nl_socket_free(sk);

    
    std::cout << "htb destroy module" << std::endl;
    return 0;
}


inline void resetCurrent( accData_t *data )
{

}

int create_filter( int flowId, filterList_t *filters )
{
	int err = 0;
	cout << "In Creating Filter" << std::endl;
	
	if (filters == NULL){
		cout << "Given Filters are null" << endl;
		return NET_TC_PARAMETER_ERROR;
	}	
	
	struct rtnl_cls *cls;
	
	err = create_u32_classifier(struct nl_sock *sock, 
	  					  struct rtnl_link *rtnlLink, 
	  					  &cls,		 
						  uint32_t prio, 
						  uint32_t parentMaj, 
						  uint32_t parentMin);
	
	if ( err != NET_TC_SUCCESS ){
		cout << "Error allocating classifier object" << endl;
		return NET_TC_PARAMETER_ERROR;
	}
	
	filterListIter_t iter;
	for ( iter = filters->begin() ; iter != filters->end() ; iter++ ) 
	{	
		filter_t filter = *iter;
		
		cout << "name:" << filter.name << endl;
		cout << "offs:" << filter.offs << endl;
		cout << "roffs:" << filter.roffs << endl;
		cout << "len:" << filter.len << endl;
		cout << "cnt:" << filter.cnt << endl;
		cout << "filterType:" << filter.mtype << endl;
		
		/*
		invert = check_for_invert(params[i], &param);
		if (invert)
			optind++;
		*/
				
		// set_inverse(defp_srcipmask, &fw.ip.invflags, invert);
		switch (filter.mtype)
		{
			case FT_EXACT:
			case FT_SET:
			{
				for ( int index = 0; i < filter.cnt; i++)
				{
					if (!strcmp( filter.name, param_names[defp_prot])
						 cout << "Protocol:" << filter.value[i]).getValue() << endl;
						 break;
					
					u32_add_key_filter(cls, (filter.value[i]).getValue(),
									(filter.mask).getValue(),
									filter.len,
									filter.offs,
									0);
					
					if ( err == NET_TC_CLASSIFIER_SETUP_ERROR)
						goto fail;
				}
				break;
										
			}
			case FT_RANGE:
			{
				// TODO AM: Not implemented yet.
				break;
			}
			case FT_WILD:
			{
				// TODO AM : Not implemented yet.
				break;
			}
		}			  
	}
	
	err = save_u32_filter(cls);
	if ( err == NET_TC_CLASSIFIER_ESTABLISH_ERROR )
		goto fail;
	else
		return NET_TC_SUCCESS;

fail:
    if (cls != NULL) ;
		rtnl_cls_put(cls);

	return err;
	
}



int initFlowSetup( configParam_t *params, filterList_t *filters, void **flowdata)
{

    accData_t *data;
    int err;
    uint64_t rate;
    int duration;
    uint32_t burst;
    uint32_t flowId;
    uint32_t priority;
    int numparams = 0;
    

    data = (accData_t *) malloc( sizeof(accData_t) );

    if (data == NULL ) {
        return NET_TC_PARAMETER_ERROR;
    }

    /* copy default timers to current timers array for a specific task */
    memcpy(data->currTimers, timers, sizeof(timers));
    

    while (params[0].name != NULL) {
		fprintf( stdout, "Evaluating parameter %s \n", params[0].name ); 
		
        if (!strcmp(params[0].name, "Rate")) {
            rate = parseLong(params[0].value);
			numparams++;
        }

        if (!strcmp(params[0].name, "Duration")) {
            duration = parseInt( params[0].value );
            numparams++;
        }

        if (!strcmp(params[0].name, "FlowId")) {
            flowId = (uint32_t) parseInt( params[0].value );
            numparams++;
        }

        if (!strcmp(params[0].name, "Burst")) {
            burst = (uint32_t) parseInt( params[0].value );
            numparams++;
        }

        if (!strcmp(params[0].name, "Priority")) {
            priority = (uint32_t) parseInt( params[0].value );
            numparams++;
        }

        params++;
     }

#ifdef DEBUG
		fprintf( stdout, "htb module: number of parameters given: %d \n", numparams );
#endif


	 if ( numparams == MOD_INI_FLOW_REQUIRED_PARAMS )
	 {
		 err = class_add_HTB(sk, nllink, flowId, rate, rate,
							 burst, burst, priority);
		
	     if ( err == NET_TC_SUCCESS )
	     {
			data->currTimers[0].ival_msec = 1000 * duration;
			create_filter(flowId, filters);
			*flowdata = data;
			return NET_TC_SUCCESS;
		 } 
		 else 
		 {
			return err;
		 }
     } 
     else{
		 std::cout << "htb FLOW init - not enought parameters" << std::endl;
		 return NET_TC_PARAMETER_ERROR;
	 }
}


int resetFlowSetup( configParam_t *params )
{
    // NOT implemented, the user have to destroy and recreate the flow.
    return 0;
}


int destroyFlowSetup( configParam_t *params, void *flowdata )
{
	accData_t *data = (accData_t *)flowdata;
    uint32_t flowId;
    int numparams = 0;
    
	free( data );

    while (params[0].name != NULL) {
		fprintf( stdout, "Evaluating parameter %s \n", params[0].name ); 

        if (!strcmp(params[0].name, "FlowId")) {
            int _flowId = atoi(params[0].value);
            if ( _flowId > 0 ) {
                flowId = (uint32_t) _flowId;
                numparams++;
            }
        }

        params++;
    }

	 if ( numparams == MOD_DEL_FLOW_REQUIRED_PARAMS )
	 {
		 return class_delete_HTB(sk, nllink, flowId);  
     } 
     else{
		 std::cout << "htb FLOW init - not enought parameters" << std::endl;
		 return NET_TC_PARAMETER_ERROR;
	 }

	std::cout << "htb destroy flow setup" << std::endl;
	return NET_TC_SUCCESS;
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
    case I_PARAMS:     return "Rate[float] : bandwidth rate to setup";
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
    std::cout << "htb get error message" << std::endl;
    return NULL;
}


int timeout( int timerID, void *flowdata )
{
	std::cout << "htb timeout" << std::endl;
    return 0;
}


timers_t* getTimers( void *flowdata )
{
	std::cout << "htb get timers 1" << std::endl;
	accData_t *data = (accData_t *)flowdata;
	
	if (data == NULL){
		std::cout << "data is null" << std::endl;
		return NULL;
	}
	else{
		std::cout << "htb get timers 2" << data->currTimers[0].ival_msec <<  std::endl;
		/* return timers record if ival > 0, else return "no timers available" (NULL) */
		return (data->currTimers[0].ival_msec > 0) ? data->currTimers : NULL;
	}
}
