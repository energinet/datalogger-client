/*
** contdaem-module: Eksempel på et simpelt modul, skrevet i 
** programmeringssproget C, til LIABSG's kontroldaemon.
** LIAB ApS, marts 2011.
*/
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>    
#include <unistd.h> 
#include "module_base.h"

/*
** Datastruktur til at opbevare alle private data for vores
** modul. Ligeledes haves en wrapperfunktion, som returnerer
** en pointer til instansen af TemplateObject
*/
typedef struct sTemplateObject
{
    struct module_base base;
    struct event_type *TempRegEvent;
    int setpoint;
    int hysteresis;
} TemplateObject;

TemplateObject* module_get_struct(struct module_base *module)
{
    return (TemplateObject*)module;
}

/*
** Her modtages events fra et andet modul. Der kan afsendes events
** herfra til andre moduler...
*/
int ReceiveEvent(EVENT_HANDLER_PAR)
{
    TemplateObject  *this;    
    struct uni_data *indata, *outdata;
    float fvalue, fsetpoint;

    this = module_get_struct(handler->base);
    indata = event->data;
    fvalue = uni_data_get_fvalue(indata);
    fsetpoint = (float)this->setpoint;

    printf("Tempreg: %5.2f (%5.2f)\n", fvalue, fsetpoint);
/*
**
**  DET ER NU *JERES* OPGAVE AT INTRODUCERE HYSTERESE I REGULATOREN!!!
**
*/
    outdata  = uni_data_create_int(fvalue < fsetpoint);
    event = module_event_create(this->TempRegEvent, outdata, NULL);
    module_base_send_event(event);

    return 0;
}

struct event_handler handlers[] = 
{ 
    {
        .name     = "Receiver", 
        .function =  ReceiveEvent,
    }, 
    {
        .name     = "",
    }
};

#define DEFAULTSETPOINT   (30)
#define DEFAULTHYSTERESIS  (2)

int TempRegEventSenderStart(XML_START_PAR) 
{
    struct modules *modules;
    struct module_base *base;
    TemplateObject *this;

    modules = (struct modules*)data; 
    base    = ele->parent->data;
    this    = module_get_struct(base);

    struct event_handler* handler = NULL;
    handler = event_handler_get(base->event_handlers, "Receiver");

    module_base_subscribe_event(base, modules->list, 
                                get_attr_str_ptr(attr, "event"), 
                                FLAG_ALL,  
                                handler, 
                                attr);

    this->setpoint   = get_attr_int(attr, "setpoint", DEFAULTSETPOINT);
    this->hysteresis = get_attr_int(attr, "hysteresis", DEFAULTHYSTERESIS);

    this->TempRegEvent = event_type_create_attr (base, NULL, attr);
    base->event_types = event_type_add(base->event_types, 
                                       this->TempRegEvent);
    return 0;
} 

/*
** Init-funktion: blive kaldt en gang når contdaem starter.
*/
int module_init(struct module_base *base, const char **attr)
{
    TemplateObject *this;    
    this = module_get_struct(base);
    this->setpoint   = DEFAULTSETPOINT;
    this->hysteresis = DEFAULTHYSTERESIS;
    return 0;
}

/*
** loop-funktion: blive kaldt når contdaem er færdig med sine
** init-funktioner. Loop-funktionen skal loope så længe base->run er
** forskellige fra 0.
*/
void *module_loop(void *parm)
{

    struct module_base  *base;
    struct module_event *event;
    struct uni_data     *data;
    TemplateObject *this;    
    
    base = (struct module_base *)parm;
    this = module_get_struct(base);

    while(base->run)
    {
        sleep(1);
    }
}

/*
** Foskellige instanser af datastrukturer, som selve contdaem har brug
** for til at holde styr på instanser af modulet.
*/

/* Liste af XML tags, der er definerer for dette modul: */
struct xml_tag module_tags[] = 
{
    { 
        .el     = "tempreg",
        .parent = "module",  
        .start  = TempRegEventSenderStart,
    },
    { 
        .el     = NULL,
    },
				
};  

/* Central datastruktur for modulet: */
struct module_type module_type = 
{
    .name             = "templatemod", 
    .xml_tags         = module_tags,
    .handlers         = handlers,
    .type_struct_size = sizeof(TemplateObject),
};

struct module_type *module_get_type()
{
    return &module_type;
}

