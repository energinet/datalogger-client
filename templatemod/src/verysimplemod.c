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
    int count;
} TemplateObject;

TemplateObject* module_get_struct(struct module_base *module)
{
    return (TemplateObject*)module;
}


/*
** Init-funktion: blive kaldt en gang når contdaem starter.
*/
int module_init(struct module_base *base, const char **attr)
{
    TemplateObject *this;    
    this = module_get_struct(base);
    this->count = 0;
    return 0;
}

/*
** loop-funktion: blive kaldt når contdaem er færdig med sine
** init-funktioner. Loop-funktionen skal loope så længe base->run er
** forskellige fra 0.
*/
void *module_loop(void *parm)
{

    struct module_base *base;
    TemplateObject *this;    
    
    base = (struct module_base *)parm;
    this = module_get_struct(base);


    while(base->run)
    {
        printf("%2d ", this->count); fflush(stdout);
        this->count++;
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
        .el     = NULL,
        .parent = NULL,  
        .start  = NULL,
    }				
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

