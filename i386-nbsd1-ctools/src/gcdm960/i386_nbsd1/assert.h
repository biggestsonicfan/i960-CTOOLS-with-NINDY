
extern void  (*db_assert_fail)();
# ifndef NDEBUG
# define _assert(expr) if (!(expr)) db_assert_fail("assertion failure at %s:%d", __FILE__, __LINE__); else
# define assert(expr)	_assert(expr)
# else
# define _assert(expr)
# define assert(expr)
# endif
