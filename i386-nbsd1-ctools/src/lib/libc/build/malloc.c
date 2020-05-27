/* malloc - memory allocator
 * Copyright (c) 1989 Intel Corporation, ALL RIGHTS RESERVED.
 */

#include <errno.h>
#include <stdio.h>
#include <signal.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

/* block is 4 words long so that memory in block will be quad word aligned */
struct block {
  unsigned magic;		/* used to check for corrupted heap	*/
  struct block *next;		/* -1 if allocated, 			*/
				/* else pointer to next free block 	*/
  struct block *last;		/* pointer to previous block in heap	*/
				/* NULL if first block in heap 		*/
  unsigned length;		/* length of block excluding header	*/
};

#define BS_SIZE          sizeof(struct block)
#define WORD		 4
#define QWORD		 (4*WORD)
#define MIN_ALLOC        (QWORD+BS_SIZE)
#define QW_ALIGN(x)      (((int)x+0xF) & 0xFFFFFFF0)
#define MAGIC		 0xACDBADCB

extern void *sbrk(unsigned incr);
extern void free(void *ptr);
extern void _Lsplit_block(struct block *block, unsigned requested_size);

/* old way */
/*
struct block *_free_list = (struct block *) NULL;
struct block *_heap_end  = (struct block *) NULL;
 */

/* new way, depends on .bss being cleared to zero AND
 * zero being equivelent to (struct block *) NULL */
struct block *_free_list;
struct block *_heap_end;

void *malloc(unsigned int requested_size)
{
    struct block *new_block, *prev_block;
    struct block *curr_block;

    if (requested_size == 0)
        return (void *) NULL;

    requested_size = QW_ALIGN(requested_size);
    
    /* search through _free_list */
    for(prev_block = (struct block *) NULL, curr_block = _free_list; 
        curr_block != (struct block *) NULL && curr_block->length < requested_size;
        prev_block = curr_block, curr_block = curr_block->next)
    { } /* empty for loop! */

    if(curr_block != (struct block *) NULL) 
    {
        /* found one big enough */
        /* remove block from free list */
        if(prev_block == (struct block *) NULL) 
        {
            _free_list = curr_block->next;
        } 
        else 
        {
            prev_block->next = curr_block->next;
        }
        curr_block->next = (struct block *) -1;
        if(requested_size + MIN_ALLOC <= curr_block->length)
        {
	    _Lsplit_block(curr_block, requested_size);
        } 
        /* mark it as allocated */
        return (void *) (curr_block + 1);
    }

    /* is the block at _heap_end free? */
    /* if so, sbrk() for requested_size - block_length */
    /* Note that we know that if the block at _heap_end */
    /* is free, that its length is less than requested_size */
    /* otherwise we would have found it in the above loop */

    if(_heap_end != (struct block *) NULL && 
       _heap_end->next != (struct block *) -1)
    {
        int delta;
	void * temp_ptr;
        /* delta will be QW_ALIGNED because both requested_size */
        /* and _heap_end->length are aligned */
        delta = (int) requested_size - (int) (_heap_end->length);
	temp_ptr = sbrk(delta);
        if(temp_ptr != NULL && temp_ptr != (void *) -1)
        {
            /* find the block on the _free_list */
            for(prev_block = (struct block *) NULL, curr_block = _free_list; 
                curr_block != (struct block *) NULL && curr_block != _heap_end;
                prev_block = curr_block, curr_block = curr_block->next)
            { } /* empty for loop! */

            if(curr_block == (struct block *) NULL)
            {
                errno = EFREE;
                raise(SIGFREE);
                return (void *) NULL;
            }
            if(prev_block == (struct block *) NULL)
            {
                _free_list = curr_block->next;
            }
            else
            {
                prev_block->next = curr_block->next;
            }
            curr_block->next = (struct block *) -1;
            curr_block->length += delta;
            return (void *)(curr_block + 1);
        }
        else
        {
            return (void *) NULL;
        }
    }
    /* try to get memory from sbrk() */
    curr_block = (struct block *) sbrk(requested_size + BS_SIZE);
    if (curr_block != (struct block *) NULL &&
        curr_block != (struct block *) -1 )
    {
        curr_block->magic = MAGIC;
        curr_block->next = (struct block *) -1;
        curr_block->length = requested_size;
        curr_block->last = _heap_end;
        _heap_end = curr_block;
        return (void *) (curr_block + 1);
    }
    return (void *) NULL;
}


