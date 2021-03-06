//
// htb_functions.c
//
// $Id: //ecodtn/0.1/include/htb_functions.c#1 $
//
// Library: NetQoS
// Package: TrafficControl
// Module:  NetworkQuality
//
// Definition of the tc_functions class.
//
// Copyright (c) 2014, Luis Andres Marentes C.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#include <stdio.h>
#include <inttypes.h>
#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/class.h>
#include <netlink/route/classifier.h>
#include <netlink/route/cls/u32.h>
#include <netlink/attr.h>
#include <arpa/inet.h>
#include <netlink/route/qdisc/htb.h>
#include <netlink/route/qdisc/sfq.h>
#include <linux/if_ether.h>
#include <netlink/attr.h>
#include "htb_functions.h"


uint32_t NET_ROOT_HANDLE_MAJOR 		= 0x00000001U;
uint32_t NET_ROOT_HANDLE_MINOR 		= 0x00000001U;
uint32_t NET_DEFAULT_CLASS 	   		= 0x0000FFFFU;
uint32_t NET_FILTER_HANDLE_MINOR 	= 0x00000001U;
uint32_t NET_HASH_FILTER_TABLE 		= 1;
uint32_t NET_UNHASH_FILTER_TABLE 	= 2;


// Build of the qdisk object at the root of the hierarchy
int qdisc_add_root_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink)
{
   int err = 0;

   if (sock == NULL){
	  printf("sock null");
	  err = -1;
	  return err;
   }

   if (rtnlLink == NULL){
	  printf("link null");
	  err = -2;
	  return err;
   }

   /* Allocation of a qdisc object */
   struct rtnl_qdisc *qdisc = rtnl_qdisc_alloc();
   if (!qdisc){
	   err = NET_TC_QDISC_ALLOC_ERROR;
	   return err;
   }

   /* Establish the link associated to the class */
   err = rtnl_tc_set_kind(TC_CAST(qdisc), "htb");
   if (err < 0)
   {
       err = NET_TC_QDISC_SETUP_ERROR;
       return err;
   }

   rtnl_tc_set_link(TC_CAST(qdisc), rtnlLink);
   rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_ROOT);
   rtnl_tc_set_handle(TC_CAST(qdisc), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,0));
   printf("Add root Qdisc message %" PRIu32 "\n",
					NET_HANDLE(NET_ROOT_HANDLE_MAJOR,0));

   /* Set default class for unclassified traffic */
   rtnl_htb_set_defcls(qdisc, NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
										NET_DEFAULT_CLASS) );
   printf("Add root Qdisc message %" PRIu32 "\n",
					NET_HANDLE(NET_ROOT_HANDLE_MAJOR,NET_DEFAULT_CLASS));

   rtnl_htb_set_rate2quantum(qdisc, 1);

   err = rtnl_qdisc_add(sock, qdisc, NLM_F_CREATE );

   /* Free the qdisc object */
   rtnl_qdisc_put(qdisc);

   if (err < 0) {
        err = NET_TC_QDISC_ESTABLISH_ERROR;
        return err;
   }

   return NET_TC_SUCCESS;
}


int qdisc_delete_root_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink )
{
    int err;
    struct rtnl_qdisc *qdisc;

    if (!(qdisc = rtnl_qdisc_alloc())) {
        printf("error on delete Qdisc");
        err = NET_TC_QDISC_ALLOC_ERROR;
        return err;
    }

    rtnl_tc_set_link(TC_CAST(qdisc), rtnlLink);
    rtnl_tc_set_parent(TC_CAST(qdisc), TC_H_ROOT);
    rtnl_tc_set_handle(TC_CAST(qdisc), TC_HANDLE(NET_ROOT_HANDLE_MAJOR,0));

    /* Submit request to kernel and wait for response */
    if ((err = rtnl_qdisc_delete(sock, qdisc))) {
		printf("error on delete Qdisc %d", err);
        err = NET_TC_QDISC_ESTABLISH_ERROR;
		return err;
    }

    /* Return the qdisc object to free memory resources */
    rtnl_qdisc_put(qdisc);
    return NET_TC_SUCCESS;
}



