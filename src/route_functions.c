#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/data.h>
#include <netlink/types.h>
#include <netlink/addr.h>

#include "route_functions.h"


int route_add(struct nl_sock *sk, struct nl_cache *link_cache, 
			  char *dst_str, char *intfc_name, char *gateway_str)
/*  dst_str     --> destination address
 *  intfc_name  --> Interface Id
 *  gateway_str --> Gateway address
 */
{

	int err; 
	struct nl_addr *dst;
	struct nl_addr *gateway;
	struct nl_addr *broadcast;
    struct rtnl_route *route_tmp;
	struct rtnl_nexthop *next_hop;
    int link_int;
 

	link_int = rtnl_link_name2i(link_cache, intfc_name);
	struct rtnl_link *link_tmp = rtnl_link_get(link_cache, link_int);
    if (link_tmp == NULL){
        err = ECODTN_NET_RT_LINK_ERROR;    
    }

 	route_tmp = rtnl_route_alloc();
    // Parse the destination address 
    err = nl_addr_parse( dst_str,  AF_INET, &dst);
	if (err < 0)
	{
        err = ECODTN_NET_RT_ROUTE_ALLOC_ERROR;
        return err;
	}

    // Parse the gateway address 
    err = nl_addr_parse( gateway_str, AF_INET, &gateway);
	if (err < 0)
	{
	    err = ECODTN_NET_RT_ADDR_PARSE_ERROR;
	    return err;
	}
		
	err = rtnl_route_set_dst(route_tmp, dst);
	if (err < 0)
	{
	    err = ECODTN_NET_RT_ROUTE_SETUP_ERROR;
        return err;
	}	
	
	rtnl_route_set_iif(route_tmp, link_int);
	rtnl_route_set_scope(route_tmp, RT_SCOPE_UNIVERSE);
	rtnl_route_set_family(route_tmp, AF_INET);
	next_hop = rtnl_route_nh_alloc ();
	
	if (next_hop != NULL)
	{
	    rtnl_route_nh_set_ifindex (next_hop, link_int);
   	    rtnl_route_nh_set_gateway (next_hop, gateway);
	    rtnl_route_add_nexthop (route_tmp, next_hop);
        err = rtnl_route_add(sk, route_tmp, 0);
		if (err < 0 )
		{
			err = ECODTN_NET_RT_ROUTE_ESTABLISH_ERROR;
			return err;
		} 	
    }
	return ECODTN_NET_RT_SUCCESS;
}

int route_delete(struct nl_sock *sk, struct nl_cache *link_cache, 
				 char *dst_str, char *intfc_name, char *gateway_str)
/*  dst_str     --> destination address
 *  intfc_name  --> Interface Id
 *  gateway_str --> Gateway address
 */
{

	int err;
	
	struct nl_addr *dst;
	struct nl_addr *gateway;
	struct nl_addr *broadcast;
    struct rtnl_route *route_tmp;
	struct rtnl_nexthop *next_hop;
    int link_int;

	link_int = rtnl_link_name2i(link_cache, intfc_name);
	struct rtnl_link *link_tmp = rtnl_link_get(link_cache, link_int);
    if (link_tmp == NULL){
        err = ECODTN_NET_RT_LINK_ERROR;
        return err;      
    }

 	route_tmp = rtnl_route_alloc();
    // Parse the destination address 
    err = nl_addr_parse( dst_str,  AF_INET, &dst);
	if (err < 0){
	    err = ECODTN_NET_RT_ADDR_PARSE_ERROR;
	    return err;
	}

    // Parse the gateway address 
    err = nl_addr_parse( gateway_str, AF_INET, &gateway);
	if (err < 0){
	    err = ECODTN_NET_RT_ADDR_PARSE_ERROR;
	    return err;
	}
		
	err = rtnl_route_set_dst(route_tmp, dst);
	if (err < 0)
	{
	    err = ECODTN_NET_RT_ROUTE_SETUP_ERROR;
        return err;
	}	
	rtnl_route_set_iif(route_tmp, link_int);
	rtnl_route_set_scope(route_tmp, RT_SCOPE_UNIVERSE);
	rtnl_route_set_family(route_tmp, AF_INET);

	next_hop = rtnl_route_nh_alloc ();
	if (next_hop != NULL)
	{
	    rtnl_route_nh_set_ifindex (next_hop, link_int);
   	    rtnl_route_nh_set_gateway (next_hop, gateway);
	    rtnl_route_add_nexthop (route_tmp, next_hop);
        err = rtnl_route_delete(sk, route_tmp, 0);
		if (err < 0 )
		{
			err = ECODTN_NET_RT_ROUTE_ESTABLISH_ERROR;
			return err;
		} 	
    }
        
	return ECODTN_NET_RT_SUCCESS;
}

