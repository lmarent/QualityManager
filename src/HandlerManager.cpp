//
// HandlerManager.cpp
//
// $Id: //ecodtn/0.1/src/HandlerManager.cpp#1 $
//
// Library: Ecodtn
// Package: TrafficControl
// Module:  NetworkQuality
//
// Definition of the HandlerManager class.
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

#include "HandlerManager.h"
#include "TrafficControlException.h"

namespace ecodtn
{

namespace net
{

HandlerManager::HandlerManager()
{
	
	leftValue = 65534;
	lastGivenValue = 0; 
	
}

HandlerManager::~HandlerManager()
{
}

uint16_t HandlerManager::getNextValue()
{
	// _mutex_next_value.lock();
	add_one_position(0);
	uint16_t nextValue = lastGivenValue;
	// _mutex_next_value.unlock();
	return nextValue;
}

void HandlerManager::releaseValue(uint16_t valueRelease)
{
	// _mutex_left_value.lock();
	// Verify if the value is in the current window
	if ((leftValue <= lastGivenValue) && (lastGivenValue <= MAX_HANDLER)){
		if ((leftValue <= valueRelease) && (valueRelease <= lastGivenValue)){
			free_value(valueRelease);
		}
		// Discard becuase the value given is not in the current window.
	}
	if (lastGivenValue < leftValue){
	    if ((leftValue <= valueRelease) && (valueRelease <= MAX_HANDLER)){
			free_value(valueRelease);
		}
		if ((0 <= valueRelease ) && (valueRelease <= lastGivenValue)){
			free_value(valueRelease);
		}
	}
	// _mutex_left_value.unlock();
}

void HandlerManager::add_one_position(int index)
{
	if (index == 0){ // NEXT_NUMBER
		if (lastGivenValue == MAX_HANDLER)
		{
			if (leftValue > 0){
				lastGivenValue = 0;
			} else {
				throw TrafficControlException("Unavailable numbers to assign the handler", -1); 
			}
		} else {
			lastGivenValue += 1;
		}
	}
	
	if (index == 1){ // LEFT VALUE
		if (leftValue == MAX_HANDLER)
			leftValue = 0;
		else
			leftValue = leftValue + 1;		
	}
}

void HandlerManager::free_value(uint16_t valueRelease)
{
	if (leftValue == valueRelease){
		add_one_position(1);
		for (release_numbers_map::iterator ii = released.begin(); ii != released.end(); ++ii) {
			if (leftValue == ii->second){
				//remove the element from the map and increment again leftValue
				released.erase (ii->first);   
				add_one_position(1);
			}
			else{
				break;
			}	
		}
	}
	else
	{
		uint32_t key;
		if (leftValue > lastGivenValue)
			key =  valueRelease + MAX_HANDLER;
		else
			key =  valueRelease;
			
		released.insert( std::pair<uint32_t,uint16_t>(key,valueRelease) );
	}
}

}  /// End net namespace

}  /// End ecodtn namespace