int class_add_HTB_root(struct nl_sock *sock, struct rtnl_link *rtnlLink,
					   uint64_t rate, uint64_t ceil,
					   uint32_t burst, uint32_t cburst, uint32_t quantum)
{
    int err;
    struct rtnl_class *class;

    //Allocate memory for the HTB class
    if (!(class = rtnl_class_alloc())) {
        printf("Can not allocate class object \n");
        err = NET_TC_CLASS_ALLOC_ERROR;
        return err;
    }

    // Assign the link and the class's parent
    rtnl_tc_set_link(TC_CAST(class), rtnlLink);
    rtnl_tc_set_parent(TC_CAST(class), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,0) );

    //add the handled for the class class
    rtnl_tc_set_handle(TC_CAST(class), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
												 NET_ROOT_HANDLE_MINOR));

    if ((err = rtnl_tc_set_kind(TC_CAST(class), "htb"))) {
        err = NET_TC_CLASS_SETUP_ERROR;
        return err;
    }
    if (rate) {
       rtnl_htb_set_rate(class, rate);
    }
    if (ceil) {
       rtnl_htb_set_ceil(class, ceil);
    }
    if (burst) {
        rtnl_htb_set_rbuffer(class, burst);
    }
    if (cburst) {
        rtnl_htb_set_cbuffer(class, cburst);
    }
    if (quantum) {
		rtnl_htb_set_quantum(class, quantum);
	}
    /* Submit request to kernel and wait for response */
    if ((err = rtnl_class_add(sock, class, NLM_F_CREATE))) {
        err = NET_TC_CLASS_ESTABLISH_ERROR;
        return err;
    }

    // Free the memory allocated for the class structure.
    rtnl_class_put(class);

    // return Ok.
    return NET_TC_SUCCESS;
}

int class_add_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink,
				  uint32_t childMin, uint64_t rate, uint64_t ceil,
				  uint32_t burst, uint32_t cburst, uint32_t prio, uint32_t quantum)
{
    int err;
    struct rtnl_class *class;

    //create a HTB class
    if (!(class = rtnl_class_alloc())) {
        err = NET_TC_CLASS_ALLOC_ERROR;
        return err;
    }

    rtnl_tc_set_link(TC_CAST(class), rtnlLink);

    rtnl_tc_set_parent(TC_CAST(class), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
												 NET_ROOT_HANDLE_MINOR));

    rtnl_tc_set_handle(TC_CAST(class), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
												 childMin));

    if ((err = rtnl_tc_set_kind(TC_CAST(class), "htb"))) {
        err = NET_TC_CLASS_SETUP_ERROR;
        return err;
    }

    //printf("set HTB class prio to %u\n", prio);
    rtnl_htb_set_prio(class, prio);

    if (rate) {
	rtnl_htb_set_rate(class, rate);
    }
    if (ceil) {
	rtnl_htb_set_ceil(class, ceil);
    }
    if (burst) {
        rtnl_htb_set_rbuffer(class, burst);
    }
    if (cburst) {
        rtnl_htb_set_cbuffer(class, cburst);
    }
    if (quantum){
		rtnl_htb_set_quantum(class, quantum);
	}

    /* Submit request to kernel and wait for response */
    if ((err = rtnl_class_add(sock, class, NLM_F_CREATE))) {
        err = NET_TC_CLASS_ESTABLISH_ERROR;
        return err;
    }
    rtnl_class_put(class);
    return NET_TC_SUCCESS;
}

int class_delete_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink,
			      	 uint32_t childMin )
{
    int err;
    struct rtnl_class *class;

    //create a HTB class
    if (!(class = rtnl_class_alloc())) {
        err = NET_TC_CLASS_ALLOC_ERROR;
        return err;
    }

    rtnl_tc_set_link(TC_CAST(class), rtnlLink);

    rtnl_tc_set_parent(TC_CAST(class), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
												 NET_ROOT_HANDLE_MINOR));

    rtnl_tc_set_handle(TC_CAST(class), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
												 childMin));

    if ((err = rtnl_tc_set_kind(TC_CAST(class), "htb"))) {
        err = NET_TC_CLASS_SETUP_ERROR;
        return err;
    }

    /* Submit request to kernel and wait for response */
    if ((err = rtnl_class_delete(sock, class))) {
        err = NET_TC_CLASS_ESTABLISH_ERROR;
        return err;
    }
    rtnl_class_put(class);
    return NET_TC_SUCCESS;
}

