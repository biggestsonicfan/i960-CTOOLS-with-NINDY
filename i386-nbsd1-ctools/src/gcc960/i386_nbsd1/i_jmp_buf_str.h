#ifndef __SETJMPSTRH
#define __SETJMPSTRH

#include <setjmp.h>

typedef struct
{ jmp_buf jmp_buf;
} jmp_buf_struct;

extern jmp_buf_struct *xmalloc_handler;
extern jmp_buf_struct xmalloc_ret_zero;

#endif /* __SETJMPSTRH */

