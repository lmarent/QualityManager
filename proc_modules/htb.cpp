
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
#include "arpa/inet.h"
#include <sys/types.h>
#include <time.h>     
#include <iostream>

#include "htb_functions.h"
#include "ProcError.h"
#include "ProcModule.h"


const int COUNTCHUNK = 20;    /* new entries per realloc */
const int MOD_INIT_REQUIRED_PARAMS = 3;
const int MOD_INI_FLOW_REQUIRED_PARAMS = 6;
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

typedef enum { 
	TC_FILTER_ADD = 0,
	TC_FILTER_DELETE
} TcFilterAction_e;

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

int string_to_number(const char *s, unsigned int min, unsigned int max,
		     unsigned int *ret)
{
    long number;
    char *end;
  
    /* Handle hex, octal, etc. */
    errno = 0;
    number = strtol(s, &end, 0);
    if ((*end == '\0') && (end != s)) {
		cout << "receive a number:" << number << endl;
		/* we parsed a number, let's see if we want this */
		if ((errno != ERANGE) && ((long)min <= number) && (number <= (long)max)) {
			*ret = number;
			return 0;
		}
    }
    return -1;
}     

u_int16_t parse_protocol(const char *s)
{
    unsigned int proto;
  
    if (string_to_number(s, 0, 255, &proto) == -1) {
		struct protoent *pent;
	 
		if ((pent = getprotobyname(s)))
			proto = pent->p_proto;
		else {
			unsigned int i;
			for (i = 0; i < sizeof(chain_protos)/sizeof(struct pprot); i++) {
			if (strcmp(s, chain_protos[i].name) == 0) {
				proto = chain_protos[i].num;
				break;
			}
			}
			if (i == sizeof(chain_protos)/sizeof(struct pprot))
			throw ProcError(NET_TC_PARAMETER_ERROR, 
							"unknown protocol `%s' specified", s);
		}
    }
  
    return (u_int16_t)proto;
}                           

u_int16_t parse_protocol(FilterValue &value)
{
	uint16_t proto;
	string ftype = value.getType();
	
	if ((ftype == "UInt8") || (ftype =="Int8")) 
	{
		proto = (uint16_t) (value.getValue())[0];
	}
	else if (ftype == "UInt16")
	{
		proto = ntohs(*((unsigned short *) value.getValue()));
	}
	else if (ftype == "Int16"){
		proto = ntohs(*((short *) value.getValue()));
	}
	else
	{
		proto = parse_protocol(reinterpret_cast<const char*> (value.getValue()));
	}
	cout << "Protocol converted:" << proto << endl;
	return proto;
}                           



void initModule( configParam_t *params )
{
    
     sk = NULL;
     link_cache = NULL;
     nllink = NULL;
     int err;
     double rate;
     std::string infc;
     uint32_t burst;
     int numparams = 0;
	 int link_int = 0;

     while (params[0].name != NULL) {
		// in all the application we establish the rates and 
		// burst parameters in bytes
        if (!strcmp(params[0].name, "Rate")) {
            rate = parseLong(params[0].value);
			numparams++;
#ifdef DEBUG
		fprintf( stdout, "htb module: setting rate to   %lf \n", rate );
#endif
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
			// in all the application we establish the rates and 
			// burst parameters in bytes.
            burst = (uint32_t) parseInt( params[0].value );
            numparams++;
#ifdef DEBUG
			fprintf( stdout, "htb module: Burst %u \n", burst );
#endif
        }

        params++;
     }

#ifdef DEBUG
	fprintf( stdout, "htb module: number of parameters given: %d \n", numparams );
#endif

	 if ( numparams == MOD_INIT_REQUIRED_PARAMS ){
		 
		 /* 1. Establish the socket and a cache 
		  *  			to list interfaces and other data */
		 sk = nl_socket_alloc();
		 if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0)
			throw ProcError(err, "Unable to connect socket");

		 if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache))< 0)
			throw ProcError(err, "Unable to allocate cache");
		 
		 nl_cache_mngt_provide(link_cache);

		 link_int = rtnl_link_name2i(link_cache, infc.c_str());
		 nllink = rtnl_link_get(link_cache, link_int);
		 if (nllink == NULL)
			throw ProcError(NET_TC_PARAMETER_ERROR, "Invalid Interface");
		
		 err = qdisc_add_root_HTB(sk, nllink);
		 if (err == 0){


			err = class_add_HTB_root(sk, nllink, rate, rate, burst, burst);
			if (err != 0)
				throw ProcError(err, "Error creating the HTB root");

			err = class_add_HTB(sk, nllink, NET_DEFAULT_CLASS, 1, 1,
							 1, 1, 1000);
			if (err != 0)
				throw ProcError(err, "Error creating the default root class");


		 }
		 else
			throw ProcError(err, "Error creating the root qdisc");
			 
     } 
     else
		 throw ProcError(NET_TC_PARAMETER_ERROR, 
					"htb init module - not enought parameters");
     
}


