#ifndef h_dosh
#define h_dosh

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif
#ifndef __HIGHC__  /* Metaware C */
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

/* EXACT TYPES */
typedef char int8e_type;
typedef unsigned char uint8e_type;
typedef short int16e_type;
typedef unsigned short uint16e_type;
typedef int int32e_type;
typedef unsigned int uint32e_type;

/* CORRECT SIZE OR GREATER */
typedef char int8_type;
typedef unsigned char uint8_type;
typedef short int16_type;
typedef unsigned short uint16_type;
typedef int int32_type;
typedef unsigned int uint32_type;


#include <memory.h>
#ifndef bcmp
#   define bcmp(b1,b2,len)         memcmp(b1,b2,len)
#endif
#ifndef bcopy
#   define bcopy(src,dst,len)      memcpy(dst,src,len)
#endif
#ifndef bzero
#      define bzero(s,n)              memset(s,0,n)
#endif

#include <string.h>
#ifndef index
#   define index(s,c)              strchr(s,c)
#endif
#ifndef rindex
#   define rindex(s,c)             strrchr(s,c)
#endif

#ifndef FALSE
#   define TRUE 1
#   define FALSE 0
#endif

#include "gnudos.h"

#endif
