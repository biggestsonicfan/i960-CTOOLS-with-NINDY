#ifndef __SETJMP_H__
#define __SETJMP_H__
/*
 * setjmp.h : define setjmp, longjmp
 */

#include <__macros.h>

/*
 * assume that every single local and global register
 * must be saved.
 *
 * ___SAVEREGS is the number of quads to save.
 *
 * Using the structure will guarantee quad-word alignment for the
 * jmp_buf type.
 */

#define ___SAVEREGS 8

#pragma i960_align __jmp_buf = 16
typedef struct __jmp_buf {
  long _q0;
  long _q1;
  long _q2;
  long _q3;
} jmp_buf[___SAVEREGS];

__EXTERN int  (setjmp)(jmp_buf);
__EXTERN void (longjmp)(jmp_buf, int);

#endif
