#ifndef hosts_sparc_H
#define STDC_HEADERS
#define FILE_OFFSET_IS_CHAR_INDEX
#if defined(__STDC__) && __GNUC__ >= 2
#define abort __hide_abort
#define exit __hide_exit
#endif
#if defined(IMSTG)
#include "std-host.h"
#else
#include "hosts/std-host.h"
#endif
#include <alloca.h>
#include <memory.h>
#undef exit
#undef abort
#define hosts_sparc_H
#endif
