
#ifdef PROCEDURE_PLACEMENT

#include <assert.h>
#include "ldplace.h"
#include "sysdep.h"
#include "ldlang.h"
#include "ldcache.h"
#include "ldsym.h"
#include "ldregion.h"
#include "bfd.h"
#include "callsite.h"


#define DEBUG 1

/* If a function arc is called less than WEIGHT_THRESHOLD number of times, */
/* don't recurse anymore when placing function arcs. */
#define WEIGHT_THRESHOLD	1000

/* A function with no arc heavier than this is placed into fluff. */
#define FLUFF_THRESHOLD		100



/* We need to recognize B and BX because of tail-call optmization. */
#define CALL  0x09
#define CALLX 0x86
#define BAL   0x0b
#define BALX  0x85
#define B     0x08
#define BX    0x84
		    

#define HMALLOC(X,T) \
    X = (T **) ldmalloc(sizeof(T *) * HASH_TABLE_SIZE);\
	    memset(X,0,(sizeof(T *) * HASH_TABLE_SIZE));

static void add_cg_node ();

static call_graph_node *lookup_cg_node();

static void append_to_call_out_list ();

static void add_cg_arc ();

void place_functions ();

#if DEBUG
static void print_arc();
static void print_cg_node();
static void print_cg_node_hash_table();
static void print_fluff();
static void print_arc_array();
static void print_cache_table();
#endif

/* We have to support looking up a lang_input_statement type based 
on a asection. This hash table will support this. */
static call_graph_node **cg_node_hash_table;

/* Hash the arcs on input/output filename pairs. */
static call_graph_arc **cg_arc_hash_table;

/* This is a linked list of functions in unnamed sections (ie. library junk) */
static call_graph_node *fluff = (call_graph_node *) 0;
static call_graph_node *fluff_p;

/* This is a table indexed by cache line of functions that want to live */
/* at the cache line of the index. For example, functions linked together */
/* at cache_table[4] want to live on line 4 of the cache. */
static call_graph_node **cache_table;

/* This becomes true when we don't have any library functions (ie fluff) left */
static int fluff_is_empty = 0;

/* This array indexed by cache lines tells us when we don't have anything */
/* left to place that wants to live on this cache line. */
static int *cache_half_line_is_empty;

static int cg_named_node_cnt = 0;
int cg_arc_cnt  = 0;



























































































































































































      
unsigned int
hash_string (string1)
char *string1;
{
   int hash_val = 0;
   while (string1 && *string1)
      hash_val += *string1++;
   return (hash_val % HASH_TABLE_SIZE);
}

      
unsigned int
hash_strings (string1, string2, string3, string4)
char *string1;
char *string2;
char *string3;
char *string4;
{
   int hash_val = 0;
   while (string1 && *string1)
      hash_val += *string1++;
   while (string2 && *string2)
      hash_val += *string2++;
   while (string3 && *string3)
      hash_val += *string3++;
   while (string4 && *string4)
      hash_val += *string4++;

   return (hash_val % HASH_TABLE_SIZE);
}



static void 
add_cg_node(section, file)
    asection *section;
    lang_input_statement_type *file;
{
   int hash_val = hash_string(section->name);
   call_graph_node *cgn;

   cgn = (call_graph_node *) ldmalloc (sizeof(call_graph_node));

   /* Non-specially named sections and unattached sections end up as fluff */
   if (strncmp (section->name, NAMED_TEXT_SECTION_PREFIX, 
                               strlen(NAMED_TEXT_SECTION_PREFIX))) {
      cgn->named_section = 0;
   }
   else {
      cgn->named_section = 1;
      cg_named_node_cnt++;
   }
   cgn->heaviest_arc_wt = 0;
   cgn->attached      = 0;
   cgn->placed        = 0;
   cgn->next_fluff    = (call_graph_node *) 0;
   cgn->next_cline    = (call_graph_node *) 0;
   cgn->file          = file;
   cgn->section       = section;
   cgn->region_list   = (region *) 0;
   cgn->call_out_list = (call_graph_arc *) 0; 
   cgn->call_in_list  = (call_graph_arc *) 0; 
   cgn->address       = -1;
   cgn->next = cg_node_hash_table[hash_val];
   cg_node_hash_table[hash_val] = cgn;
}