/*
* function that adds a new SFQ qdisc as a leaf for a HTB class
*/
int qdisc_add_SFQ_leaf(struct nl_sock *sock, struct rtnl_link *rtnlLink,
					   uint32_t childMin, int quantum, int limit, int perturb)
{
    int err;
    struct rtnl_qdisc *qdisc;

    if (!(qdisc = rtnl_qdisc_alloc())) {
        err = NET_TC_QDISC_ALLOC_ERROR;
        return err;
    }

    rtnl_tc_set_link(TC_CAST(qdisc), rtnlLink);
    rtnl_tc_set_parent(TC_CAST(qdisc), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
												 childMin));
    rtnl_tc_set_handle(TC_CAST(qdisc), NET_HANDLE(childMin,0));

    if ((err = rtnl_tc_set_kind(TC_CAST(qdisc), "sfq"))) {
        err = NET_TC_QDISC_SETUP_ERROR;
        return err;
    }

    if(quantum) {
        rtnl_sfq_set_quantum(qdisc, quantum);
    } else {
        rtnl_sfq_set_quantum(qdisc, 16000); // tc default value
    }
    if(limit) {
        rtnl_sfq_set_limit(qdisc, limit); // default is 127
    }
    if(perturb) {
        rtnl_sfq_set_perturb(qdisc, perturb); // default never perturb the hash
    }

    /* Submit request to kernel and wait for response */
    if ((err = rtnl_qdisc_add(sock, qdisc, NLM_F_CREATE))) {
        err = NET_TC_QDISC_ESTABLISH_ERROR;
		return err;
    }

    /* Return the qdisc object to free memory resources */
    rtnl_qdisc_put(qdisc);
    return NET_TC_SUCCESS;
}


int qdisc_delete_SFQ_leaf(struct nl_sock *sock, struct rtnl_link *rtnlLink,
					      uint32_t childMin )
{
    int err;
    struct rtnl_qdisc *qdisc;

    if (!(qdisc = rtnl_qdisc_alloc())) {
        err = NET_TC_QDISC_ALLOC_ERROR;
        return err;
    }

    rtnl_tc_set_link(TC_CAST(qdisc), rtnlLink);

    rtnl_tc_set_parent(TC_CAST(qdisc), NET_HANDLE(NET_ROOT_HANDLE_MAJOR,
												 childMin));

    rtnl_tc_set_handle(TC_CAST(qdisc), NET_HANDLE(childMin,0));

    if ((err = rtnl_tc_set_kind(TC_CAST(qdisc), "sfq"))) {
        err = NET_TC_QDISC_SETUP_ERROR;
        return err;
    }

    /* Submit request to kernel and wait for response */
    if ((err = rtnl_qdisc_delete(sock, qdisc))) {
        err = NET_TC_QDISC_ESTABLISH_ERROR;
		return err;
    }

    /* Return the qdisc object to free memory resources */
    rtnl_qdisc_put(qdisc);

    return NET_TC_SUCCESS;
}

/* some functions are copied from iproute-tc tool */
int get_u32(__u32 *val, const char *arg, int base)
{
	unsigned long res;
	char *ptr;

	if (!arg || !*arg)
		return -1;
	res = strtoul(arg, &ptr, base);
	if (!ptr || ptr == arg || *ptr || res > 0xFFFFFFFFUL)
		return -1;
	*val = res;
	return 0;
}


/** Function that creates a unit32_t value from a handler represented as
 *  a string
 * This function is taken from libnl-3 complex hash filters.
 */
int get_u32_handle(__u32 *handle, const char *str)
{
	__u32 htid=0, hash=0, nodeid=0;
	char *tmp = strchr(str, ':');

	if (tmp == NULL) {
		if (memcmp("0x", str, 2) == 0)
			return get_u32(handle, str, 16);
		return -1;
	}
	htid = strtoul(str, &tmp, 16);
	if (tmp == str && *str != ':' && *str != 0)
		return -1;
	if (htid>=0x1000)
		return -1;
	if (*tmp) {
		str = tmp+1;
		hash = strtoul(str, &tmp, 16);
		if (tmp == str && *str != ':' && *str != 0)
			return -1;
		if (hash>=0x100)
			return -1;
		if (*tmp) {
			str = tmp+1;
			nodeid = strtoul(str, &tmp, 16);
			if (tmp == str && *str != 0)
				return -1;
			if (nodeid>=0x1000)
				return -1;
		}
	}
	*handle = (htid<<20)|(hash<<12)|nodeid;
	return 0;
}

