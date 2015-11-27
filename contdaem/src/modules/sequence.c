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

#include "module_base.h"
#include "module_value.h"

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
#include <math.h>
#include <assert.h>

#include "sequence_io.h"




/**
 * @addtogroup module_xml
 * @{
 * @section seqmodxml Sequence module
 * Module for handeling sequences 
 * <b>Typename: "sqauence" </b>\n
 * <b> Tags: </b>
 * <ul>
 * <li><b>input:</b> Configure input event\n  
 *   - Configures as an evalue 
 *   - Ex.
 @verbatim  <input event="inputs.button" name="button" type="int" default="-1" /> @endverbatim
 *   - Note: By setting the default value to -1 the sequence will wait until an event arrives. 
 * <li><b>output:</b> Configures an output event\n
 *   - Configures as an evalue 
 @verbatim <output name="pump" type="int" default="0"/>  @endverbatim
 * <li><b>sequence:</b>  The sequence to follow \n
 * </ul>
 * @subsection seqmodxmlseq The sequence definition 
 * <ul>
 * <li><b>wait:</b> Wait for event or time \n  
 * This can either be configured to wait for at predefined time or for an event to happen. The wait event uses two attributes 
 * \b until and \b value. 
 *  - \b until: The event to wait for. This can either be an input defined in the module og an ename. 
 *  - \b value: The value to wait for. If this attribut is left out any value will continue the sequence.
 *  - \b time: The time to wait in miliseconds. Either timeout for an event or stand alone timer.
 *  - Ex. Wait until "button" is pressed with no timeout:
 @verbatim  <wait until="button" value="1"/> @endverbatim
 *  - Ex. Wait 5 seconds (5000 ms):
 @verbatim <wait time="5000"/> @endverbatim
 * <li><b>set:</b> Set an output\n
 *   This tags instructs the sequence to set an output value.
 * - \b output: The name of the output to set.
 * - \b value: The value to set
 * - Ex. Set the output pump to 1 
 @verbatim  <set output="pump" value="1"/> @endverbatim
 * <li><b>sequence:</b>  The sequence to follow \n
 * </ul>
 * @subsection seqmodxmlex An example setup
 * This examble shows a setup with one input and four outputs. The sequence in this example will wait until the
 * \b button is pressed (value=1). Then it will set the putputs \b pump and \b brine to 1 an wait five seconds before 
 * settings the outputs \b condenc and \b heatpump to 1. After the first part it will wait until the \b button is released. 
 * Then set \b heatpump, \b pump, \b brine and wait 5 sec before setting \b condentc to 0


@verbatim <module type="sequence" name="sequence" verbose="true" >
	<output name="pump" type="int" default="0"/>
	<output name="brine" type="int" default="0"/>
	<output name="condentc" type="int" default="0"/>
	<output name="heatpump" type="int" default="0"/>
	<input event="inputs.button" name="button" type="int" default="-1" />
	<sequence>
		<wait event="button" value="1"/>
		<set output="pump" value="1"/>
		<set output="brine" value="1"/>
		<wait time="5000"/>
		<set output="condentc" value="1"/>
		<set output="heatpump" value="1"/>
		<wait event="button" value="0"/>
		<set output="heatpump" value="0"/>
		<set output="pump" value="0"/>
		<set output="brine" value="0"/>
		<wait time="5000"/>
		<set output="condentc" value="0"/>		
	</sequence>
</module>@endverbatim
 * @} 
 */


/**
 * @defgroup modules Available modules
 * @{
 */

/**
 * @addtogroup module_sequence Sequence module 
 * @{
 */


#define SEQTAG "sequence"

enum seq_type {
	SEQ_NONE,
	SEQ_WAIT_EVENT,
	SEQ_WAIT_TIME,
	SEQ_SET
};

/**
 * Trigger type sytings 
 * @ref trigtype
 */
const char *seq_type_strs[] = { "none", "waitevt", "waittime" , "set", NULL };


struct seq_item_wait_event {
    /**
     * Input object */
    struct seq_io *input;
    /**
     * Timeout in msec. Set to 0 for no timeout */
    int timeout;
    /**
     * Value to wait for */
    float value;	
};

struct seq_item_wait_time {
	int msec;
};

struct seq_item_set {
	struct seq_io *output;
	float value;
};


struct seq_item {
	enum seq_type type;
	char *label;
	union {
		struct seq_item_wait_event w_evt;
		struct seq_item_wait_time w_time;
		struct seq_item_set set;
	} p;
	struct seq_item *next;
};



