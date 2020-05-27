#ifndef __STD_H__
#define __STD_H__

/*
 * std.h
 *
 * Adapted for the 960,19-Sep-89 [atw]
 *
 */

#include <stddef.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

__EXTERN void		(_cleanup)(__NO_PARMS);
__EXTERN int		(access)(__CONST char*,int);
__EXTERN unsigned	(alarm)(unsigned);

__EXTERN int		(bcmp)(__CONST void*,__CONST void*,long);
__EXTERN char*		(bcopy)(char*,char*,int);
__EXTERN void*		(brk)(void*);
__EXTERN void 		(brk2)(__NO_PARMS);
__EXTERN void		(bzero)(char*,int);

__EXTERN int		(callx)(int,int);
__EXTERN void		(cfree)(void*,unsigned,unsigned);
__EXTERN int		(chdir)(__CONST char*);
__EXTERN int		(chmod)(__CONST char*,int);
__EXTERN int		(chown)(__CONST char*,int,int);
__EXTERN int		(chroot)(__CONST char*);
__EXTERN int		(close)(int);
__EXTERN int		(creat)(__CONST char*,int);
__EXTERN char*		(crypt)(char*,__CONST char*);
__EXTERN char*		(ctermid)(char*);
__EXTERN char*		(cuserid)(char* );

__EXTERN int		(dup)(int);
__EXTERN int		(dup2)(int,int);

__EXTERN void		(encrypt)(char*,int);
__EXTERN void		(endgrent)(__NO_PARMS);
__EXTERN void		(endpwent)(__NO_PARMS);
__EXTERN int		(execl)(__CONST char*,__CONST char*,...);
__EXTERN int		(execle)(__CONST char*,__CONST char*,...);
__EXTERN int 		(execlp)(__CONST char*,__CONST char*,...);
__EXTERN int		(execn)(__CONST char*);
__EXTERN int		(execv)(__CONST char*,__CONST char**);
__EXTERN int		(execve)(__CONST char*,__CONST char**,char**);
__EXTERN int		(execvp)(__CONST char*,__CONST char**);
__EXTERN __VOLATILE_FUNC void (_exit)(int);

__EXTERN int		(ffs)(int);
__EXTERN int		(fork)(__NO_PARMS);

__EXTERN char*		(getcwd)( char*,int);
__EXTERN char		(getegid)(__NO_PARMS);
__EXTERN short		(geteuid)(__NO_PARMS);
__EXTERN char		(getgid)(__NO_PARMS);
__EXTERN int		(gethostname)( char*,int);
__EXTERN char*		(getlogin)(__NO_PARMS);
__EXTERN char*		(getpass)(__CONST char*);
__EXTERN int		(getpid)(__NO_PARMS);
__EXTERN int 		(getppid)(__NO_PARMS);
__EXTERN char*		(gets)(char*);
__EXTERN short		(getuid)(__NO_PARMS);
__EXTERN char*		(getwd)(char*);

__EXTERN char*		(index)(__CONST char*,int);
__EXTERN int		(isatty)(int);

__EXTERN int		(kill)(int,int);

__EXTERN int		(len)(__CONST char*);
__EXTERN int		(link)(__CONST char*,__CONST char*);
__EXTERN int		(lock)(__CONST char*);
__EXTERN long		(lrand)(__NO_PARMS);
__EXTERN long		(seed)(long);
__EXTERN char*		(lsearch)(__CONST char *, char *, unsigned *,
                                  unsigned, int(*)(__CONST void *, __CONST void *));
__EXTERN char*		(lfind)(__CONST char *, __CONST char *, unsigned *,
                                unsigned, int(*)(__CONST void *, __CONST void *));
__EXTERN long		(lseek)(int,long,int);

__EXTERN int		(mknod)(__CONST char*,int,int);
__EXTERN char*		(mktemp)(char*);

__EXTERN int		(mount)(__CONST char*,__CONST char*,int);

__EXTERN int		(open)(__CONST char*,int,...);

__EXTERN int		(pause)(__NO_PARMS);
__EXTERN void		(perror)(__CONST char*);
__EXTERN int		(pipe)(int*);
__EXTERN int		(printf)(__CONST char*,...);
__EXTERN void		(printk)(__CONST char*,...);
__EXTERN void		(prints)(__CONST char*,...);
__EXTERN long		(ptrace)(int,int,long,long);
__EXTERN int		(puts)(__CONST char*);
__EXTERN int		(putenv)(char*);

__EXTERN int		(read)(int,void*,int);
__EXTERN int		(rename)(__CONST char*,__CONST char*);
__EXTERN char*		(rindex)(__CONST char*,int);

__EXTERN void*		(sbrk)(int);
__EXTERN int		(scanf)(__CONST char*,...);
__EXTERN int		(setgid)(int);
__EXTERN int		(setgrent)(__NO_PARMS);
__EXTERN int		(sethostname)( char*,int );
__EXTERN void		(setkey)(__CONST char*);
__EXTERN int		(setpwent)(__NO_PARMS);
__EXTERN int		(setuid)(int);
__EXTERN unsigned	(sleep)(unsigned);
__EXTERN int		(sprintf)(char*,__CONST char*,...);
__EXTERN int		(sscanf)(__CONST char*,__CONST char*,...);
__EXTERN void		(std_err)(__CONST char*);
__EXTERN int		(stime)(long*);
__EXTERN void		(swab)(void*,void*,int);
__EXTERN int		(sync)(__NO_PARMS);
__EXTERN void		(sys_abort)(__NO_PARMS);
__EXTERN void		(sys_exec)(int,void*,int);
__EXTERN void		(sys_fork)(int,int,int,unsigned short);
__EXTERN void		(sys_fresh)(int,void*,unsigned short,unsigned short*, unsigned short*); 
__EXTERN void		(sys_getsp)(int,long*);
__EXTERN void		(sys_kill)(int,int);
__EXTERN void		(sys_newmap)(int,void*);
__EXTERN void		(sys_sig)(int,int,int(*)());
__EXTERN void		(sys_times)(int,long*);
__EXTERN int		(sys_trace)(int,int,long,long*);
__EXTERN void		(sys_xit)(int,int,unsigned short*,unsigned short*);

__EXTERN void		(tell_fs)(int,int,int,int);
__EXTERN int		(tgetent)(char*,__CONST char*);
__EXTERN int		(tgetflag)(__CONST char*);
__EXTERN int		(tgetnum)(__CONST char*);
__EXTERN char*		(tgetstr)(__CONST char*,char**);
__EXTERN char*		(tgoto)(__CONST char*,int,int);
__EXTERN char*		(tmpname)(char*);
__EXTERN char*		(tempnam)(char*,char*);

__EXTERN int		(tputs)(__CONST char*,int,int(*)());
__EXTERN char*		(ttyname)(int);

__EXTERN int		(umask)(int);
__EXTERN int		(umount)(__CONST char*);
__EXTERN int		(unlink)(__CONST char*);
__EXTERN void		(unlock)(__CONST char*);
__EXTERN int		(utime)(__CONST char*,long*);

__EXTERN int		(wait)(int*);
__EXTERN int		(write)(int,__CONST void*,int);

__EXTERN int		(_map_length)(int, __CONST void *, size_t);
#endif	/* __STD_H__  */
