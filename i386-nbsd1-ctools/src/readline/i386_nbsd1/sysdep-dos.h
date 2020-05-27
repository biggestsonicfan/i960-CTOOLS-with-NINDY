/* System-dependent stuff, for DOS systems */

#define __MSDOS__

#ifdef __GNUC__
#define alloca __builtin_alloca
#endif

#include "gnudos.h"


#if defined (IMSTG) && defined (WIN95)
#   define closedir _closedir
#   define opendir  _opendir
#   define readdir  _readdir
#   define DIR      struct _dir

    typedef struct
    {
        char *d_name;
    } dirent;

    struct _dir;  /* Incomplete type, defined in win95.c */

    extern int    _closedir(DIR *);
    extern DIR    *_opendir(char *);
    extern dirent *_readdir(DIR *);
#endif

#if defined (IMSTG) && defined (__HIGHC__)
#   include <dirent.h>
#   define closedir _closedir
#   define opendir  _opendir
#   define readdir  _readdir
    typedef struct _dirent dirent;
#endif