struct seq_module{
    struct module_base base;
	struct seq_item *sequence;
	struct seq_io *inputs;
	struct seq_io *outputs;
};


struct seq_io *seq_module_input_getcreat(struct seq_module *this, const char *name)
{
    struct seq_io *io = seq_io_get_by_name(this->inputs, name);
    
    if(io)
	return io;

    io = seq_io_create_input(&this->base, name);

    return io;
    
}

struct seq_module* module_get_struct(struct module_base *base){
    return (struct seq_module*) base;
}


int seq_item_lst_size(struct seq_item *list);


struct seq_item *seq_item_create_attr(struct seq_module *this, const char *el ,const char **attr )
{
    struct module_base *base = &this->base;
    struct seq_item *new = malloc(sizeof(*new));
    assert(new);

    new->label = modutil_strdup(get_attr_str_ptr(attr, "label"));
    
    if(!new->label){
	int num = seq_item_lst_size(this->sequence);
	char buf[32];
	sprintf(buf, "%d", num);
	new->label = strdup(buf);
    }

    if((strcmp(el, "wait")==0) || (strcmp(el, "waitevt")==0) || (strcmp(el, "waittime")==0)){
	PRINT_MVB(base, "Creating sequence wait");
	const char *event_str = get_attr_str_ptr(attr, "event");
	if(!event_str){
	    new->type = SEQ_WAIT_TIME;
	    new->p.w_time.msec = get_attr_int(attr, "time", 1000);
	} else {
	    new->type = SEQ_WAIT_EVENT;
	    new->p.w_evt.input = seq_module_input_getcreat(this, event_str);
	    const char *value_str = get_attr_str_ptr(attr, "value");
	    if(value_str)
		new->p.w_evt.value = atof(value_str);
	    else
		new->p.w_evt.value = 0.0/0.0;

	    PRINT_MVB(base, "Created sequence wait for %s type %d", seq_io_get_name(new->p.w_evt.input), isnan(new->p.w_evt.value))	;
	}
	
    } else if (strcmp(el, "mark")==0) {
	PRINT_MVB(base, "Createing sequence mark");
	new->type = SEQ_NONE;
    } else if (strcmp(el, "set")==0) {
	PRINT_MVB(base, "Createing sequence output");
	new->type = SEQ_SET;
	new->p.set.output = seq_io_get_by_name(this->outputs,get_attr_str_ptr(attr, "output"));
	new->p.set.value =  get_attr_float(attr, "value", 0);
	PRINT_MVB(base, "Created sequence output %s", seq_io_get_name(new->p.set.output))	;
    } else {
	free(new);
	PRINT_ERR("Error: Element '%s' is not supported by sequence", el);
	return NULL;
    }

    new->next = NULL;

    PRINT_MVB(base, "Created sequence item %s type: %s , 0x%p", el, seq_type_strs[new->type], new)	;
    return new;
}


struct seq_item *seq_item_lst_add(struct seq_item *list, struct seq_item *new)
{

	struct seq_item *ptr = list;

    if(!list){
        return new;
    }
	
    while(ptr->next){
        ptr = ptr->next;
    }
    
    ptr->next = new;
    return list;
}

void seq_item_delete(struct seq_item *item)
{
	if(!item){
		return ;
	}

	//ToDo

	free(item);
}

int seq_item_lst_size(struct seq_item *list)
{
    int count = 0;
    struct seq_item *ptr = list;

    while(ptr){
	count++;
        ptr = ptr->next;
    }

    return count;

}


void seq_item_print(struct module_base *base, struct seq_item *item)
{
    
    if(!item)
	return;
    
    switch(item->type){
    case SEQ_WAIT_EVENT:
	PRINT_MVB(base, "item: wait for event");
	break;
    case SEQ_WAIT_TIME:
	PRINT_MVB(base, "item: wait time %d ms", item->p.w_time.msec);
	break;
    case SEQ_SET:
	PRINT_MVB(base, "item: set output %s to %f", seq_io_get_name(item->p.set.output), item->p.set.value);
	break;
    case SEQ_NONE:
	PRINT_MVB(base, "item: none action");
	break;
    default:
	PRINT_MVB(base, "item: unknowntype");
	break;
    }
    
    seq_item_print(base,item->next);
	
}




struct seq_item *seq_item_run(struct seq_module *this, struct seq_item *item)
{
	
    PRINT_MVB(&this->base, "Running sequence '%s' item 0x%p, type %s",item->label,  item, seq_type_strs[item->type]);
    
