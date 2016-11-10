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

std::string getNewRule()
{
    int ruleId = nextRuleId;
    std::ostringstream convert;   
    convert << ruleId;
    return convert.str();    
}

std::string getAddress()
{
    int mask = 254;
    std::string prefix("10.0.");
    
    int next_part1 = nextRuleId / mask;
    int next_part2 = nextRuleId % mask;

    std::ostringstream convert;
    convert << prefix;
    convert << next_part1 << ".";
    convert << next_part2;
    return convert.str();    
    
}

std::string getRate()
{

    int maxValue = 1500;
    
    srand((unsigned int) time (NULL)); //activates the generator
    //...
    int rate = rand()%maxValue;        //gives a random from 0 to 9

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

void replace_rule_id(std::string &str)
{
     std::string rule_id = getNewRule();
     std::size_t found = str.find("rule_id_param");
     if (found!=std::string::npos)
        str.replace(found, std::string("rule_id_param").size(), rule_id);
}

void replace_ip_address(std::string &str)
{
     std::string ip_address = getAddress();
     std::size_t found = str.find("ip_address_param");
     if (found!=std::string::npos)
        str.replace(found, std::string("ip_address_param").size(), ip_address);
}

void replace_rate(std::string &str)
{
     std::string rate_str = getRate();
     std::size_t found = str.find("rate_param");
     if (found!=std::string::npos)
        str.replace(found, std::string("rate_param").size(), rate_str);
}

void replace_burst(std::string &str)
{
     std::string burst_str("1500");
     std::size_t found = str.find("burst_param");
     if (found!=std::string::npos)
        str.replace(found, std::string("burst_param").size(), burst_str);
}

void replace_priority(std::string &str)
{
     std::string priority_str("2");
     std::size_t found = str.find("priority_param");
     if (found!=std::string::npos)
        str.replace(found, std::string("priority_param").size(), priority_str);
}

void replace_duration(std::string &str)
{
     std::string duration_str("1000");
     std::size_t found = str.find("duration_param");
     if (found!=std::string::npos)
        str.replace(found, std::string("duration_param").size(), duration_str);
}

int delete_rule(int rule)
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
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:12244/rm_task");
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    

    std::ostringstream convert;   
    convert << rule;

    std::string bodyStr = getRuleSet() + std::string(".") +convert.str();
       
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

int post_rule(void) 
{

  CURL *curl;

  CURLcode res;

  struct MemoryStruct chunk;

  struct data config;

  int rule = nextRuleId;
  
  config.trace_ascii = 1; /* enable ascii tracing */

  std::ifstream t("rule.xml");
  std::string strFile((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());


  chunk.memory = (char*) malloc(1);

  chunk.size = 0;

  chunk.memory[chunk.size] = 0;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();

  if(curl) {

    // host:post/action
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:12244/add_task");
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);    
    
    // This part replaces the variables in the xml file. That makes every rule different
        
    std::string bodyStr = strFile;
    replace_rule_set(bodyStr);
    replace_ip_address(bodyStr);
    replace_rule_id(bodyStr);
    replace_rate(bodyStr);
    replace_burst(bodyStr);
    replace_priority(bodyStr);
    replace_duration(bodyStr);
    incrementRuleNumber();

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

  return rule;

}


int main(void)
{
    int i=0;
    int rule=0;
    int MAX_COUNT=1;
    
    while (i <= MAX_COUNT)
    {
        rule = post_rule();
        // delete_rule(rule);
        i = i + 1;
    }
}
