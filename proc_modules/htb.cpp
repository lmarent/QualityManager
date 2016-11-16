
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
const int MOD_INIT_REQUIRED_PARAMS = 4;
const int MOD_INI_FLOW_REQUIRED_PARAMS = 6;
const int MOD_DEL_FLOW_REQUIRED_PARAMS = 2;
int64_t bandwidth_available = 0;



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

/* mapping table, map mcl parameter to netfilter modules parameter */

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
    {       1,  1000 * 0, TM_ALIGNED },
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
     double rate = 0;
     std::string infc;
     uint32_t burst = 0;
     int numparams = 0;
	 int link_int = 0;
	 bool useIPv6 = false;
	 uint32_t prio = 1; // TODO AM: we need to create a function that
					   //		   takes the protocol and return the prio.

#ifdef DEBUG
	fprintf( stdout, "htb module: start init module \n");
#endif

     while (params[0].name != NULL) {
		// in all the application we establish the rates and
		// burst parameters in bytes
        if (!strcmp(params[0].name, "Rate")) {
            rate = parseLong(params[0].value);
			numparams++;
#ifdef DEBUG
		fprintf( stdout, "htb module: Rate: %lf \n", rate );
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

        if (!strcmp(params[0].name, "UseIPv6")) {
			// in all the application we establish the rates and
			// burst parameters in bytes.
            useIPv6 = parseBool( params[0].value );
            numparams++;
#ifdef DEBUG
			fprintf( stdout, "htb module: UseIpv6: %s \n", useIPv6 ? "true" : "false");
#endif
        }

        params++;
     }

#ifdef DEBUG
	fprintf( stdout, "htb module: number of parameters given: %d \n", numparams );
#endif

	 if ( numparams == MOD_INIT_REQUIRED_PARAMS ){

         fprintf( stdout, "htb module pass the number of parameters: %d \n", numparams );

		 /* 1. Establish the socket and a cache
		  *  			to list interfaces and other data */
		 sk = nl_socket_alloc();
		 if ((err = nl_connect(sk, NETLINK_ROUTE)) < 0)
			throw ProcError(err, "Unable to connect socket");

		 if ((err = rtnl_link_alloc_cache(sk, AF_UNSPEC, &link_cache))< 0)
			throw ProcError(err, "Unable to allocate cache");

         fprintf( stdout, "after connect and link creation \n");

		 nl_cache_mngt_provide(link_cache);

		 link_int = rtnl_link_name2i(link_cache, infc.c_str());
		 nllink = rtnl_link_get(link_cache, link_int);
		 if (nllink == NULL)
			throw ProcError(NET_TC_PARAMETER_ERROR, "Invalid Interface");

         fprintf( stdout, "after connecting the interface \n");

		 err = qdisc_add_root_HTB(sk, nllink);
		 if (err == 0){

            fprintf( stdout, "after creating the root htb \n");

			uint32_t quantum = 10; // recommended for rates greater than 12kbps
			err = class_add_HTB_root(sk, nllink, rate, rate, burst, burst, quantum);
			if (err != 0)
				throw ProcError(err, "Error creating the HTB root");

			quantum = 1; // recommended for rates less than 12kbps
			err = class_add_HTB(sk, nllink, NET_DEFAULT_CLASS, 1, 1, 1, 1, 1000, quantum);
			if (err != 0)
				throw ProcError(err, "Error creating the default root class");

			if (!useIPv6){
				err = create_hash_configuration(sk, nllink,
									prio, NET_ROOT_HANDLE_MAJOR, 0);
				if (err != 0)
					throw ProcError(err, "Error creating the hash table for classifiers");
			}

			// Initialize the bandwidth available.
			bandwidth_available = rate;

            fprintf( stdout, "ending the htb initialization  \n");

		 }
		 else
			throw ProcError(err, "Error creating the root qdisc");

     }
     else
		 throw ProcError(NET_TC_PARAMETER_ERROR,
					"htb init module - not enought parameters");

}


