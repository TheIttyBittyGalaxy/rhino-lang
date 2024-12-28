#include "analyse.h"
#include <string.h>

void analyse(Compiler *c, Program *apm)
{
    // Determine main function
    bool determined_main_function = false;
    for (size_t i = 0; i < apm->function.count; i++)
    {
        Function *funct = get_function(apm->function, i);
        if (substr_is(c->source_text, funct->identity, "main"))
        {
            apm->main = i;
            determined_main_function = true;
            break;
        }
    }

    if (!determined_main_function)
    {
        raise_compilation_error(c, NO_MAIN_FUNCTION, strlen(c->source_text));
    }
}