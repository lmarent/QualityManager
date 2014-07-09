#include "RouteSys.h"

namespace ecodtn
{

namespace net
{

RouteSys::RouteSys(void)
{
}

RouteSys::~RouteSys(void)
{
}

void RouteSys::initialize(Poco::Util::Application &app)
{
    app.logger().information("Ini RouteSys");
}

void RouteSys::reinitialize(Poco::Util::Application &app)
{
}

void RouteSys::uninitialize()
{
}

void RouteSys::defineOptions(Poco::Util::OptionSet& options)
{
    Subsystem::defineOptions(options);
    options.addOption(
        Poco::Util::Option("mhelp","mh","Display help about sql")
        .required(false)
        .repeatable(false));
}

const char* RouteSys::name() const
{
    return "SUB-Route";
}

}  /// End net namespace

}  /// End ecodtn namespace
