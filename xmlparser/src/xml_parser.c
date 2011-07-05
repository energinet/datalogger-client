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

#include "xml_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <expat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * XML parser object
 * @private 
 */
typedef struct app_data {
	/**
	 * The xml_tag list */
	struct xml_tag *tags;
	/**
	 * Stack depth of the current element */
	struct stack_ele* stack;

	/**
	 * The depth of the stack  */
	int depth;

	/**
	 * The user data object  */
	void *data;
} AppData;

/////////////////////////////
// Attribute get functions //
/////////////////////////////

/**
 * Get the number of the attribute in the attr array
 * @param **attr the attribute array
 * @param *id the id to search for
 * @return the index to the attribute value of -1 if not found
 */
int get_attr_no(const char **attr, const char *id) {
	int i = 0;

	while (attr[i]) {
		if (strcmp(attr[i], id) == 0)
			return i + 1;
		i = i + 2;
	}

	return -1;
}

/**
 * Get the value from a specific attribute in the attr array af an integer
 * Note: this function only handles positive integers
 * @param **attr the attribute array
 * @param *id the id to search for
 * @return integer or -1 if not found 
 */
int get_attr_int_dec(const char **attr, const char *id) {
	int attr_no;
	int ret;

	attr_no = get_attr_no(attr, id);

	if (attr_no < 0) {
		ret = -1;
		return ret;
	}

	sscanf(attr[attr_no], "%d", &ret);

	return ret;
}

int get_attr_int(const char **attr, const char *id, int rep_value) {
	int attr_no;
	int ret;
	char *format;

	attr_no = get_attr_no(attr, id);

	if (attr_no < 0) {
		return rep_value;
	}

	if (strncmp("true", attr[attr_no], 4) == 0) {
		return 1;
	} else if (strncmp("false", attr[attr_no], 5) == 0) {
		return 0;
	} else if (strncmp("0x", attr[attr_no], 2) == 0)
		format = "0x%x";
	else
		format = "%d";

	if (sscanf(attr[attr_no], format, &ret) != 1) {
		printf("Attribute %s could not be red: %s (format %s)\n", id,
				attr[attr_no], format);
		return rep_value;
	}

	return ret;
}

const char *get_attr_str_ptr(const char **attr, const char *id) {
	int attr_no;

	attr_no = get_attr_no(attr, id);

	if (attr_no < 0) {
		return NULL;
	}

	return attr[attr_no];
}

float get_attr_float(const char **attr, const char *id, float rep_value) {
	int attr_no;
	float ret;
	char *format;

	attr_no = get_attr_no(attr, id);

	if (attr_no < 0) {
		return rep_value;
	}

	if (sscanf(attr[attr_no], "1/%f", &ret) == 1) {
		if (ret <= 0)
			ret = 1;
		return 1 / ret;
	} else if (sscanf(attr[attr_no], "%f", &ret) == 1) {
		return ret;
	}

	printf("Attribute %s could not be red: %s (format %s)\n", id,
			attr[attr_no], format);

	return rep_value;

}

/**
 * @ defgroup xml_parser_stack XML parser stack handeling
 * @ingroup xml_parser
 * @private
 * @{
 */
/**
 * Add stack element 
 * @private 
 */
struct stack_ele* add_stack_ele(struct stack_ele *parent) {
	struct stack_ele* new_ele = NULL;

	if (parent != NULL)
		parent->chil_cnt++;

	new_ele = malloc(sizeof(struct stack_ele));

	if (new_ele != NULL) {
		memset(new_ele, 0, sizeof(struct stack_ele));
		new_ele->parent = parent;
	}

	return new_ele;
}

/**
 * Remove stack element 
 * @private 
 */
struct stack_ele* rem_stack_ele(struct stack_ele *ele) {

	struct stack_ele* parent = NULL;

	if (ele != NULL) {
		parent = ele->parent;
		if (ele->name != NULL)
			free(ele->name);
		if (ele->data != NULL)
			free(ele->data);
		free(ele);
	}

	return parent;

}

/**
 * Print a stack element
 * @param ele Stack element 
 * @private
 */
void print_stack_elements(struct stack_ele *ele) {
	while (ele != NULL) {
		if (ele->name != NULL)
			printf("%s,", (char*) ele->name);
		ele = ele->parent;
	}

	printf("\n");

}

/**
 * Pushes an new element onto the stack 
 * @private 
 * @return the depth of the stack ot -1 if error
 */

struct stack_ele* push_stack(AppData *ad) {
	struct stack_ele* new_ele = NULL;

	ad->depth++;

	new_ele = add_stack_ele(ad->stack);

	if (new_ele != NULL)
		ad->stack = new_ele;

	return ad->stack;
}

/**
 * Pop an element from the stack 
 * @private 
 * @return the depth of the stack or -1 if error
 */
struct stack_ele* pop_stack(AppData *ad) {
	if (ad->depth <= 0) {
		printf("ERR: pop_stack: ad->depth <= 0\n");
		return NULL;
	}

	ad->depth--;
	ad->stack = rem_stack_ele(ad->stack);
	return ad->stack;
}

/**
 *@}
 */

/////////////////////////////
// Parser functions        //
/////////////////////////////


struct xml_tag *find_entry_in_tags(struct xml_tag *tags, struct stack_ele* ele) {
	int i = 0;

