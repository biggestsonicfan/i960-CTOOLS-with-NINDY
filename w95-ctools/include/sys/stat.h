#ifndef __STAT_H__
#define __STAT_H__

/*
 *stat.h - file stat structure
 */

#include <__macros.h>

#ifndef _time_t
#define _time_t
typedef unsigned long time_t;
#endif

#define S_IFMT  0xE000 		/* mask */
#define	S_IFIFO	0x1000		/* fifo */
#define S_IFCHR 0x2000 		/* character device */
#define S_IFDIR 0x4000 		/* directory */
#define	S_IFBLK	0x6000		/* block special */
#define S_IFREG 0x8000 		/* regular file */

/* file access permission levels */
#define	S_ISUID	 0x800		/* set user id on execution */
#define	S_ISGID	 0x400		/* set group id on execution */
#define	S_ISVTX	 0x200		/* save swapped text even after use */
#define	S_IREAD	 0x100		/* read permission, owner */
#define	S_IWRITE 0x80		/* write permission, owner */
#define	S_IEXEC  0x40		/* execute/search permission, owner */
#define	S_ENFMT	 S_ISGID	/* record locking enforcement flag */
#define	S_IRWXU  0x1c0		/* read, write, execute: owner */
#define	S_IRUSR	 0x100		/* read permission: owner */
#define	S_IWUSR	 0x80		/* write permission: owner */
#define	S_IXUSR	 0x40		/* execute permission: owner */
#define	S_IRWXG	 0x38		/* read, write, execute: group */
#define	S_IRGRP	 0x20		/* read permission: group */
#define	S_IWGRP	 0x10		/* write permission: group */
#define	S_IXGRP	 0x8		/* execute permission: group */
#define	S_IRWXO	 0x7		/* read, write, execute: other */
#define	S_IROTH	 0x4		/* read permission: other */
#define	S_IWOTH	 0x2		/* write permission: other */
#define	S_IXOTH	 0x1		/* execute permission: other */

#pragma i960_align stat = 16
struct	stat {
  short			st_dev;
  unsigned short	st_ino;
  unsigned short 	st_mode;
  short  		st_nlink;
  unsigned short 	st_uid;
  unsigned short 	st_gid;
  short			st_rdev;
  long			st_size;
  time_t		st_atime;
  time_t		st_mtime;
  time_t		st_ctime;
};

__EXTERN
int (fstat)(int, struct stat *);
__EXTERN
int (stat)(__CONST char *, struct stat *);

#endif

