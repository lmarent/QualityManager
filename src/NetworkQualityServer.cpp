#include <Poco/Net/SocketReactor.h>
#include <Poco/Net/SocketAcceptor.h>
#include <Poco/Thread.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/OptionCallback.h>
#include <Poco/Util/HelpFormatter.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/DateTimeFormat.h>
#include <Poco/Net/ServerSocket.h>
#include "NetworkQualityServer.h"
#include "RouteSys.h"
#include "TrafficControlSys.h"
#include "ConnectionHandler.h"

namespace ecodtn
{

namespace net
{

    NetworkQualityServer::NetworkQualityServer(): _helpRequested(false)
    {
    }

    NetworkQualityServer::~NetworkQualityServer()
    {
    }

    void NetworkQualityServer::initialize(Poco::Util::Application& self)
    {
        loadConfiguration();
        // Initialize the router subsystem.
		Poco::AutoPtr<RouteSys> routeSys(new RouteSys());
		addSubsystem(routeSys);
		_subsystems.push_back(routeSys);
		
		// Initialize the traffic control subsystem.
		Poco::AutoPtr<TrafficControlSys> trafficControlSys(new TrafficControlSys());		
		addSubsystem(trafficControlSys);
		_subsystems.push_back(trafficControlSys);
		
		// Finally Initialize the application.
        Poco::Util::ServerApplication::initialize(self);               
    }

    void NetworkQualityServer::uninitialize()
    {
        Poco::Util::ServerApplication::uninitialize();
    }

    void NetworkQualityServer::defineOptions(Poco::Util::OptionSet& options)
    {
        Poco::Util::ServerApplication::defineOptions(options);

        options.addOption(
        Poco::Util::Option("help", "h", "display argument help information")
            .required(false)
            .repeatable(false)
            .callback(Poco::Util::OptionCallback<NetworkQualityServer>(
                this, &NetworkQualityServer::handleHelp)));
    }

    void NetworkQualityServer::handleHelp(const std::string& name, 
                    const std::string& value)
    {
        Poco::Util::HelpFormatter helpFormatter(options());
        helpFormatter.setCommand(commandName());
        helpFormatter.setUsage("OPTIONS");
        helpFormatter.setHeader(
            "A server that establish quality parameters and routes.");
        helpFormatter.format(std::cout);
        stopOptionsProcessing();
        _helpRequested = true;
    }

    int NetworkQualityServer::main(const std::vector<std::string>& args)
    {
        if (!_helpRequested)
        {
			// Server Socket
			Poco::Net::ServerSocket svs(2222);
			// Reactor-Notifier
			Poco::Net::SocketReactor reactor;
			Poco::Timespan timeout(2000000); // 2Sec
			reactor.setTimeout(timeout);
			// Server-Acceptor
			Poco::Net::SocketAcceptor<ConnectionHandler> acceptor(svs, reactor);
			// Threaded Reactor
			Poco::Thread thread;
			thread.start(reactor);
			// Wait for CTRL+C
			waitForTerminationRequest();
			// Stop Reactor
			reactor.stop();
			thread.join();
			return Poco::Util::ServerApplication::Application::EXIT_OK; 
        }
        return Poco::Util::ServerApplication::EXIT_OK;
    }

} /// End net namespace

}  /// End ecodtn namespace