static
call_graph_node *lookup_cg_node (section)
   asection *section;
{
   int hash_val = hash_string(section->name);
   call_graph_node *cgn = cg_node_hash_table[hash_val];

   while (cgn) {
      if (cgn->section == section)
         return cgn;
      cgn = cgn->next;
   }
   abort();
}


static void
append_to_call_out_list (node_p, arc_p)
call_graph_node *node_p;
call_graph_arc  *arc_p;
{
   arc_p->out_arc_next   = node_p->call_out_list;
   node_p->call_out_list = arc_p;
}


static void
append_to_call_in_list (node_p, arc_p)
call_graph_node *node_p;
call_graph_arc  *arc_p;
{
   arc_p->in_arc_next   = node_p->call_in_list;
   node_p->call_in_list = arc_p;
}


static void
add_cg_arc (in_file, in_sec, out_file, out_sec, addr)
lang_input_statement_type *in_file;
asection *in_sec;
lang_input_statement_type *out_file;
asection *out_sec;
unsigned int addr;
{
   int hash_val;
   call_graph_arc *arc_p;
   call_graph_node *call_fm;
   call_graph_node *call_to;
   call_site       *tmp_call_site;

   hash_val = hash_strings (in_file->filename, in_sec->name, out_file->filename,   out_sec->name);
   call_fm = lookup_cg_node (in_sec);
   call_to = lookup_cg_node (out_sec);

   /* If caller and callee are both named sections, create the arc. */
   if (call_fm->named_section && call_to->named_section) {
      cg_arc_cnt++;
  
      /* Caller and callee are attached now: nodes which end up */
      /* unattached become fluff, since we don't have an arc to place them */
      call_fm->attached = 1;
      call_to->attached = 1;

      arc_p = (call_graph_arc *) ldmalloc(sizeof(call_graph_arc));
      tmp_call_site   = get_next_call_site(in_file, in_sec, out_file, out_sec);
      if (tmp_call_site != (call_site *) 0) {
         arc_p->weight = tmp_call_site->weight;
         arc_p->profiled = tmp_call_site->profiled;
      }
      else {
         arc_p->weight      = 0.0;
         arc_p->profiled    = 0;
      }
         
      if (call_fm->heaviest_arc_wt < (int) arc_p->weight)
         call_fm->heaviest_arc_wt = (int) arc_p->weight;
      if (call_to->heaviest_arc_wt < (int) arc_p->weight)
         call_to->heaviest_arc_wt = (int) arc_p->weight;

      arc_p->arc_num     = cg_arc_cnt;
      arc_p->constrained = 0;
      arc_p->call_region = (region *) 0;
      arc_p->call_to     = call_to;
      arc_p->call_fm     = call_fm;
      arc_p->offset      = addr;
      append_to_call_out_list (call_fm, arc_p);
      append_to_call_in_list  (call_to, arc_p);
      arc_p->next = cg_arc_hash_table[hash_val];
      cg_arc_hash_table[hash_val] = arc_p;
   }
}



void build_cg (isec, ifile, ibfd, reloc_vec, data)
asection                  *isec;
lang_input_statement_type *ifile;
bfd                       *ibfd;
arelent                   **reloc_vec;
unsigned char             *data;
{
  arelent **parent;
  unsigned long prev_address = -1;
  unsigned char opcode_byte;


  for (parent = reloc_vec; *parent; parent++) 
  {
    int oc_at_ip=0;  /* Opcode should be at the ip address, or ip+1word. */

    /* If the target is defined, then ... */
    if ((*parent)->sym_ptr_ptr) 
    { 
       /* If relocation type is 24bit pc relative or opt call, then we
          know that the opcode is at this address. */
      bfd_reloc_type parent_reloc_type = (*parent)->howto->reloc_type;
      oc_at_ip = (parent_reloc_type == bfd_reloc_type_24bit_pcrel ||
                  parent_reloc_type == bfd_reloc_type_opt_call);

      if ((prev_address != (*parent)->address) &&
          (oc_at_ip || parent_reloc_type == bfd_reloc_type_32bit_abs))
      {
        /* Table search Lower/upper bounds and fudge factor for BE files */
        int lb,ub,i,f=0;  

        static unsigned char instructs[] = { CALL, BAL, B, CALLX, BALX, BX };
        /* Set the search factors: */
        if (oc_at_ip)  
        {
          /* relocation address corresponds to the word with the opcode */
          lb=0;
          ub=2;
          f=BFD_BIG_ENDIAN_TARG_P(ibfd) ? 0 : 3;
        }
        else 
        {
          /* Else address corresponds to the word FOLLOWING the opcode. */
          lb=3;
          ub=5;
          f=BFD_BIG_ENDIAN_TARG_P(ibfd) ? -4 : -1;
        }
        prev_address = (*parent)->address;
	opcode_byte = data[(*parent)->address + f];
        /* Find the opcode in the section data. */
        for (i=lb;i <= ub;i++)
        {
        /* If the section data corresponds to a call instruction opcode... */
          if (opcode_byte == instructs[i]) {
            asection *outsec;
            asymbol  *callee_symbol;
            callee_symbol = (*(*parent)->sym_ptr_ptr);

            /* ...determine what section it is calling to. */
            outsec = callee_symbol->section;
            if (!outsec) {
              /* Lookup symbol since it is not known by the sym_ptr_ptr. */
              ldsym_type *s;
              asymbol **def;
              s = ldsym_get_soft(callee_symbol->name);
              def = s ? s->sdefs_chain : (asymbol **) 0;
              if (def && *def && (*def)->section)
                outsec = (*def)->section;
            }
            /* At this point, if we know the output section,
             * then add an arc for this call.  The only case when isec 
             * will equal outsec is when he has callx or balx symbol.  */
            if (outsec && isec != outsec) {
               call_graph_node *out_node;
               out_node = lookup_cg_node(outsec);
               add_cg_arc(ifile, isec, out_node->file, outsec, 
                          (*parent)->address);
            }
            break;
          }            
        }
      }
    }
  }
}