uint32_t get_u32_parse_handle(const char *cHandle)
{
	uint32_t handle=0;

	if(get_u32_handle(&handle, cHandle)) {
		printf ("Illegal \"ht\"\n");
		return -1;
	}

	if (handle && TC_U32_NODE(handle)) {
		printf("\"link\" must be a hash table.\n");
		return -1;
	}
	return handle;
}



/**
 * This function adds a new filter and attach it to a hash table
 * and set a next hash table link with hash mask
 *
 */
int u32_add_filter_on_ht_with_hashmask(struct nl_sock *sock, struct rtnl_link *rtnlLink,
		uint32_t prio, uint32_t parentMaj, uint32_t parentMin,
		uint32_t keyval, uint32_t keymask, int keyoff, int keyoffmask,
		uint32_t htid, uint32_t htlink, uint32_t hmask, uint32_t hoffset )
{
    struct rtnl_cls *cls;
    int err;

    cls=rtnl_cls_alloc();

    if (!(cls)) {
        printf("Can not allocate classifier hash table\n");
        return NET_TC_CLASSIFIER_ALLOC_ERROR;
    }

    rtnl_tc_set_link(TC_CAST(cls), rtnlLink);

    if ((err = rtnl_tc_set_kind(TC_CAST(cls), "u32"))) {
        printf("Cannot set classifier as u32\n");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    rtnl_cls_set_prio(cls, prio);
    rtnl_cls_set_protocol(cls, ETH_P_IP);

    rtnl_tc_set_parent(TC_CAST(cls),
					   TC_HANDLE(parentMaj, parentMin));

    if (htid)
		rtnl_u32_set_hashtable(cls, htid);

    rtnl_u32_add_key_uint32(cls, keyval, keymask, keyoff, keyoffmask);

    rtnl_u32_set_hashmask(cls, hmask, hoffset);

    rtnl_u32_set_link(cls, htlink);


    if ((err = rtnl_cls_add(sock, cls, NLM_F_CREATE))) {
        printf("Error adding classifier %s \n", nl_geterror(err));
        return NET_TC_CLASSIFIER_ESTABLISH_ERROR;
    }
    rtnl_cls_put(cls);
    return 0;
}

/**
 * This function adds a new filter and attach it to a hash table
 * and set a the bucket in 0.
 *
 */
int u32_add_filter_on_ht_without_hashmask(struct nl_sock *sock, struct rtnl_link *rtnlLink,
		uint32_t prio, uint32_t parentMaj, uint32_t parentMin,
		uint32_t keyval, uint32_t keymask, int keyoff, int keyoffmask,
		uint32_t htid, uint32_t htlink )
{
    struct rtnl_cls *cls;
    int err;

    cls=rtnl_cls_alloc();

    if (!(cls)) {
        printf("Can not allocate classifier hash table\n");
        return NET_TC_CLASSIFIER_ALLOC_ERROR;
    }

    rtnl_tc_set_link(TC_CAST(cls), rtnlLink);

    if ((err = rtnl_tc_set_kind(TC_CAST(cls), "u32"))) {
        printf("Cannot set classifier as u32\n");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    rtnl_cls_set_prio(cls, prio);
    rtnl_cls_set_protocol(cls, ETH_P_IP);

    rtnl_tc_set_parent(TC_CAST(cls),
					   TC_HANDLE(parentMaj, parentMin));

    if (htid)
		rtnl_u32_set_hashtable(cls, htid);

    rtnl_u32_add_key_uint32(cls, keyval, keymask, keyoff, keyoffmask);

    rtnl_u32_set_link(cls, htlink);

    if ((err = rtnl_cls_add(sock, cls, NLM_F_CREATE))) {
        printf("Error adding classifier %s \n", nl_geterror(err));
        return NET_TC_CLASSIFIER_ESTABLISH_ERROR;
    }
    rtnl_cls_put(cls);
    return 0;
}



/**
 * Add a new hash table for classifiers
 */
int u32_add_ht(struct nl_sock *sock, struct rtnl_link *rtnlLink,
			   uint32_t prio, uint32_t parentMaj, uint32_t parentMin,
			   uint32_t htid, uint32_t divisor)
{

    int err;
    struct rtnl_cls *cls;

    cls=rtnl_cls_alloc();
    if (!(cls)) {
        printf("Can not create hash table\n");
        return NET_TC_CLASSIFIER_ALLOC_ERROR;
    }

    rtnl_tc_set_link(TC_CAST(cls), rtnlLink);

    if ((err = rtnl_tc_set_kind(TC_CAST(cls), "u32"))) {
        printf("Cannot set classifier as u32\n");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    rtnl_cls_set_prio(cls, prio);
    rtnl_cls_set_protocol(cls, ETH_P_IP);
    rtnl_tc_set_parent(TC_CAST(cls), TC_HANDLE(parentMaj, parentMin));

    rtnl_u32_set_handle(cls, htid, 0x0, 0x0);
    //printf("htid: 0x%X\n", htid);
    rtnl_u32_set_divisor(cls, divisor);

    if ((err = rtnl_cls_add(sock, cls, NLM_F_CREATE))) {
        printf("Error adding classifier %s \n", nl_geterror(err));
        return NET_TC_CLASSIFIER_ESTABLISH_ERROR;
    }
    rtnl_cls_put(cls);
    return 0;
}


/**
 * Delete a hash table created to maintain classifiers
 */
int u32_delete_ht(struct nl_sock *sock, struct rtnl_link *rtnlLink,
				  uint32_t prio, uint32_t parentMaj, uint32_t parentMin,
				  uint32_t htid, uint32_t divisor)
{

    int err;
    struct rtnl_cls *cls;

    cls=rtnl_cls_alloc();
    if (!(cls)) {
        printf("Can not create hash table\n");
        return NET_TC_CLASSIFIER_ALLOC_ERROR;
    }

    rtnl_tc_set_link(TC_CAST(cls), rtnlLink);

    if ((err = rtnl_tc_set_kind(TC_CAST(cls), "u32"))) {
        printf("Cannot set classifier as u32\n");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    rtnl_cls_set_prio(cls, prio);
    rtnl_cls_set_protocol(cls, ETH_P_IP);
    rtnl_tc_set_parent(TC_CAST(cls), TC_HANDLE(parentMaj, parentMin));

    rtnl_u32_set_handle(cls, htid, 0x0, 0x0);

    if ((err = rtnl_cls_delete(sock, cls, 0)) < 0) {
        printf("Error deleting classifier, error: %s \n", nl_geterror(err));
        err = NET_TC_CLASSIFIER_ESTABLISH_ERROR;
        return err;
    }
    rtnl_cls_put(cls);
    return 0;
}


/**
 * Function that adds the main hast table.
 *    We create different filter lists depending on the ip address' last byte.
 *
 * Create u32 first hash filter table
 *    Upper limit number of hash tables: 4096 0xFFF
 *    Upper limit in buckets by hash table: 256
 *
 */
int create_hash_configuration(struct nl_sock *sock, struct rtnl_link *rtnlLink,
							  uint32_t priority, uint32_t parentMaj, uint32_t parentMin)
{

#ifdef DEBUG
	fprintf( stdout, "htb functions: init create_hash_configuration \n" );
#endif


    uint32_t htlink, htid, direction;
    char chashlink[16];

    // Creates one hash table
    // table 1 is dedicated to the last byte in the IP address


    htid = NET_HASH_FILTER_TABLE;
	u32_add_ht(sock, rtnlLink, priority, parentMaj, parentMin, htid, 256);

    // Table 2 is the table for filters that do not have a unique source ipaddress
    // only one bucket.
    htid = NET_UNHASH_FILTER_TABLE;
	u32_add_ht(sock, rtnlLink, priority, parentMaj, parentMin, htid, 1);


    /*
     * attach a u32 filter to the first hash
     * that redirects all traffic and make a hash key
     * from the fist byte of the IP address
     */
    // Test without hashing

    direction = 12; // Source IP.
	sprintf(chashlink, "%d:",NET_HASH_FILTER_TABLE);
    htlink = 0x0;		// is used by get_u32_handle to return the correct value of hash table (link)

    if(get_u32_handle(&htlink, chashlink)) {
        fprintf (stdout, "Illegal \"link\"");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    if (htlink && TC_U32_NODE(htlink)) {
		fprintf (stdout, "\"link\" must be a hash table.\n");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    // the hash mask will hit the hash table (link) Nbr 1:
    // a hash filter is added which match the first byte (see the hashmask value 0x000000ff)
    // of the source IP (offset 12 in the packet header)

    u32_add_filter_on_ht_with_hashmask(sock, rtnlLink, priority, parentMaj,
									   parentMin, 0x0, 0x0, direction,
									   0, 0, htlink, 0x000000ff, direction);



    /*
     * attach a u32 filter to the destiny ipaddress, takes all destiny ipaddress.
     */
    direction = 16; // destiny IP.
	sprintf(chashlink, "%d:",NET_UNHASH_FILTER_TABLE);
    htlink = 0x0;		// is used by get_u32_handle to return the correct value of hash table (link)

    if(get_u32_handle(&htlink, chashlink)) {
        fprintf (stdout, "Illegal \"link\"");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    if (htlink && TC_U32_NODE(htlink)) {
		fprintf (stdout,"\"link\" must be a hash table.\n");
        return NET_TC_CLASSIFIER_SETUP_ERROR;
    }

    u32_add_filter_on_ht_without_hashmask(sock, rtnlLink, priority, parentMaj,
										  parentMin, 0x0, 0x0, direction,
										  0, 0, htlink);
	return 0;
}


/**
 * Function that adds the main hast table.
 *    We create different filter lists depending on the ip address' last byte.
 *
 * Create u32 first hash filter table
 *    Upper limit number of hash tables: 4096 0xFFF
 *    Upper limit in buckets by hash table: 256
 *
 */
int delete_hash_configuration(struct nl_sock *sock, struct rtnl_link *rtnlLink,
							  uint32_t priority, uint32_t parentMaj, uint32_t parentMin)
{

    int err = 0;
    uint32_t htid;

    // Delete hash table 1, which is dedicated to the last byte in the IP address,
    htid = NET_HASH_FILTER_TABLE;
    err = u32_delete_ht(sock, rtnlLink, priority, parentMaj, parentMin, htid, 256);

    if (err == 0){
		htid = NET_UNHASH_FILTER_TABLE;
		return u32_delete_ht(sock, rtnlLink, priority, parentMaj, parentMin, htid, 1);
	}

	return err;

}



/**
 * This function allocate and prepare the link for creating a u32 classifier,
 * it should be called before any key is introduced.
 */
int create_u32_classifier(struct nl_sock *sock,
						  struct rtnl_link *rtnlLink,
						  struct rtnl_cls **cls_out,
						  uint32_t prio,
						  uint32_t parentMaj,
						  uint32_t parentMin,
						  int classfierMaj,
						  int classfierMin,
						  int htid,
						  int hashkey)
{
#ifdef DEBUG
	fprintf( stdout, "htb functions: create_u32_classifier ht:%d - hk:%d \n", htid, hashkey );
#endif

    int err;
    uint32_t ht1, htid1;
    char chashkey[20];

    struct rtnl_cls *cls = (struct rtnl_cls *) rtnl_cls_alloc();

    if (!(cls)) {
        err = NET_TC_CLASSIFIER_ALLOC_ERROR;
		return err;
    }

    rtnl_tc_set_link(TC_CAST(cls), rtnlLink);

    if ((err = rtnl_tc_set_kind(TC_CAST(cls), "u32"))) {
        err = NET_TC_CLASSIFIER_SETUP_ERROR;
        return err;
    }

	rtnl_cls_set_protocol(cls, ETH_P_IP);

    rtnl_cls_set_prio(cls, prio);
    rtnl_tc_set_parent(TC_CAST(cls),
					   NET_HANDLE(parentMaj, parentMin));

    sprintf(chashkey, "%x:%x:", htid, hashkey);
    ht1	= get_u32_parse_handle(chashkey);
    htid1 = (ht1&0xFFFFF000);

#ifdef DEBUG
	fprintf( stdout, "htb functions: create_u32_classifier ht-hk:%d \n", htid1 );
#endif
    rtnl_u32_set_hashtable(cls, htid1);

	rtnl_u32_set_handle(cls, htid, hashkey, classfierMin);

	*cls_out = cls;
	return NET_TC_SUCCESS;
}


/**
 * This function allocate and prepare the link for creating a u32 classifier,
 * it should be called before any key is introduced.
 */
int delete_u32_classifier(struct nl_sock *sock,
						  struct rtnl_link *rtnlLink,
						  struct rtnl_cls **cls_out,
						  uint32_t prio,
						  uint32_t parentMaj,
						  uint32_t parentMin,
						  int classfierMaj,
						  int classfierMin,
						  int htid,
						  int hashkey)
{
    int err;
    struct rtnl_cls *cls = (struct rtnl_cls *) rtnl_cls_alloc();

    if (!(cls)) {
        err = NET_TC_CLASSIFIER_ALLOC_ERROR;
		return err;
    }

    rtnl_tc_set_link(TC_CAST(cls), rtnlLink);

    if ((err = rtnl_tc_set_kind(TC_CAST(cls), "u32"))) {
        err = NET_TC_CLASSIFIER_SETUP_ERROR;
        return err;
    }

	rtnl_cls_set_protocol(cls, ETH_P_IP);

    rtnl_cls_set_prio(cls, prio);
    rtnl_tc_set_parent(TC_CAST(cls),
					   NET_HANDLE(parentMaj, parentMin));

	rtnl_u32_set_handle(cls, htid, hashkey, classfierMin);

	*cls_out = cls;
	return NET_TC_SUCCESS;
}


int u32_add_key_filter(struct rtnl_cls *cls,
					   const unsigned char *keyval_str,
					   const unsigned char *keymask_str,
					   unsigned short len,
					   int keyoff,
					   int keyoffmask )
{
	uint8_t keyval8, keyval8_mask;
	uint16_t keyval16, keyval16_mask;
	uint32_t keyval32, keyval32_mask;
	int err = 0;

	switch (len)
	{
		case 1:
			keyval8 = keyval_str[0];
			keyval8_mask = keymask_str[0];
			err = rtnl_u32_add_key_uint8(cls, keyval8,
								keyval8_mask, keyoff, keyoffmask);
			if (err < 0)
				err = NET_TC_CLASSIFIER_SETUP_ERROR;
			break;

		case 2:
			keyval16 = (uint16_t)
					keyval_str[0] << 8 | (uint16_t)keyval_str[1];
			keyval16_mask = (uint16_t)
					keymask_str[0] << 8 | (uint16_t)keymask_str[1];

			err = rtnl_u32_add_key_uint16(cls, keyval16,
							keyval16_mask, keyoff, keyoffmask);
			if (err < 0)
				err = NET_TC_CLASSIFIER_SETUP_ERROR;
			break;

		case 4:
			keyval32 = (uint32_t)keyval_str[0] << 24 |
					   (uint32_t)keyval_str[1] << 16 |
					   (uint32_t)keyval_str[2] << 8  |
					   (uint32_t)keyval_str[3];

			keyval32_mask = (uint32_t)keymask_str[0] << 24 |
							(uint32_t)keymask_str[1] << 16 |
							(uint32_t)keymask_str[2] << 8  |
							(uint32_t)keymask_str[3];

			err = rtnl_u32_add_key_uint32(cls, keyval32,
								keyval32_mask, keyoff, keyoffmask);
			if (err < 0)
				err = NET_TC_CLASSIFIER_SETUP_ERROR;

			break;

		default:
			err = 0;
			break;
	}

	return err;
}

int save_add_u32_filter(struct nl_sock *sock,
						struct rtnl_cls *cls)
{
    int err;
    int flags = NLM_F_CREATE;

    rtnl_u32_set_cls_terminal(cls);

    if ((err = rtnl_cls_add(sock, cls, flags))) {
        printf("Error adding classifier %s \n", nl_geterror(err));
        err = NET_TC_CLASSIFIER_ESTABLISH_ERROR;
        return err;
    }

    rtnl_cls_put(cls);
    return NET_TC_SUCCESS;
}

int save_delete_u32_filter(struct nl_sock *sock, struct rtnl_cls *cls)
{
    int err;

    if ((err = rtnl_cls_delete(sock, cls, 0)) < 0) {
        printf("Error deleting classifier, error: %s \n", nl_geterror(err));
        err = NET_TC_CLASSIFIER_ESTABLISH_ERROR;
        return err;
    }
    rtnl_cls_put(cls);
    return NET_TC_SUCCESS;

}

