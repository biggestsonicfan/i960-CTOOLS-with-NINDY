
#include <stdio.h>

/******************************************************************************
 * paginator():
 *	
 *	This guy was meant to assure correct pagnation for the help switches 
 *	(especially when using DOS).  Now it's just an empty stub, for 
 *	pagination use 'more'  
 *
 *****************************************************************************/
void paginator( help_text )

char *help_text[];	/* the actual help message */
{
	int i;

        for(i=0; help_text[i] != NULL;i++) {
                fprintf( stdout, "%s\n",help_text[i] );
        }
}