    switch(item->type){
	  case SEQ_WAIT_EVENT:
	      seq_io_wait(item->p.w_evt.input, item->p.w_evt.value);
	      break;
	case SEQ_WAIT_TIME:
	    usleep(1000*item->p.w_time.msec );
	    break;
	case SEQ_SET:{
	    struct seq_item_set *set = &item->p.set;
	    PRINT_MVB(&this->base, "Setting value %s to %f", seq_io_get_name(set->output), set->value);
	    seq_io_setf(set->output, set->value);
	}break;
	default:
	case SEQ_NONE:
	    break;
	}

	return item->next;
}


int  start_seq_item(XML_START_PAR)
{
	struct module_base* base = ele->parent->data;
    struct seq_module *this = module_get_struct(base);

    fprintf(stderr, "item base %p %s\n", base, el);
	
	struct seq_item *item = seq_item_create_attr(this, el ,attr );
	
	if(!item){
		PRINT_ERR("Error: Could not create swquence item");
		return -1;
	}

	PRINT_MVB(base, "Added sequence item 0x%p, type %s to %p", item, seq_type_strs[item->type], this->sequence);
	
	this->sequence = seq_item_lst_add(this->sequence, item);

    return 0;

}


struct xml_tag seq_tags[] = {
    { "mark"  , SEQTAG , start_seq_item , NULL, NULL},
    { "wait"  , SEQTAG , start_seq_item , NULL, NULL},
    { "set"   , SEQTAG , start_seq_item , NULL, NULL},
    { "" , ""  , NULL, NULL, NULL }
};
    

int  start_output(XML_START_PAR)
{

    struct module_base* base = ele->parent->data;
    struct seq_module *this = module_get_struct(base);
    
    struct seq_io *output = seq_io_create_attr(base, attr);
    if(!output){
	PRINT_ERR("Error: Could not create output");
	return -1;
    }	
    
    this->outputs = seq_io_lst_add(this->outputs, output);
    
    return 0;
}



int  start_input(XML_START_PAR)
{
    struct module_base* base = ele->parent->data;
    struct seq_module *this = module_get_struct(base);
    
    struct seq_io *input = seq_io_create_attr(base, attr);
    if(!input){
	PRINT_ERR("Error: Could not create output");
	return -1;
    }	
    
    this->inputs = seq_io_lst_add(this->inputs, input);
    
    return 0;
}


int  start_sequence(XML_START_PAR)
{
	ele->data =  ele->parent->data;
	ele->ext_tags = seq_tags;

	struct module_base* base = ele->parent->data;

	fprintf(stderr, "seq base %p\n", base);

	return 0;
}


void end_sequence(XML_END_PAR)
{
	ele->ext_tags = NULL;


	struct seq_module *this = module_get_struct(ele->data);
	fprintf(stderr, "seq base %p, sequence %p\n", ele->data,this->sequence );			

	ele->data = NULL;

	seq_item_print(&this->base, this->sequence);



	return;
}



struct xml_tag module_tags[] = {
    { "output" , "module" , start_output, NULL, NULL},
    { "input" , "module" , start_input, NULL, NULL},
    {  SEQTAG  , "module" ,start_sequence, NULL,end_sequence },
    { "" , ""  , NULL, NULL, NULL }
};
    


int module_init(struct module_base *module, const char **attr)
{
	struct seq_module* this = module_get_struct(module);

	this->sequence = NULL;
	this->outputs  = NULL;
 
    return 0;
}


void module_deinit(struct module_base *base)
{
	//ToDo delete objects

    return;
}


/**
 *  LED panel update loop
 * @memberof ledpanel_object
 * @private
 */
void* module_loop(void *parm)
{
    struct seq_module *this = module_get_struct(parm);
	struct module_base *base = ( struct module_base *)parm;
	
	struct seq_item *ptr = this->sequence;

	PRINT_MVB(&this->base, "loop started %p (base %p)", ptr, base);

	if(ptr == NULL){
		PRINT_ERR("Sequence is empty");
		return NULL;
	}


	base->run = 1;
	while(base->run){ 
		
		ptr = seq_item_run(this, ptr);

		if(ptr == NULL)
			ptr = this->sequence;
		
	}

	return NULL;

}




struct module_type module_type = {    
    .name       = "sequence",       
    .xml_tags   = module_tags,                      
    .type_struct_size = sizeof(struct seq_module), 
};


struct module_type *module_get_type()
{
    return &module_type;
}


/**
 * @} 
 */

/**
 * @} 
 */