void destroyModule( configParam_t *params)
{
	 bool useIPv6 = false;
     int numparams = 0;
     int err=0;
	 uint32_t prio = 1; // TODO AM: we need to create a function that
					   //		   takes the protocol and return the prio.

     while (params[0].name != NULL) {



        if (!strcmp(params[0].name, "UseIPv6")) {
			// in all the application we establish the rates and
			// burst parameters in bytes.
            useIPv6 = parseBool( params[0].value );
            numparams++;
//#ifdef DEBUG
			fprintf( stdout, "htb module: UseIpv6 %s \n", useIPv6 ? "true" : "false");
//#endif
        }

        params++;
     }

// #ifdef DEBUG
	fprintf( stdout, "htb module: number of parameters given: %d \n", numparams );
// #endif

	if (!useIPv6){
		if ((sk != NULL) and (nllink != NULL)){
			err = class_delete_HTB(sk, nllink, NET_DEFAULT_CLASS);

			if (err != 0){
// #ifdef DEBUG
                fprintf( stdout, "Error deleting HTB root class\n" );
// #endif
				throw ProcError(err, "Error deleting HTB root class");
			}
			qdisc_delete_root_HTB(sk, nllink);

			// Reinitialize the bandwidth available.
			bandwidth_available = 0;

		}
	}

    if (link_cache != NULL)
		nl_cache_free(link_cache);

	if (sk != NULL)
		nl_socket_free(sk);

// #ifdef DEBUG
	fprintf( stdout, "HTB destroy module \n" );
// #endif

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

int create_hask_key(filterList_t *filters)
{
#ifdef DEBUG
	fprintf( stdout, "htb : init create_hash_key \n" );
#endif


	uint32_t keyval32, hashkey;
	filterListIter_t iter;

	for ( iter = filters->begin() ; iter != filters->end() ; iter++ )
	{
#ifdef DEBUG
        fprintf( stdout, "htb - filter type: %s \n", (iter->type).c_str() );
#endif

		if ( (strcmp((iter->type).c_str(), "IPAddr") == 0) and
		     ((iter->mtype) == FT_EXACT) )
		{

			keyval32 = (uint32_t)(iter->value[0]).getValue()[0] << 24 |
					   (uint32_t)(iter->value[0]).getValue()[1] << 16 |
					   (uint32_t)(iter->value[0]).getValue()[2] << 8  |
					   (uint32_t)(iter->value[0]).getValue()[3];

#ifdef DEBUG
            fprintf( stdout, "htb - IP address as decimal: %d \n", keyval32 );
#endif

			hashkey = (keyval32&0x000000FF);

#ifdef DEBUG
			fprintf( stdout, "htb : hash key value: %d \n", hashkey);
#endif

			return hashkey;
		}
	}

	return -1; // We must to assign the filter to the non hashed table.
}

void modify_filter( int flowId, filterList_t *filters,
					int bidir, TcFilterAction_e action )
{
	int err = 0;
	int hashkey = -1;
	int htid=0;
	uint32_t prio = 1; // TODO AM: we need to create a function that
					   //		   takes the protocol and return the prio.
	struct rtnl_cls *cls = NULL;

#ifdef DEBUG
	fprintf( stdout, "htb: ------------------------  init modify filter" );
#endif

	if (filters == NULL)
		throw ProcError(NET_TC_PARAMETER_ERROR, "Filters given are null");

	// Creates the hash key based on the last byte of the source IP address.
	hashkey = create_hask_key(filters);
	if (hashkey == -1)
	{
		htid = NET_UNHASH_FILTER_TABLE;
		hashkey = 0;
	}
	else
	{
		htid = NET_HASH_FILTER_TABLE;
	}

	fprintf( stdout, "htb: flowId:%d hash table: %d - hashkey %d ", flowId, htid, hashkey );


	if (action == TC_FILTER_ADD)
	{

		// Allocate the new classifier.
		err = create_u32_classifier(sk, nllink, &cls, prio,
											NET_ROOT_HANDLE_MAJOR, 0,
									NET_ROOT_HANDLE_MAJOR, flowId, htid, hashkey );

		if ( err != NET_TC_SUCCESS )
			throw ProcError(err, "Error allocating classifier objec");

		err = rtnl_u32_set_classid(cls, NET_HANDLE(
					(uint32_t) NET_ROOT_HANDLE_MAJOR, (uint32_t) flowId));

		if (err < 0)
			throw ProcError(err, "Error establishing class id for the classifier");

		filterListIter_t iter;
		for ( iter = filters->begin() ; iter != filters->end() ; iter++ )
		{
			filter_t filter = *iter;

	#ifdef DEBUG
			cout << "name:" << filter.name << endl;
			// cout << "offs:" << filter.offs << endl;
			// cout << "roffs:" << filter.roffs << endl;
			// cout << "len:" << filter.len << endl;
			// cout << "cnt:" << filter.cnt << endl;
			// cout << "filterType:" << filter.mtype << endl;
	#endif

			switch (filter.mtype)
			{
				case FT_EXACT:
				case FT_SET:

					for ( int index = 0; index < filter.cnt; index++)
					{

						 int offset = filter.offs + calculateRelativeOffSet(filter.refer);
						 int roffset = filter.roffs + calculateRelativeOffSet(filter.refer);
						 int maskoffset = calculateRelativeMaskOffSet(filter.refer);

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

		err = save_add_u32_filter(sk, cls);
		if ( err == NET_TC_CLASSIFIER_ESTABLISH_ERROR )
			goto fail;
		else
			goto ok;
	}
	else
	{

		// Allocate the new classifier.
		err = delete_u32_classifier(sk, nllink, &cls, prio,
									NET_ROOT_HANDLE_MAJOR, 0,
									NET_ROOT_HANDLE_MAJOR, flowId, htid, hashkey);

		if ( err != NET_TC_SUCCESS ){
            fprintf( stdout, "Error deleting classifier %d \n", err);
			throw ProcError(err, "classifier allocate error during deleting");
        }

		err = save_delete_u32_filter(sk, cls);
		if ( err == NET_TC_CLASSIFIER_ESTABLISH_ERROR ){
			fprintf( stdout, "Error deleting filter %d \n", err);
			goto fail;

		}
		else{
			goto ok;
        }
	}

fail:
    if (cls != NULL){
		rtnl_cls_put(cls);
	}
	throw ProcError(err, "Error setting up Filters");

ok:
	err = 0;
	fprintf( stdout, "htb: end modify filter \n" );

}




void initFlowSetup( int rule_id,
					int action_id, configParam_t *params,
					filterList_t *filters, void **flowdata)
{

    accData_t *data;
    int err;
    int64_t rate = 0;
    int duration = 0;
    uint32_t burst = 0;
    uint32_t flowId = 0;
    uint32_t priority = 0;
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


	 if ( numparams == MOD_INI_FLOW_REQUIRED_PARAMS )
	 {
		 uint32_t quantum = 10;
		 err = class_add_HTB(sk, nllink, flowId, rate, rate,
							 burst, burst, priority, quantum);

	     if ( err == NET_TC_SUCCESS )
	     {
			data->currTimers[0].ival_msec = 1000 * duration;
			modify_filter(flowId, filters, bidir, TC_FILTER_ADD);
			bandwidth_available = bandwidth_available - rate;
			*flowdata = data;
		 }
		 else
			throw ProcError(err, "Error adding HTB class");

     }
     else
		 throw ProcError(NET_TC_PARAMETER_ERROR,
							"HTB Flow init - not enought parameters");

#ifdef DEBUG
		fprintf( stdout, "Success creating flowsetup \n" );
#endif

}


void resetFlowSetup( configParam_t *params )
{
    // NOT implemented, the user have to destroy and recreate the flow.
#ifdef DEBUG
		fprintf( stdout, "init resetFlowSetup \n" );
#endif
}

/*! \short  check if bandwidth available for the rule is enought.

    \arg \c params - rule parameters
    \returns 0 - on success (bandwidth is valid), <0 - else
*/
int checkBandWidth( configParam_t *params )
{
	int64_t rate;
	int numparams = 0;

#ifdef DEBUG
	fprintf( stdout, "check bandwidth - Current value:%f \n", (double) bandwidth_available);
#endif

    while (params[0].name != NULL) {

        if (!strcmp(params[0].name, "Rate")) {
            rate = parseLong(params[0].value);
			numparams++;
			break;
        }
        params++;
     }

	if (numparams == 1){

#ifdef DEBUG
        fprintf( stdout, "check bandwidth - rate:%f \n", (double) rate);
#endif

		if (( bandwidth_available - rate ) >= 0)
        {
#ifdef DEBUG
            fprintf( stdout, "check bandwidth - enough bandwidth \n");
#endif
			return 0;
        }
		else
        {
#ifdef DEBUG
            fprintf( stdout, "check bandwidth - not enough bandwidth \n");
#endif
			return NET_TC_RATE_AVAILABLE_ERROR;
        }
	}

#ifdef DEBUG
	fprintf( stdout, "param bandwidth was not provided \n");
#endif

	return NET_TC_RATE_AVAILABLE_ERROR;
}

void destroyFlowSetup( int rule_id, int action_id,
					   configParam_t *params,
					   filterList_t *filters,
					   void *flowdata )
{
	accData_t *data = (accData_t *)flowdata;
    uint32_t flowId = 0;
    int numparams = 0;
    int err;
    int bidir = 0;
    int64_t rate = 0;

#ifdef DEBUG
		fprintf( stdout, "init destroy FlowSetup \n" );
#endif

	free( data );

    while (params[0].name != NULL) {
		fprintf( stdout, "Evaluating parameter %s value %s \n", params[0].name, params[0].value );

		if (!strcmp(params[0].name, "Rate")) {

            rate = parseLong(params[0].value);
			numparams++;

#ifdef DEBUG
			fprintf( stdout, "rate: %d \n", rate );
#endif

        }

        if (!strcmp(params[0].name, "FlowId")) {
            flowId = (uint32_t) parseInt( params[0].value );
            numparams++;

#ifdef DEBUG
			fprintf( stdout, "Flowid: %d \n", flowId );
#endif

        }

        params++;
    }

    fprintf( stdout, "Flow Id to delete: %d - num parameters:%d \n", flowId, numparams );

	if ( numparams == MOD_DEL_FLOW_REQUIRED_PARAMS )
	{

		 modify_filter(flowId, filters, bidir, TC_FILTER_DELETE);

		 err = class_delete_HTB(sk, nllink, flowId);
		 if (err != 0 )
         {
            fprintf( stdout, "error deleting HTB Class \n" );
			throw ProcError(err, "Error deleting HTB class");
         }
		 else
         {
			bandwidth_available = bandwidth_available + rate;
         }

    }
    else
    {
		 throw ProcError(NET_TC_PARAMETER_ERROR,
							"HTB Flow destroy - not enought parameters Given:%d Required:%d", (int) numparams, (int) MOD_DEL_FLOW_REQUIRED_PARAMS);
    }

    fprintf( stdout, "end destroy FlowSetup \n" );

}


const char* getModuleInfo(int i)
{
    /* fprintf( stderr, "count : getModuleInfo(%d)\n",i ); */

    switch(i) {
    case I_MODNAME:    return "Hierarchical Token Buckets Queue Discipline";
    case I_ID:		   return "htb";
    case I_VERSION:    return "0.1";
    case I_CREATED:    return "2015/03/09";
    case I_MODIFIED:   return "2015/03/09";
    case I_BRIEF:      return "rules to setup bandwidth and priority";
    case I_VERBOSE:    return "rules to setup bandwidth and priority - use the hierarchical token buckets discipline";
    case I_HTMLDOCS:   return "http://www.uniandes.edu.co/... ";
    case I_PARAMS:     return " \n Rate[long (bytes)] : bandwidth rate to setup \n Burst[long (bytes)] : burst to be used \n Priority[int] : rule's priority \n Bidir[bool] : is it dibirectional? \n Duration[int (seconds)] : elapsed time for the rule ";
    case I_RESULTS:    return "Creates a new htb rule and the filters for classify the packets";
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
	accData_t *data = (accData_t *)flowdata;

	if (data == NULL){
		return NULL;
	}
	else{
		std::cout << "htb get timers 2" << data->currTimers[0].ival_msec <<  std::endl;
		/* return timers record if ival > 0, else return "no timers available" (NULL) */
		return (data->currTimers[0].ival_msec > 0) ? data->currTimers : NULL;
	}
}
