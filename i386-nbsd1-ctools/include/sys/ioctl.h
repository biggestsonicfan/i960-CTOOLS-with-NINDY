#ifndef _IOCTL_H__
#define _IOCTL_H__

#include <__macros.h>
/*******************************************************************************

This implementation of ioctl() does NOT follow any standard implementation.
It is specific to the GNU 960 compiler product.

*******************************************************************************/

/*

Ioctl() returns ERROR when it does not understand the request.

*/

#define ERROR        -1

/*

The following are not implemented but are retained because the implementation
may support a similar standard interface in the future:

*/

#define TCGETA	     0
#define TCSETAF	     1

/*

The following four requests to ioctl() apply only to the stdin input stream.

The following two requests to ioctl() are called in the following fashion.
Ioctl() sets flags according to the request and the passed new value, and
returns the previous value.  For example:

int old_value,new_value = 1;

old_value = ioctl(0, G960_CBREAK, &new_value);

Old_value above will be set to the previous value of the flag and will reset
the flag to the new_value (1) (enabling G960_CBREAK functionality).

*/

/*

G960_CBREAK will cause read()s on stdin to be non-buffered.  They will return
immediately after the number of bytes specified have been read.  (The default
behavior is to buffer).

*/
#define G960_CBREAK  2

/*

G960_ECHO causes read()s on stdin to not echo the characters according to the
value of *ptr.  If *ptr is non zero it will echo them, otherwise it will not
echo them.  (The default behavior is to echo them).

*/
#define G960_ECHO 3

/*

The following request options are retained for backward compatibility.

If ioctl() is called with these values, it ignores the third argument and
assigns the laser flag to TRUE.  To make the laser flag false, choose a value
that is not one of the request options (such as G960_NON_REQUEST below).

*/

#define LPTON        0x53 	/* chosen randomly to not interfere with */
#define LPTOFF       0x53	/* standard IOCTL definitions */


/*

This is not a valid request.  Upon receiving a non valid request, ioctl() will
return ERROR.

*/

#define G960_NON_REQUEST 0x55

__EXTERN int	(ioctl)(int,int,int*);

#endif
