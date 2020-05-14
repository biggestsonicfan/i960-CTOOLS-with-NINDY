#ifndef __SEARCH_H__
#define __SEARCH_H__

/*
 * search.h - searching and sorting function prototypes
 * Copyright (c) 1988, 89 Intel Corporation, ALL RIGHTS RESERVED.
 */
#include <__macros.h>

__EXTERN
char * (lfind)(__CONST char *, __CONST char *, unsigned *, unsigned, 
               int (*)(__CONST void *, __CONST void *));
__EXTERN
char * (lsearch)(__CONST char *, char *, unsigned *, unsigned, 
                 int (*)(__CONST void *, __CONST void *));

#endif
