/*!\file   FlowIdSource.cpp

    Copyright 2014-2015 Universidad de los Andes, Bogota, Colombia

    This file is part of Network QoS System (NETQOS).

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

	manage unique numeric Resource Request id space

    $Id: FlowIdSource.cpp 2016-11-15 23:14:00Z amarentes $
*/

#include "FlowIdSource.h"


FlowIdSource::FlowIdSource(int _unique)
  : num(1), unique(_unique)
{

}

FlowIdSource::~FlowIdSource()
{
    // nothing to do
}

unsigned short FlowIdSource::newId(void)

{

    unsigned short id;

    if (freeIds.empty())
    {
		int comp = num + 1;
		if (comp < 4094)
		{
			num++;
		}
		else
		{
			throw Error("No Flow Id number available at this moment - current number:%d", num);
		}

        while (true)
        {

			IdsReservIterator i = find(idReserved.begin(), idReserved.end(), num);
			if (i != idReserved.end())
			{
				int comp = num + 1;
				if (comp < 4094)
				{
					num++;
				}
				else
				{
					throw Error("No Flow Id number available at this moment - current number:%d", num);
				}
			}
			else
			{
				return num;
			}
		}
    }

    // else use id from free list
    id = freeIds.front();
    freeIds.pop_front();

    return id;

}

void FlowIdSource::freeId(unsigned short id)

{
  cout << "freeId - actual number" << num << endl;
  if (!unique) {
	if (id == num){
    	num--; // Only start to operate when the free list size is 0.
	}
	else {
    	freeIds.push_back(id);
	}
  }

  cout << "freeId - new number" << num << endl;
}

void FlowIdSource::dump( ostream &os )
{

    os << "FlowIdSource dump:" << endl
       << "Number of used ids is : " << num - freeIds.size() << endl;

}

ostream& operator<< ( ostream &os, FlowIdSource &aim )

{

    aim.dump(os);
    return os;

}