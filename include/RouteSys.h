#ifndef RouteSys_INCLUDED
#define RouteSys_INCLUDED

#include <Poco/Util/Application.h>
#include <Poco/Util/Subsystem.h>
#include <Poco/Util/Option.h>
#include <Poco/Util/OptionSet.h>
#include <Poco/Util/HelpFormatter.h>
#include <iostream>

namespace ecodtn
{

namespace net
{

class RouteSys : public Poco::Util::Subsystem
{
public:
    RouteSys(void);
    ~RouteSys(void);
protected:
    void initialize(Poco::Util::Application &app);
    void reinitialize(Poco::Util::Application &app);
    void uninitialize();

    void defineOptions(Poco::Util::OptionSet& options);
    virtual const char* name() const;
private:
    char p_cName;
};

}  /// End net namespace

}  /// End ecodtn namespace

#endif // RouteSys_INCLUDED
