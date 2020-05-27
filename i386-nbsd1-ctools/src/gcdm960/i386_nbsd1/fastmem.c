/******************************************************************/
/*       Copyright (c) 1990,1991,1992,1993 Intel Corporation

   Intel hereby grants you permission to copy, modify, and 
   distribute this software and its documentation.  Intel grants
   this permission provided that the above copyright notice 
   appears in all copies and that both the copyright notice and
   this permission notice appear in supporting documentation.  In
   addition, Intel grants this permission provided that you
   prominently mark as not part of the original any modifications
   made to this software or documentation, and that the name of 
   Intel Corporation not be used in advertising or publicity 
   pertaining to distribution of the software or the documentation 
   without specific, written prior permission.  

   Intel Corporation provides this AS IS, without any warranty,
   including the warranty of merchantability or fitness for a
   particular purpose, and makes no guarantee or representations
   regarding the use of, or the results of the use of, the software 
   and documentation in terms of correctness, accuracy, reliability, 
   currentness, or otherwise; and you rely on the software, 
   documentation and results solely at your own risk.
 */
/******************************************************************/

#include "cc_info.h"
#include "fastmem.h"
#include "gcdm960.h"

extern dbase prog_db;

typedef struct {
  st_node *sym;
  long size;
  long usage_count;
  int  fmem_allocated; /* flag indicating whether this has had
                          fast memory allocated to it. */
  int  fmem_addr;
  int  fmem_id;        /* unique number for qsort consistency across hosts */
} fmem_dec_node;

typedef
struct fast_mem_node {
  struct fast_mem_node * next;
  unsigned long lo_addr;
  unsigned long mem_size;
} * fmem_p;

#define NODES_IN_BLK 500

typedef struct fmem_block {
  struct fmem_block *  next;
  int                  nodes_left;
  struct fast_mem_node nodes[NODES_IN_BLK];
} * fmem_block_p;

static fmem_block_p fmem_blks;
static fmem_p fast_mem;
static fmem_p cur_fmem_p;

static fmem_dec_node * fmemnode_p;
static int num_candidates;
static int max_fmem_size = 0;

static fmem_p
fmem_alloc DB_PROTO((void))
{
  struct fmem_block *first_blk = fmem_blks;

  if (first_blk == 0 || first_blk->nodes_left <= 0)
  {
    first_blk = (fmem_block_p)db_malloc(sizeof(struct fmem_block));

    first_blk->nodes_left = NODES_IN_BLK;
    first_blk->next = fmem_blks;
    fmem_blks = first_blk;
  }

  first_blk->nodes_left -= 1;
  return (&first_blk->nodes[first_blk->nodes_left]);
}

void
add_fmem (addr, size)
unsigned long addr;
unsigned long size;
{
  fmem_p new_p;

  new_p = fmem_alloc();
  new_p->next = 0;
  new_p->lo_addr = addr;
  new_p->mem_size = size;

  if (cur_fmem_p == 0)
    fast_mem = new_p;
  else
    cur_fmem_p->next = new_p;
  cur_fmem_p = new_p;
}

void
free_fmem DB_PROTO((void))
{
  fmem_block_p t_blk;
  fmem_block_p nxt_blk;

  t_blk = fmem_blks;
  while (t_blk != 0)
  {
    nxt_blk = t_blk->next;
    free(t_blk);
    t_blk = nxt_blk;
  }

  fmem_blks = 0;
  fast_mem = 0;
  cur_fmem_p = 0;
}

static int
fmem_cmp(s1_p, s2_p)
fmem_dec_node *s1_p;
fmem_dec_node *s2_p;
{
  int size1 = (s1_p->size + 3) >> 2;  /* round up to number of words */
  int size2 = (s2_p->size + 3) >> 2;  /* round up to number of words */
  int use1 = s1_p->usage_count;
  int use2 = s2_p->usage_count;

  if (size1 <= 0) size1 = 1;
  if (size2 <= 0) size2 = 1;

  use1 = use1 / size1;
  use2 = use2 / size2;

  if (use1 > use2)
    return -1;
  if (use1 < use2)
    return 1;
  if (size1 < size2)
    return -1;
  if (size1 > size2)
    return 1;

  /*
   * This last bit just guarantees no randomness due to hosts qsort
   * implementation.
   */
  if (s1_p->fmem_id < s2_p->fmem_id)
    return -1;
  return 1;
}

