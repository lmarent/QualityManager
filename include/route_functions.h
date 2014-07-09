#ifndef Route_functions_INCLUDED
#define Route_functions_INCLUDED

#include <netlink/netlink.h>
#include <netlink/route/link.h>
#include <netlink/cache.h>
#include <netlink/utils.h>
#include <netlink/data.h>
#include <netlink/types.h>
#include <netlink/addr.h>

#define ECODTN_NET_RT_SUCCESS 0
#define ECODTN_NET_RT_LINK_ERROR -1
#define ECODTN_NET_RT_ROUTE_ALLOC_ERROR -2
#define ECODTN_NET_RT_ADDR_PARSE_ERROR -3
#define ECODTN_NET_RT_ROUTE_SETUP_ERROR -4
#define ECODTN_NET_RT_ROUTE_ESTABLISH_ERROR -5


int route_add(struct nl_sock *sk, struct nl_cache *link_cache, 
			  char *dst_str, char *intfc_name, char *gateway_str);
			  
int route_delete(struct nl_sock *sk, struct nl_cache *link_cache, 
				 char *dst_str, char *intfc_name, char *gateway_str);

#endif // Route_functions_INCLUDED
