#include "xm-i386.h"
#include <stdio.h>

/* Inhibit cccp.c's definition of putenv.  */
#define HAVE_PUTENV

/* Use semicolons to separate elements of a path.  */
#define PATH_SEPARATOR ';'

/* Suffix for executable file names.  */
#define EXECUTABLE_SUFFIX ".exe"

#define bcopy(src,dst,len) memcpy ((dst),(src),(len))
#define bzero(dst,len) memset ((dst),0,(len))
#define bcmp(left,right,len) memcmp ((left),(right),(len))

#define rindex strrchr
#define index strchr

#define sbrk(x) (0x1000000)		/* no sbrk on dos just used for printing a report */

/* dos has a fixed size stack so don't use builtin alloca() */
#define USE_C_ALLOCA
