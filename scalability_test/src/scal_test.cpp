/*

 * HTTP POST with authentiction using "basic" method.

 * Hybrid of anyauthput.c and postinmemory.c

 *

 * Usage:

 * cc basicauthpost.c -lcurl -o basicauthpost

 * ./basicauthpost

 *

 */

#include <stdio.h>

#include <stdlib.h>

#include <string.h> // memcpy

#include <string>

#include <curl/curl.h>

#include <fstream>

#include <streambuf>

#include <ctime>

#include <sstream>      // std::ostringstream

#include <iostream>

#include "Logger.h"
#include "stdincpp.h"
#include "CommandLineArgs.h"

struct MemoryStruct {

  char *memory;

  size_t size;

};


static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *context) {

  size_t bytec = size * nmemb;

  struct MemoryStruct *mem = (struct MemoryStruct *)context;

  mem->memory = (char *) realloc(mem->memory, mem->size + bytec + 1);

  if(mem->memory == NULL) {

    printf("not enough memory (realloc returned NULL)\n");

    return 0;

  }

  memcpy(&(mem->memory[mem->size]), ptr, bytec);

  mem->size += bytec;

  mem->memory[mem->size] = 0;

  return nmemb;

}


struct data {
  char trace_ascii; /* 1 or 0 */
};

