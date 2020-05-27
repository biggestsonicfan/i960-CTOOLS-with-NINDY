/*
; ; ; ; ;
;	80960 FPAC/DPAC		Floating Point Library
;	DPTEST				Testbed (SP/DP)
;
;	Copyright (C) 1990 By
;	United States Software Corporation
;	14215 N.W. Science Park Drive
;	Portland, Oregon 97229
;
;	This software is furnished under a license and may be used
;	and copied only in accordance with the terms of such license
;	and with the inclusion of the above copyright notice.
;	This software or any other copies thereof may not be provided
;	or otherwise made available to any other person.  No title to
;	and ownership of the software is hereby transferred.
;
;	The information in this software is subject to change without 
;	notice and should not be construed as a commitment by United
;	States Software Corporation.
;
;	Version:	See VSNLOG.TXT
;	Released:	31 January 1990
; ; ; ; ;
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include	"dpac.h"

/* special i/o data types */

typedef	unsigned int	word;	/* word type */

/* special constant definitions */

#define	FALSE	0
#define	TRUE	1
#define	cr		0x0D
#define	lf		0x0A
#define	bell	0x07
#define	bs		0x08
#define	tab		0x09
#define	rub		0x7F
#define	ctlq	0x11
#define	ctls	0x13
#define	ctlu	0x15
#define	ctlx	0x18
#define	eos		0

/* special i/o file access */

FILE    *ifp, *ofp, *fopen();

/* i/o routines */

putchr(char ch)
{
if ( ifp != stdin)
	putc(ch, ofp);
putchar(ch);
}


int	issep(char **lptr)		/* is char a separator */
{
if ((**lptr==',') || (**lptr==';') || (**lptr==' ') || (**lptr==':'))
	return(TRUE);
else
	return(FALSE);
}


int	skpspc(char **lptr)		/* skip spaces */
{
while (**lptr == ' ')
	*lptr = *lptr + 1;
}


getstg(char **lptr,char *sptr)	/* get a string */
{
skpspc(lptr);
while ((**lptr != eos) && (! issep(lptr)))
	{
	*sptr = **lptr;
	sptr = sptr + 1;
	*lptr = *lptr + 1;
	}
*sptr = eos;
}


char getchr()			/* get char from stdin or file */
{
char	ch;
if (ifp == stdin)		/* use kbd if stdin */
	return(getchar());
else
	{
	ch = getc(ifp);		/* read from file */
	putchr(ch);			/* echo file char */
	return(ch);			/* return ch */
	}
}


int	putspc(char count)		/* output spaces */
{
while (count != 0)
	{
	putchr(' ');
	count = count - 1;
	}
}


int	putcr()			/* output crlf */
{
putchr(cr);
putchr(lf);
}
	

int	putstg(char *stg)	/* output zero terminated string */
{
while (*stg != eos)
	{
	putchr(*stg);
	stg = stg + 1;
	}
}


putln(char *stg)	/* output zero terminated line */
{
putstg(stg);
putcr();
}


int	putnum(int num)		/* output a number (decimal) */
{
int	digit,suppress;
int	divisor;
if (num < 0)
	{
	putchr('-');
	num = -num;
	}
divisor = 10000;
suppress = TRUE;
while (divisor != 1)
	{
	if (num >= divisor)
		digit = num / divisor;
	else
		digit = 0;
	num = num - (digit * divisor);
	divisor = divisor / 10;
	if (suppress && (digit != 0))
		suppress = FALSE;
	if (!suppress)
		putchr('0'+digit);
	}
putchr('0'+num);
}


int	puthex(int num)		/* output hex digit */
{
if (num < 10)
	putchr('0' + num);
else
	putchr('A' + num - 10);
}


char upper(char ch)			/* convert char to uppercase */
{
if ((ch >= 'a') && (ch <= 'z'))
	return(ch - 'a' + 'A');
else
	return(ch);
}


