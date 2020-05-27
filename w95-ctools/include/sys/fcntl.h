#ifndef __FCNTL_H__
#define __FCNTL_H__

/* 
 * "posix" file control defines
 */

#include <__macros.h>

#define  O_RDONLY  0x0000  /* read only */
#define  O_WRONLY  0x0001  /* write only */
#define  O_RDWR    0x0002  /* read/write, update mode */
#define  O_APPEND  0x0008  /* append mode */
#define  O_CREAT   0x0100  /* create and open file */
#define  O_TRUNC   0x0200  /* length is truncated to 0 */
#define  O_EXCL    0x0400  /* used with O_CREAT to produce an error if file 
			   already exists */
#define  O_TEXT    0x4000  /* ascii mode, <cr><lf> xlated */
#define  O_BINARY  0x8000  /* mode is binary (no translation) */

#endif