static int
get_word (p_arg, is_big_endian)
char *p_arg;
int is_big_endian;
{
  int i;
  unsigned long val;
  unsigned char *p = (unsigned char *)p_arg;

  if (is_big_endian) {
     val = *p++;
     for (i=0; i<3; i++) {
        val = (val << 8) | *p++;
     }
  }
  else {
     val = 0;
     for (i=3; i>=0; i--) {
        val <<= 8;
        val |= p[i] & 0x0ff;
     }

  }
  return val;
}

unsigned int
ctrl_instr(instr)
unsigned int instr;
{
   unsigned char opcode_byte;
   unsigned int  target_disp;
   opcode_byte = instr >> 24;

   /* Interesting ctrl instruction opcodes are : */
    target_disp = 0;
    switch (opcode_byte) {
       case 0x08:		/* b    */
       case 0x09:		/* call */
       case 0x0b:		/* bal  */
       case 0x10:		/* bno  */
       case 0x11:		/* bg   */
       case 0x12:		/* be   */
       case 0x13:		/* bge  */
       case 0x14:		/* bl   */
       case 0x15:		/* bne  */
       case 0x16:		/* ble  */
       case 0x17:		/* bo   */
                    target_disp = (instr >> 2) & 0x003fffff;
                    break;

       default:     break;
   }
   if (target_disp >> 22) 
      target_disp |= 0xffc00000;
 
   return target_disp;
}

unsigned int
cobr_instr(instr)
unsigned int instr;
{
   
   unsigned char opcode_byte;
   unsigned int  target_disp;
   opcode_byte = instr >> 24;

   /* Interesting cobr instruction opcodes are : (page d-10) */
 
   target_disp = 0;

   if (opcode_byte >= 0x30 && opcode_byte <= 0x3f) {
      target_disp = (instr >> 2) & 0x000007ff;

      /* at this point, target addr contains the displacement from 
         the current ip in words. Now do sign extension. */

      if (target_disp >> 10) 
         target_disp |= 0xfffff800;

   }
   return target_disp;
}