int	getiln(char *lptr)		/* input line */
{
int		looking;
char	ch,col;
char	*orgptr;

orgptr = lptr;				/* remember bol */
col = 0;
looking = TRUE;
while (looking)
	{
	ch = getchr();					/* get character */
	if (ch == (unsigned char) EOF)	/* check end of file */
		{
		*orgptr = 'Q';				/* stomp 'Q' for auto mode term */
		orgptr++;
		*orgptr = eos;				/* add eos */
		looking = FALSE;			/* we are out of here */
		}
	else if (ch <= ' ')				/* use any control for cr */
		{
		if (ch == ' ')				/* echo cr for stacked command */
			putcr();
		*lptr = eos;				/* end the input line */
		looking = FALSE;			/* the line is finished */
		}
	else if ((ch == bs) || (ch == rub))	/* backspace */
		{
		if (col != 0)
			{
			col = col - 1;
			lptr = lptr - 1;
			putchr(bs);	 putchr(' ');	putchr(bs);
			}
		}
	else if ((ch == ctlu) || (ch == ctlx))	/* delete line */
		{
		while (col != 0)
			{
			col = col - 1;
			lptr = lptr - 1;
			putchr(bs);	 putchr(' ');	putchr(bs);
			}
		}
	else if (ch < ' ')				/* skip other control chars */
		{
		}
	else							/* retain everything else */
		{
		*lptr = upper(ch);			/* add char to line */
		col = col + 1;				/* next column */
		lptr = lptr + 1;			/* next line location */
		}
	}
}


int	skpsep(char **lptr)		/* skip a separator (and spaces) */
{
skpspc(lptr);
if (issep(lptr))
	*lptr = *lptr + 1;
skpspc(lptr);
}


int	getnum(char **lptr)		/* get a number (decimal) */
{
int		num, sign=0;
skpsep(lptr);
num = 0;
if (**lptr == '-')
{
	(*lptr)++;
	sign = 1;
}
while ((**lptr >= '0') && (**lptr <= '9'))
	{
	num = (num * 10) + **lptr - '0';
	*lptr = *lptr + 1;
	}
if (sign) num = 0 - num;
return(num);
}


long  getlng(char **lptr)			/* get a number (decimal) */
{
long		num;
skpsep(lptr);
num = 0;
while ((**lptr >= '0') && (**lptr <= '9'))
	{
	num = (num * 10) + **lptr - '0';
	*lptr = *lptr + 1;
	}
return(num);
}


long	getlhx(char **lptr)		/* get a long (hex) */
{
long	num;
int		idx;
char	ch;

skpsep(lptr);
num = 0;
for (idx=1;idx<=8;idx++)		/* force 8 hex digs */
	{
	num = num << 4;
	ch = **lptr;
	if	((ch >= '0') && (ch <= '9'))
		{
		num = num | (ch - '0');
		*lptr = *lptr + 1;
		}
	else if	((ch >= 'A') && (ch <= 'F'))
		{
		num = num | (ch - 'A' + 10);
		*lptr = *lptr + 1;
		}
	}
return(num);
}


int	getwrd(char **lptr)		/* get a word (hex) */
{
int		looking,digit,num;
char		ch;
skpsep(lptr);
num = 0;
looking = TRUE;
while (looking)
	{
	ch = **lptr;
	if	((ch >= '0') && (ch <= '9'))
		digit = ch - '0';
	else if	((ch >= 'A') && (ch <= 'F'))
		digit = ch - 'A' + 10;
	else
		looking = FALSE;
	if (looking)
		{
		num = (num << 4) | digit;
		*lptr = *lptr + 1;
		}
	}
return(num);
}


int	getadr(char **lptr)		/* get address */
{
int	addr;

addr = getwrd(lptr);
return(addr);
}


getbyt(char **lptr)			/* get a byte (hex) */
{
int		digfnd,digit,num;
char		ch;
skpsep(lptr);
num = 0;
ch = **lptr;
digfnd = TRUE;
if	((ch >= '0') && (ch <= '9'))
	digit = ch - '0';
else if	((ch >= 'A') && (ch <= 'F'))
	digit = ch - 'A' + 10;
else
	digfnd = FALSE;
if (digfnd)
	{
	*lptr = *lptr + 1;
	ch = **lptr;
	num = (num << 4) | digit;
	if	((ch >= '0') && (ch <= '9'))
		digit = ch - '0';
	else if	((ch >= 'A') && (ch <= 'F'))
		digit = ch - 'A' + 10;
	else
		digfnd = FALSE;
	if (digfnd)
		{
		*lptr = *lptr + 1;
		num = (num << 4) | digit;
		}
	}
return(num);
}