void *realloc(void *bptr, unsigned requested_size)
{
    struct block *block_ptr, *last_block, *next_block;
    struct block *prev_free, *curr_free;
    int delta;

    if (bptr == NULL) 
    {
        /* act as malloc() */
        return malloc(requested_size);
    }
    if (requested_size == 0) 
    {
        /* free bptr if size = 0 and bptr != NULL */
        /* we will have done the above malloc() if bptr == NULL */
        free((void *)bptr);
        return (void *) NULL;
    }
    block_ptr = ((struct block *)bptr) - 1;
    if (block_ptr->magic != MAGIC || block_ptr->next != (struct block *) -1)
    {
        errno = EFREE;
        raise(SIGFREE);
        return (void *) NULL;
    }

    requested_size = QW_ALIGN(requested_size);

    /* cast to (int), otherwise delta never < 0 due */
    /* to unsigned arithmetic and data conversion */
    delta = (int) requested_size - (int) block_ptr->length;

    /* If delta is minus, can we split this block? */
    if(delta <= 0)
    {
        _Lsplit_block(block_ptr, requested_size);
        return (void *) bptr;
    }

    /* If we are here, then delta is > 0 */
    /* also note that delta will be QW_ALIGN'd */
    /* because requested_size and the block->length */
    /* are both aligned */

    /* Are we at the end of the heap? */
    if(block_ptr == _heap_end)
    {
	char * temp_ptr;

        /* If so can we extend this block? */
	temp_ptr = sbrk(delta);
        if(temp_ptr != NULL && temp_ptr != (char *) -1 )
        {
            block_ptr->length += delta;
            return (void *) bptr;
        }
    }
    else /* we're not at _heap_end */
    {
        /* is next block in heap free? */
        next_block = (struct block *)(((char *)block_ptr) + 
                                        BS_SIZE + block_ptr->length);
        if(next_block->next != (struct block *) -1)
        {
            /* next_block is free, find on _free_list */
            for(prev_free = (struct block *) NULL, curr_free = _free_list;
                curr_free != next_block && curr_free != (struct block *) NULL;
                prev_free = curr_free, curr_free = curr_free->next)
            { }   /* empty for loop! */
            
            if(curr_free == (struct block *) NULL)
            {
                errno = EFREE;
                raise(SIGFREE);
                return (void *) NULL;
            }
            /* remove from _free_list, adjust _free_list */
            if(prev_free == (struct block *) NULL)
            {
                _free_list = curr_free->next;
            }
            else
            {
                prev_free->next = curr_free->next;
            }
            block_ptr->length += BS_SIZE + curr_free->length;
            if(_heap_end == curr_free)
            {
                _heap_end = block_ptr;
            }
            else /* adjust heap */
            {
                next_block = (struct block *) (((char *)block_ptr) + 
                                     BS_SIZE + block_ptr->length);
                next_block->last = block_ptr;
            }
            if(block_ptr->length >= requested_size)
            {
                _Lsplit_block(block_ptr, requested_size);
                return (void *) (block_ptr + 1);
            }
        }
    }

    /* note that if we get to here that we will have to move */
    /* the data since we have tried both possibilities of extending */
    /* the block inplace */

    /* are we the first block in the heap? */
    /* if not, is previous block free? */
    if(block_ptr->last != (struct block *) NULL)
    {
        last_block = block_ptr->last;
        if(last_block->next != (struct block *) -1)
        {
            /* is it big enough? */
            /* note that block_ptr->length already includes the next block */
            /* if the next block had been free, */
            if((last_block->length + BS_SIZE + block_ptr->length) >= requested_size)
            {
                /* find on free list */
                for(prev_free = (struct block *) NULL, curr_free = _free_list;
                    curr_free != last_block && curr_free != (struct block *) NULL;
                    prev_free = curr_free, curr_free = curr_free->next)
                { }   /* empty for loop! */

                if(curr_free == (struct block *) NULL)
                {
                    errno = EFREE;
                    raise(SIGFREE);
                    return (void *) NULL;
                }
                /* combine blocks before data copy to avoid possible overwrite */
                /* of block_ptr header info */
                last_block->length += BS_SIZE + block_ptr->length;
                if(_heap_end == block_ptr)
                {
                    _heap_end = last_block;
                }
                else
                {
                    next_block = (struct block *) (((char *)block_ptr) +
                                 BS_SIZE + block_ptr->length);
                    next_block->last = last_block;
                }
                /* remove from _free_list */
                if(prev_free == (struct block *) NULL)
                {
                    _free_list = last_block->next;
                }
                else
                {
                    prev_free->next = last_block->next;
                }
                last_block->next = (struct block *) -1;
                memcpy(last_block + 1, bptr, (requested_size - delta));
                _Lsplit_block(last_block, requested_size);
                return (void *) (last_block + 1);
            }
        }
    }

    /* get it from malloc(), if possible */
    if((next_block = (struct block *) malloc(requested_size)) !=
                                                  (struct block *) NULL)
    {
        memcpy(next_block, bptr, (requested_size - delta));
        free(bptr);
        return (void *) (next_block);
    }
    else
    {
        /* did not successfully realloc, bptr unchanged, data unchanged */
        return (void *) NULL;
    }
}

