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

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> //sleep
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <linux/input.h>
#include <assert.h>

/**
 * @ingroup modules 
 * @{
 */

/**
 * @defgroup modules_ledpanel Ledpanel Module
 * @{
 * Module for controling front ledbanels.\n
 * <b>Typename: "ledpanel" </b>\n
 * <b>Attributes:</b>\n
 * - \b config: The configuration name @ref sgconf and @ref dinconf\n
 * .
 * <b> Tags: </b>
 * - <b>led: Set up a LED</b>\n
 *    <b>Attributes:</b>\n
 *     - \b name: The name of the LED in the chosen configuration.
 *     - \b mode: The output related to the inputs. See the output modes section
 *     - \b event: The name of event the event to connect
 *     - \b def: Default value
 *     .
 * .
 * <b> Output modes </b>\n
 * The mode attribute is a string describing the LED behavior. The string consists if a number of chars:
 *     - \b 'b': black
 *     - \b 'g': green
 *     - \b 'y': yellow 
 *     - \b 'r': red
 *     - \b '_': blink
 *     .
 * Ex.\n
 *  String : "g_gyrb"
 * <TABLE>
 * <TR><TH>Input</TH><TH>Output</TH></TR>
 * <TR><TD>0</TD><TD>Green</TD></TR>
 * <TR><TD>1</TD><TD>Blink green</TD></TR>
 * <TR><TD>2</TD><TD>Yellew</TD></TR>
 * <TR><TD>3</TD><TD>Red</TD></TR>
 * <TR><TD>4</TD><TD>Black (off)</TD></TR>
 * </TABLE> 
 * <b> Example of XML a block in the configuration </b>
 * @verbatim
 <module type="ledpanel" name="ledpanel" verbose="0">
 <led name="net"  mode="r_ygb" def="3" event="net.state"/>
 <led name="app"  mode="gyrb" def="3" event="system.state" />
 <led name="1pt3"  mode="gr_rb" def="3" event="pt1000.fault"/>
 <led name="4pt6"  mode="gr_rb" def="3"  />
 <led name="flow"  mode="grb" def="2" event="vfs.fault"/>
 <led name="rs485"  mode="g_gyrb" event="modbus.fault" def="4" />
 <led name="cont"  mode="bgb" event="inputs.button" def="2" />
 </module>
 @endverbatim
 */

/**
 * Maximal modes for a LED
 * @memberof led_obj
 * @private
 */
#define MAXMODES 10

/**
 * Mode bits
 * @memberof led_obj
 * @private
 */
enum led_mode {
	LED_OFF = 0, LED_RED = 1, LED_GREEN = 2, LED_YELLOW = 3, LED_BLINK = 16,
};

/**
 * LED configuration struct
 */
struct ledconf {
	/**
	 * LED number */
	int number;
	/**
	 * LED name */
	char *name;
	/**
	 * Path to the control of the red LED
	 * @note Ignored if NULL*/
	char *red;
	/**
	 * Path to the control of the green LED
	 * @note Ignored if NULL*/
	char *green;
};

/**
 * LED configuration for LIAB SG
 */
struct ledconf sgconf[] = { { 5, "puls", "/sys/class/leds/led_00r/brightness",
		"/sys/class/leds/led_00g/brightness" }, { 6, "net",
		"/sys/class/leds/led_01r/brightness",
		"/sys/class/leds/led_01g/brightness" }, { 7, "app",
		"/sys/class/leds/led_02r/brightness",
		"/sys/class/leds/led_02g/brightness" }, { 8, "1pt3",
		"/sys/class/leds/led_03r/brightness",
		"/sys/class/leds/led_03g/brightness" }, { 9, "4pt6",
		"/sys/class/leds/led_04r/brightness",
		"/sys/class/leds/led_04g/brightness" }, { 10, "flow",
		"/sys/class/leds/led_05r/brightness",
		"/sys/class/leds/led_05g/brightness" }, { 11, "rs485",
		"/sys/class/leds/led_06r/brightness",
		"/sys/class/leds/led_06g/brightness" }, { 11, "cont",
		"/sys/class/leds/led_07r/brightness",
		"/sys/class/leds/led_07g/brightness" }, { -1, NULL, NULL, NULL }, };

/**
 * LED configuration for LIAB DIN
 */