static int
find_fmem (size, addr_p)
long size;
unsigned long *addr_p;
{
  int align;  /* set to the bits that need to get cleared to make the
                 proper alignment. */
  fmem_p tmem;
  fmem_p prev;

  if (fast_mem == 0)
    return -1;
    
  switch (size)
  {
    default:
      align = 16; break;
    case 1:
      align = 1; break;
    case 2:
    case 3:
      align = 2; break;
    case 4:
    case 5:
    case 6:
    case 7:
      align = 4; break;

    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      align = 8; break;
  }

  /*
   * search the list of unallocated fast memory for one that fits both the
   * size and alignment.  Update the fast memory list to take away stuff
   * that is already allocated.
   */
  prev = 0;
  for (tmem = fast_mem; tmem != 0; prev = tmem, tmem = tmem->next)
  {
    unsigned long baseaddr;
    unsigned long length;
    unsigned long tlength;

    baseaddr = tmem->lo_addr;
    length   = tmem->mem_size;

    /* align baseaddr, and adjust length accordingly. */
    baseaddr = (baseaddr + align - 1) & ~(align - 1);
    tlength = length;
    length -= baseaddr - tmem->lo_addr;

    if (length > tlength)
      length = 0;

    if (size <= length)
    {
      /*
       * we can allocate it. We must update the list to show that
       * the memory has been allocated.
       */
      if (tmem->lo_addr == baseaddr)
      {
        /*
         * either have to delete this node completely, or just update
         * its address and length.
         */
        if (size == tmem->mem_size)
        {
          /* delete it. */
          if (prev == 0)
            fast_mem = tmem->next;
          else
            prev->next = tmem->next;
        }
        else
        {
          /* update size and length */
          tmem->lo_addr = baseaddr+size;
          tmem->mem_size -= size;
        }
      }
      else
      {
        if (size == length)
        {
          /*
           * The allocation uses the whole end of the block, so just update
           * the size of the current block.
           */
          tmem->mem_size = baseaddr - tmem->lo_addr;
        }
        else
        {
          /*
           * We have to adjust the size of the original block, as well
           * as allocate a new block.
           */
          fmem_p new_block;

          new_block = fmem_alloc();
          new_block->next = tmem->next;
          new_block->lo_addr = baseaddr + size;
          new_block->mem_size = tmem->mem_size - (baseaddr - tmem->lo_addr);
          new_block->mem_size -= size;

          tmem->next = new_block;
          tmem->mem_size = baseaddr - tmem->lo_addr;
        }
      }

      *addr_p = baseaddr;
      return 1;
    }
  }
  return 0;
}

/*
 * This routine reserves space at 0x100 in SRAM for fpem_CA_AC in the
 * floating point library.
 */
static void
reserve_fpem_CA_AC()
{
  fmem_p tmem;
  fmem_p prev = 0;

  for (tmem = fast_mem; tmem != 0; prev = tmem, tmem = tmem->next)
  {
    unsigned long lo = tmem->lo_addr;
    unsigned long hi = lo + tmem->mem_size - 1;

    /* cases:
     *
     * 1. tmem's range starts < 0x100, 0x100 <= ends <= 0x103.
     * 2. tmem's range starts < 0x100, ends > 0x103.
     * 3. tmem's range 0x100 <= starts <= 0x103, ends > 0x103.
     * 4. tmem's range 0x100 <= starts <= 0x103, 0x100 <= ends <= 0x103.
     */

    if (lo < 0x100)
    {
      if (0x100 <= hi && hi <= 0x103)
      {
        /* case 1 - adjust tmem's mem_size so as to just lose the end. */
        tmem->mem_size = 0xff - lo + 1;
      }
      else if (hi > 0x103)
      {
        /*
         * case 2 - just split this into two blocks, the second of which
         * starts at 0x104, then adjust both sizes appropriately.
         */
        fmem_p new_p = fmem_alloc();
        new_p->next = tmem->next;
        tmem->next = new_p;

        new_p->lo_addr = 0x104;
        new_p->mem_size = hi - 0x104 + 1;
        tmem->mem_size = 0xff - lo + 1;
      }
    }
    else if (0x100 <= lo && lo <= 0x103)
    {
      if (hi >0x103)
      {
        /*
         * case 3 - just adjust lo_addr and mem_size to lose the beginning
         * of the block.
         */
        tmem->lo_addr = 0x104;
        tmem->mem_size = hi - 0x104 + 1;
      }
      else if (0x100 <= hi && hi <= 0x103)
      {
        /*
         * case 4 - just delete tmem.
         */
        if (prev == 0)
          fast_mem = tmem->next;
        else
          prev->next = tmem->next;
      }
    }
  }
}

void
get_fmem_cands(p)
st_node *p;
{
  if (p->rec_typ == CI_VDEF_REC_TYP)
  {
    long usage = db_rec_var_usage(p);
    long size = db_rec_var_size(p);

    /* make sure no variables are allocated to SRAM already. */
    db_rec_var_make_fmem(p, 0);

    if (usage > 0 &&
        size > 0 && size <= max_fmem_size)
    {
      fmemnode_p->sym = p;
      fmemnode_p->usage_count = usage;
      fmemnode_p->size  = size;
      fmemnode_p->fmem_allocated = 0;
      fmemnode_p->fmem_addr = 0;
      fmemnode_p->fmem_id = num_candidates;
      fmemnode_p++;
      num_candidates ++;
    }
  }
}

int num_vars;
int num_funcs;