/* _Lsplit_block(), requested_size must already be quad-word aligned */

void _Lsplit_block(struct block *block_ptr, unsigned requested_size)
{
    struct block *split_block, *next_block;

    if(requested_size + MIN_ALLOC <= block_ptr->length)
    {
        /* Split block and put extra on _free_list */
        split_block = (struct block *)(((char *)block_ptr) + 
                          BS_SIZE + requested_size);
        split_block->magic = MAGIC;
        split_block->last = block_ptr;
        split_block->length = block_ptr->length - (requested_size + BS_SIZE);
        split_block->next = (struct block *) -1;
        block_ptr->length = requested_size;
        if(_heap_end == block_ptr)
        {
            _heap_end = split_block;
        }
        else
        {
            next_block = (struct block *)(((char *)split_block) + 
                                      BS_SIZE + split_block->length);
            next_block->last = split_block;
        }
	free(split_block + 1);
    }
    return;
}

void free(void *ptr)
{
    struct block *free_ptr;
    struct block *next_block, *last_block;
    struct block *prev_block, *curr_block;

    if (ptr == NULL)                    /* no action */
        return;

    free_ptr = ((struct block *)ptr) - 1;
    if (free_ptr->magic != MAGIC || free_ptr->next != (struct block *) -1)
    {
        errno = EFREE;
        raise(SIGFREE);
        return;
    }
    
    /* if theres anything on the _free_list, see if we can coalesce */
    if(_free_list != (struct block *) NULL)
    {
        /* don't check next block if this is last block in heap */
        if(free_ptr != _heap_end)
        {
            /* is next block in heap also free? */
            next_block = (struct block *)(((char *)free_ptr) + 
                             BS_SIZE + free_ptr->length);
            if(next_block->next != (struct block *) -1)
            {
                /* find block on _free_list */
                for(prev_block = (struct block *) NULL, curr_block = _free_list;
                    curr_block != next_block && curr_block != (struct block *) NULL;
                    prev_block = curr_block, curr_block = curr_block->next)
                { ; } /* empty for loop! */

                if(curr_block == (struct block *) NULL)
                {
                    errno = EFREE;
                    raise(SIGFREE);
                    return;
                }
                /* coalecse */
                free_ptr->length += BS_SIZE + next_block->length;
                if(_heap_end == next_block)
                {
                    /* reset _heap_end */
                    _heap_end = free_ptr;
                }
                else
                {
                    /* calculate new "next_block" */
                    next_block = (struct block *) (((char *)free_ptr) +
                                    BS_SIZE + free_ptr->length);
                    next_block->last = free_ptr;
                }
                /* remove from _free_list */
                if(prev_block == (struct block *) NULL)
                {
                    _free_list = curr_block->next;
                }
                else
                {
                    prev_block->next = curr_block->next;
                }
                /* free_ptr will be put on _free_list later */
            }
        }
	/* is there a block before us on the heap? */
        if(free_ptr->last != (struct block *) NULL)
        {
            last_block = free_ptr->last;
	    /* is it free? */
            if(last_block->next != (struct block *) -1)
            {
                /* find it on _free_list */
                for(prev_block = (struct block *) NULL, curr_block = _free_list;
                    curr_block != last_block && curr_block != (struct block *) NULL;
                    prev_block = curr_block, curr_block = curr_block->next)
                { ; } /* empty for loop! */

                if(curr_block == (struct block *) NULL)
                {
                    errno = EFREE;
                    raise(SIGFREE);
                    return;
                }
                last_block->length += BS_SIZE + free_ptr->length;
                if(_heap_end == free_ptr)
                {
                    _heap_end = last_block;
                }
                else
                {
                    next_block = (struct block *)(((char *)last_block) +
                        BS_SIZE + last_block->length);
                    next_block->last = last_block;
                }
                /* We coalesced with a block on the _free_list so we're done */
                return;
            }
        }
    }
    /* put on unordered free list */
    free_ptr->next = _free_list;
    _free_list = free_ptr;
    return;
}

void *calloc(size_t nelem, size_t elsize)
{
    unsigned long size;
    struct block *bp;

    size = (unsigned long)nelem * (unsigned long)elsize;
    if (size == 0 )
        return NULL;

    if (bp = (struct block *)malloc(size)) {
	size = (bp-1)->length;	/* get actual length allocated */
        memset((char *)bp, 0, size);
    }
    return (void *)bp;
}
