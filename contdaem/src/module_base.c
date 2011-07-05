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

#include "module_base.h"

#include <dirent.h> 
#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include "module_event.h"

int module_base_start(struct module_base* module);
int module_base_run(struct module_base* module);

void module_base_print(struct module_base* module) {
	struct event_type *tptr = module->event_types;

	printf("module %s (%d, %d)\n", module->name, module->run, module->verbose);
	module_type_print(module->type);

	while (tptr) {
		printf("%s.%s , %s , \"%s\"\n", module->name, tptr->name, tptr->unit,
				tptr->hname);
		tptr = tptr->next;
	}
}

int start_module(XML_START_PAR) {
	struct modules *modules = (struct modules*) data;
	struct module_base *new = NULL;
	const char *name = get_attr_str_ptr(attr, "name");

	if (load_list_check(modules->loadlist, name, get_attr_str_ptr(attr,
			"loadlist")) == 0) {
		PRINT_ERR("Not loading \"%s\"", name);
		return 0;
	}

	new = module_base_create(module_type_get_by_name(modules->types,
			get_attr_str_ptr(attr, "type")), get_attr_str_ptr(attr, "name"),
			attr);

	if (!new) {
		PRINT_ERR("unknown type \"%s\"", get_attr_str_ptr(attr, "type"));
		return -1;
	}

	new->verbose = get_attr_int(attr, "verbose", modules->verbose);

	PRINT_MVB(new, "module \"%s\" of type \"%s\" is loading", get_attr_str_ptr(attr, "name"), get_attr_str_ptr(attr, "type") );

	ele->ext_tags = new->type->xml_tags;

	modules->list = module_base_list_add(modules->list, new);

	ele->data = new;

	return 0;
}

void end_module(XML_END_PAR) {
	struct module_base* module = ele->data;

	if (module) {
		module_base_start(module);
	} else
		PRINT_ERR("module is NULL");
	ele->data = NULL;
}

void end_modules(XML_END_PAR) {
	struct modules *modules = (struct modules*) data;
	struct module_base* ptr = modules->list;
	/* start all modules */

	while (ptr) {
		module_base_run(ptr);
		ptr = ptr->next;
	}

}

/**
 * XML Base tags
 */
struct xml_tag base_xml_tags[] = { { "modules", "", NULL, NULL, end_modules },
		{ "module", "modules", start_module, NULL, end_module }, { "", "",
				NULL, NULL, NULL } };

/**
 * Create a new module 
 * @param base A pointer to the module  base
 * @return zero on success
 */
int module_base_start(struct module_base* base) {
	struct module_type* type = base->type;
	int retval = 0;

	PRINT_MVB(base, "Starting" );

	/* run module type start function */
	if (type->start) {
		retval = type->start(base);
		if (retval < 0)
			return retval;
	}

	if (base->verbose)
		module_base_print(base);

	PRINT_MVB(base, "Started" );

	return 0;
}

/**
 * Run module thread 
 * @param base the base module struct
 * @return 0 on success or negative if error
 */
int module_base_run(struct module_base* base) {
	struct module_type* type = base->type;
	int retval = 0;

	/* start loop function */
	if (type->loop) {
		/* Start module loop */
		base->run = 1;
		if ((retval
				= pthread_create(&base->loop_thread, NULL, type->loop, base))
				< 0) {
			PRINT_ERR("Failure starting loop in %s: %s ", base->name, strerror(retval));
			return retval;
		}
	}

	return retval;

}

struct module_base* module_base_create(struct module_type* type,
		const char *name, const char** attr) {

	struct module_base* new = NULL;
	int retval = -EFAULT;
	if (!type) {
		PRINT_ERR("no type");
		return NULL;
	}

	if (!name) {
		PRINT_ERR("no name");
		return NULL;
	}

	if (type->type_struct_size < sizeof(struct module_base)) {
		PRINT_ERR("size < base size (%d < %d)", type->type_struct_size, sizeof(struct module_base));
		return NULL;
	}

	new = malloc(type->type_struct_size);

	if (!new) {
		PRINT_ERR("mallco failed");
		return NULL;
	}

	memset(new, 0, type->type_struct_size);

	new->flags = DEFAULT_FLAGS;
	new->name = strdup(name);

	new->verbose = get_attr_int(attr, "verbose", new->verbose);
	PRINT_MVB(new,"creating");

	new->type = type;

	if (type->handlers) {
		int i = 0;
		PRINT_MVB(new,"handlers");
		while (type->handlers[i].name[0] != '\0') {
			struct event_handler *event_handler;
			PRINT_MVB(new,"h:%d %s", i,type->handlers[i].name);
			event_handler = event_handler_create(type->handlers[i].name,
					type->handlers[i].function, new, NULL);
			new->event_handlers = event_handler_list_add(new->event_handlers,
					event_handler);
			i++;
		}
	}
	PRINT_MVB(new,"do init");
	if (type->init) {
		retval = type->init(new, attr);
		if (retval < 0)
			goto out_wfree;
	}

	new->flags = event_type_get_flags(get_attr_str_ptr(attr, "flags"),
			new->flags);

	PRINT_MVB(new, "created");

	return new;

	out_wfree: free(new);

	return NULL;
}