void destroyModule()
{
    if ((sk != NULL) and (nllink != NULL))
		qdisc_delete_root_HTB(sk, nllink);
    
    if (link_cache != NULL)
		nl_cache_free(link_cache);
	
	if (sk != NULL) 
		nl_socket_free(sk);

#ifdef DEBUG
	fprintf( stdout, "HTB destroy module" );
#endif
    
}

int calculateRelativeOffSet( refer_t refer)
{
	int val_return = 0;
	
	switch (refer)
	{
		case MAC:
		case IP:
		case TRANS:
		case DATA:
			val_return = offset_refer[refer];
			break;
		default:
			val_return = 0;
	}
	
	return val_return;
}

// If refer is more than or equal to TRANS then it 
// has to put offsetmask in -1 ( following header in the packet )
int calculateRelativeMaskOffSet(refer_t refer )
{
	int val_return = 0;
	
	switch (refer)
	{
		case MAC:
		case IP:
			val_return = 0;
			break;
		case TRANS:
		case DATA:
			val_return = -1;
			break;
		default:
			val_return = 0;
	}

	return val_return;

}

inline void resetCurrent( accData_t *data )
{

}

void modify_filter( int flowId, filterList_t *filters, 
					int bidir, TcFilterAction_e action )
{
	int err = 0;
	uint32_t prio = 1; // TODO AM: we need to create a function that 
					   //		   takes the protocol and return the prio.
	struct rtnl_cls *cls;

#ifdef DEBUG
	fprintf( stdout, "In Creating Filter" );
#endif
	
	if (filters == NULL)
		throw ProcError(NET_TC_PARAMETER_ERROR, "Filters given are null");
		
	// Allocate the new classifier.
	err = create_u32_classifier(sk, nllink, &cls, prio, 
								NET_ROOT_HANDLE_MAJOR, 0,
								NET_ROOT_HANDLE_MAJOR, flowId);
								
	if ( err != NET_TC_SUCCESS )
		throw ProcError(err, "Error allocating classifier objec");

	
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
			
				for ( int index = 0; index < filter.cnt; index++)
				{
					 cout << "Filter lenght:" << filter.len << "Offset:" << filter.offs << endl;
					 cout << "refer:" << filter.refer << "rrefer:" << filter.refer << endl;
					 cout << "roffset:" << filter.roffs << "rname:" << filter.rname << endl;
					 
					 int offset = filter.offs + calculateRelativeOffSet(filter.refer);
					 int roffset = filter.roffs + calculateRelativeOffSet(filter.refer);
					 
					 int maskoffset = calculateRelativeMaskOffSet(filter.refer);

					 cout << "offset:" << offset << "name:" << filter.name << endl;
					 					 
					 err = u32_add_key_filter(cls, (filter.value[index]).getValue(),
									(filter.mask).getValue(), filter.len,
									offset, maskoffset);
					 
					 if ( err == NET_TC_CLASSIFIER_SETUP_ERROR)
					 {
						goto fail;
					 }
					 else
					 {
						if ((bidir == 1) 
									and (filter.rname.length() > 0))
						{
							err = u32_add_key_filter(cls, (filter.value[index]).getValue(),
									(filter.mask).getValue(), filter.len,
									roffset, maskoffset);
							
							if ( err == NET_TC_CLASSIFIER_SETUP_ERROR)
								goto fail;
						}
					 }
				}
				
				break;						

			case FT_RANGE:
				// TODO AM: Not implemented yet.
				break;
			case FT_WILD:
				// TODO AM : Not implemented yet.
				break;
		}			  
	}
	
	if (action == TC_FILTER_ADD)
	{
		err = save_add_u32_filter(sk, cls);
		if ( err == NET_TC_CLASSIFIER_ESTABLISH_ERROR )
			goto fail;
		else
			goto ok;
	}
	else 
	{
		err = save_delete_u32_filter(sk, cls);
		if ( err == NET_TC_CLASSIFIER_ESTABLISH_ERROR )
			goto fail;
		else
			goto ok;
	}

