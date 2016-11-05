
/*!\file   ProcModule.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogotá, Colombia.

    This file is part of Network Measurement and Accounting System (NETQoS).

    NETQoS is free software; you can redistribute it and/or modify 
    it under the terms of the GNU General Public License as published by 
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    NETQoS is distributed in the hope that it will be useful, 
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this software; if not, write to the Free Software 
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Description:
    container class for packet proc module

    $Id: ProcModule.cpp 748 2015-03-05 17:18:00 amarentes $
*/

#include "Error.h"
#include "Logger.h"
#include "ProcError.h"
#include "ProcModule.h"
#include "ProcModuleInterface.h"
#include "ConfigManager.h"


/* ----- init static class members ----- */

Logger *ProcModule::s_log = NULL;
int     ProcModule::s_ch = 0;

string ProcModule::INFOLABEL[] = 
  { 
      "name",  // module name
      "id", 
      "version", 
      "created", 
      "modified", 
      "brief", 
      "verbose", 
      "htmldoc", 
      "parameters", 
      "results", 
      "name",  // module author name
      "affiliation", 
      "email", 
      "homepage" 
  };


/* ------------------------- ProcModule ------------------------- */

ProcModule::ProcModule( ConfigManager *_cnf, string libname, string libfile, 
                        libHandle_t libhandle, string confgroup ) :
    Module( libname, libfile, libhandle ), confgroup(confgroup), cnf(_cnf)
{    
    if (s_log == NULL ) {
        s_log = Logger::getInstance();
        s_ch = s_log->createChannel("ProcModule");
    }

    s_log->log(s_ch, "Creating - configGroup:%s", confgroup.c_str()  );

    checkMagic(PROC_MAGIC);

    funcList = (ProcModuleInterface_t *) loadAPI( "func" );
 
	setOwnName(libname); // TODO (change): read ownName from module properties XML file

	try
	{
		cnf->dump(cout);
		configItemList_t list = cnf->getItems(confgroup, libname );
		configItem_t * intfc = cnf->getItem("NetInterface");
		configItem_t * intfc2 = cnf->getItem("UseIPv6");
		list.push_back(*intfc);
		list.push_back(*intfc2);
        s_log->log(s_ch, "Configurated Nbr Items:%d", (int) list.size() ); 
		configParam_t *params = cnf->getParamList(list);
		funcList->initModule(params);
	}
	catch(ProcError &e)
	{
		s_log->elog(s_ch, "initialization for module '%s' failed: %s",
					libname.c_str(), funcList->getErrorMsg(e.getErrorNo())
				   );
	
	}
    moduleInfoXML = makeModuleInfoXML();
#ifdef MODULE_DEBUG
    cerr << getModuleInfoXML();
#endif
}


/* ------------------------- parseAttribList ------------------------- */


static string checkNullStr( const char* c ) {
    return (c == NULL) ? "" : c;
}


/* -------------------- makeModuleInfoXML -------------------- */

string ProcModule::makeModuleInfoXML()
{
    ostringstream s;

    // append header
    s << "<moduleinfo>" << endl;

    // append module section
    s << "\t<module>" << endl;
    for (int i = I_MODNAME /*0*/; i <= I_RESULTS; i++ ) {
        s << "\t\t<" << INFOLABEL[i] << ">" 
          << checkNullStr( funcList->getModuleInfo((int)i) )
          << "</" << INFOLABEL[i] << ">" << endl;
    }
    s << "\t</module>" << endl;

    // append author section
    s << "\t<author>" << endl;
    for (int i = I_AUTHOR /*0*/; i < I_NUMINFOS; i++ ) {
        s << "\t\t<" << INFOLABEL[i] << ">" 
          << checkNullStr( funcList->getModuleInfo((int)i) )
          << "</" << INFOLABEL[i] << ">" << endl;
    }
    s << "\t</author>" << endl;

    // append footer
    s << "</moduleinfo>" << endl;
	cout << "module information:" << s.str() << endl;
    return s.str();
}


ProcModule::~ProcModule()
{
	configItemList_t list = cnf->getItems(confgroup, getOwnName() );
	configItem_t * intfc = cnf->getItem("NetInterface");
	configItem_t * intfc2 = cnf->getItem("UseIPv6");
	list.push_back(*intfc);
	list.push_back(*intfc2);
	configParam_t *params = cnf->getParamList(list);
	
    funcList->destroyModule(params);

#ifdef DEBUG
    s_log->dlog(s_ch, "Destroyed" );
#endif
}


/* ------------------------- getTypeInfoText ------------------------- */

string ProcModule::getTypeInfoText()
{
    typeInfo_t* info = typeInfo;
    ostringstream s;
    int indent, i;
   

    // print all info from loaded module
    indent = 0;
    i = 0;
    while (1) {
        if (info[i].type == EXPORTEND ) {
            break;
        }

        if (info[i].type <= INVALID1 || info[i].type >= INVALID2 ) {
            s << endl <<  "error in type description!" << endl << endl
              <<  "-> check 'exportInfo' struct in module source" << endl
              << "    (is it terminated with EXPORT_END?)" << endl;
            break;
        }

        if (info[i].type == LIST ) {	    
            s << "field \t = " << typeLabel(info[i].type) << " ,   content = \""
              << info[i].name << "\"" << endl;
            indent++;

        } else if (info[i].type == LISTEND ) {
            indent--;
        } else  {
            string spaces(50-3*indent, ' ');
            s << "field \t = " <<  spaces << typeLabel( info[i].type ) << " ,  content = \""
              << info[i].name << "\"" << endl;
            if (indent > 0 ) {
                indent--;
            }
        }	
        i++;
    }
	cout << "string to print:" << s.str() << endl;
    return s.str();
}


/* ------------------------- typeLabel ------------------------- */

string ProcModule::typeLabel( int i )
{
    switch (i) {
    case INT8:     return "int8";
    case INT16:    return "int16";
    case INT32:    return "int32";
    case INT64:    return "int64";
    case UINT8:    return "uint8";
    case UINT16:   return "uint16";
    case UINT32:   return "uint32";
    case UINT64:   return "uint64";
    case LIST:     return "list";
    case STRING:   return "string";
    case BINARY:   return "binary";
    case CHAR:     return "char";
    case FLOAT:    return "float";
    case DOUBLE:   return "double";
    case IPV4ADDR: return "IPv4Addr";
    case IPV6ADDR: return "IPv6Addr";
    case EXPORTEND:return "ExportEND";
    case LISTEND:  return "ListEND";
    case INVALID1:
    case INVALID2:
    default:       return "invalid!";
    }
}


/* ------------------------- dump ------------------------- */

void ProcModule::dump( ostream &os )
{
    Module::dump(os);
}


/* ------------------------- operator<< ------------------------- */

ostream& operator<< ( ostream &os, ProcModule &obj )
{
    obj.dump(os);
    return os;
}
