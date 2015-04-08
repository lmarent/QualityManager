//
// tc_functions.h
//
// $Id: //ecodtn/0.1/include/tc_functions.h#1 $
//
// Library: Ecodtn
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

#ifndef Tc_functions_INCLUDED
#define Tc_functions_INCLUDED

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

#define DEFAULT_CLASS 0xffff;


#define ECODTN_NET_TC_SUCCESS 0
#define ECODTN_NET_TC_QDISC_ALLOC_ERROR -1
#define ECODTN_NET_TC_QDISC_SETUP_ERROR -2
#define ECODTN_NET_TC_QDISC_ESTABLISH_ERROR -3
#define ECODTN_NET_TC_CLASS_ALLOC_ERROR -4
#define ECODTN_NET_TC_CLASS_SETUP_ERROR -5
#define ECODTN_NET_TC_CLASS_ESTABLISH_ERROR -6
#define ECODTN_NET_TC_CLASSIFIER_ALLOC_ERROR -7
#define ECODTN_NET_TC_CLASSIFIER_SETUP_ERROR -8
#define ECODTN_NET_TC_CLASSIFIER_ESTABLISH_ERROR -9
#define ECODTN_NET_TC_LINK_ERROR -10


int qdisc_add_root_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink);

int qdisc_delete_root_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink);

int class_add_HTB_root(struct nl_sock *sock, struct rtnl_link *rtnlLink, 
					   uint64_t rate, uint64_t ceil, uint32_t burst, 
					   uint32_t cburst);

int class_add_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink, 
				  uint32_t parentMaj, uint32_t parentMin, 
				  uint32_t childMaj, uint32_t childMin, uint64_t rate, 
				  uint64_t ceil, uint32_t burst, uint32_t cburst, uint32_t prio);
				  
int class_delete_HTB(struct nl_sock *sock, struct rtnl_link *rtnlLink, 
			      uint32_t parentMaj, uint32_t parentMin, 
				  uint32_t childMaj, uint32_t childMin );				  
				  
int qdisc_add_SFQ_leaf(struct nl_sock *sock, struct rtnl_link *rtnlLink, 
					   uint32_t parentMaj, uint32_t parentMin, 
					   int quantum, int limit, int perturb);

int qdisc_delete_SFQ_leaf(struct nl_sock *sock, struct rtnl_link *rtnlLink, 
					   uint32_t parentMaj, uint32_t parentMin );

int u32_add_filter(struct nl_sock *sock, struct rtnl_link *rtnlLink, 
				   uint32_t prio, const char *keyval_str, const char *keymask_str, 
				   int keyoff, int keyoffmask, uint32_t parentMaj, 
			       uint32_t parentMin, uint32_t classfierMah, 
			       uint32_t classfierMin);
			       
int u32_delete_filter(struct nl_sock *sock, struct rtnl_link *rtnlLink, 
				   uint32_t prio, const char *keyval_str, const char *keymask_str, 
				   int keyoff, int keyoffmask, uint32_t parentMaj, 
			       uint32_t parentMin, uint32_t classfierMah, uint32_t classfierMin );			       
					   
#endif 