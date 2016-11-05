#include "QualityManager.h"



/*
  version string embedded into executable file
  can be found via 'strings <path>/netqos | grep version'
*/
const char *NETQOS_VERSION = "NetQoS version " VERSION ", (c) 2014-2015 Universidad de los Andes, Colombia";

const char *NETQOS_OPTIONS = "compile options: "
"multi-threading support = "
#ifdef ENABLE_THREADS
"[YES]"
#else
"[NO]"
#endif
", secure sockets (SSL) support = "
#ifdef USE_SSL
"[YES]"
#else
"[NO]"
#endif
" ";



/* ------------------------- main() ------------------------- */


// Log functions are not used before the logger is initialized

int main(int argc, char *argv[])
{

    try {
        // start up the netmate (this blocks until Ctrl-C !)
        cout << NETQOS_VERSION << endl;
#ifdef DEBUG
        cout << NETQOS_OPTIONS << endl;
#endif
        auto_ptr<QualityManager> quality(new QualityManager(argc, argv));
        cout << "Up and running." << endl;

        // going into main loop
        quality->run();

        // shut down the meter
        cout << "Terminating netqos." << endl;

    } catch (Error &e) {
        cerr << "Terminating netqos on error: " << e.getError() << endl;
        exit(1);
    }
}