/**
 * Add a module to the module list
 * @param first is a pointer to the first module
 * @param new is a pointer the new module
 */
struct module_base* module_base_list_add(struct module_base* first,
		struct module_base* new) {
	struct module_base *ptr = first;

	if (!first) {
		/* first module in the list */
		new->first = new;
		return new;
	}

	/* find the last module in the list */
	while (ptr->next) {
		ptr = ptr->next;
	}

	ptr->next = new;
	new->first = first;
	return first;

}

/**
 * Delete a base module and list
 */
void module_base_delete(struct module_base* module) {
	struct module_type* type = NULL;
	int retval = -EFAULT;
	if (!module) {
		PRINT_ERR("no module");
		return;
	}

	if (module->next)
		module_base_delete(module->next);

	printf("unloading module: %s", module->name);

	type = module->type;

	if (type->loop) {
		void* retptr = NULL;

		module->run = 0;
		retval = pthread_join(module->loop_thread, &retptr);
		if (retval < 0)
			PRINT_ERR("pthread_join returned %d in module %s : %s", retval, module->name, strerror(-retval));
		else
		PRINT_MVB(module, "pthread joined");
	}

	if (module->event_types) {
		PRINT_MVB(module,"Removing events");
		event_type_delete(module->event_types);
	}

	if (module->event_handlers) {
		PRINT_MVB(module,"Removing handlers");
		event_handler_delete(module->event_handlers);
	}

	if (type->deinit) {
		PRINT_MVB(module,"deinit");
		type->deinit(module);
		PRINT_MVB(module,"deinit done");
	}

	PRINT_MVB(module,"freeing module");
	free(module);

}

/**
 * Remove a module from the module list
 * @param first is the first module in the list
 * @param rem is the module to be removed
 */
