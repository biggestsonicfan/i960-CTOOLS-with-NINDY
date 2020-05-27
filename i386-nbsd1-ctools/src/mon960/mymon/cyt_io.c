/*
 * i/o routines for tests.  Greg Ames, 9/17/90.
 *
 * Version: @(#)test_io.c	1.2 8/26/93
 */


void hex8out (unsigned char num);
void hex32out (unsigned long num);
long hexIn (void);
void print (char *string);

#define TRUE	1
#define FALSE	0

extern int atod();
extern char ci();
extern void prtf();
void readline();

/*
 * naive implementation of "gets"
 * (big difference from fgets == strips newline character)
 */
 
char *
  sgets(s)
char *s;
{
#if 0
  char *retval=s;
  char ch;
 
  while ( ch = ci() ){
    if (ch == '\n'){
      break;
    }
    *s++ = ch;   
  }
  *s = '\0';
  return retval; 
#endif
    readline(s, 40);	/* read up to 40 characters */
}

/* Returns true if theChar is a valid hex digit, false if not */
 
char    ishex(theChar)
char    theChar;
{
        switch(theChar) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'a':
        case 'B':
        case 'b':
        case 'C':
        case 'c':
        case 'D':
        case 'd':
        case 'E':
        case 'e':
        case 'F':
        case 'f':
                return 1;
        default:
                return 0;
        }
}

/* Returns true if theChar is a valid decimal digit, false if not */
 
char    isdec(theChar)
char    theChar;
{
        switch(theChar) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
                return 1;
        default:
                return 0;
	}
}

/* Convert ascii code of hex digit to number (0-15) */
 
char    hex2dec(hex)
char    hex;
{
        if ((hex >= '0') && (hex <= '9'))
                return hex - '0';
        else if ((hex >= 'a') && (hex <= 'f'))
                return hex - 'a' + 10;
        else
                return hex - 'A' + 10;
}
 
/* Convert number (0-15) to ascii code of hex digit */
 
char    dec2hex(dec)
char    dec;
{
        return (dec <= 9) ? (dec + '0') : (dec - 10 + 'A');
}

/* Output an 8 bit number as 2 hex digits */

void	hex8out(unsigned char num)
{
	(void) prtf("%B",num);
}

/* Output an 32 bit number as 8 hex digits */

void	hex32out(unsigned long num)
{
	(void) prtf("%X",num);
}

/* Input a number as (at most 8) hex digits - returns value entered */

long	hexIn(void)
{
    	char	input[40];
	long	num;
	register int i;

	i = 0;
	num = 0;

	if (sgets (input))  /* grab a line */
	{
            num = hex2dec(input[i++]);       	/* Convert MSD to dec */
            while(ishex(input[i]) && input[i])  /* Get next hex digit */
	    {
                num <<= 4;                 	/* Make room for next digit */
                num += hex2dec(input[i++]); 	/* Add it in */
            }

	}
	return num;
}

/* Input a number as decimal digits - returns value entered */

long	decIn(void)
{
    	char	input[40];
	long	num;
#if 0
	int 	tmp;
	register int i;

	i = 0;
	num = 0;

	if (sgets (input))  /* grab a line */
	{
            atod(input[i++], &num);      	/* Convert MSD to decimal */
            while(isdec(input[i]) && input[i])  /* Get next decimal digit */
	    {
                num *= 10;                 	/* Make room for next digit */
		atod(input[i++], &tmp);
                num += tmp; 			/* Add it in */
            }

	}
#endif

	sgets (input);
	if (atod (input, &num) == FALSE)
	    num = 0;
	return num;
}

void	print(char *string)
{
	(void) prtf("%s", string);
}
