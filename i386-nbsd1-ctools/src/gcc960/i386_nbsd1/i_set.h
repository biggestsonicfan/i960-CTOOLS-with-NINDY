typedef unsigned long * set;
typedef unsigned long set_array;

#define SET_SIZE(n) \
  ((n + HOST_BITS_PER_LONG - 1) / HOST_BITS_PER_LONG)

#define ALLOC_SETS(num, n) \
  ((set)xmalloc((num)*sizeof(unsigned long)*(n)))

#define SET_SET_ELT(s1,elt) \
  ((s1)[((unsigned)(elt))/HOST_BITS_PER_LONG] |= \
    (1 << (((unsigned)(elt))%HOST_BITS_PER_LONG)))

#define CLR_SET_ELT(s1,elt) \
  ((s1)[((unsigned)(elt))/HOST_BITS_PER_LONG] &= \
    ~(1 << (((unsigned)(elt))%HOST_BITS_PER_LONG)))

#define IN_SET(s1,elt) \
  (((s1)[((unsigned)(elt))/HOST_BITS_PER_LONG] & \
    (1 << (((unsigned)(elt))%HOST_BITS_PER_LONG))) != 0)

#define SET_SET_ALL(s1,n) \
  do { \
    int xx1; \
    for (xx1 = 0; xx1 < (n); xx1++) \
      (s1)[xx1] = -1; \
  } while (0)

#define COPY_SET(dst,src,n) \
  do { \
    register int xx1 = n; \
    while (--xx1 >= 0) \
      dst[xx1] = src[xx1]; \
  } while (0)

#define CLR_SET(dst,n) \
  do { \
    register int xx1 = n; \
    while (--xx1 >= 0) \
      dst[xx1] = 0; \
  } while (0)

#define FILL_SET(dst,n) \
  do { \
    register int xx1 = n; \
    while (--xx1 >= 0) \
      dst[xx1] = -1; \
  } while (0)

#define OR_SETS(dst,s1,s2,n) \
  do { \
    register int xx1 = n; \
    while (--xx1 >= 0) \
      dst[xx1] = s1[xx1] | s2[xx1]; \
  } while (0)

#define AND_SETS(dst,s1,s2,n) \
  do { \
    register int xx1 = n; \
    while (--xx1 >= 0) \
      dst[xx1] = s1[xx1] & s2[xx1]; \
  } while (0)

#define AND_COMPL_SETS(dst,s1,s2,n) \
  do { \
    register int xx1 = n; \
    while (--xx1 >= 0) \
      dst[xx1] = s1[xx1] & ~s2[xx1]; \
  } while (0)

#define EQUAL_SETS(s1,s2,n,equal) \
  do { \
    int xx1 = n; \
    equal = 1; \
    while (--xx1 >= 0) \
    { \
      if (s1[xx1] != s2[xx1]) \
      { \
        equal = 0; \
        break; \
      } \
    } \
  } while (0)

#define FORALL_SET_BEGIN(s1, n, var) \
  do { \
    int xx1; \
    int xx2; \
    for (xx1=0,xx2=(n); xx1 < xx2; xx1++) \
    { \
      int xx3 = 0; \
      unsigned long xx4 = (s1)[xx1]; \
      while (xx4 != 0) \
      { \
        if ((xx4 & 1) != 0) \
        { \
          var = xx1*HOST_BITS_PER_LONG + xx3; \
          {

#define FORALL_SET_END \
          } \
        } \
        xx4 >>= 1; \
        xx3 += 1; \
      } \
    } \
  } while (0)