struct ledconf dinconf[] = { { 14, "d201",
		"/sys/class/leds/addon-led-14/brightness", NULL }, { 13, "d202",
		"/sys/class/leds/addon-led-13/brightness", NULL }, { 12, "d203",
		"/sys/class/leds/addon-led-12/brightness", NULL }, { 11, "d204",
		"/sys/class/leds/addon-led-11/brightness", NULL }, { 10, "d205",
		"/sys/class/leds/addon-led-10/brightness", NULL }, { 9, "d206",
		"/sys/class/leds/addon-led-9/brightness", NULL }, { 8, "d207",
		"/sys/class/leds/addon-led-8/brightness", NULL }, { 7, "d208",
		"/sys/class/leds/addon-led-7/brightness", NULL }, { 6, "d209",
		"/sys/class/leds/addon-led-6/brightness", NULL }, { 5, "d210",
		"/sys/class/leds/addon-led-5/brightness", NULL }, { 4, "d211",
		"/sys/class/leds/addon-led-4/brightness", NULL }, { 3, "d212",
		"/sys/class/leds/addon-led-3/brightness", NULL }, { 2, "d213",
		"/sys/class/leds/addon-led-2/brightness", NULL }, { 1, "d214",
		"/sys/class/leds/addon-led-1/brightness", NULL }, { -1, NULL, NULL,
		NULL }, };

/**
 * LED object struct
 * @private
 */
struct led_obj {
	/**
	 * Path to red LED */
	char *red;
	/**
	 * Path to green LED */
	char *green;
	/**
	 * The current output mode*/
	enum led_mode set_color;
	/**
	 * The list of output modes*/
	enum led_mode modes[MAXMODES];
	/**
	 * Toggle (used for blinking)*/
	int toggle;
	/**
	 * Next led in the list*/
	struct led_obj *next;
};

/**
 * LED panel object struct
 * \extends module_base
 */
struct ledpanel_object {
	/**
	 * Module base */
	struct module_base base;
	/**
	 * Configuration */
	struct ledconf *conf;
	/**
	 * List of LED objects */
	struct led_obj *leds;
};

/**
 * Return the module pointer 
 * @memberof ledpanel_object
 * @param base The base module
 * @return @ref ledpanel_object
 * @private
 */
struct ledpanel_object* module_get_struct(struct module_base *base) {
	return (struct ledpanel_object*) base;
}

/**
 * Det LED output
 * @memberof ledconf
 * @private
 */
int led_set_output(const char *path, int value) {
	FILE *file = NULL;

	if (!path) {
		return 0;
	}

	file = fopen(path, "w");

	if (!file) {
		PRINT_ERR("error opening %s: %s", path, strerror(errno));
		return -errno;
	}

	fprintf(file, "%d", value);

	fclose(file);

	return 0;

}

/**
 * Get led configuration of the name
 * @memberof ledconf
 * @private
 */
struct ledconf *ledconf_get(struct ledconf *list, const char *name) {
	struct ledconf *ptr = list;

	if (!name) {
		PRINT_ERR("led name = NULL");
		return NULL;
	}

	if (!list) {
		PRINT_ERR("led conf list is null");
		return NULL;
	}

	while (ptr->name) {
		if (strcmp(ptr->name, name) == 0)
			return ptr;
		ptr++;
	}

	PRINT_ERR("unknown led name %s", name);
	ptr = list;
	fprintf(stderr, "list: ");
	while (ptr->name) {
		fprintf(stderr, "%s,", ptr->name);
		ptr++;
	}
	fprintf(stderr, "\n");

	return NULL;
}

/**
 * Create a LED object 
 * @memberof led_obj
 * @private
 */
struct led_obj *led_obj_create(void) {
	struct led_obj *new = malloc(sizeof(*new));
	assert(new);
	memset(new, 0, sizeof(*new));

	return new;
}

/**
 * Create a LED object from @ref ledconf
 * @memberof led_obj
 * @private
 */
struct led_obj *led_obj_from_conf(struct ledconf *conf) {
	struct led_obj *new = NULL;
	if (!conf) {
		return NULL;
	}

	new = led_obj_create();

	if (conf->red)
		new->red = strdup(conf->red);

	if (conf->green)
		new->green = strdup(conf->green);

	return new;
}

/**
 * Add a LED object to a list
 * @memberof led_obj
 * @private
 */
struct led_obj* led_obj_add(struct led_obj *list, struct led_obj *new) {
	struct led_obj *ptr = list;

	if (!list)
		return new;

	while (ptr->next) {
		ptr = ptr->next;
	}

	ptr->next = new;

	return list;
}

/**
 * Deleta a LED object or list of
 * @memberof led_obj
 * @private
 */
void led_obj_delete(struct led_obj *led) {
	if (!led)
		return;

	acc_obj_delete(led->next);
	free(led->red);
	free(led->green);
	free(led);
}

/**
 * Set LED output
 * @memberof led_obj
 * @private
 */
void led_obj_upd_output(struct led_obj *led) {

	if (LED_BLINK & led->set_color && (led->toggle++) % 2) {
		led_set_output(led->green, 0);
		led_set_output(led->red, 0);
	} else {
		led_set_output(led->green, LED_GREEN & led->set_color);
		led_set_output(led->red, LED_RED & led->set_color);
	}

}

/**
 * Set LED mode 
 * @memberof led_obj
 * @private
 */