	if (!tags)
		return NULL;

	while (tags[i].el[0] != '\0') {
		struct xml_tag *entry = tags + i++;
		if (strcmp(ele->name, entry->el) == 0) {
			//            printf("test tag: %s\n", entry->el);
			/* entry name match */
			if (entry->parent[0] == '\0') {
				/* element do not need a parent */
				return entry;
			} else if (ele->parent == NULL) {
				/* ERROR:  entry has no parent */
				printf("Element %s outside section (parent=NULL)\n", ele->name);
				return NULL;
			}

			if (strcmp(ele->parent->name, entry->parent) == 0) {
				/* parent name does match */
				return entry;
			}
		}
	}

	return NULL;

}

struct xml_tag *find_entry(AppData *ad, struct stack_ele* ele) {

	struct stack_ele* parent;
	struct xml_tag *tag;

	/* Find in primary tags */
	if ((tag = find_entry_in_tags(ad->tags, ele)) != NULL)
		return tag;
	/* Find in parent tags */
	parent = ele->parent;
	if (parent) {
		if (parent->ext_tags) {
			if ((tag = find_entry_in_tags(parent->ext_tags, ele)) != NULL)
				return tag;
		}
		parent = parent->parent;
	}

	return NULL;
}

/**
 * Handle the start of a xml node 
 * @param *data the AppData object
 * @param *el element name string 
 * @param *attr the attributes
 */
void tag_start(void *data, const char *el, const char **attr) {
	AppData *ad = (AppData *) data;
	struct stack_ele* ele = push_stack(ad);
	int retval;

	if (ele == NULL) {
		printf("ERR in xml start: ele == NULL!!! for element %s\n", el);
		exit(128);
	}

	ele->name = strdup(el);

	ele->entry = find_entry(ad, ele);

	if (ele->entry == NULL) {
		printf("ERR in xml start: entry == NULL!!!\n");
		exit(128);
	}

	struct xml_tag *entry = ele->entry;

	if (entry->start != NULL) {
		retval = entry->start(ad->data, el, attr, ele);
		if (retval < 0) {
			printf("Element %s returned err: %s (%d)\n", el, strerror(-retval),
					retval);
			exit(128);
		}

	}
}

/**
 * Handle the end of a xml node 
 * @param *data the AppData object
 * @param *el element name string 
 */

void tag_end(void *data, const char *el) {
	AppData *ad = (AppData *) data;

	if (ad == NULL)
		printf("ERR: end ad == NULL \n");

	struct stack_ele* ele = ad->stack;

	if (ele != NULL)
		if (ele->entry != NULL) {

			struct xml_tag *entry = ele->entry;
			if (entry->end != NULL) {
				entry->end(ad->data, el, ele);
			}
		}

	pop_stack(ad);

}

/**
 * Handle a string in the xml document 
 * @param *data the AppData object
 * @param *txt element name string 
 * @param txtlen is the length of the text
 */

void char_hndl(void *data, const char *txt, int txtlen) {

	AppData *ad = (AppData *) data;

	struct stack_ele* ele = ad->stack;
	if (ele != NULL)
		if (ele->entry != NULL) {
			struct xml_tag *entry = ele->entry;
			if (entry->char_hndl != NULL)
				entry->char_hndl(ad->data, txt, txtlen, ele);
		}
}

AppData *newAppData(struct xml_tag *tags, void *data) {
	AppData *ret;
	ret = (AppData *) malloc(sizeof(AppData));
	if (ret == NULL) {
		fprintf(stderr, "Couldn't allocate memory for application data\n");
		return NULL;
	}

	if (!tags) {
		fprintf(stderr, "Couldn't allocate memory for application data\n");
		return NULL;
	}

	/* Initialize */
	ret->tags = tags;
	ret->depth = 0;
	ret->stack = NULL;
	ret->data = data;
	return ret;
} /* End */

int parse_xml_file(const char *filename, struct xml_tag *tags, void *appdata) {

	int fd = 0;
	struct stat file_stat;
	char *file_map;
	XML_Parser p = XML_ParserCreate(NULL);
	int retval = 0;
	AppData *ad = newAppData(tags, appdata);

	if (!p) {
		fprintf(stderr, "Couldn't allocate memory for parser\n");
		return -1;
	}

	XML_SetUserData(p, (void *) ad);
	XML_SetElementHandler(p, tag_start, tag_end);
	XML_SetCharacterDataHandler(p, char_hndl);

	/* open file */
	if ((fd = open(filename, O_RDONLY)) < 0) {
		fprintf(stderr, "Could not open file %s\n", filename);
		return -2;
	}

	/* get file size */
	fstat(fd, &file_stat);

	/* map the file */
	file_map = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (file_map == MAP_FAILED) {
		close(fd);
		return -1;
	}

	/* parse xml */
	if (!XML_Parse(p, file_map, file_stat.st_size, 1)) {
		fprintf(stderr, "Parse error at line %d:\n%s\n",
				(int) XML_GetCurrentLineNumber(p), XML_ErrorString(
						XML_GetErrorCode(p)));
		retval = -5;
	}

	munmap(file_map, file_stat.st_size);
	close(fd);

	return retval;

}

/**
 * @}
 */
