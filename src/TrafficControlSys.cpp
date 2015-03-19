#include <Poco/Net/NetworkInterface.h>
#include <Poco/AutoPtr.h>
#include <vector>
#include "TrafficControlSys.h"
#include "TrafficControlException.h"

namespace ecodtn
{

namespace net
{

TrafficControlSys::TrafficControlSys(void)
{
}

TrafficControlSys::~TrafficControlSys(void)
{
}

void TrafficControlSys::initialize(Poco::Util::Application &app)
{
    app.logger().information("Initialization TraffiControl System");
    
    // Read all interfaces
    std::vector<Poco::Net::NetworkInterface> list = Poco::Net::NetworkInterface::list();
    std::vector<Poco::Net::NetworkInterface>::iterator it = list.begin();
    for(;it!=list.end();it++)
	{
		Poco::Net::NetworkInterface netIntf = *it;
		if (!((netIntf.address()).isLoopback())){
			std::string infc_name = netIntf.name();
			initializeInterface(infc_name);
		}
	}
	
	// Adds the subnetworks
    std::vector<Poco::Net::NetworkInterface> list2 = Poco::Net::NetworkInterface::list();
    std::vector<Poco::Net::NetworkInterface>::iterator it2 = list2.begin();
    for(;it2!=list2.end();it2++)
	{
		Poco::Net::NetworkInterface netIntf2 = *it2;
		if (!((netIntf2.address()).isLoopback())){
			addSubnetwork(netIntf2.name(), netIntf2);
		}
	}	
	
	// Temporal to prove that the functions are Ok.
	/*addClassHTB(Poco::Net::IPAddress ipaddr, 
					 Poco::Net::IPAddress submask, uint64_t rate, 
					 uint64_t ceil, uint32_t burst, uint32_t cburst, 
					 uint32_t prio, int quantum, int limit, int perturb);
					 */
}


int TrafficControlSys::getIndexOfInterface(std::string netIntf)
{
	unsigned i=0;
	std::vector<NetworkInterfacePtr>::size_type sz = _subInterfaces.size();
	for (; i<sz; i++) 
		if (netIntf.compare((*_subInterfaces[i]).getName())==0)
			break;
	if (i == sz)
		return -1;
	else
		return (int) i;
}

void TrafficControlSys::initializeInterface(std::string network_interface_name)
{
	int val_return = 0;
	uint64_t rate = 12500000;
	uint64_t ceil = 12500000;
	uint32_t burst = 25000;
	uint32_t cburst = 25000;
	
	int index = getIndexOfInterface(network_interface_name);
	if (index < 0){ // The interface is not initialized
	   
	   try{
		   std::cout << "1" << std::endl;
		   // Creates the Traffic Control Manager for the interface.
		   uint32_t HandlerMaj = 1; 
		   uint32_t HandlerMin = 0;
		   std::cout << "2" << std::endl;
		   NetworkInterfacePtr netIntfc(new NetworkInterface(network_interface_name, HandlerMaj, HandlerMin));
			
		   _subInterfaces.push_back(netIntfc);
		   std::cout << "3" << std::endl;
		   
		   // delete possible Qdisc already created in the system.
		   (*netIntfc).deleteQdiscRootHTB();
		   std::cout << "Possible Qdiscs were deleted" << std::endl;
		   
		   //Creates the root Qdisc 
		   (*netIntfc).addQdiscRootHTB();
		   std::cout << "Added the root Qdisc" << std::endl;

		   // Creates the root class
		   (*netIntfc).addClassRootHTB(rate, ceil, burst, cburst);
		   std::cout << "Added the root class" << std::endl;
		   
		} catch(TrafficControlException &e){
		   std::cout << "Error: The interface could not be initialized" << std::endl;
		}	
	}
}

void TrafficControlSys::addSubnetwork(std::string network_interface_name, 
									  Poco::Net::NetworkInterface netIfc)
{
	std::cout << "addSubnetwork" << std::endl; 
	int index = getIndexOfInterface(network_interface_name);
	if (index < 0){ // The interface is not initialized
		std::cout << "Error: The interface is not initialized" << std::endl;
	} else{
		NetworkInterfacePtr netIntfc = _subInterfaces[index];
		(*netIntfc).addSubNetworkInterface(netIfc);
	}
}

void TrafficControlSys::reinitialize(Poco::Util::Application &app)
{
}

void TrafficControlSys::uninitialize()
{
}

void TrafficControlSys::defineOptions(Poco::Util::OptionSet& options)
{
    Subsystem::defineOptions(options);
    options.addOption(
        Poco::Util::Option("mhelp","mh","Display help about sql")
        .required(false)
        .repeatable(false));
}

const char* TrafficControlSys::name() const
{
    return "SUB-Traffic-Control";
}

}  /// End net namespace

}  /// End ecodtn namespace