putone(char num)			/* output a one char and a space */
{
puthex((num >> 4) & 0x0F);
puthex(num        & 0x0F);
putspc(1);
}


/* testbed global variable definitions */

struct dou {
		double tmp3;
	   };

struct lng {
		long tmp1;
		long tmp2;
	    };

typedef union double_or_long{
			struct dou town;
			struct lng gown;
			} number;

union u { int ui[2]; float uf[2]; } u1;
union du { int in1[2]; double db1; } du1 , du2;

char	fpbuf[80];		/* floating point buffer */
float	freg[32];		/* sp registers */
double  dreg[32];		/* dp registers */

int		mode;			/* sp/dp mode variable */
int		idx,i;
long	lngnum;			/* long number */
int		intnum;			/* int number */
char	*bytptr;
long	*lngptr;

float   fftemp,f1,f2,f3,f4;     /* single precision stack */
int		fchk,fkey;
double  ddtemp,d1,d2,d3,d4;     /* double precision stack */
int		dchk,dkey;
int		status;


/* testbed routines */


puthlp()					/* help OPTIONS */
{
putln(
  "F/DPAC     CMD   ACTIONS               F/DPAC    CMD   ACTIONS");
putln(
  "---------  ----  ------------------    --------  ----  ------------------");
putln(
  "f/dascbin  fpn   X = fpn                         Q     QUIT 80960 TestBed");
putln(
  "           Hhex  X = hex                         B     Batch Test Mode");
putln(
  "           M     toggle SP/DP                    =     Batch compare X & Y");
putln(
  "f/dpadd    +     X = Y + X                       ?     X = compare X & Y");
putln(
  "f/dpsub    -     X = Y - X             f/dpsub   _     X = X - Y");
putln(
  "f/dpmul    *     X = Y * X             f/dpdiv   /     X = Y / X");
putln(
  "f/dpdiv    \\     X = Y / X                       Ffpn  X = fpn");
putln(
  "                                       f/dpsqrt  FR    X = sqrt(X)");
putln(
  "f/daint    A     X = floor(X)          f/dpflt   Fint  X = int");
putln(
  "f/dint     I     X = int(X)            f/dpsin   FS    X = sin(X)");
putln(
  "f/dfix     Z     X = fix(X)            f/dpcos   FC    X = cos(X)");
putln(
  "f/dbinasc  C     X -> ASCII            f/dptan   FT    X = tan(X)");
putln(
  "sptodp     D     X = dp(X)             f/dpatn   FA    X = atn(X)");
putln(
  "dptosp     S     X = sp(X)             f/dpexp   FE    X = X to e pow");
putln(
  "           R     roll X>Y>Z>T>X        f/dpxtoi  F^int X = X to int pow");
putln(
  "           Gint  X = f/dreg[int]       f/dpln    FN    X = log e  (X)");
putln(
  "           Sint  f/dreg[int] = X       f/dllog   FL    X = log 10 (X)");
}


int	putspn(float spn)		/* output spn as hex bytes */
{
char	*spnptr;
spnptr = (char *) &spn;	spnptr = spnptr + 3;

putone(*spnptr);	spnptr = spnptr--;
putone(*spnptr);	spnptr = spnptr--;
putone(*spnptr);	spnptr = spnptr--;
putone(*spnptr);
}


int	putdpn(double dpn)		/* output dpn as hex bytes */
{
char	*dpnptr;
dpnptr = (char *) &dpn;	dpnptr = dpnptr + 7;

putone(*dpnptr);	dpnptr = dpnptr--;
putone(*dpnptr);	dpnptr = dpnptr--;
putone(*dpnptr);	dpnptr = dpnptr--;
putone(*dpnptr);	dpnptr = dpnptr--;
putone(*dpnptr);	dpnptr = dpnptr--;
putone(*dpnptr);	dpnptr = dpnptr--;
putone(*dpnptr);	dpnptr = dpnptr--;
putone(*dpnptr);
}


ptfspn(float spn)			/* output spn */
{
u1.uf[0] = spn;
if (u1.ui[0] == -1)			/* Handle NaN */
	fftemp = 1.0;
else
	fftemp = spn;
if (fftemp < 0)				/* use absolute value */
	fftemp = -spn;
if ((fftemp == 0.0) || (fftemp >= 0.00000001))
	fbinasc(spn,&fpbuf, 16, 8);
else
	fbinasc(spn,&fpbuf, 16, 16);	/* force E notation */
putstg(&fpbuf);
}


