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
 
#ifndef UNI_DATA_H_
#define UNI_DATA_H_

#include <sys/time.h>
//#include "contdaem.h"
//#include "module_util.h"

struct ftolunit *ftlunit;
/**
 * @defgroup uni_data_grp Universal data object
 * @{
 * The possible datatypes are described in @ref data_types
 */

/**
* Data types
* @enum 
*/
enum data_types{
    /**
     * Integer data type */
    DATA_INT, 
    /**
     * Float data type 
     * @note uni_data::data is a pointer to a float value*/
    DATA_FLOAT,
    /**
     * Flow data type (float and time) 
     * @note uni_data::data is a pointer to a float value*/
    DATA_FLOW, 
    /**
     * State data type (integer state and text) 
     * @note uni_data::data may be a pointer to a text buffer */
    DATA_STATE,
    /**
     * Text data type 
     * @note uni_data::data is a pointer to a text buffer */
    DATA_TEXT, 
    /**
     * Fault data type (integer fault state and text) 
     * @note uni_data::data may be a pointer to a text buffer */
    DATA_FAULT,
};


/**
 * Universal data struct 
 */
struct uni_data{
    /**
     * Datatype */
    enum data_types type; 
    /**
     * Integer value (if available) */
    int value; 
    /**
     * Milliseconds since last measurement */
    unsigned long mtime; 
    /**
     * Pointer to type data (text og float) */
    void *data;
    /**
     * Placed in heap if true 
     * @private */
    int inheap;
};

/**
 * Creates a unidata object 
 * @param data_type datatype 
 * @param value integer value
 * @param data data pointer
 * @param ms milleseconds since last measurment
 * @private 
 * @note use other conmstructers
 * @return a pointer to the object
 */
struct uni_data *uni_data_create(enum data_types data_type, 
				 unsigned long value, void* data, 
				 unsigned long ms);


/**
 * Copy a universal data object 
 * @param source the source data object 
 * @return a pointer to the copy
 */
struct uni_data *uni_data_copy(struct uni_data *source);


/**
 * Creates an integer datatype 
 * @param value The integer value
 * @return a pointer to the data object
 */
struct uni_data *uni_data_create_int(int value);

/**
 * Creates a float datatype 
 * @param value The integer value
 * @return a pointer to the data object
 */
struct uni_data *uni_data_create_float(float value);

/**
 * Creates a flow datatype 
 * @param count The counted units
 * @param ms milliseconds since last measurement
 * @return a pointer to the data object
 */
struct uni_data *uni_data_create_flow(float count, unsigned long ms);

/**
 * Creates a text datatype 
 * @param text A pointer the the text string (the text will be copied)
 * @return a pointer to the data object
 */
struct uni_data *uni_data_create_text(const char *text);

/**
 * Creates a state datatype 
 * @param state A integer state
 * @param text A pointer the the text string (the text will be copied)
 * @return a pointer to the data object
 */
struct uni_data *uni_data_create_state(unsigned long state, const char *text);


/**
 * Creates a state datatype 
 * @param type A the output datatype
 * @param str The input string
 * @return a pointer to the data object
 */
struct uni_data *uni_data_create_from_str(enum data_types type, const char *str);

/**
 * Delete a data object 
 * @param rem The data object to delete
 */
void uni_data_delete(struct uni_data *rem);



/**
 * Print the value of a data object to a string
 * @param data The databoject to print
 * @param buffer The string buffer
 * @param max_size The size of the string buffer
 * @return The length of the printed string
 * @todo Describe the difference from uni_data_get_txt_value
 */
int uni_data_get_txt_value(struct uni_data *data, char* buffer, int max_size);

/**
 * Print the level value of data object to a string
 * @param data The databoject to print
 * @param buffer The string buffer
 * @param max_size The size of the string buffer
 * @param ftlunit The contersion unit
 * @return The length of the printed string
 * @todo Make consistend with uni_data_get_fvalue
 */
int uni_data_get_txt_fvalue(struct uni_data *data, char* buffer, int max_size, struct ftolunit *ftlunit);

/**
 * Return the float value of a data object
 * @param data The databoject to print
 * @return The float value 
 * @todo Describe the difference from uni_data_get_value
 */
float uni_data_get_fvalue(struct uni_data *data);

/**
 * Return the float value of a data object
 * @param data The databoject to print
 * @return The float value 
 * @todo Describe the difference from uni_data_get_fvalue
 */
float uni_data_get_value(struct uni_data *data);


/**
 * Return the internal text string 
 * @param data The dataobject 
 * @return The string or NULL is no string
 * @note Only the text and state may contain a string;
 */
const char *uni_data_get_strptr(struct uni_data *data);

/**
 * Get enumerated datatype 
 * @param str datatype string
 * @return enumerated datatype
 */
enum data_types uni_data_type_str(const char *str);

/**
 * Get datatype string
 * @param type Enumerated datatype
 * @return a pointer to a datatype string
 */
const char * uni_data_str_type(enum data_types type);


/**
 * @}
 */


#endif /* UNI_DATA */
