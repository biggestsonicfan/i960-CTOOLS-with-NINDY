#ifndef __TYPES_H__
#define __TYPES_H__

/*
 * These pollute the name space somewhat but are there 
 * for ic960 compatibility.
 */
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

typedef short		dev_t;
typedef long		off_t;

#ifndef _mode_t
#define _mode_t
typedef unsigned long mode_t;
#endif

#ifndef _size_t
#define _size_t
typedef unsigned size_t;       /* result of sizeof operator */
#endif

#endif /* __TYPES_H__ */