void 
build_cfg (isec, ifile, ibfd, reloc_vec, data)
asection                  *isec;
lang_input_statement_type *ifile;
bfd                       *ibfd;
arelent                   **reloc_vec;
unsigned char             *data;
{
   call_graph_node *node_p;
   region *region_p;
   unsigned char opcode_byte;
   unsigned char mode_byte;
   unsigned int instr_word1;
   unsigned int target_addr;
   int bytes_in_instr;
   int target_disp;
   int is_big_endian;
   int opcode_byte_offset;
   int mode_byte_offset;
   int i;

   node_p = lookup_cg_node (isec);

   /* for big endian code, opcode is at data[i], */
   /* for little endian code, opcode is at data[i+3]. */

   is_big_endian = BFD_BIG_ENDIAN_TARG_P(ibfd);
   if (is_big_endian) {
      opcode_byte_offset = 0;
      mode_byte_offset   = 2;
   }
   else {
      opcode_byte_offset = 3;
      mode_byte_offset   = 1;
   }

   for (i = 0; i < isec->size; ) {
      opcode_byte = data[i+opcode_byte_offset];
      target_disp = 0;

      if ((opcode_byte >> 4) == 0 || (opcode_byte >> 4) == 1) 
      {
         instr_word1 = get_word(&data[i], is_big_endian);
         target_disp = ctrl_instr(instr_word1);
      }
      else if ((opcode_byte >> 4) == 3)
      {
         instr_word1 = get_word(&data[i], is_big_endian);
         target_disp = cobr_instr(instr_word1);
      }

      /* check to see if this is a mem-b format instruction. */
      bytes_in_instr = 4;
      if (opcode_byte >> 7) {
         /* bit 12 clear -> mem-a, bit 12 set -> mem-b */
         mode_byte = data[i+mode_byte_offset];
         if (mode_byte & 0x20) {
            bytes_in_instr += 4;
         }
      }

      /* check if this is a backwards branch. */
      if (target_disp < 0) { 
         /* branch out of this function ? */
         if (((target_disp * 4) + i) >= 0) {
            target_addr = (target_disp * 4) + i;
            region_p    = new_region(target_addr, i, isec);

            /* add region to list of regions for this call graph node */
            insert_in_region_list(node_p, region_p);
         }
      }
      i += bytes_in_instr;
   }
   /* add whole func as region */
   region_p = new_region (0, i, isec);
   insert_in_region_list (node_p, region_p);
}


/*
 * walk_bfd - this function is responsible for building this section's
 *            portion of the call_graph and control flow graph. It reads
 *            the relocations and data for this section and then calls
 *            build_cg and build_cfg.
 */

void walk_bfd(isec,ifile)
asection *isec;
lang_input_statement_type *ifile;
{
   bfd *ibfd;
   lang_input_statement_type *ofile;
   arelent **reloc_vector = (arelent **) 0;
   unsigned char *data;
   int num_relocs;

   ibfd = ifile->the_bfd;

   /* Read the relocation vector from the bfd file. */
   num_relocs = get_reloc_upper_bound (ibfd, isec);
   reloc_vector = (arelent **) ldmalloc (num_relocs);

   if (bfd_canonicalize_reloc(ibfd,isec,reloc_vector,ifile->asymbols)) {
      /* Read the data from the file and build the graphs. */
      data = (unsigned char *) ldmalloc(isec->size);
 
      if (bfd_get_section_contents(ibfd,isec,data,0,isec->size)) {
         build_cg (isec, ifile, ibfd, reloc_vector, data);
         build_cfg (isec, ifile, ibfd, reloc_vector, data);
         free (data);
      }
      else
         info("\nerror reading section from: %B\n",ibfd);
   }
     
   free(reloc_vector);
}



/* For qsort. */
static int
cmp_cg_arc (arc1, arc2)
   call_graph_arc **arc1;
   call_graph_arc **arc2;
{
#if 0
   /* Division by four here assumes 2 way set-associative cache -> Cx */
   int num_half_lines = number_of_cache_lines() * 4;
   int arc1_caches = (*arc1)->call_region->half_lines / num_half_lines;
   int arc2_caches = (*arc2)->call_region->half_lines / num_half_lines;

   /* If one of these arcs is contained in a huge region, it's probably */
   /* too big to fit into cache anyway. */

   if (arc1_caches && !arc2_caches)
      return 1;
   else if (arc2_caches && !arc1_caches)
      return -1;
   else
   {
#endif
      if ((*arc1)->weight < (*arc2)->weight) {
         return 1;
      }
      else if ((*arc1)->weight > (*arc2)->weight) {
         return -1;
      }
      else {
         if ((*arc1)->arc_num < (*arc2)->arc_num)
            return 1;
         else
            return -1;
      }
#if 0
   }
#endif
}


void
fill_arc_array (arc_array)
call_graph_arc *arc_array[];
{
   int i;
   int j;
   call_graph_arc *tmp_arc;

   j = 0;
   for (i = 0; i < HASH_TABLE_SIZE; i++) {
      tmp_arc = cg_arc_hash_table[i];
      while (tmp_arc) {
         if (tmp_arc->call_fm->named_section && tmp_arc->call_to->named_section)
         {
            arc_array[j++] = tmp_arc;
         }
         tmp_arc = tmp_arc->next;
      }
   }
}

