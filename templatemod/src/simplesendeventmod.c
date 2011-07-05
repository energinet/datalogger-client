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
    struct event_type *TikTakEvent;
    int interval;
    int count;
} TemplateObject;

TemplateObject* module_get_struct(struct module_base *module)
{
    return (TemplateObject*)module;
}

#define MINIMUMINTERVAL (100)
#define DEFAULTINTERVAL (1000)
#define MAXIMUMINTERVAL (10000)

int TikTakEventSenderStart(XML_START_PAR) 
{
    struct modules *modules;
    struct module_base *base;
    TemplateObject *this;

    modules = (struct modules*)data; 
    base    = ele->parent->data;
    this    = module_get_struct(base);

    this->interval = get_attr_int(attr, "interval", DEFAULTINTERVAL);
    if (this->interval < MINIMUMINTERVAL)
        this->interval = MINIMUMINTERVAL;
    if (this->interval > MAXIMUMINTERVAL)
        this->interval = MAXIMUMINTERVAL;

    this->TikTakEvent = event_type_create_attr (base, NULL, attr);
    base->event_types = event_type_add(base->event_types, 
                                       this->TikTakEvent);
    return 0;
} 

/*
** Init-funktion: blive kaldt en gang når contdaem starter.
*/
int module_init(struct module_base *base, const char **attr)
{
    TemplateObject *this;    
    this = module_get_struct(base);
    this->count = 0;
    this->TikTakEvent = NULL;
    this->interval = DEFAULTINTERVAL;
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
        printf("%2d ", this->count); fflush(stdout);
        this->count++;
        
        if(this->TikTakEvent != NULL)
        {
            data  = uni_data_create_int(this->count % 2); 
            event = module_event_create(this->TikTakEvent, data, NULL);
            module_base_send_event(event);
        }
        usleep(1000*this->interval);
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
        .el     = "tiktak",
        .parent = "module",  
        .start  = TikTakEventSenderStart,
    },
    { 
        .el     = NULL,
    },
				
};  

/* Liste af event handlers, der er definerer for dette modul: */
struct event_handler handlers[] = 
{ 
    { 
        .name = ""  
    } 
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

