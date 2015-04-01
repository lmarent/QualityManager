
/*! \file   TcNetqosErrorCode.h

    Copyright 2014-2015 Universidad de los Andes, Bogota, Colombia

    This file is part of Network Measurement and Accounting System (NETQos).

    NETQos is free software; you can redistribute it and/or modify 
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
    Error class used for exceptions

    $Id: TcNetqosErrorCode.h 748 2015-03-11 16:15:00 amarentes $
*/


#ifndef _TC_NETQOS_ERRORCODE_H_
#define _TC_NETQOS_ERRORCODE_H_

#define NET_TC_SUCCESS 0
#define NET_TC_QDISC_ALLOC_ERROR -1
#define NET_TC_QDISC_SETUP_ERROR -2
#define NET_TC_QDISC_ESTABLISH_ERROR -3
#define NET_TC_CLASS_ALLOC_ERROR -4
#define NET_TC_CLASS_SETUP_ERROR -5
#define NET_TC_CLASS_ESTABLISH_ERROR -6
#define NET_TC_CLASSIFIER_ALLOC_ERROR -7
#define NET_TC_CLASSIFIER_SETUP_ERROR -8
#define NET_TC_CLASSIFIER_ESTABLISH_ERROR -9
#define NET_TC_LINK_ERROR -10
#define NET_TC_PARAMETER_ERROR -11


#endif // _TC_NETQOS_ERRORCODE_H_