ptfdpn(dpn)					/* output dpn */
double dpn;
{
du1.db1 = dpn;
if (du1.in1[1] == -1)		/* Handle NaN */
	ddtemp = 1.0;
else
	ddtemp = dpn;
if (ddtemp < 0)				/* use absolute value */
	ddtemp = -dpn;
if ((ddtemp == 0.0) || (ddtemp >= 0.000000000000001))
	dbinasc(dpn,&fpbuf, 24, 16);
else
	dbinasc(dpn,&fpbuf, 24, 24);	/* force E notation */
putstg(&fpbuf);
}


int	xstkfn()			/* exchange spn in TOS/NOS */
{
float	ftmp;
ftmp = f1;
f1 = f2;
f2 = ftmp;
return(0);
}


int	xstkdn()			/* exchange dpn in TOS/NOS */
{
double	dtmp;
dtmp = d1;
d1 = d2;
d2 = dtmp;
return(0);
}


int	pshspn(float spn)		/* push spn */
{
f4 = f3;
f3 = f2;
f2 = f1;
f1 = spn;
}


float popspn()				/* pop spn */
{
float	ftmp;
ftmp = f1;
f1 = f2;
f2 = f3;
f3 = f4;
return(ftmp);
}


pshdpn(dpn)
double dpn;
{
d4 = d3;
d3 = d2;
d2 = d1;
d1 = dpn;
}


double popdpn()			/* pop dpn */
{
double	dtmp;
dtmp = d1;
d1 = d2;
d2 = d3;
d3 = d4;
return(dtmp);
}


dpysts()
{
if (mode==0)
	{
	putstg(" X: ");	 putdpn(d1);  ptfdpn(d1);  putcr();
	putstg(" Y: ");  putdpn(d2);  ptfdpn(d2);  putcr();
	putstg(" Z: ");  putdpn(d3);  ptfdpn(d3);  putcr();
	putstg(" T: ");  putdpn(d4);  ptfdpn(d4);  putcr();
	}
else
	{
	putstg("                                         X: ");	
	putspn(f1);  ptfspn(f1);  putcr();
	putstg("                                     	 Y: ");	
	putspn(f2);  ptfspn(f2);  putcr();
	putstg("                                         Z: ");
	putspn(f3);  ptfspn(f3);  putcr();
	putstg("                                         T: ");
	putspn(f4);  ptfspn(f4);  putcr();
	}
}


dpyalt()				/* display alternate mode */
{
mode = 1 - mode;
dpysts();
mode = 1 - mode;
}


