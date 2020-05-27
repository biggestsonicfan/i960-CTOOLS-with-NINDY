
extern char * malloc(),*realloc();

#define EXTRA_SIZE                 12
#define SIZE(PTR)                  (*((int *)(PTR)))
#define MEM_HASH_NODE(PTR)         (*(((struct mem_hash_node **)(PTR))+1))
#define MEM_NODE(PTR)              (*(((struct mem_node **)(PTR))+2))
#define REAL_MALLOC(PTR)           ((PTR)-EXTRA_SIZE)
#define FAKE_MALLOC(PTR)           ((PTR)+EXTRA_SIZE)

#define MEM_HASH_TABLE_SIZE 17

struct mem_node {
    char *address;
    int epoch;
    struct mem_node *previous_mem_node,*next_mem_node;
};

static struct mem_hash_node {
    int cumulative_size,current_size;
    char *file;
    int line;
    struct mem_hash_node *next_mem_hash_node;
    struct mem_node *top,*bottom;
} *mem_hash_table[MEM_HASH_TABLE_SIZE];

static int mem_hash(file,line)
    char *file;
    int line;
{
    line += *file;
    return (MEM_HASH_TABLE_SIZE-1) & line;
}

static struct mem_hash_node *lookup_mem_hash_node(file,line)
    char *file;
    int line;
{
    int hk = mem_hash(file,line);
    struct mem_hash_node **p = &(mem_hash_table[hk]);

    while (*p) {
	if ((*p)->line == line &&
	    !strcmp(file,(*p)->file))
		return (*p);
	p = &((*p)->next_mem_hash_node);
    }
    (*p) = (struct mem_hash_node *) malloc(sizeof(struct mem_hash_node));
    (*p)->current_size = (*p)->cumulative_size = 0;
    (*p)->file = file;
    (*p)->line = line;
    (*p)->next_mem_hash_node = 0;
    (*p)->top = 0;
    (*p)->bottom = 0;
    return (*p);
}

static int peakusage,max_usage;

add_to_peak(n)
    int n;
{
    peakusage += n;

    if (peakusage > max_usage) {
	max_usage = peakusage;
#if 0
	pauls_print_out_hash_table();
#endif
    }
}

static int gepoch;

char * pauls_malloc(size,file,line)
    int size;
    char *file;
    int line;
{
    char *rv = malloc(size+EXTRA_SIZE);
    struct mem_hash_node *p = lookup_mem_hash_node(file,line);
    struct mem_node *mn = (struct mem_node *) malloc(sizeof (struct mem_node));

    add_to_peak(size);
    p->current_size += size;
    p->cumulative_size += size;

    MEM_NODE(rv)       = mn;
    SIZE(rv)           = size;
    MEM_HASH_NODE(rv)  = p;

    mn->address = FAKE_MALLOC(rv);

    mn->next_mem_node = mn->previous_mem_node = 0;
    if (p->bottom) {
	mn->previous_mem_node = p->bottom;
	p->bottom->next_mem_node = mn;
	p->bottom = mn;
    }
    else
	    p->top = p->bottom = mn;

    mn->epoch = gepoch++;
    return FAKE_MALLOC(rv);
}

void pauls_free(ptr)
    char *ptr;
{
    char *rm                      = REAL_MALLOC(ptr);
    int ptrsize                   = SIZE(rm);
    struct mem_hash_node *old_ptr = MEM_HASH_NODE(rm);
    struct mem_node *p            = MEM_NODE(rm);
    int i;


    add_to_peak(- ptrsize);
    old_ptr->current_size -= ptrsize;

    if (p->previous_mem_node)
	    p->previous_mem_node->next_mem_node = p->next_mem_node;
    else
	    old_ptr->top = p->next_mem_node;
    if (p->next_mem_node)
	    p->next_mem_node->previous_mem_node = p->previous_mem_node;
    else {
	if (p->previous_mem_node)
		old_ptr->bottom = p->previous_mem_node;
	else {
	    old_ptr->bottom = old_ptr->top = 0;
	    if (old_ptr->current_size != 0)
		    abort();
	}
    }
    free(p);

    for (i=0;i < ptrsize+EXTRA_SIZE;i++)
	    rm[i] = 0xff;
    free(rm);
}

char * pauls_realloc(ptr,size,file,line)
    int size;
    char *ptr,*file;
    int line;
{
    int old_size = SIZE(REAL_MALLOC(ptr));
    char * rv = pauls_malloc(size,file,line);

    memcpy(rv,ptr,old_size < size ? old_size : size);
    pauls_free(ptr);
    return rv;
}

#include <stdio.h>

static write_entry(f,p)
    FILE *f;
    struct mem_hash_node *p;    
{
    int size = 0;
    struct mem_node *q = p->top;
    fprintf(f,"%s %d %d %d\n",p->file,p->line,p->cumulative_size,p->current_size);
    fprintf(f,"cum: %d, cur: %d\n",p->cumulative_size,p->current_size);
    for (;q;q = q->next_mem_node) {
	size += SIZE(REAL_MALLOC(q->address));
	fprintf(f,"epoch: %d size: %d addr: 0x%x \n",q->epoch,SIZE(REAL_MALLOC(q->address)),
		q->address);
    }
    if (p->current_size != size)
	    abort();
}

int pauls_print_out_hash_table()
{
    int i;

    FILE *f = fopen("mem","w");

    fprintf(f,"max mem usage: %d, current mem usage: %d\n",max_usage,peakusage);
    fprintf(f,"filename: lineno: cumulative_size: current_size:\n");
    for (i=0;i < MEM_HASH_TABLE_SIZE;i++) {
	struct mem_hash_node *p;
	for (p=mem_hash_table[i];p;p = p->next_mem_hash_node)
		write_entry(f,p);
    }
    fclose(f);
    return max_usage;
}

char * my_malloc(n)
    int n;
{
    return pauls_malloc(n,__FILE__,__LINE__);
}

char *pauls_strdup(s,file,line)
    char *s,*file;
    int line;
{
    char *r = pauls_malloc(strlen(s)+1,file,line);
    strcpy(r,s);
    return r;
}

void
browse_hash_lists(file,line)
    char *file;
    int line;
{
    int i;

    for (i=0;i < MEM_HASH_TABLE_SIZE;i++) {
	struct mem_hash_node *p = mem_hash_table[i];

	for (;p;p = p->next_mem_hash_node) {
	    if (p->line == line && !strcmp(file,p->file))
		    write_entry(stdout,p);
	}
    }
}
