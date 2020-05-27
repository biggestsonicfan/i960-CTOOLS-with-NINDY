
char *pauls_malloc(size)
    int size;
{
    char *r = malloc(size + 4);
    int *i = (int *) r;

    *i = size;
    memset(r+4,-1,size);
    return r+4;
}

char *pauls_realloc(ptr,size)
    char *ptr;
    int size;
{
    int old_size = *((int*)(ptr-4));

    if (old_size > size) {
	char *r = realloc(ptr-4,size+4);
	int *i = (int*)r;

	*i = size;
	return r+4;
    }
    else {
	char *r = realloc(ptr-4,size+4);
	int *i = (int*)r;

	*i = size;
	memset(r+4+old_size,-1,size-old_size);
	return r+4;
    }
}

void pauls_free(ptr)
    char *ptr;
{
    int size = *((int *)(ptr-4));

    memset(ptr-4,-1,size+4);
    free (ptr-4);
}