void led_obj_set_mode(struct led_obj *led, int mode) {
	if (mode < 0 || mode >= MAXMODES) {
		PRINT_ERR("mode is out of bound %d (max%d)", mode, MAXMODES);
		return;
	}

	led->set_color = led->modes[mode];

	led_obj_upd_output(led);
}

/**
 * Set LED modes from string
 * @memberof led_obj
 * @private
 */
int led_obj_set_modes(struct led_obj *led, const char *modes) {
	int i = 0;
	int n = 0;

	memset(led->modes, 0, sizeof(led->modes));

	if (!modes)
		return -EFAULT;

	fprintf(stderr, "mode %s\n", modes);

	while (i <= MAXMODES && modes[n] != '\0') {
		switch (modes[n]) {
		case 'r':
			led->modes[i] |= LED_RED;
			i++;
			break;
		case 'g':
			led->modes[i] |= LED_GREEN;
			i++;
			break;
		case 'y':
			led->modes[i] |= LED_YELLOW;
			i++;
			break;
		case '_':
			led->modes[i] |= LED_BLINK;
			break;
		case 'b':
			led->modes[i] = LED_OFF;
			i++;
			break;
		case ',':
		default:
			break;
		}

		n++;
	}

	return 0;
}

/**
 * Receive handler for @ref led_obj 
 * @memberof led_obj
 * @note See @ref EVENT_HANDLER_PAR for paramaters
 */
int handler_rcv(EVENT_HANDLER_PAR) {
	struct ledpanel_object* this = module_get_struct(handler->base);
	struct led_obj *led = (struct led_obj*) handler->objdata;
	struct uni_data *data = event->data;

	led_obj_set_mode(led, data->value);

	return 0;
}

/**
 * Start a led tag
 * @private
 */
int start_led(XML_START_PAR) {
	struct modules *modules = (struct modules*) data;
	struct ledpanel_object* this = (struct ledpanel_object*) ele->parent->data;
	struct event_handler *event_handler = NULL;
	struct led_obj *new = led_obj_from_conf(ledconf_get(this->conf,
			get_attr_str_ptr(attr, "name")));
	const char *event_name = get_attr_str_ptr(attr, "event");

	if (!new) {
		PRINT_ERR("unknown led: %s event: %s", get_attr_str_ptr(attr, "name"), event_name);
		return -EFAULT;
	}

	led_obj_set_modes(new, get_attr_str_ptr(attr, "mode"));

	this->leds = led_obj_add(this->leds, new);
	led_obj_set_mode(new, get_attr_int(attr, "def", 0));

	/* create event handler */
	event_handler = event_handler_create(get_attr_str_ptr(attr, "name"),
			handler_rcv, &this->base, (void*) new);

	this->base.event_handlers = event_handler_list_add(
			this->base.event_handlers, event_handler);

	if (event_name) {
		return module_base_subscribe_event(&this->base, modules->list,
				event_name, FLAG_ALL, event_handler, attr);
	}
	return 0;

}

/**
 * LED panel XML tags
 * @memberof ledpanel_object
 * @private
 */
struct xml_tag module_tags[] = { { "led", "module", start_led, NULL, NULL }, {
		"", "", NULL, NULL, NULL } };

/**
 * LED panel init 
 * @memberof ledpanel_object
 * @private
 */
int module_init(struct module_base *module, const char **attr) {
	struct ledpanel_object* this = module_get_struct(module);

	const char *confstr = get_attr_str_ptr(attr, "config");

	this->conf = sgconf;

	if (confstr) {
		if (strcmp(confstr, "ensg") == 0) {
			this->conf = sgconf;
		} else if (strcmp(confstr, "din") == 0) {
			this->conf = dinconf;
		}
	}

	return 0;
}

/**
 *  LED panel update loop
 * @memberof ledpanel_object
 * @private
 */
void* module_loop(void *parm) {
	struct ledpanel_object *this = module_get_struct(parm);
	struct module_base *base = (struct module_base *) parm;
	int retval;
	time_t prev_time;
	base->run = 1;

	while (base->run) {
		struct led_obj *ptr = this->leds;

		while (ptr) {
			led_obj_upd_output(ptr);
			ptr = ptr->next;
		}
		sleep(1);
	}

	PRINT_MVB(base, "loop returned");

	return NULL;

}

/**
 * LED panel handlers
 * @memberof ledpanel_object
 * @private
 */
struct event_handler handlers[] = { { .name = "rcv", .function = handler_rcv },
		{ .name = "" } };

/**
 * LED panel type object
 * @memberof ledpanel_object
 * @private
 */
struct module_type module_type = { .name = "ledpanel", .xml_tags = module_tags,
		.handlers = handlers,
		.type_struct_size = sizeof(struct ledpanel_object), };

/**
 * LED panel get type function
 * @memberof ledpanel_object
 * @private
 */
struct module_type *module_get_type() {
	return &module_type;
}

/**
 * @} 
 */

/**
 * @} 
 */
