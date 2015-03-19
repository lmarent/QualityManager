
/*! \file   MeterInfo.h

    Copyright 2014-2015 Universidad de los Andes, Bogotá, Colombia

    This file is part of Network Quality Manager System (NETQoS).

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
    meter info data structs used in getinfo mete command 
    Code based on Netmate

    $Id: CommandLineArgs.h 748 2015-03-03 14:40:19 amarentes $
*/

#ifndef _QUALITYMANAGERINFO_H_
#define _QUALITYMANAGERINFO_H_


#include <iostream>
#include "Error.h"

//! types of information available via get_info command
enum infoType_t {
    I_UNKNOWN = -1,
    I_ALL = 0,
    I_QUALITYMANAGER_VERSION, 
    I_UPTIME, 
    I_TASKS_STORED, 
    I_CONFIGFILE,  
    I_USE_SSL, 
    I_HELLO,
    I_TASKLIST,
    I_TASK,
    // insert new items here
    I_NUMQUALITYMANAGERINFOS
};

//! type and value of a single meter info
struct info_t
{
    infoType_t type;
    string     param;
    
    info_t( infoType_t t, string p = "" ) {
        type = t;
        param = p;
    }
};

//! list of meter infos (type + parameters)
typedef vector< info_t >            infoList_t;
typedef vector< info_t >::iterator  infoListIter_t;

//! get numeric info type from string
typedef std::map< string, infoType_t >            typeMap_t;
typedef std::map< string, infoType_t >::iterator  typeMapIter_t;


class QualityManagerInfo {
    
  private:
    
    static char *INFOS[];      //!< names of the info items
    static typeMap_t typeMap; 

    //!< currently stored list of info items
    infoList_t *list;  

public:

    QualityManagerInfo();

    ~QualityManagerInfo();

    //! get info name from info type
    static string getInfoString(infoType_t info);

    //! get info type from info name
    static infoType_t getInfoType (string item);

    //! add info by type
    void addInfo( infoType_t type, string param = "");

    //! add info by name
    void addInfo( string item, string param = "");

    /*! \brief release list of added info_t items.
               caller is responsible for the disposal of this list
    */
    inline infoList_t *getList()
    {
        infoList_t *tmp = list;
        list = NULL;
        return tmp;
    }

};


#endif // _QUALITYMANAGERINFO_H_
