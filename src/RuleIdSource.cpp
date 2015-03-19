
/*!\file   RuleIdSource.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogota, Colombia

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
	manage unique numeric rule id space - rule idis the flowid.

    $Id: RuleIdSource.cpp 748 2015-03-10 15:24:00 amarentes $

*/

#include "RuleIdSource.h"



RuleIdSource::RuleIdSource(int _unique)
  : num(0), unique(_unique)
{
	idReserved.push_back(0); 		// Reserve for qdisc
	idReserved.push_back(1);  		// Reserve for root class
	idReserved.push_back(65535);	// Reserve for default class
}


RuleIdSource::~RuleIdSource()
{
    // nothing to do
}


unsigned short RuleIdSource::newId(void)
{
    unsigned short id;
    
    if (freeIds.empty()) {
		
        num++;
        while (true)
        {
			IdsReservIterator i = find(idReserved.begin(), idReserved.end(), num);
			if (i != idReserved.end()) {
				num++;
			} else {
				return num;
			}
		}
    }

    // else use id from free list
    id = freeIds.front();
    freeIds.pop_front();

    return id;
}


void RuleIdSource::freeId(unsigned short id)
{
  if (!unique) {
    freeIds.push_back(id);
    num--;
  }
}


void RuleIdSource::dump( ostream &os )
{
    os << "RuleIdSource dump:" << endl
       << "Number of used ids is : " << num - freeIds.size() << endl;
}


ostream& operator<< ( ostream &os, RuleIdSource &rim )
{
    rim.dump(os);
    return os;
}
