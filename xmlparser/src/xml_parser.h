/*
 * Energinet Datalogger
 * Copyright (C) 2009 - 2011 LIAB ApS <info@liab.dk>
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

#ifndef XML_PARSER_H_
#define XML_PARSER_H_

/**
 * @defgroup xml_parser XML parser library
 * @{
 */

/**
 * Parameters for the xml start tag callback
 * @param data The userdata object
 * @param attr The attribute pointer
 * @param ele The stack element 
 * @return Zero on success
 */
#define XML_START_PAR void *data, const char *el, const char **attr, struct stack_ele* ele

/**
 * Parameters for the xml char handler callback
 * @param data The userdata object
 * @param txt Text string from the XML
 * @param txtlen Text length
 * @retuen zero on success
 */
#define XML_CHARHNDL_PAR  void *data, const char *txt, int txtlen, struct stack_ele* ele

/**
 * Parameters for the xml end tag callback
 * @param data The userdate object
 * @param el The element to end 
 * @param ele The stack element 
 * @return zero on success 
 */
#define XML_END_PAR  void *data, const char *el, struct stack_ele* ele

struct xml_tag;
/**
 * XML stack element 
 */
struct stack_ele {
	/**
	 * Element tag name
	 */
	char *name;
	/**
	 * A pointer to the parent element
	 * @note NULL on first element
	 */
	struct stack_ele *parent;
	/**
	 * Counted number of chileds
	 * @note Will be valid at the end the tage
	 */
	int chil_cnt;
	/**
	 * A pointer to a userdata object belonging to this element
	 */
	void *data;
	/**
	 * The xml_tag object for this tag
	 */
	struct xml_tag *entry;
	/**
	 * A list of extra mlx_tag for this element
	 */
	struct xml_tag *ext_tags;
};

/**
 * XML tage definition struct
 */
struct xml_tag {
	/**
	 * Elament tag name
	 */
	char el[32];
	/**
	 * Parent tag name
	 * @note Ignored if NULL
	 */
	char parent[32];
	/**
	 * Callback for start xml tag
	 * @note Ignored if NULL
	 */
	int (*start)(XML_START_PAR);
	/**
	 * Callback for text in xml tag
	 * @note Ignored if NULL
	 */
	void (*char_hndl)(XML_CHARHNDL_PAR);
	/**
	 * Callback for text end xml tag
	 * @note Ignored if NULL
	 */
	void (*end)(XML_END_PAR);
};

/**
 * Start parsing an xml file
 * @param filename The path to the xml file
 * @param tags A array of xml tags
 * @param appdata The userdata object
 * @return zero on success
 */
int parse_xml_file(const char *filename, struct xml_tag *tags, void *appdata);

/**
 * @defgroup get_attr XML attribute get functions 
 * @{
 */

/**
 * Get the attribute number
 * @param **attr the attribute array
 * @param *id the id to search for
 * @return the index to the attribute value of -1 if not found
 */
int get_attr_no(const char **attr, const char *id);

/**
 * Get integer value from attributes 
 * \note this function only handles positive integers
 * @param attr the attribute array
 * @param id the id to search for
 * @return integer or -1 if not found 
 */
int get_attr_int_dec(const char **attr, const char *id);

/**
 * Get an integer value from attributes 
 * @param attr The attribute list
 * @param id The attribute name
 * @param rep_value The default value if no attrubute 
 * @return The read or the default value 
 */
int get_attr_int(const char **attr, const char *id, int rep_value);

/**
 * Get a float value from attributes
 * @param attr The attribute list
 * @param id The attribute name
 * @param rep_value The default value if no attrubute 
 * @return The read or the default value 
 */
float get_attr_float(const char **attr, const char *id, float rep_value);

/**
 * Get a string pointer from the attributes
 * @param attr The attribute list
 * @param id The attribute name
 * @return A pointer the string on success or NULL if ont found 
 */
const char *get_attr_str_ptr(const char **attr, const char *id);

/**
 *@} @}
 */

#endif /* XML_PARSER_H_ */