call_graph_arc **
build_sorted_arc_array ()
{
   call_graph_arc **arc_array;
   arc_array = (call_graph_arc **)ldmalloc(cg_arc_cnt*sizeof(call_graph_arc*));
   fill_arc_array (arc_array);
   qsort (arc_array, cg_arc_cnt, sizeof(call_graph_arc *), cmp_cg_arc);
   return arc_array;
}

/*
 * build_fluff - when creating a call graph node, if it doesn't have
 * a specially named section it is already fluff. However, a specially
 * named section might not be part of any arc, and so we are free to
 * place it anywhere; hence, we fluffify it.
 */
static void
build_fluff ()
{
   int i;
   call_graph_node *node_p;

   for (i = 0; i < HASH_TABLE_SIZE; i++)
   {
      node_p = cg_node_hash_table[i];
      while (node_p)
      {
#if RANDOM
         node_p->next_fluff = fluff;
         fluff = node_p;
#else
         if ((!node_p->attached) || (!node_p->named_section) || node_p->heaviest_arc_wt < FLUFF_THRESHOLD)
         {
            node_p->next_fluff = fluff;
            fluff = node_p;
         }
#endif
         node_p = node_p->next;
      }
   }
}

/* Return a call graph */
void
ldplace_form_call_graph()
{
   call_graph_arc **arc_array;
   lang_input_statement_type *f;
   extern lang_statement_list_type file_chain;

   HMALLOC(cg_arc_hash_table, call_graph_arc);
   HMALLOC(cg_node_hash_table, call_graph_node);

   for (f = (lang_input_statement_type *)file_chain.head; f;
        f = (lang_input_statement_type *)f->next) {
      asection *as;
      for (as = f->the_bfd->sections; as; as=as->next) {
         if ((!as->output_section) && (as->flags & SEC_CODE))
            add_cg_node(as,f);
      } 
   } 

   for (f = (lang_input_statement_type *)file_chain.head; f;
        f = (lang_input_statement_type *)f->next) {
      asection *as;
      for (as = f->the_bfd->sections; as; as=as->next) {
         /* add call graph arcs (if any) from this section of this file */
	  if ((!as->output_section) && (as->flags & SEC_CODE) && as->size != 0){
	      walk_bfd(as, f);
	  }
      }
  }

   arc_array = build_sorted_arc_array ();
   add_regions_to_arcs (arc_array);
   build_fluff ();
   place_functions (arc_array);
#if 0
   printf ("\n\nPrinting Arc Array:\n");
   print_arc_array (arc_array);
   printf ("\n\n\n");
#endif

#if 0
   printf ("\n\nPrinting Fluff:\n");
   print_fluff ();
   printf ("\n\n\n");
#endif

#if 0
   printf ("\n\nPrinting CG Node Hash Table:\n");
   print_cg_node_hash_table();
   printf ("\n\n\n");
#endif

#if 0
   printf ("\n\nPrinting Cache Table:\n");
   print_cache_table ();
   printf ("\n\n");
#endif 

   free (arc_array);
   free(cg_node_hash_table);
   free(cg_arc_hash_table);
}

void
place_arc (arc_p)
call_graph_arc *arc_p;
{
   int half_line_num;
   region*  region_p;
   call_graph_arc *tmp_arc;
   call_graph_arc *heavy_arc;

   region_p = arc_p->call_region;
   if (region_p == (region *) 0)
      return;

   if (arc_p->call_to->address == -1)
   {
      /* If caller isn't constrained, set it to 0. */
      if (arc_p->call_fm->address == -1)
      {
         arc_p->call_fm->address = 0;
      }
      half_line_num  = arc_p->call_fm->address + region_p->half_lines;
      arc_p->call_to->address = half_line_num % (2 * number_of_cache_lines ());
      arc_p->constrained = 1;
   }
   else
   {
      if (arc_p->call_fm->address == -1)
      {
         /* Callee constrained, but not caller. */
         half_line_num = arc_p->call_to->address - region_p->half_lines;
         while (half_line_num < 0)
            half_line_num += (2 * number_of_cache_lines());
         arc_p->call_fm->address = half_line_num;
         arc_p->constrained = 1;
      }
      else
      {
         /* both are constrained */
         arc_p->constrained = 1;
         return;
      }
   }

   /* Now find next unplaced outgoing arc from callee with highest weight */
   tmp_arc   = arc_p->call_to->call_out_list;
   heavy_arc = (call_graph_arc *) 0;
   while (tmp_arc)
   {
      if (!tmp_arc->constrained) {
         if (heavy_arc == (call_graph_arc *) 0)
            heavy_arc = tmp_arc;
         else {
            if (tmp_arc->weight > heavy_arc->weight)
               heavy_arc = tmp_arc;
         }
      }
      tmp_arc = tmp_arc->out_arc_next;
   }
   /* See if there's a heavier arc coming into the caller */
   tmp_arc = arc_p->call_fm->call_in_list;
   while (tmp_arc)
   {
      if (!tmp_arc->constrained) {
         if (heavy_arc == (call_graph_arc *) 0)
            heavy_arc = tmp_arc;
         else {
            if (tmp_arc->weight > heavy_arc->weight)
               heavy_arc = tmp_arc;
         }
      }
      tmp_arc = tmp_arc->in_arc_next;
   }

   /* If we found a heavy arc, go ahead and place it. */
   if (heavy_arc && heavy_arc->weight >= WEIGHT_THRESHOLD 
#if 0
       && ((double)heavy_arc->weight >= 0.5*(double)arc_p->weight) 
#endif
       && !heavy_arc->constrained)
      place_arc (heavy_arc);
}