void
count_var_and_func(p,i)
st_node *p;
int i;
{
  switch (p->rec_typ)
  {
    case CI_VDEF_REC_TYP:
      num_vars++;
      break;

    case CI_LIB_FDEF_REC_TYP:
    case CI_FDEF_REC_TYP:
    case CI_FREF_REC_TYP:
      num_funcs++;
      break;
  }
}

void
make_fmem_decisions DB_PROTO((void))
{
  /*
   * First create a table of variables that are under consideration
   * for allocation to SRAM.
   * Initial criteria is that the size of the variable be greater than 0
   * bytes, and the size must be less than 128 bytes, and that usage
   * counts must be greater than 5.
   */
  int num_nodes;
  fmem_dec_node *fmem_nodes;  /* array of sram decision nodes */
  fmem_p tmem;
  int i;
  char header_buf[1000];
  char *buf_p;
  int n_lines;

  reserve_fpem_CA_AC();

  if (flag_print_variables || flag_print_decisions)
  {
    buf_p = header_buf;
    sprintf (buf_p, "\n");
    buf_p += strlen(buf_p);

    sprintf (buf_p, "Variables Allocated to Fast Memory\n");
    buf_p += strlen(buf_p);
    if (fast_mem != 0)
    {
      sprintf (buf_p, " (Fast Memory = ");
      buf_p += strlen(buf_p);

      tmem = fast_mem;
      sprintf (buf_p, "0x%x-0x%x",
               tmem->lo_addr, tmem->lo_addr + tmem->mem_size - 1);
      buf_p += strlen(buf_p);

      for (tmem = tmem->next; tmem != 0; tmem = tmem->next)
      {
        sprintf (buf_p, ",0x%x-0x%x",
                 tmem->lo_addr, tmem->lo_addr + tmem->mem_size - 1);
      buf_p += strlen(buf_p);
      }
      sprintf (buf_p, ")\n\n");
      buf_p += strlen(buf_p);
      
      sprintf (buf_p,
               "Variable                   Size  Usage Count  Address\n");
      buf_p += strlen(buf_p);
      sprintf (buf_p,
               "=========================|======|===========|==========\n");
      buf_p += strlen(buf_p);
    }
    else
    {
      sprintf (buf_p, "No Fast Memory Specified\n");
      buf_p += strlen(buf_p);
    }

    fprintf (decision_file.fd, "%s", header_buf);
    n_lines = 6;
  }

  if (fast_mem == 0)
  {
    if (flag_print_summary)
    {
      fprintf (decision_file.fd, "\n0 variables were allocated to fast memory.\n");
    }
    return;
  }

  for (tmem = fast_mem; tmem != 0; tmem = tmem->next)
  {
    if (tmem->mem_size > max_fmem_size)
      max_fmem_size = tmem->mem_size;
  }

  num_nodes = num_vars;
  fmem_nodes = (fmem_dec_node *)db_malloc(num_nodes * sizeof(fmem_dec_node));

  /*
   * Fill the nodes in.
   */
  num_candidates = 0;
  fmemnode_p = &fmem_nodes[0];
  dbp_for_all_sym(&prog_db, get_fmem_cands);

  /* sort the candidates according to usage count. */
  qsort(fmem_nodes, num_candidates, sizeof(fmem_dec_node), fmem_cmp);

  /*
   * Now just walk through allocating variables to fast memory based on
   * how much space is left, and how need to fill.
   */
  for (i = 0, fmemnode_p = fmem_nodes; i < num_candidates; i++, fmemnode_p++)
  {
    int var_base_address;
    int success;

    success = find_fmem (fmemnode_p->size, &var_base_address);
    if (success < 0)
    {
      num_candidates = i+1;  /* makes printing shorter */
      break;  /* no more fast memory at all */
    }

    if (success > 0)
    {
      db_rec_var_make_fmem (fmemnode_p->sym, var_base_address);
      fmemnode_p->fmem_allocated = 1;
      fmemnode_p->fmem_addr = var_base_address;
    }
  }

  if (flag_print_variables || flag_print_decisions)
  {
    /* print fmem allocations */
    for (i = 0, fmemnode_p = fmem_nodes; i < num_candidates; i++, fmemnode_p++)
    {
      if (fmemnode_p->fmem_allocated)
      {
        if (n_lines > db_page_size())
        {
          fprintf (decision_file.fd, "%s", header_buf);
          n_lines = 6;
        }

        fprintf (decision_file.fd, "%-25.25s %6d %11d %#10x\n",
                 fmemnode_p->sym->name,
                 fmemnode_p->size,
                 fmemnode_p->usage_count,
                 fmemnode_p->fmem_addr);

        n_lines += 1;
      }
    }
  }

  if (flag_print_summary)
  {
    int num_variables_allocated = 0;

    for (i = 0, fmemnode_p = fmem_nodes; i < num_candidates; i++, fmemnode_p++)
    {
      if (fmemnode_p->fmem_allocated)
         num_variables_allocated++;
    }
    fprintf (decision_file.fd, "\n%d variable%s allocated to fast memory.\n", 
             num_variables_allocated, 
             num_variables_allocated == 1 ? " was" : "s were");
  }
}
