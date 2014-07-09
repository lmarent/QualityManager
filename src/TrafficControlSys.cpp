#include "TrafficControlSys.h"
#include <Poco/Net/NetworkInterface.h>
#include <vector>

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
    
    // Reads all interfaces
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
    
    
}


void TrafficControlSys::initializeInterface(std::string network_interface_name)
{
	int val_return = 0;
	uint64_t rate = 12500000;
	uint64_t ceil = 12500000;
	uint32_t burst = 25000;
	uint32_t cburst = 25000;
	
	if (_subInterfaces.count(network_interface_name)>0){
	
	} else{
	   
	   // Creates the Traffic Control Manager for the interface.
	   uint32_t HandlerMaj = 1; 
	   uint32_t HandlerMin = 0;
	   // NetworkInterface netIntfc(network_interface_name, HandlerMaj, HandlerMin);
	   // _subInterfaces.insert( std::pair<std::string, NetworkInterface>(network_interface_name,netIntfc));
	   // Creates the root Qdisc 
	   // netIntfc.addQdiscRootHTB();

	   // Creates the root class
	   // netIntfc.addClassRootHTB(rate, ceil, burst, cburst);
		
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