main()					/* 80960 FPAC/DPAC TestBed */
{
char	inchr, *lnptr;
char	line[80],cmd[80];
char    tstopn[80], opch[80];
int     i;
unsigned int i1, i2, i3, i4;
int 	scanning,dpyflg,autofle;

ifp = stdin;			/* start using keyboard */
putln("80960 FPAC/DPAC TestBed");
putcr();

mode = 0;				/* start in dp mode */
autofle = 0;			/* no auto file yet */
scanning = TRUE;
while (scanning)
	{
	dpyflg = TRUE;
	putstg("Enter operation> ");
	getiln(line);
	lnptr = &line[0];

	getstg(&lnptr, &cmd[0]);
	if (cmd[0] == 'Q')					/* Quit DPTEST */
		{
		if (ifp != stdin)				/* auto mode term */
			{
			putcr();
    		putstg("Automatic Testing Completed: ");
			putln(tstopn);
			putcr();
			ifp = stdin;				/* shift to manual */
			mode = 0;					/* shift to dp mode */
			}
		else							/* user term */
			scanning = FALSE;
		}
    else if ( cmd[0] == '=' )               /* compare the answer */
			 {
             if ( mode == 0 )
                {
                du1.db1 = d1;      
                du2.db1 = d2;
				if ((du1.in1[0] != du2.in1[0]) || (du1.in1[1] != du2.in1[1]) )
					{
					putstg("expected ");
					ptfdpn(d1);
					putstg(" got ");
					ptfdpn(d2);
                    du1.db1 = d1;      
                    du2.db1 = d2;
					i1 = du1.in1[1]&0x7fffffff; i2 = du2.in1[1]&0x7fffffff;
					i3 = du1.in1[0]; i4 = du2.in1[0];
					if ( ((i1|i3) == 0) || (i1 >= 0x7ff00000) ||
				 	    ((i2|i4) == 0) || (i2 >= 0x7ff00000) )
						scanning = FALSE;
					i1 = du1.in1[1]; i2 = du2.in1[1];
				    if (i3 < i4)
					    if (++i3 == 0) i1++;
				    if (i3 < i4)
					    if (++i3 == 0) i1++;
				    if (i4 < i3)
					    if (++i4 == 0) i2++;
				    if (i4 < i3)
					    if (++i4 == 0) i2++;
					if ( (i1 != i2) || (i3 != i4) )
						scanning = FALSE;
					if (scanning == FALSE)
					   putstg(" TEST FAILED!");
					else
					   putstg(" DUE TO ROUNDING");
					putcr();
					}
              }
            else
               {
						u1.uf[0] = f1;
						u1.uf[1] = f2;
						if ( u1.ui[0] != u1.ui[1])
							{
							putstg("expected ");
							ptfspn(f1);
							putstg(" got ");
							ptfspn(f2);
						    u1.uf[0] = f1;
						    u1.uf[1] = f2;
							i1 = u1.ui[0]&0x7fffffff; i2 = u1.ui[1]&0x7fffffff;
							if ( (i1 == 0) || (i1 >= 0x7f800000) ||
							    (i2 == 0) || (i2 >= 0x7f800000) )
						        scanning = FALSE;
							i1 = u1.ui[0]; i2 = u1.ui[1];
				    		if (i1 < i2) i1++;
				    		if (i1 < i2) i1++;
				    		if (i2 < i1) i2++;
				    		if (i2 < i1) i2++;
							if (i1 != i2) 
						        scanning = FALSE;
							if (scanning == FALSE)
							    putstg(" TEST FAILED!");
							else
							    putstg(" DUE TO ROUNDING");
							putcr();
							}
						}
                }
	else if (cmd[0] == 'M')		/* Change dp/sp MODE */
		{
		mode = 1 - mode;
		}
	else if ( cmd[0] == 'R' )		/* Roll Stack */
		{
		if ( mode == 0 )
			{
			pshdpn(d4);
			}
		else
			{
			pshspn(f4);
			}
		}
	else if (cmd[0] == 'A')	   		/* faint function ("A")*/
		{
		if ( mode == 0 )
			{
			ddtemp = popdpn();
			pshdpn(daint(ddtemp));
			}
		else
			{
			fftemp = popspn();
			pshspn(faint(fftemp));
			}
		}
	else if (cmd[0] == 'I')		/* INT REQUEST */
		{
		if ( mode == 0 )
			{
			ddtemp = dpint(d1);	
			pshdpn(ddtemp);
			}
		else
			{
			pshspn(fpint(f1));
			}
		}		
	else if (cmd[0] == 'Z')		/* FIX REQUEST */
		{
		if ( mode == 0 )
			{
			ddtemp = dpfix(d1);
			pshdpn(ddtemp);
			}
		else
			{
			pshspn(fpfix(f1));
			}
		}		
	else if ( cmd[0] == 'D')	/* convert single to double */
		{	
		pshdpn(sptodp(f1));
		dpyalt();
		}		
	else if (cmd[0] == 'X')		/* exchange top two stack values */
		{
		if ( mode == 0 )
			xstkdn();
		else
			xstkfn();
		}		
	else if (cmd[0] == '?')		/* compare top two stack num */
		{
		if ( mode == 0 )
			{
			i = dpcmp(d2,d1);
			ddtemp = i;
			pshdpn(ddtemp);
			}
		else
			{
			i = fpcmp(f2,f1);
			fftemp = i;
			pshspn(fftemp);
			}
		}		
	else if ( cmd[0] == 'S' )		 /* move stack into reg[n] */
		{
		if (  (line[1] >= '0') && (line[1] <= '9') )	/* integer */
			{
			lnptr = &line[1];
			intnum = getlng(&lnptr);
			intnum = intnum & 0x1F;		/* force 0..31 */
			if ( mode == 0) 		
				{
				dreg[intnum] = d1;
				putstg("DREG[");
				putnum(intnum);
				putstg("] = ");
 				ptfdpn( dreg[intnum] );
				putcr();
				}
			else
				{
				freg[intnum] = f1;
				putstg("					");
				putstg("FREG[");
				putnum(intnum);
				putstg("] = ");
 				ptfspn( freg[intnum] );
				putcr();
				}
			}
		else  		 /* convert double to single */
			{
			pshspn(dptosp(d1));
			dpyalt();
			}
		}	
	else if ( cmd[0] == 'G' )	/* push reg[n] onto stack */
		{
		if (  (line[1] >= '0') && (line[1] <= '9') )	/* integer */
			{
			lnptr = &line[1];
			intnum = getlng(&lnptr);
			intnum = intnum & 0x1F;		/* force 0..31 */
			if ( mode == 0) 		
				{
				pshdpn( dreg[intnum] );
				}
			else
				{
				pshspn( freg[intnum] );
				}
			}
		}
	else if (cmd[0] == 'C') 
		{
		if ( mode == 0 )
			{
			putstg("dbinasc(X) = ");
			ddtemp = popdpn();
			ptfdpn(ddtemp);
			putcr();	
			}
		else
			{
			putstg("					fbinasc(X) = ");
			fftemp = popspn();	
			ptfspn(fftemp);
			putcr();	
			}
		}
	else if ( cmd[0] == '+')		/* Addition */
		{
		 if ( mode == 0 )
			{
			ddtemp = dpadd(d2,d1);
			popdpn();
			popdpn();
			pshdpn(ddtemp);
			}
		else
			{	
			fftemp = fpadd(f2,f1);
			popspn();
			popspn();
			pshspn(fftemp);
			}	
		}
	else if (  cmd[0] == '_' )		/* Reverse Subtract */
		{
		 if ( mode == 0 )
			{
			ddtemp = dpsub(d2,d1);
			popdpn();
			popdpn();
			pshdpn(ddtemp);
			}
		else
			{
			fftemp = fpsub(f2,f1);
			popspn();
			popspn();
			pshspn(fftemp);
			}
		}
	else if (  cmd[0] == '*' )		/* Multiply */
		{
		 if ( mode == 0 )
			{
			ddtemp = dpmul(d2,d1);
			popdpn();
			popdpn();
			pshdpn(ddtemp);
			}
		else
			{
			fftemp = fpmul(f2,f1);
			popspn();
			popspn();
			pshspn(fftemp);
			}
		}
	else if (  cmd[0] == '/' )		/* Divide */
		{
		 if ( mode == 0 )
			{
			ddtemp = dpdiv(d2,d1);
			popdpn();
			popdpn();
			pshdpn(ddtemp);
			}
		else
			{
			fftemp = fpdiv(f2,f1);
			popspn();
			popspn();
			pshspn(fftemp);
			}
		}
	else if (  cmd[0] == '\\' )		/* Reverse Divide */
		{
		 if ( mode == 0 )
			{
			ddtemp = dpdiv(d1,d2);
			popdpn();
			popdpn();
			pshdpn(ddtemp);
			}
		else
			{
			fftemp = fpdiv(f1,f2);
			popspn();
			popspn();
			pshspn(fftemp);
			}
		}
	else if ( cmd[0] == 'F')		/* Functions */
		{
		if ( (line[1] >= '0') && (line[1] <= '9') )	/* integer */
			{
			lnptr = &line[1];
			lngnum = getlng(&lnptr);
			if ( mode == 0) 		
				{
				intnum = lngnum;
				pshdpn(dpflt(intnum));
				}
			else
				{
				intnum = lngnum;
				pshspn(fpflt(intnum));
				}
			}
		else if ( line[1] == '-') 	/* integer */
			{
			lnptr = &line[2];
			lngnum = getlng(&lnptr);
			if ( mode == 0) 		
				{
				intnum = lngnum;
				pshdpn(dpflt(-intnum));
				}
			else
				{
				intnum = lngnum;
				pshspn(fpflt(-intnum));
				}
			}
		else if ( cmd[1] == '^' )		/* x to i */
			{
			lnptr = &line[2];
			intnum = getnum(&lnptr);
			if ( mode == 0  )
				{
				ddtemp = popdpn();
				ddtemp = dpxtoi(ddtemp,intnum);
				pshdpn(ddtemp);
				}
			else
				{
				fftemp = popspn();
				fftemp = fpxtoi(fftemp,intnum);
				pshspn(fftemp);
				}
			}
		else if ( cmd[1] == 'E')		/* "E"?  (FE - E to power specified)*/
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dpexp(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fpexp(fftemp));
				}
			}	
		else if ( cmd[1] == 'N')
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dpln(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fpln(fftemp));
				}
			}	
		else if ( cmd[1] ==  'R')
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dpsqrt(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fpsqrt(fftemp));
				}
			}	
		else if ( cmd[1] == 'C')
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dpcos(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fpcos(fftemp));
				}
			}	
		else if ( cmd[1] == 'S')
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dpsin(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fpsin(fftemp));
				}
			}	
		else if (cmd[1] == 'T')
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dptan(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fptan(fftemp));
				}
			}	
		else if ( cmd[1] == 'A')
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dpatn(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fpatn(fftemp));
				}
			}	
		else if ( cmd[1] == 'L')
			{
			if ( mode == 0 )
				{
				ddtemp = popdpn();
				pshdpn(dplog(ddtemp));
				}
			else
				{
				fftemp = popspn();
				pshspn(fplog(fftemp));
				}
			}	
		}	
	else if (   (cmd[0] >= '0') && (cmd[0] <= '9') 	/* fpn */
 	 	     || (cmd[0] == '.') )
		{
		lnptr = &line[0];
		if (mode == 0)
			{
			ddtemp = dascbin(lnptr, &i);
			pshdpn(ddtemp);
			}
		else
			{
			fftemp = fascbin(lnptr, &i);
			pshspn(fftemp);
			}
		}
	else if (cmd[0] == '-') 	 
		{
		if ( cmd[1] != 0 )
			{
			lnptr = &line[0];
			if (mode == 0)
				{
				ddtemp = dascbin(lnptr, &i);
				pshdpn(ddtemp);
				}
			else
				{
				fftemp = fascbin(lnptr, &i);
				pshspn(fftemp);
				}
			}
		else
			{	
			 if ( mode == 0 )
				{
				ddtemp = dpsub(d2,d1);
				popdpn();
				popdpn();
				pshdpn(ddtemp);
				}
			else
				{
				fftemp = fpsub(f2,f1);
				popspn();
				popspn();
				pshspn(fftemp);
				}
			}
		}
	else if ( cmd[0] == 'H' )
		{
		lnptr = &line[1];
		if ( mode == 0 )
			{
			lngnum = getlhx(&lnptr);
			bytptr = (char *) &ddtemp;   bytptr = bytptr + 4;
			lngptr = (long *) bytptr;
			*lngptr = lngnum;
			lngnum = getlhx(&lnptr);
			lngptr = (long *) &ddtemp;
			*lngptr = lngnum;
			pshdpn(ddtemp);
			}
		else
			{
			lngnum = getlhx(&lnptr);
			lngptr = (long *) &fftemp;
			*lngptr = lngnum;
			pshspn(fftemp);
			}
		}
	else if ( cmd[0] == 'O' )		/* help OPTIONS */
		{
		puthlp();
		}
	else if ( cmd[0] == 'B' )		/* batch testing */
		{
		if (autofle == 0)
			{
			ofp = fopen("result.chk", "w");		/* open auto output file */
			autofle++;
			}
        ifp = stdin;
   	    putstg("Enter automatic test file? ");
		getiln(tstopn);
		if (tstopn[0] == eos)			/* no file specified */
			{
			putln("No test file specified...");
			}
		else
			{
    	    ifp = fopen(tstopn, "r");
			if ( ifp )
				{
				putcr();   putcr();   putcr();
	    		putstg("Automatic Testing Initiated: ");
				putln(tstopn);
				putcr();
				mode = 0;				/* start in dp mode */
				}
			else
				{
				ifp = stdin;
				putln("Can't find/open specified test file.");
				}
			}
		}
	else if ( cmd[0] == eos )
		{
		}
	else if ( cmd[0] == cr )
		{
		dpyflg = FALSE;
		}
	else if ( cmd[0] == lf )
		{
		dpyflg = FALSE;
		}
	else
		{
		dpyflg = FALSE;
		putchr(bell);
		putln("*** Invalid Command, Type O for help OPTIONS ***");
		}
	if (dpyflg)
		dpysts();
	}	/* end while */
}	/* end main */
