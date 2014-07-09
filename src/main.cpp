#include "NetworkQualityServer.h"

int main(int argc, char *argv[])
{
	
    ecodtn::net::NetworkQualityServer app;
    return app.run(argc, argv);
	
	/*
    std::vector<Poco::Net::NetworkInterface> list = Poco::Net::NetworkInterface::list();
    std::vector<Poco::Net::NetworkInterface>::iterator it = list.begin();
    for(;it!=list.end();it++)
	{
		Poco::Net::NetworkInterface netIntf = *it;
		Poco::Net::IPAddress ipAddr = netIntf.address();
		std::string aprStd = ipAddr.toString();
		std::string name = netIntf.name();
		int interfIndex = netIntf.index();
		std::cout << "address:" << aprStd << "\n";
		std::cout << "name:" << name << "\n";
		std::cout << "index:";
		std::cout << interfIndex;
		std::cout << "\n";
	}
    */
}