// The id for the next rule to send to NetQoS
int nextRuleId = 1;
int ruleSet = 1;

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;

  unsigned int width=0x10;

  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;

  fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
          text, (long)size, (long)size);

  for(i=0; i<size; i+= width) {

    fprintf(stream, "%4.4lx: ", (long)i);

    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i+c < size)
          fprintf(stream, "%02x ", ptr[i+c]);
        else
          fputs("   ", stream);
    }

    for(c = 0; (c < width) && (i+c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
        i+=(c+2-width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
        i+=(c+3-width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}

static
int my_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{

  fprintf(stdout, "in my_trace");

  struct data *config = (struct data *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */

  switch (type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
  default: /* in case a new one is introduced to shock us */
    return 0;

  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }

  dump(text, stdout, (unsigned char *)data, size, config->trace_ascii);
  return 0;
}


void incrementRuleNumber()
{
    nextRuleId++;
}

std::string getRuleSet()
{

    std::ostringstream convert;
    convert << ruleSet;
    return convert.str();

}

std::string getNewRule(int rule)
{
    int ruleId = rule;
    std::ostringstream convert;
    convert << ruleId;
    return convert.str();
}

std::string getNewPort(int dst_port)
{
    int port = dst_port;
    std::ostringstream convert;
    convert << port;
    return convert.str();
}


std::string getRate()
{

    int maxValue = 8500;

    srand((unsigned int) time (NULL)); //activates the generator
    //...
    int rate = rand()%maxValue;        //gives a random from 0 to 15000

	rate = maxValue;

    std::ostringstream convert;
    convert << rate;
    return convert.str();

}

void replace_rule_set(std::string &str)
{
    std::string rule_set = getRuleSet();
    std::size_t found = str.find("rule_set_param");
    if (found!=std::string::npos)
        str.replace(found, std::string("rule_set_param").size(), rule_set);

}

void replace_rule_id_uplink(int rule, std::string &str)
{
     std::string rule_id = getNewRule(rule);
     std::size_t found = str.find("rule_id_param1");
     if (found!=std::string::npos)
        str.replace(found, std::string("rule_id_param1").size(), rule_id);
}

void replace_ip_address_uplink(std::string ip_address, std::string &str)
{
     std::size_t found = str.find("ip_address_param1");
     if (found!=std::string::npos)
        str.replace(found, std::string("ip_address_param1").size(), ip_address);
}

void replace_dst_port_uplink(int dst_port, std::string &str)
{
 	std::string dst_port_str =  getNewPort(dst_port);
 	std::size_t found = str.find("dst_port_param1");
     if (found!=std::string::npos)
        str.replace(found, std::string("dst_port_param1").size(), dst_port_str);
}

void replace_rate_uplink(std::string &str)
{
     std::string rate_str = getRate();
     std::size_t found = str.find("rate_param1");
     if (found!=std::string::npos)
        str.replace(found, std::string("rate_param1").size(), rate_str);
}

void replace_burst_uplink(std::string &str)
{
     std::string burst_str("1500");
     std::size_t found = str.find("burst_param1");
     if (found!=std::string::npos)
        str.replace(found, std::string("burst_param1").size(), burst_str);
}

void replace_priority_uplink(std::string &str)
{
     std::string priority_str("2");
     std::size_t found = str.find("priority_param1");
     if (found!=std::string::npos)
        str.replace(found, std::string("priority_param1").size(), priority_str);
}

void replace_duration_uplink(std::string &str)
{
     std::string duration_str("30");
     std::size_t found = str.find("duration_param1");
     if (found!=std::string::npos)
        str.replace(found, std::string("duration_param1").size(), duration_str);
}


void replace_rule_id_downlink(int rule, std::string &str)
{
     std::string rule_id = getNewRule(rule);
     std::size_t found = str.find("rule_id_param2");
     if (found!=std::string::npos)
        str.replace(found, std::string("rule_id_param2").size(), rule_id);
}

void replace_ip_address_downlink(std::string ip_address, std::string &str)
{
     std::size_t found = str.find("ip_address_param2");
     if (found!=std::string::npos)
        str.replace(found, std::string("ip_address_param2").size(), ip_address);
}

void replace_rate_downlink(std::string &str)
{
     std::string rate_str = getRate();
     std::size_t found = str.find("rate_param2");
     if (found!=std::string::npos)
        str.replace(found, std::string("rate_param2").size(), rate_str);
}

void replace_burst_downlink(std::string &str)
{
     std::string burst_str("1500");
     std::size_t found = str.find("burst_param2");
     if (found!=std::string::npos)
        str.replace(found, std::string("burst_param2").size(), burst_str);
}

void replace_priority_downlink(std::string &str)
{
     std::string priority_str("2");
     std::size_t found = str.find("priority_param2");
     if (found!=std::string::npos)
        str.replace(found, std::string("priority_param2").size(), priority_str);
}

void replace_duration_downlink(std::string &str)
{
     std::string duration_str("30");
     std::size_t found = str.find("duration_param2");
     if (found!=std::string::npos)
        str.replace(found, std::string("duration_param2").size(), duration_str);
}

int delete_rule_set(std::string host, int host_port, int rule)
{

  CURL *curl;

  CURLcode res;

  struct MemoryStruct chunk;

  struct data config;

  config.trace_ascii = 1; /* enable ascii tracing */

  chunk.memory = (char*) malloc(1);

  chunk.size = 0;

  chunk.memory[chunk.size] = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();

  if(curl) {

    // host:post/action

    std::ostringstream url;
    url << "http://";
    url << host;
    url << ":";
    url << host_port;
    url << "/rm_task"; // action


    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
    curl_easy_setopt(curl, CURLOPT_URL, (url.str()).c_str());
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    std::ostringstream convert;
    convert << rule;

    std::string bodyStr = getRuleSet() + std::string(".") + convert.str();

    char *body = curl_easy_escape(curl, bodyStr.c_str(), 0);

    char *fields = (char *) malloc(4 + strlen(body) + 1);

    sprintf(fields, "RuleID=%s", body );

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);

    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);

    curl_easy_setopt(curl, CURLOPT_USERNAME, "admin");

    curl_easy_setopt(curl, CURLOPT_PASSWORD, "admin");

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    // The output from the example URL is easier to read as plain text.

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Accept: text/plain");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Make the example URL work even if your CA bundle is missing.

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {

      fprintf(stderr, "curl_easy_perform() failed: %s\n",

              curl_easy_strerror(res));

    } else {

      printf("%s\n",chunk.memory);

    }

    // Remember to call the appropriate "free" functions.

    free(fields);

    curl_free(body);

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    free(chunk.memory);

    curl_global_cleanup();

  }

  return 0;

}

int post_rule_set(std::string clientaddress, std::string host, int host_port, std::string ruleFile, int ruleuplink, int ruledownlink, int dst_port)
{

  CURL *curl;

  CURLcode res;

  struct MemoryStruct chunk;

  struct data config;
  config.trace_ascii = 1; /* enable ascii tracing */

  std::ifstream t(ruleFile.c_str());
  std::string strFile((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());


  chunk.memory = (char*) malloc(1);

  chunk.size = 0;

  chunk.memory[chunk.size] = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();

  if(curl) {

    std::ostringstream url;
    url << "http://";
    url << host;
    url << ":";
    url << host_port;
    url << "/add_task"; // action


    // host:post/action
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);
    curl_easy_setopt(curl, CURLOPT_URL, (url.str()).c_str());
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // This part replaces the variables in the xml file. That makes every rule different

    std::string bodyStr = strFile;
    replace_rule_set(bodyStr);

    // Replace fields associated with the uplink
    replace_ip_address_uplink(clientaddress, bodyStr);
    replace_rule_id_uplink(ruleuplink, bodyStr);
    replace_dst_port_uplink(dst_port, bodyStr);
    replace_rate_uplink(bodyStr);
    replace_burst_uplink(bodyStr);
    replace_priority_uplink(bodyStr);
    replace_duration_uplink(bodyStr);

    // Replace fields associated with the downlink
    replace_ip_address_downlink(clientaddress, bodyStr);
    replace_rule_id_downlink(ruledownlink, bodyStr);
    replace_rate_downlink(bodyStr);
    replace_burst_downlink(bodyStr);
    replace_priority_downlink(bodyStr);
    replace_duration_downlink(bodyStr);


    std::cout << "Body:" << bodyStr << std::endl;


    char *body = curl_easy_escape(curl, bodyStr.c_str(), 0);

    char *fields = (char *) malloc(4 + strlen(body) + 1);

    sprintf(fields, "Rule=%s", body );

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, fields);

    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_BASIC);

    curl_easy_setopt(curl, CURLOPT_USERNAME, "admin");

    curl_easy_setopt(curl, CURLOPT_PASSWORD, "admin");

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    // The output from the example URL is easier to read as plain text.

    struct curl_slist *headers = NULL;

    headers = curl_slist_append(headers, "Accept: text/plain");
    headers = curl_slist_append(headers, "Expect:");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Make the example URL work even if your CA bundle is missing.

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
    {

      fprintf(stderr, "curl_easy_perform() failed: %s\n",

              curl_easy_strerror(res));

    } else {

      printf("%s\n",chunk.memory);

    }

    // Remember to call the appropriate "free" functions.

    free(fields);

    curl_free(body);

    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);

    free(chunk.memory);

    curl_global_cleanup();

  }

  return 0;

}

