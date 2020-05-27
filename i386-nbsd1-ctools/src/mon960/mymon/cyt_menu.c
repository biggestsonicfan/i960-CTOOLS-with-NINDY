/* menu.c - Menu dispatching routine */

/*
modification history
--------------------
01a,04mar92,bwb		written.
02a,09apr92,tga		modified to run with Nindy.  Changed #includes,
			gets() to readline() and sscanf() to atod() in
			menuGetChoice, printf() to prtf() throughout.
*/

/*
DESCRIPTION

A table-driven menu dispatcher
*/

#include "cyt_menu.h"
#include "common.h"

#define TRUE			1
#define FALSE			0
#define QUIT			-1
#define MAX_INPUT_LINE_SIZE	80

void prtf();
char *sgets();
int atod();

/*
* Internal routines
*/
static int menuGetChoice (MENU_ITEM	menuTable[],
			  int		numMenuItems,
			  char		*title,
			  unsigned long	options);
static void printMenu (MENU_ITEM	menuTable[],
		       int		numMenuItems,
		       char		*title);


/***************************************************************************
*
* menu - a table-driven menu dispatcher
*
* RETURNS:
*
*	The menu item argument, or NULL if the item chosen is QUIT.
*/

MENU_ARG
menu (
      MENU_ITEM	menuTable[],
      int		numMenuItems,
      char		*title,
      unsigned long	options
      )
{

    int		item;		/* User's menu item choice */


    /*
     * Get the user's first choice.  Always display the menu the first time.
     */
    item = menuGetChoice (menuTable, numMenuItems, title, MENU_OPT_NONE);
    if (item == QUIT)
	return (NULL);

    /*
     * If the user just wants a value returned, return the argument.  If the
     * argument is null, return the item number itself.
     */
    if (options & MENU_OPT_VALUE)
    {
	if (menuTable[item].arg == NULL)
	    return ((void *)item);
	else
	    return (menuTable[item].arg);
    }

    /*
     * Process menu items until the user selects QUIT
     */
    while (TRUE)
    {
	/*
	 * Call the action routine for the chosen item.  If the argument is
	 * NULL, pass the item number itself.
	 */
	if (menuTable[item].actionRoutine != NULL)
	{
	    if (menuTable[item].arg == NULL)
	    {
		prtf("\n");
		(*menuTable[item].actionRoutine) ((void *)item);
	    }
	    else
	    {
		prtf("\n");
		(*menuTable[item].actionRoutine) (menuTable[item].arg);
	    }
	}

	/*
	 * Get the next choice, using any display options the user specified.
	 */
	item = menuGetChoice (menuTable, numMenuItems, title, options);
	if (item == QUIT)
	    return (NULL);

    }

} /* end menu () */


/***************************************************************************
*
* menuGetChoice - Get the user's menu choice.
*
* If display is not suppressed, display the menu, then prompt the user for
* a choice.  If the choice is out of range or invalid, display the menu and
* prompt again.  Continue to display and prompt until a valid choice is made.
*
* RETURNS:
*	The item number of the user's menu choice. (-1 if they chose QUIT)
*/

static int
menuGetChoice (
	       MENU_ITEM	menuTable[],
	       int		numMenuItems,
	       char		*title,
	       unsigned long	options
	       )
{
    char	inputLine[MAX_INPUT_LINE_SIZE];
    int		choice;


    /*
     * Suppress display of the menu the first time if we're asked
     */
    if (!(options & MENU_OPT_SUPPRESS_DISP))
	printMenu (menuTable, numMenuItems, title);

    /*
     * Prompt for a selection.  Redisplay the menu and prompt again
     * if there's an error in the selection.
     */
    choice = -1;
    while (choice < 0 || choice > numMenuItems)
    {
	prtf ("\nEnter the menu item number (0 to quit): ");
#if 0
	if (gets (inputLine))
	    status = sscanf (inputLine, "%d", &choice);

	readline (inputLine, MAX_INPUT_LINE_SIZE, FALSE);
	printf ("\n");
	if (atod (inputLine, &choice) == FALSE)
	    choice = -1;
#endif
	sgets (inputLine);
	prtf ("\n");
	if (atod (inputLine, &choice) == FALSE)
	    choice = -1;
	if (choice < 0 || choice > numMenuItems)
	    printMenu (menuTable, numMenuItems, title);
    }

    if (choice == 0)
	return (QUIT);

    return (choice - 1);

} /* end menuGetChoice () */


/***************************************************************************
*
* printMenu - Print the menu
*
*
*/

static void
printMenu (
	   MENU_ITEM	menuTable[],
	   int		numMenuItems,
	   char		*title
	   )
{
    int		i;


    prtf("\n%s\n\n", title);

    for (i = 0; i < numMenuItems; i++)
    {
	if (i < 9)
	    prtf(" ");
        /* note that prtf doesn't really support %d may need to be fixed... */
	prtf ("%d - %s\n", i+1, menuTable[i].itemName);
    }

    prtf("    0 - quit");

} /* end printMenu () */
