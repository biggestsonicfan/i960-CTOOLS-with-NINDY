#ifndef __ERRNO_H__
#define __ERRNO_H__

#include <__macros.h>
#include <reent.h>

#define OK                 0
#define EPERM              1
#define ENOENT             2
#define ESRCH              3
#define EINTR              4
#define EIO                5
#define ENXIO              6
#define E2BIG              7
#define ENOEXEC            8
#define EBADF              9
#define ECHILD            10
#define EAGAIN            11
#define ENOMEM            12
#define EACCES            13
#define EFAULT            14
#define ENOTBLK           15
#define EBUSY             16
#define EEXIST            17
#define EXDEV             18
#define ENODEV            19
#define ENOTDIR           20
#define EISDIR            21
#define EINVAL            22
#define ENFILE            23
#define EMFILE            24
#define ENOTTY            25
#define ETXTBSY           26
#define EFBIG             27
#define ENOSPC            28
#define ESPIPE            29
#define EROFS             30
#define EMLINK            31
#define EPIPE             32
#define EDOM              __EDOM
#define ERANGE            __ERANGE
#define EDEADLOCK         36      /* locking violation */
#define ESIGNAL           37      /* Bad signal vector */
#define EFREE             38      /* Bad free pointer */
#define _NUM_ERR_NUMS     39      /* for strerror() */

#define E_LOCKED          101
#define E_BAD_CALL        102
#define E_LONG_STRING     103

#define	errno __errno

#endif /* _ERRNO_H */
