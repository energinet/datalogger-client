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

#ifndef MODULE_UTIL_H_
#define MODULE_UTIL_H_

#include "contdaem.h"
#include "uni_data.h"



/**
 * @addtogroup general_xml
 * @{
 * @section calcxml The calc attribute
 * The control daemon includes a general library for calculations, which modules can support.
 * A calculation can be defined by a text string that can be placed in a configuration file.
 * The calculation utilitys currently supports the following types fo callculation:
 * <ul>
 * <li><b>1st degree polynomial</b>\n
 *      @f${y=ax+b}@f$\n
 *      Syntax: \n
 *      <tt> poly1:a\<a>b\<b> </tt> where \<a> and \<b> are @f${a}@f$ and @f${b}@f$ from the
 *      equation above.\n
 *      Ex.: <tt> poly1:a0.62b-0.56 </tt>
 * <li><b>2nd degree polynomial</b>\n
 *      @f${y=ax^2+bx+c}@f$\n
 *      Syntax: \n
 *      <tt> poly1:a\<a>b\<b> </tt> where \<a> \<b> and \<c> are @f${a}@f$, @f${b}@f$ and @f${c}@f$
 *      from the equation above.\n
 *      Ex.: <tt>poly2:a0.023b-0.12c5.1</tt>
 * <li><b>Minimum threshold</b>\n
 *      @f$y=\left\{\begin{array}{ll}0&\mbox{ if } x<t \\ x & \mbox{ if }  t \leq x  \end{array}\right @f$\n
 *      Syntax: \n
 *      <tt> thrhz:\<t> </tt> where \<t> is the threshold,  @f${a}@f$ \n
 *      Ex.: <tt>thrhz:0.3</tt>
 * <li><b>Look up table</b>\n
 *      A list of points mapped to a list of points.
 *      Syntax: \n
 *      <tt> ltable:[\<x1>:\<y1>],[\<x2>:\<y2>],[\<x3>:\<y3>],... </tt> Where \<x1> is mapped to \<y1>, \<x2> to \<y2> and so on. 
 *      The values in between are mapped linary. Values outside the range result in NaN (Not a number value). Note 
 *      that the @p inf and @p -inf value can be used for minus or plus infinity \n
 *      Ex.: <tt>ltable:[-inf:0],[0:0],[10:5],[inf:5]</tt>\n
 *      Will result in:\n
 *      @f$y=\left\{\begin{array}{ll}0&\mbox{ if } x < 0 \\ x/2 & \mbox{ if }  0 \leq x < 10 \\ 5 & \mbox{ if }  x \geq 10  \end{array}\right @f$\n
 *      Ex.: <tt>ltable:[-inf:0],[0:0],[5:10],[10:15],[inf:15]</tt>\n
 *      Will result in:\n
 *      @f$y=\left\{\begin{array}{ll} 0&\mbox{ if } x \leq 0 \\ 2x & \mbox{ if }  0 \leq x < 5 \\ x+5 & \mbox{ if }  5 \leq x < 10  \\ 15 & \mbox{ if }  x \geq 10  \end{array}\right @f$\n

 * <li><b>Equation from eeprom</b>\n
 *      Read an equation string from the eeprom. 
 *      Syntax: \n
 *      <tt> eeprom:\<var name>\#\<fallback> </tt> where \<var name> is the eeprom variable name
 *      and \<fallback> is a fallback equation, in case the \<var name> is not found.\n
 *      Ex.: <tt>eeprom:adc02_calc\#poly1:a0.105b-273</tt> \n
 *      "adc02_calc" is the eeprom var name and "poly1:a0.105b-273" is the fallback.\n
 * </ul>
 * <b> Chaining </b>\n
 * A calculation can be chained, so multible calculation can be defined in one line.\n
 * Equations can be chained using \b ; as separator. \n 
 * Ex.: <tt>poly1:a0.015462239b-4.3333;thrhz:0.3</tt>
 * The polynomium will be done first and the result will go through the threshold afterwards.
 * @}
 */


/**
 * @defgroup module_util module utilities
 * @{
 * Usefulle utility library for module implementations.
 */

/**
 * @defgroup  module_util_calc calculation library
 */

/**
 * Calculation types 
 */
