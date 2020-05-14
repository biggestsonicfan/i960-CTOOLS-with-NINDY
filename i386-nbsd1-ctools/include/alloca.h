#ifndef __ALLOCA_H__
#define __ALLOCA_H__

#include <__macros.h>

#if ! defined(__NO_BUILTIN)
#define alloca(x) __builtin_alloca(x)
#else
#error "alloca is supported only as a builtin"
#endif

#endif