static void
build_cache_table ()
{
   int i;
   int num_cache_half_lines;
   call_graph_node *node_p;

   num_cache_half_lines = 2 * number_of_cache_lines ();

   cache_table = (call_graph_node **) 
                   ldmalloc (sizeof (call_graph_node*) * num_cache_half_lines);

   memset (cache_table, 0, sizeof (call_graph_node*) * num_cache_half_lines);

   for (i = 0; i < HASH_TABLE_SIZE; i++) {
      node_p = cg_node_hash_table[i];
      while (node_p)
      {
         if (node_p->address != -1)
         {
            assert (node_p->address >= 0);
            assert (node_p->address <= num_cache_half_lines-1);
            node_p->next_cline = cache_table [node_p->address];
            cache_table [node_p->address] = node_p;
         }
         node_p = node_p->next;
      }
   }

   /* Now create the table that tells us when to skip over a cache line. */
   cache_half_line_is_empty = (int *) ldmalloc (sizeof(int) * num_cache_half_lines);
   for (i = 0; i < num_cache_half_lines; i++)
      cache_half_line_is_empty[i] = 0;

   /* fluff_p is a global ptr to the linked list of call_graph_nodes that */
   /* represent library functions. It is used in next_function so that */
   /* we don't have to traverse the entire list everytime looking for fluff */
   fluff_p = fluff;
}


/*
 * check_alignments - walk the list of call_graph_nodes and see if any
 * are preconstrained by alignment requirements.
 */

static void
check_alignments ()
{
   int i;
   unsigned int cache_alignment;
   call_graph_node *node_p;

   cache_alignment = cache_line_alignment ();
   for (i = 0; i < HASH_TABLE_SIZE; i++) {
      node_p = cg_node_hash_table[i];
      while (node_p)
      {
         /* If node has alignment requirements > cache alignment */
         if (node_p->section->alignment_power > cache_alignment)
         {
            node_p->address = 0;
         }
         node_p = node_p->next;
      }
   }
}

/*
 * place_functions - walk through the sorted list of call graph arcs
 * and place arcs if they have a corresponding call region and they haven't
 * been placed already.
 */

void
place_functions (arc_array)
call_graph_arc **arc_array;
{
   int i;

   check_alignments ();
  
   for (i = 0; i < cg_arc_cnt; i++)
   {
      call_graph_arc *arc_p = arc_array[i];
      if (arc_p->constrained == 0 && arc_p->call_region != (region *) 0)
         place_arc (arc_p);
   }
   build_cache_table ();
}


/* 
 * next_function - this function takes an address, and a pointer to a section/
 * file pair. This functions returns:
 *
 *   0 - If there are no more functions to place.
 *  -1 - If we are returning a valid section/filename to place at this address.
 *   x - where x is a positive integer signifies that we are to pad this many
 *       bytes.
 * Basically, this function tries to do 3 things:
 *   if we can place a function at the current address, do it.
 *   if there's fluff at the current address, place the fluff.
 *   else round up to the next cache line and try to place something there.
 */