enum calc_type{
    /**
     * 1st degree polynomial. */
    CALC_POLY1,
    /**
    * 2nd degree polynomial. */  
    CALC_POLY2,
    /**
     * Threshold. */
    CALC_THRH,
    /**
     * Table look up. */
    LTABLE,
};

struct module_calc {
    /**
     * Calculation type */
    enum calc_type type;
    /**
     * coefficient a */
    float a;
    /**
     * coefficient b */
    float b;
    /**
     * coefficient c */
    float c;
    /**
     * verbose calculation */
    int verbose;
    /**
     * custom calculating data object */
    void *calcobj;
    /**
     * Next element in the calculation chain */
    struct module_calc *next;
};


/**
 * Create a @ref module_calc chain from a text string.
 * @see module_calc_grp for a input syntax
 * @param calc_str The input string
 * @param verbose Do it verbose 
 * @return @ref module_calc chain 
 */
struct module_calc *module_calc_create(const char *calc_str, int verbose);


/**
 * Calculate a chain list
 * @param calc The calculation chain
 * @param input The input value
 * @return The output value
 */
float module_calc_calc(struct module_calc *calc, float input);


/**
 * Calculate a chain list
 * @param calc The calculation chain
 * @param input The input value
 * @return The output value
 */
int module_calc_calc_int(struct module_calc *calc, int input);

/**
 * @} 
 */

/**
 * @defgroup module_util_ftol Flow to level
 * @{
 * Utility for flow to level conversion
 */


enum fltconv{
    CONV_NONE,
    CONV_DIFF,
    CONV_INTG,
};

/**
 * Flow to level conversion struct 
 */
struct convunit{
    /**
     * Flow unit */
    char *unit_in;
    /**
     * Level unit */
    char *unit_out;
    /**
     * Scale  */
    float scale;
    /**
     * Do differentiation */
    enum fltconv conv;
    /**
     * Number of decimals */
    int decs; 
};

/**
 * Search for a @ref flow to level conversion 
 * @param unit_in A flow unit
 * @param unit_out A level unit
 * @return @ref convunit  or NULL if not found
 */
struct convunit *module_conv_get_level(const char *unit_in, const char *unit_out);

/**
 * Search for a @ref level to flow conversion 
 * @param unit_in A level unit
 * @param unit_out A flow unit
 * @return @ref convunit or NULL if not found
 */
struct convunit *module_conv_get_flow(const char *unit_in, const char *unit_out);



/**
 * Calc conversion 
 * @param usec milliseconds for the differentiation
 * @param units The input value
 * @param ftlunit The @ref ftolunit 
 * @todo use msec
 */
float module_conv_calc(struct convunit *convunit, unsigned long msec, float units);



/**
 * @} 
 */

/**
 * @defgroup module_util_time Time
 * @{
 * Utility for time handeling.
 */


/**
 * Calculate the millisecond difference.
 * @param now The current time or the newest time
 * @param prev The previous time
 * @return Difference in milliseconds
 * @note behavior is unspecified if \p prev > \p now
 */
unsigned long modutil_timeval_diff_ms(struct timeval *now, struct timeval *prev);

/**
 * Add milliseconds to a timeval
 * @param time The timeval
 * @param diff The number of milliseconds to add
 * @return \p time (same as input timeval pointer)
 */
struct timeval * modutil_timeval_add_ms(struct timeval *time, int diff);

/**
 * Sleep until the next second 
 * @param time The current time
 * @note Segmentationfault if \p time is NULL
 */
void modutil_sleep_nextsec(struct timeval *time);

/**
 * @} 
 */

/**
 * Get item index from a list

 * @param text The search text
 * @param def Default value if \p text not found
 * @param items An item list. \n Ex:
 * @code 
 * const char *items[] = { "index0", "index1", "other" , NULL};
 * @endcode
 * @return found index or default value if \p text not found
 * @note The list must be terminated by a NULL pointer
 */
int modutil_get_listitem(const char *text, int def, const char** items);


/**
 * String duplicater with NULL handling
 * @param str The string to be duplicated
 * @return A duplicate of the input string or NULL if input string is NULL
 */
char *modutil_strdup(const char *str);




struct mureplace_obj{
    const char *search;
    int (*callback)(void *userdata, char* buffer, int max_size);
};


int moduleutil_replace(const char *inbuf, char* outbuf, int maxoutsize, struct mureplace_obj *repl_lst, void *userdata);


/**
 * @} 
 */


#endif /* MODULE_UTIL */