struct module_base* module_base_list_remove(struct module_base* first,
		struct module_base* rem) {
	struct module_base *ptr = first;
	struct module_base *prev = NULL;

	/* find the last module in the list */
	while (ptr) {
		if (rem == ptr) {
			if (prev)
				prev->next = ptr->next;
			else
				first = ptr->next;

			ptr->next = NULL;
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}

	return first;

}

/**
 * Get a module by unique name 
 * @param first is the first module in the list
 * @param name is the unique name 
 * @return a pointer to the module if found or NULL if fault
 */
struct module_base* module_base_get_by_name(struct module_base* first,
		const char *name) {
	struct module_base *ptr = first;
	if (!name)
		return NULL;

	if (name[0] == '*')
		return first;

	while (ptr) {
		if (strcmp(ptr->name, name) == 0)
			return ptr;
		ptr = ptr->next;
	}

	PRINT_ERR("module \"%s\" is not at found", name);
	ptr = first;
	while (ptr) {
		PRINT_ERR(" \"%s\" ", ptr->name);
		ptr = ptr->next;
	}

	return NULL;
}

int module_base_subscribe(struct module_base* subscriber,
		struct event_type *event, struct event_handler *handler) {
	/* create callback object */
	/* struct callback_list *new_callback = callback_list_create(function, userdata); */
	struct callback_list *new_callback = callback_list_create(handler);
	if (!new_callback)
		return -EFAULT;

	/* add to event callback list */
	event->subscribers = callback_list_add(event->subscribers, new_callback);

	return 0;
}

int module_base_subscribe_event(struct module_base* subscriber,
		struct module_base* module_list, const char *name, unsigned long mask,
		struct event_handler *handler, const char **attr) {
	struct event_search search;
	struct event_type *event;
	int count = 0;
	int retval = -EFAULT;
	event_search_init(&search, module_list, name, mask);

	PRINT_MVB(subscriber, "searching for '%s'" , name );
	while ((event = event_search_next(&search))) {
		struct event_handler *objhandler;
		PRINT_MVB(subscriber, "Found %d: %s.%s" , count, event->base->name , event->name );
		if (subscriber->type->subscribe) {
			objhandler = subscriber->type->subscribe(subscriber, event->base,
					event, attr);
		} else {
			objhandler = handler;
		}

		if (!objhandler)
			continue;

		if ((retval = module_base_subscribe(subscriber, event, objhandler)) < 0)
			return retval;

		count++;
	}

	return retval;

}

int module_base_subscribe_event__(struct module_base* subscriber,
		struct module_base* module_list, const char *name, unsigned long mask,
		struct event_handler *handler, const char **attr) {
	struct module_base* source = NULL;
	struct event_type *event = NULL;
	char* mname;
	char* ename;
	int retval = -EFAULT;

	if (!name) {
		PRINT_ERR("no event name");
		return -EFAULT;
	}

	/* get module and event names */
	mname = strdup(name);
	ename = strchr(mname, '.');

	if (!ename) {
		PRINT_ERR("%s is not at valid event name", name);
		return -EFAULT;
	}

	ename[0] = '\0';
	ename++;

	/* get source */
	source = module_base_get_by_name(module_list, mname);
	PRINT_MVB(subscriber, "subscribe %s.%s %p", source->name, ename,source->event_types );

	while (source) {
		/* get the event from the module */
		event = event_type_get_by_name(source->event_types, ename);

		while (event) {
			PRINT_MVB(subscriber, "subscribe %s.%s", source->name , event->name);

			/* run local dubscribe function */
			if ((event->flags & mask) && (event->flags & FLAG_AVAI)) {
				if (subscriber->type->subscribe) {
					struct event_handler *objhandler =
							subscriber->type->subscribe(subscriber, source,
									event, attr);
					if (objhandler)
						if ((retval = module_base_subscribe(subscriber, event,
								objhandler)) < 0)
							return retval;
				} else {
					if ((retval = module_base_subscribe(subscriber, event,
							handler)) < 0)
						return retval;
				}
			}

			event = (ename[0] != '*') ? NULL : event->next;
		}

		source = (mname[0] != '*') ? NULL : source->next;
	}

	return retval;

}

struct event_type *module_base_get_event(struct module_base* module_list,
		const char *ename) {

	return NULL;
}

struct module_event *module_base_read_all(struct module_base* module_list,
		const char *name, unsigned long mask) {
	struct event_search search;
	struct event_type *eptr;
	struct module_event *retlst = NULL;

	event_search_init(&search, module_list, name, mask);

	while ((eptr = event_search_next(&search))) {
		retlst = module_event_add(retlst, event_type_read(eptr));
	}

	return retlst;

}

struct module_event *module_base_read_all__(struct module_base* module_list,
		const char *mname, const char *ename, unsigned long mask) {
	struct module_base* mptr = module_list;
	struct module_event *retlst = NULL;

	while (mptr) {
		struct event_type *eptr = mptr->event_types;

		while (eptr) {
			if (mask & eptr->flags)
				retlst = module_event_add(retlst, event_type_read(eptr));
			eptr = eptr->next;
		}
		mptr = mptr->next;
	}

	return retlst;

}

int module_base_send_event(struct module_event *event) {
	struct module_base* source = event->source;
	struct event_type *event_type = event->type;
	struct callback_list *callback = event_type->subscribers;

	while (callback) {
		struct event_handler *handler = callback->handler;
		PRINT_DBG(source->verbose, "send event ");
		handler->function(handler, event);
		callback = callback->next;
	}

	module_event_delete(event);

	return 0;
}

/**
 * Create or add an element to the load list
 */
struct load_list *load_list_add(struct load_list *list, char const *name) {
	struct load_list *ptr = list;
	struct load_list *new = malloc(sizeof(*new));

	assert(name);
	assert(new);

	memset(new, 0, sizeof(*new));

	if (name[0] == '-') {
		new->unload = 1;
		new->name = strdup(name + 1);
	} else if (name[0] == '+') {
		new->name = strdup(name + 1);
	} else {
		new->name = strdup(name);
	}

	if (!list) {
		return new;
	}

	/* find the last module in the list */
	while (ptr->next) {
		ptr = ptr->next;
	}

	ptr->next = new;

	return list;

}

/**
 * Check a name from in the load list
 * @return 0 if no change, 1 of must load, -1 of not load
 */
int load_list_check(struct load_list *list, char const *name,
		char const *xmlarg) {
	while (list) {
		if (strcmp(list->name, name) == 0) {
			if (list->unload)
				return 0;
			else
				return 1;
		}

		list = list->next;

	}

	if (xmlarg)
		return 0;

	return 1;

}