int
next_function (addr, sec_p)
unsigned long addr;
lang_input_section_type *sec_p;
{
   int i;
   unsigned int line_num;
   call_graph_node *node_p;
   unsigned int num_cache_half_lines;
   unsigned int low_bits;

   /* If we get an address that is not cache half-line boundary aligned, */
   /* return enough padding to align to the next cache half-line boundary. */
   low_bits = (cache_line_size() >> 1) - 1;
   if (addr & low_bits)
      return ((addr | low_bits) - addr) + 1;

   line_num = cache_half_line (addr);
#if !RANDOM
   /* If doing random placement, everything resides in fluff. */
   
   /* If there's anything at this cache half-line number, place it there. */
   if (!cache_half_line_is_empty[line_num]) {
      node_p = cache_table[line_num];
      while (node_p) {
         if (!node_p->placed) {
            node_p->placed = 1;
            sec_p->section = node_p->section;
            sec_p->ifile   = node_p->file;
            return -1;
         }
         node_p = node_p->next_cline;
      }
   }
   cache_half_line_is_empty[line_num] = 1;
#endif

   /* If there's any fluff, place it there. */
   /* Note: fluff_p is a global that is initialized in build_cache_table; */
   /* this way, we don't have to traverse the entire list every time. */
   if (!fluff_is_empty) {
      while (fluff_p) {
         if (!fluff_p->placed) {
            fluff_p->placed = 1;
            sec_p->section = fluff_p->section;
            sec_p->ifile   = fluff_p->file;
            return -1;
         }
         fluff_p = fluff_p->next_fluff;
      }
   }
   fluff_is_empty = 1;

#if !RANDOM
   /* Else find next non-empty cache line, and pad out to that. */
   num_cache_half_lines = 2 * number_of_cache_lines();
   for (i = 0; i < num_cache_half_lines; i++)
   {
      line_num = (line_num + 1) % num_cache_half_lines;
      if (!cache_half_line_is_empty[line_num])
      {
         int num_bytes;
         /* How many bytes to pad to this cache half-line ? */
         num_bytes = (i + 1) * (1 << (log2_bytes_in_cache_line()/2) );
         return num_bytes;
      }
   }
#endif
   
   /* we're done */
   return 0;
}



#if DEBUG
static void
print_cache_table ()
{
   int i;
   int num_cache_lines;
   call_graph_node *node_p;

   num_cache_lines = number_of_cache_lines ();
   for (i = 0; i < num_cache_lines; i++) {
      node_p = cache_table[i];
      printf ("======================================================\n");
      printf ("Cache Line %d\n", i);
      while (node_p) {
         print_cg_node (node_p);
         node_p = node_p->next_cline;
      }
      printf ("======================================================\n");
   }
}

static void
print_arc (arc_p)
call_graph_arc *arc_p;
{
   printf ("\nCaller/Callee : %s(%s) = %d     \t%s(%s) = %d\n", 
                    arc_p->call_fm->section->name,
                    arc_p->call_fm->file->filename,
                    arc_p->call_fm->address,
                    arc_p->call_to->section->name,
                    arc_p->call_to->file->filename,
                    arc_p->call_to->address);
   printf ("Offset/Arc Wt : %u   %7.2f\n", arc_p->offset, arc_p->weight);
   

   if (arc_p->call_region)
      print_region (arc_p->call_region);
}

static void
print_cg_node (node_p)
call_graph_node *node_p;
{
   region *region_p;
   fprintf(stdout, "\nSection Name: %s\n", node_p->section->name);
   fprintf(stdout, "File Name:    %s\n", node_p->file->filename);
   region_p = node_p->region_list;
   while (region_p)
   { 
      print_region (region_p);
      region_p = region_p->next;
   }
}

static void
print_cg_node_hash_table()
{
   int i;
   call_graph_node *node_p;
 
   for (i = 0; i < HASH_TABLE_SIZE; i++) 
   {
      node_p = cg_node_hash_table[i];
      while (node_p) 
      {
         print_cg_node (node_p);
         node_p = node_p->next;
      }
   }
}

static void
print_fluff ()
{
   call_graph_node *node_p;

   node_p = fluff;
   while (node_p) {
      print_cg_node (node_p);
      node_p = node_p->next_fluff;
   }
}

static void
print_arc_array (arc_array)
call_graph_arc **arc_array;
{
   int i;
  
   printf ("Printing Sorted Arc Array:\n");
   for (i = 0; i < cg_arc_cnt; i++)
      print_arc (arc_array[i]);
}

#endif
#endif
