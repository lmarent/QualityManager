//
// NetworkInterface.h
//
// $Id: //ecodtn/0.1/include/NetworkInterface.h#1 $
//
// Library: Ecodtn
// Package: TrafficControl
// Module:  NetworkQuality
//
// Definition of the NetworkInterface class.
//
// Copyright (c) 2014, Luis Andres Marentes C.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//

#ifndef NetworkInterface_INCLUDED
#define NetworkInterface_INCLUDED


#include <Poco/RefCountedObject.h>
#include "HandlerManager.h"
#include "TrafficControlManager.h"

namespace ecodtn
{

namespace net
{

class NetworkInterface : public HandlerManager, public Poco::RefCountedObject
{

public: 	
		
	
	NetworkInterface( std::string intfc_name, uint32_t HandlerMaj_, uint32_t HandlerMin_);
					   
	~NetworkInterface();
	
	std::string getName();
	
	uint32_t getMajorHandler();
	
	uint32_t getMinorHandler();
	
	void addSubNetworkInterface(Poco::Net::NetworkInterface intf);
	
	void deleteSubNetworkInterface(Poco::Net::NetworkInterface intf);
	
	void addQdiscRootHTB(void);
	
	void deleteQdiscRootHTB(void);
	
	void addClassRootHTB(uint64_t rate, uint64_t ceil, 
						  uint32_t burst, uint32_t cburst);
	
	void addClassHTB(Poco::Net::IPAddress ipaddr, 
					 Poco::Net::IPAddress submask, uint64_t rate, 
					 uint64_t ceil, uint32_t burst, uint32_t cburst, 
					 uint32_t prio, int quantum, int limit, int perturb);
     /// The value keyoff means what part of the octec to read in order to compare, 
     /// value 12 means source IP ; value 16 means destination IP 
     /// If we put the mask 255.255.255.0 it will match all ip that start with 10.100.1.
     /// If we put the mask 255.255.0.0 it will match all ip that start with 10.100.
					 
	
	void deleteClassHTB(Poco::Net::IPAddress ipaddr, uint64_t rate );

private:

	std::string _intfc_name;
	uint32_t _handlerMaj; 
	uint32_t _handlerMin;
    TrafficControlManager _manager;
};

}  /// End net namespace

}  /// End ecodtn namespace

#endif // NetworkInterface_INCLUDED
