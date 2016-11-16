
/*! \file  ProcModuleInterface.h

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
    Interface definition for QoS policy execution modules

    $Id: ProcModuleInterface.h 748 2015-03-15 16:19:00 amarentes $

*/

#ifndef __PROCMODULEINTERFACE_H
#define __PROCMODULEINTERFACE_H

#include "stdinc.h"
#include "metadata.h"
#include "FilterDefParser.h"

// configuration parameter passed to the module
typedef struct {
    char *name;
    char *value;
} configParam_t;


//! short   the magic number that will be embedded into every action module
#define PROC_MAGIC   ('N'<<24 | 'M'<<16 | '_'<<8 | 'P')


/*!
    DataType_e identifiers are used within the runtime type
    information struct inside each ProcModule
*/
enum DataType_e {
    INVALID1 = -1,
    EXPORTEND = 0,
    LIST,
    LISTEND,
    CHAR,
    INT8,
    INT16,
    INT32,
    INT64,
    UINT8,
    UINT16,
    UINT32,
    UINT64,
    STRING,
    BINARY,
    IPV4ADDR,
    IPV6ADDR,
    FLOAT,
    DOUBLE,
    INVALID2
};


/*! run time type information array */
typedef struct {
    enum DataType_e type;
    char *name;
} typeInfo_t;


#define LIST_END       { LISTEND, "LEnd" }
#define EXPORT_END     { EXPORTEND, "EEnd" }


/*! parameter values used for a call to 'getModuleInfo' */
enum ActionInfoNumbers_e {
    /* module function attributes */
    I_MODNAME = 0,
    I_ID,
    I_VERSION,
    I_CREATED,
    I_MODIFIED,
    I_BRIEF,
    I_VERBOSE,
    I_HTMLDOCS, /* new */
    I_PARAMS,
    I_RESULTS,
    /* module author attributes */
    I_AUTHOR,
    I_AFFILI,
    I_EMAIL,
    I_HOMEPAGE,
    I_NUMINFOS
};


typedef struct {
    unsigned int id;
    unsigned int ival_msec;
    unsigned int flags;
} timers_t;


typedef enum {
    TM_NONE = 0, TM_RECURRING = 1, TM_ALIGNED = 2, TM_END = 4
    // , TM_NEXTFLAG = 8, TM_NEXTNEXTFLAG = 16, 32, 64 etc pp.
} timerFlags_e;

#define TIMER_END  { (unsigned int)-1, 0 /*ival==0 marks list end*/, TM_END }


static const int offset_refer[] = {
    ( -14 ),
    ( 0 ),
    ( 0 ),
    ( 0 )
};


// FIXME document!
const int MAX_FILTER_SET_SIZE = 16;

//! match/filter types
typedef enum
{
    FT_EXACT =0,
    FT_RANGE,
    FT_SET,
    FT_WILD
} filterType_t;

//! definition of a filter
typedef struct
{
    string name;
    string rname;
    string type;
    filterType_t mtype;
    refer_t refer;
    refer_t rrefer;
    unsigned short offs;
    unsigned short roffs;
    unsigned short len;
    //! number of values
    unsigned short cnt;

    //! mask from filter definition
    FilterValue fdmask;
    //! position of lowest set bit in fdmask (only set when len == 1)
    unsigned char fdshift;
    //! mask for the filter value
    FilterValue mask;
    //! value definition:
    //! EXACT -> value[0]
    //! RANGE -> min in value[0], max in value[1]
    //! SET -> value[0-n] where value.len>0
    //! WILD -> no value
    FilterValue value[MAX_FILTER_SET_SIZE];
} filter_t;

//! filter list (only push_back & sequential access)
typedef std::list<filter_t>            filterList_t;
typedef std::list<filter_t>::iterator  filterListIter_t;


typedef int (*proc_timeout_func_t)( int timerID, void *flowdata );


/*! \short   initialize the action module upon loading
   \returns 0 - on success, <0 - else
*/
void initModule( configParam_t *params );


/*! \short   cleanup action module structures before it is unloaded
   \returns 0 - on success, <0 - else
*/
void destroyModule( configParam_t *params );


