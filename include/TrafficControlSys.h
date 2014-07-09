#ifndef TrafficControlSys_INCLUDED
#define TrafficControlSys_INCLUDED

#include <Poco/Util/Application.h>
#include <Poco/Util/Subsystem.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/HelpFormatter.h>
#include <iostream>

#include "HandlerManager.h"
#include "NetworkInterface.h"

namespace ecodtn
{

namespace net
{

typedef std::pair<std::string, NetworkInterface> interface_node;

class TrafficControlSys : public Poco::Util::Subsystem
{
public:
    TrafficControlSys(void);
    ~TrafficControlSys(void);
protected:
    void initialize(Poco::Util::Application &app);
    void reinitialize(Poco::Util::Application &app);
    void uninitialize();

    void initializeInterface(std::string netIntf);
    void defineOptions(Poco::Util::OptionSet& options);
    virtual const char* name() const;

private:
    char p_cName;
    std::map <std::string, NetworkInterface> _subInterfaces;
};


}  /// End net namespace

}  /// End ecodtn namespace

#endif // TrafficControlSys_INCLUDED