fail:
    if (cls != NULL)
		rtnl_cls_put(cls);
		
	throw ProcError(err, "Error setting up Filters");

ok:
	err = 0;
#ifdef DEBUG
	fprintf( stdout, "Filters sucessfully setup" );
#endif	

}

	


void initFlowSetup( configParam_t *params, 
					filterList_t *filters, void **flowdata)
{

    accData_t *data;
    int err;
    uint64_t rate;
    int duration;
    uint32_t burst;
    uint32_t flowId;
    uint32_t priority;
    int numparams = 0;
    int bidir = 0;
    

    data = (accData_t *) malloc( sizeof(accData_t) );

    if (data == NULL ) 
        throw ProcError(NET_TC_PARAMETER_ERROR, 
							"HTB Flow init - allocation flow data error");

    /* copy default timers to current timers array for a specific task */
    memcpy(data->currTimers, timers, sizeof(timers));
    

    while (params[0].name != NULL) {
		
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

        if (!strcmp(params[0].name, "Bidir")) {
            bidir = (uint32_t) parseBool( params[0].value );
            numparams++;
        }

        params++;
     }

#ifdef DEBUG
		fprintf( stdout, "htb module: number of parameters given: %d \n", numparams );
#endif


	 if ( numparams == MOD_INI_FLOW_REQUIRED_PARAMS ){
		 err = class_add_HTB(sk, nllink, flowId, rate, rate,
							 burst, burst, priority);
		
	     if ( err == NET_TC_SUCCESS ){
			data->currTimers[0].ival_msec = 1000 * duration;
			modify_filter(flowId, filters, bidir, TC_FILTER_ADD);
			*flowdata = data;
		 } 
		 else
			throw ProcError(err, "Error adding HTB class");
			
     } 
     else
		 throw ProcError(NET_TC_PARAMETER_ERROR, 
							"HTB Flow init - not enought parameters");
}


void resetFlowSetup( configParam_t *params )
{
    // NOT implemented, the user have to destroy and recreate the flow.
#ifdef DEBUG
		fprintf( stdout, "init resetFlowSetup \n" );
#endif
}


void destroyFlowSetup( configParam_t *params, 
					   filterList_t *filters,	
					   void *flowdata )
{
	accData_t *data = (accData_t *)flowdata;
    uint32_t flowId;
    int numparams = 0;
    int err;
    int bidir = 0;

#ifdef DEBUG
		fprintf( stdout, "destroy FlowSetup \n" );
#endif
    
	free( data );

    while (params[0].name != NULL) {
		fprintf( stdout, "Evaluating parameter %s \n", params[0].name ); 

        if (!strcmp(params[0].name, "FlowId")) {
            flowId = (uint32_t) parseInt( params[0].value );
            numparams++;
        }

        params++;
    }
#ifdef DEBUG
		fprintf( stdout, "Flow Id to delete: %d \n", flowId );
#endif

	if ( numparams == MOD_DEL_FLOW_REQUIRED_PARAMS )
	{

		 modify_filter(flowId, filters, bidir,  TC_FILTER_DELETE);

		 err = class_delete_HTB(sk, nllink, flowId);
		 if (err != 0 )
			throw ProcError(err, "Error deleting HTB class"); 
		
    } 
    else
		 throw ProcError(NET_TC_PARAMETER_ERROR, 
							"HTB Flow destroy - not enought parameters"); 

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


void timeout( int timerID, void *flowdata )
{
	std::cout << "htb timeout" << std::endl;
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
