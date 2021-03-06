/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2012 LIAB ApS <info@liab.dk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
 
#ifndef CONTDAEM_H_
#define CONTDAEM_H_

#include "uni_data.h"
#include "module_base.h"
#include "module_event.h"
#include "module_type.h" 
#include "module_util.h"

/**
 * @mainpage Datalogger and control daemon
 * @{
 * @section intro_sec Introduction
 * The datalogger and control daemon is a modular application for controlling outputs and loggin 
 * inputs. The short name is \p contdaem which is a shortening of controle and daemon. 
 * @image html kontroldaemon.png "The blocks it the datalogger and control daemon"
 * @image latex kontroldaemon.png "The blocks it the datalogger and control daemon" width=10cm
 * The contdaem consists of several parts: The main application which includes a module loader which loads several
 * modules and configuration parser which parses a configuration file. 
 * 
 * @subsection main_cmdline Command line options 
 * You can read about the application cas some command line options which can be acced by the @p -h option.\n
 * See @subpage cmd_options
 * 
 * @subsection main_conf The Configuration File
 * Most of the operrations can be forfilled by useing extensive number of included modules. Se more about 
 * writing a configuration file and the included modules on the about configureing the modules.\m
 * See @subpage page_conf_xml
 * 
 * @subsection main_develop Developing your own modules
 * If the extensive number of build modules can't fullfill your needs it is possible to
 * develop your own modules.
 * The program consists of a main application and some modules types. The confoguration for 
 * the program is controlled by a configuration file. The configuration file contains a XML
 * description of the modules to load. 
 *
 * @section modsavai_sec The Available Modules
 * The application all-ready contains a number for installed module types. These can be used by using
 * the module type in the configuration file. See available module page for more information about
 * these modules. The detailed descriptions includes custom XML tags and attributes.
 *  @ref modules
 *  @ref module_xml


 * @section modscreat_sec Creating Custom Modules
 * @subpage page_conf_xml

 */

/**
 * @defgroup conf_xml The Configuration
 * @{
 * @page page_conf_xml The Configuration 
 * 
* @section conf_sec The Configuration File
 * The configuration is a XML file describing all the modules to load. 
 * The file is structures with a \<modules> section containing a list of \<module> tags,
 * each creating a module. An example of the XML structure:
 * @verbatim 
   <modules>
     <module type="type_name" name="module_name" custom_attribute="value">
       <custom_tag attribute1="text" attribute2="10" />
     </module>
   </modules>@endverbatim 
 * The \p type_name is the name of the module type and \p module_name is the name of the
 * module witch other modules use for referencing the module. A module can have custom tags 
 * and attributes.
 * The modules are loaded sequentially and can only subscribe to event types published prior 
 * to the loading. The start-up of and loading of the modules is shown the following diagram.
 * @image html contdaemstart.png "The start-up"
 * @image latex contdaemstart.png "The start-up"
 * 
 * XML tags of a base module, which all modules support:
 *   - \b name: Module name [mandatory]
 *   - \b type: Module type name [mandatory]
 *   - \b verbose: Verbose output [optional]
 *   - \b flags: Event flags [optional] 
 *
 * A module can have custom tags. Many of these tags corresponds to a event type. If the 
 * tag creates an event type the following tags will be present.
 *   - \b text: Text to be shown to the user, ether in the chart or the event list. [optional] 
 *   - \b unit: Unit name, could be a SI unit e.g. V, W, bar... [optional] 
 *   - \b name: sort name, for referencing the type [mandatory]
 *   - \b flags: Event flags [optional] 
 *
 * @subsection conf_subs_src Subscription 
 * Other module can subscribe to event types in other modules.  The event type is reference by
 * a ename consisting of the module name and the event type short name, separated by a dot: 
 * "<module_name>.<short_name>" for example "pt1000.tttop", there the event type "tttop" in the 
 * module "pt1000" is referenced. A subscription of an event type is often using the attribute 
 * \p event e.g. event="pt1000.tttop".
 *
 * Some modules support mass subscription, where it is possible to replace the module name 
 * or event type name with \p '*' to get all modules or event types. Ex. "pt1000.*" will 
 * subscribe to all events types published by the pt1000 module.
 * 
 * A module can both subscribe and transmit events. In that way the module and events types 
 * can chained like this:
 * @image html modulechain.png "A module chain"
 * @image latex modulechain.png "A module chain" width=10cm
 * A module like the @ref modules_calcacc supports chaining and will for every subscribed event type
 * publish a new event type. The text and unit and event type name are copied. Ex if accumulate module 
 * is named "acc" and it subscribes "pt1000.tttop" then it will publish the event "acc.tttop"
 *
 * @defgroup general_xml General XML tags and attrubutes 
 * @defgroup module_xml XML configuration of modules
 * @}
 */

/**
 * @defgroup develop_c Programming custom modules
 * @{
 * @page page_develop_c  Programming custom modules
 * 
 * @subsection moduletype_sec Module Types
 * A module type is a template for a module. A module type contains functions and structs 
 * for a certain service, just like a class with methods. All module types create one or more 
 * modules, like objects. A module consists of a base module, containing the basic module 
 * functionality. 
 * @see module_type
 * @see module_base
 *
 * @subsection event_sec Events
 * A module can publish a number of event types. A event type could be a measurement 
 * for instance a temperature, water level or a kWh counter. All other modules can subscribe
 * to a published event and receive events generated by this event type. An event can be 
 * generated by a periodically measurement for example a temperature reading every second. 
 *
 * A event contains information about time it was generated, the source module, the event 
 * type and the data from the measurement. A event type contains information about the name and
 * unit of the measurement e.g. a SI unit.
 * @see module_event_type @see module_event_event
 *
 * @subsection unidata_sec Universal data type
 * Data generated by an event can be for several kinds. The data transmitted in a event is 
 * therefor encapsulated in a universal data type. The universal data type supports integer,
 * float value, a flow, a text, a state with text and fault state with text. All can be 
 * converted into text using the functions for this object and logged by the database module.
 * @see uni_data_grp \n * @ref data_types "data types"
 * 
 
 * @subsection calc_src Calculation
 * 
 * A utility library for calculations are available to all modules, in order to make a standardized 
 * way of entering equations. Se more about entering equations in the detailed description of
 * @ref module_util_calc
 *
 * @}
 * @}
 */



/**
 * Macro for printing errors
 */
#define PRINT_ERR(str,arg...) fprintf(stderr,"ERR:%s: "str"\n",__FUNCTION__, ## arg)

/**
 * Macro for printing debug
 */
#define PRINT_DBG(cond, str,arg...) {if(cond){fprintf(stderr,"%s: "str"\n",__FUNCTION__, ## arg);}}


/**
 * Macro for printing debig in modules
 * Will print if a module is marked verbose 
 */
#define PRINT_MVB(module, str,arg...) {if((module)->verbose){fprintf(stderr,"%s: "str"\n",(module)->name, ## arg);}}




#endif /* CONTDAEM_H_ */



/*  LocalWords:  structs
 */