/* -------------------- getHelloMsg -------------------- */

std::string getHelloMsg()
{
    std::ostringstream s;

    static char name[128] = "\0";

    if (name[0] == '\0') { // first time
        gethostname(name, sizeof(name));
    }

    s << "scalTest build " << BUILD_TIME
      << ", running at host \"" << name << "\"," << std::endl
      << "compile options: "
#ifndef ENABLE_THREADS
      << "_no_ "
#endif
      << "multi-threading support, "
#ifndef USE_SSL
      << "_no_ "
#endif
      << "secure sockets (SSL) support"
      << std::endl;

    return s.str();
}



int main(int argc, char *argv[])
{
    auto_ptr<CommandLineArgs> args;

	try
	{
		auto_ptr<CommandLineArgs> _args(new CommandLineArgs());
		args = _args;

		// basic args
		args->add('r', "RuleFile", "<path/rulefile>", "use alternative rule file",
					  "MAIN", "rulefile");
		args->add('l', "LogFile", "<path/file>", "use alternative log file",
					  "MAIN", "logfile");
		args->addFlag('V', "RelVersion", "show version info and exit",
						  "MAIN", "version");
#ifdef USE_SSL
		args->addFlag('x', "UseSSL", "use SSL for control communication",
						  "MAIN", "usessl");
#endif
		args->add('C', "ClientAddres", "<clientAddres>", "use alternative client address",
					  "MAIN", "caddr");

		args->add('H', "RuleManagerAddres", "<rhostAddres>", "use alternative rule manager address",
					  "MAIN", "raddr");

		args->add('P', "RuleManagerPort", "<rportnumber>", "use alternative rule manager port",
					  "MAIN", "rport");

		args->add('U', "uplinkRuleId", "<uruleidnumber>", "use alternative uplink rule id number",
					  "MAIN", "urid");

		args->add('D', "downlinkRuleId", "<druleidnumber>", "use alternative downlink rule id number",
					  "MAIN", "drid");

		args->add('O', "HostPort", "<hportnumber>", "use alternative host port number",
					  "MAIN", "hport");


		// parse command line args
		if (args->parseArgs(argc, argv)) {
			 // user wanted help
				exit(0);
		}

		if (args->getArgValue('V') == "yes") {
			cout << getHelloMsg();
			exit(0);
		}

		string rulefile = args->getArgValue('r');
		if (rulefile.empty())
		{
			rulefile = DEFAULT_RULE_FILE;
		}
		string logfile = args->getArgValue('l');
		if (logfile.empty())
		{
			logfile = DEFAULT_LOG_FILE;
		}

		string clientaddress = args->getArgValue('C');
		if (clientaddress.empty())
		{
			Error("Parameter clientaddress is required");
		}

		string hostaddress = args->getArgValue('H');
		if (hostaddress.empty())
		{
			Error("Parameter hostaddress is required");
		}

		string ruleport = args->getArgValue('P');
		if (ruleport.empty())
		{
			Error("Parameter rule port is required");
		}

		int r_port = atoi(ruleport.c_str());
		if ((r_port <= 0) && (r_port > 65535)){
			Error("Wrong rule port number");
		}

		int uplinkRule;

		string uplinkRuleId = args->getArgValue('U');
		if (uplinkRuleId.empty())
		{

			int maxRule = 65000;

			srand((unsigned int) time (NULL)); //activates the generator

			// Generates a rule id randomly.
			uplinkRule = rand()%maxRule;        //gives a random from 0 to 650000

		}
		else
		{
			uplinkRule = atoi(uplinkRuleId.c_str());
		}


		int downlinkRule;

		string downlinkRuleId = args->getArgValue('D');
		if (downlinkRuleId.empty())
		{

			int maxRule = 65000;

			srand((unsigned int) time (NULL)); //activates the generator

			// Generates a rule id randomly.
			downlinkRule = rand()%maxRule;        //gives a random from 0 to 650000

		}
		else
		{
			downlinkRule = atoi(downlinkRuleId.c_str());
		}


		string hostport = args->getArgValue('O');
		if (hostport.empty())
		{
			Error("Parameter host port is required");
		}

		int h_port = atoi(hostport.c_str());
		if ((h_port <= 0) && (h_port > 65535)){
			Error("Wrong host port number");
		}


		cout << rulefile << endl;

		post_rule_set(clientaddress, hostaddress, r_port, rulefile, uplinkRule, downlinkRule, h_port);
	}
	catch(Error &err){
		cout << err << endl;
	}


    // while (i <= MAX_COUNT)
    // {
    //    rule = post_rule(rule);
    //    // delete_rule(rule);
    //    i = i + 1;
	//    incrementRuleNumber();
    //}
	//
}
