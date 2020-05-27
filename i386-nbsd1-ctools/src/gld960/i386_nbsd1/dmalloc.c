
#include <stdio.h>

extern char * malloc(),*realloc();

#define HEADER_MAGIC  0x12345678
#define TRAILER_MAGIC 0xfeedface

static struct heap_mem_node {                                  /* Element is at
								  Offset:  */
    int header_magic1;                                         /* 0 */
    struct heap_mem_node *next_heap_node,*previous_heap_node;  /* 4,8 */
    int size;                                                  /* 12 */
    int header_magic2;                                         /* 16 */
    /* char heap[size]; goes here. */                          /* 20 */
    int trailer;
}
*heap_root,*bottom_of_heap_list;

/* Given a struct heap_mem_node *P, and the SIZE of the heap, returns an lvalue
   for the trailer. */
#define ADDR_OF_TRAILER(P,SIZE)    ((unsigned char *) (((char *) P) + 20 + SIZE))

#if 0
#define THE_TRAILER(P,SIZE)        (*((int *) (((char *) P) + 20 + SIZE)))
#else
static unsigned long THE_TRAILER(P,SIZE)
    char *P;
    int SIZE;
{
    unsigned long rv;

    memcpy(&rv,ADDR_OF_TRAILER(P,SIZE),sizeof(unsigned long));
    return rv;
}

#endif


#define ERROR(X) { fprintf(stderr," HEAPDEBUGERROR: (%s:%d) %s",__FILE__,__LINE__,X);fflush(stderr);abort(); }

static void check_heap()
{
    struct heap_mem_node *p;

    for (p=heap_root;p;p = p->next_heap_node) {
	if (p->header_magic1 != HEADER_MAGIC ||
	    p->header_magic2 != HEADER_MAGIC)
		ERROR("Header was stomped on\n");

	if (THE_TRAILER(p,p->size) != TRAILER_MAGIC)
		ERROR("Trailer was stomped on\n");
    }
}

char * pauls_malloc(size)
    int size;
{
    struct heap_mem_node *p;
    unsigned long tmp = TRAILER_MAGIC;

    if (size <= 0) {
	ERROR("Attempt to malloc 0 bytes.\n");
	check_heap();
	return NULL;
    }

    p = (struct heap_mem_node *)
	    malloc(sizeof(struct heap_mem_node) + size);
    p->header_magic1 = HEADER_MAGIC;
    p->header_magic2 = HEADER_MAGIC;
    p->next_heap_node = 0;
    p->previous_heap_node = bottom_of_heap_list;
    if (bottom_of_heap_list)
	    bottom_of_heap_list->next_heap_node = p;
    if (!heap_root)
	    heap_root = p;
    bottom_of_heap_list = p;
    p->size = size;
    memcpy(ADDR_OF_TRAILER(p,size),&tmp,sizeof(tmp));
    check_heap();
    return ((char *)p)+20;
}

void pauls_free(ptr)
    char *ptr;
{
    struct heap_mem_node *p,*q;
    int i,n;

    if (ptr == NULL) {
	ERROR("Attempt to free address 0\n");
	return;
    }
    p = (struct heap_mem_node *) (ptr-20);
    if (p->header_magic1 != HEADER_MAGIC ||
	p->header_magic2 != HEADER_MAGIC) {
	ERROR("Attempt to free non-malloc'd memory\n");
	return;
    }
    check_heap();

    if (q = p->next_heap_node)
	q->previous_heap_node = p->previous_heap_node;
    if (q = p->previous_heap_node)
	    q->next_heap_node = p->next_heap_node;
    if (p == heap_root) {
	if (!(heap_root = heap_root->next_heap_node))
		bottom_of_heap_list = heap_root = 0;
	else
		heap_root->previous_heap_node = 0;
    }
    if (bottom_of_heap_list == p)
	    if (bottom_of_heap_list = p->previous_heap_node)
		    bottom_of_heap_list->next_heap_node = 0;
    for (i=0,n=p->size;i < sizeof(struct heap_mem_node) + n;i++)
	    ((char *) p)[i] = 0xff;
    check_heap();
    free(p);
}

char * pauls_realloc(ptr,size)
    char *ptr;
    int size;
{
    struct heap_mem_node *p,*q;
    char * rv;

    if (ptr == NULL) {
	ERROR("Attempt to realloc address 0\n");
	return NULL;
    }
    p = (struct heap_mem_node *) (ptr-20);
    if (p->header_magic1 != HEADER_MAGIC ||
	p->header_magic2 != HEADER_MAGIC) {
	ERROR("Attempt to realloc non-malloc'd memory\n");
	abort();
    }

    rv = pauls_malloc(size);

    memcpy(rv,ptr,p->size < size ? p->size : size);
    pauls_free(ptr);
    return rv;
}
