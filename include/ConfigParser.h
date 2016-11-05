
/*!  \file   ConfigParser.h

    Copyright 2014-2015 Universidad de los Andes, Bogotá, Colombia

    This file is part of Network Quality of Service System (NETQOS).

    NETQOS is free software; you can redistribute it and/or modify 
    it under the terms of the GNU General Public License as published by 
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NETQOS is distributed in the hope that it will be useful, 
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this software; if not, write to the Free Software 
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Description:
    parses configuration file and adds item into config db

    $Id: ConfigParser.h 748 2015-07-26 10:07:00 amarentes $
*/

#ifndef _CONFIGPARSER_H_
#define _CONFIGPARSER_H_


#include "stdincpp.h"
#include "libxml/parser.h"
#include "Logger.h"
#include "XMLParser.h"


//! configuration item
typedef struct configItem
{
    string group;
    string module;
    string name;
    string value;
    string type;
} configItem_t;


//! allow or deny
typedef enum 
{ 
    ALLOW=0, 
    DENY=1
} ad_t;

//! access configuration item
typedef struct
{
    ad_t ad;
    string value;
    string type;
    /*! may contain IP address if host based item and value is
        valid IP address or host name */
    string resolve_addr;
} configADItem_t;

//! list of config items
class configItemList_t : public list<configItem_t> {
  public:
    string getValue( string name );
};

typedef list<configItem_t>::iterator configItemListIter_t;
typedef list<configItem_t>::const_iterator configItemListConstIter_t;


//! list of access items
typedef list<configADItem_t> configADList_t;
typedef list<configADItem_t>::iterator configADListIter_t;

//! misc list (random access based on name required)
typedef map<string,configItem_t>            miscList_t;
typedef map<string,configItem_t>::iterator  miscListIter_t;
typedef map<string,configItem_t>::const_iterator  miscListConstIter_t;


class ConfigParser : public XMLParser
{

  private:

    static string bindir;  //<! directory where program binary is located, i.e. argv[0]

    Logger *log;
    int ch;

    //! parse a single config item (aka preference)
    configItem_t parsePref(xmlNodePtr cur);

  public:

    /*! construct a ConfigParser
	\arg \c filename - filename to read configuration from
        \arg \c binary - location of binary file, i.e. argv[0]
     */
    ConfigParser(string dtdfilename, string filename, string binary = "");

    //! destroy a config parser
    virtual ~ConfigParser() {}

    /* \short  parses the input and add parse config items to list and
               add parsed access config items to access list

       \arg \c list    list for config items to be put in
       \arg \c alist   list for access config items to be put in
    */
    virtual void parse(configItemList_t *list, configADList_t *alist);

};

//! overload operator << for configItem_t so that it can be thorown in an ostream
ostream& operator<< ( ostream &os, configItem_t &item );

//! overload operator << for configItemList_t
ostream& operator<< ( ostream &os, configItemList_t &list );

//! overload operator << for configADList_t
ostream& operator<< ( ostream &os, configADList_t &list );

//! overload operator << for configADItem_t so that it can be thorown in an ostream
ostream& operator<< ( ostream &os, configADItem_t &item );


#endif //_CONFIGPARSER_H_