/*! \short   initialize flow data record for a rule

    The freshly allocated router configuration for a Qos task (for
    one module) is initialized here and the
    module parameter string can be parsed and checked

    \arg \c  params    - rule_id    identifier for the rule
    \arg \c  params    - action     action identifier for the rule
    \arg \c  params    - module parameter text from inside '( )'
    \arg \c  flowdata  - place for action module specific data from flow table
    \returns 0 - on success (parameters are valid), <0 - else
*/
void initFlowSetup( int rule_id, int action_id, configParam_t *params, filterList_t *filters, void **flowdata );


/*! \short   get list of default timers for this proc module
    \arg \c  flowdata  - place for action module specific data from flow table
    \returns   list of timer structs
*/
timers_t* getTimers( void *flowdata );


/*! \short   dismantle router configuration for a Qos Task

    attention: do NOT free this slice of memory itself
    \arg \c  params    - rule_id      identifier for the rule
    \arg \c  params    - action_id    action identifier for the rule
    \arg \c  flowdata  - place of action module specific data from flow table
    \returns 0 - on success, <0 - else
*/
void destroyFlowSetup( int rule_id, int action_id, configParam_t *params, filterList_t *filters, void *flowdata );


/*! \short   reset flow data record for a rule

    \arg \c  flowdata  - place of action module specific data from flow table
    \returns 0 - on success, <0 - else
*/
void resetFlowSetup( configParam_t *params );


/*! \short  check if bandwidth available for the rule is enought.

    \arg \c params - rule parameters
    \returns 0 - on success (bandwidth is valid), <0 - else
*/
int checkBandWidth( configParam_t *params );


/*! \short   provide textual information about this action module

    A string is returned that describes one property (e.g.author) of the
    action module in detail. \n A list of common properties follows in the
    argument list

    \arg \c I_NAME    - name of the action module
    \arg \c I_UID     - return unique module id number (as string)
    \arg \c I_BRIEF   - brief description of the action module functionality
    \arg \c I_AUTHOR  - name/e-mail of the author of this module
    \arg \c I_CREATE  - info about module creation (usually date and similar)
    \arg \c I_DETAIL  - detailed module functionality description
    \arg \c I_PARAM   - description of parameter(s) of module
    \arg \c I_RESULT  - information about nature and format of measurement
                        results for this module
    \arg \c I_RESERV  - reserved info entry
    \arg \c I_USER    - entry open for free use
    \arg \c I_USER+1  - entry open for free use
    \arg \c I_USER+2  - entry open for free use
    \arg \c I_USER+n  - must return NULL

    \returns - a string which contains textual information about a
               property of this action module \n
    \returns - pointer to a '\0' string if no information is available \n
    \returns - NULL for index after last stored info string
*/
const char* getModuleInfo( int i );


/*! \short   this function is called if the module supports a timeout callback function every x seconds and its invokation is configured to make use of the timeout feature
 */
void timeout( int timerID, void *flowdata );


/*! \short   return error message for last failed function

    \arg \c    - error number (return value from failed function)
    \returns 0 - textual description of error for logging purposes
*/
char* getErrorMsg( int code );



/*! \short   definition of interface struct for Action Modules

  this structure contains pointers to all functions of this module
  which are part of the Action Module API. It will be automatically
  set for an Action Module upon compilation (don't forget to include
  ActionModule.h into every module!)
*/

typedef struct {

    int version;

    void (*initModule)( configParam_t *params );
    void (*destroyModule)( configParam_t *params );

    /*    int (*getFlowRecSize)(); -- deprecated -- */
    void (*initFlowSetup)( int ruleid, int action_id, configParam_t *params, filterList_t *filters, void **flowdata );
    timers_t* (*getTimers)( void *flowdata );
    void (*destroyFlowSetup)( int ruleid, int action_id, configParam_t *params, filterList_t *filters, void *flowdata );

    void (*resetFlowSetup)( configParam_t *params );
    int (*checkBandWidth)( configParam_t *params );
    void (*timeout)( int timerID, void *flowdata );

    const char* (*getModuleInfo)(int i);
    char* (*getErrorMsg)( int code );

} ProcModuleInterface_t;

#endif /* __PROCMODULEINTERFACE_H */
