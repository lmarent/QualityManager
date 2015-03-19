
/*! \file   ProcError.cpp

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

    $Id: ProcError.h 748 2015-03-11 16:15:00 amarentes $
*/

#ifndef _PROC_ERROR_H__
#define _PROC_ERROR_H_


#include "stdincpp.h"


/*! \short   generic error exception

Error represents an exception that can be thrown.
It includes a string that can be used to further 
specify the error encountered. Optionally it also
includes a number.
*/

class ProcError
{
  protected:

    //! error number
    int errorNo;

    //!< error string
    std::string error;
    
  public:
    //!< create unnamed error exception
    ProcError() : errorNo(0), error("") {}     

     //!< create named error exception
    ProcError(const std::string new_error="") : errorNo(0), error(new_error) {}

    //!< create named error exception
    ProcError(const int err_no=0, const std::string err_str="") : errorNo(err_no), error(err_str) {}

    //!< printf-style constructor
    ProcError(const int err_no, const char *fmt, ... );

    //!< printf-style constructor
    ProcError(const char *fmt, ... );

    //!< get error msg from exception
    std::string getError() 
    { 
        return error;
    } 

    //! get error number
    int getErrorNo()
    {
        return errorNo;
    }

    void dump( std::ostream &os ) ;
};


//! overload for << so that a Error object can be thrown into an ostream
std::ostream& operator<< ( std::ostream &os, ProcError &e );


#endif // _PROC_ERROR_H_
