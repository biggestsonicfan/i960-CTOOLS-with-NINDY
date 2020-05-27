
#ifdef PROCEDURE_PLACEMENT
#include "callsite.h"


call_list *call_list_hash_table[HASH_TABLE_SIZE];



void
print_hash_table ()
{

   int i;
   call_list *clist_p;
   call_site *csite_p;

   for (i = 0; i < HASH_TABLE_SIZE; i++)
   {
      clist_p = call_list_hash_table[i];
      while (clist_p)
      {
         printf ("Caller/Callee: %s(%s) - %s(%s) \n",
                  clist_p->caller_file->filename,
                  clist_p->caller_sec->name,
                  clist_p->callee_file->filename,
                  clist_p->callee_sec->name);
         csite_p = clist_p->call_site_list;

         while (csite_p)
         {
            printf ("\t\tsite num -- weight -- profiled -- visitied\n\t\t %7d %9d %11d %11d\n",
                     csite_p->number,
                     csite_p->weight,
                     csite_p->profiled,
                     csite_p->visited);
            csite_p = csite_p->next;
         }
         printf ("\n\n");
         clist_p = clist_p->next;
      }
   }
}

static
free_call_list (csite_p)
call_site *csite_p;
{
   call_site *next_csite_p;
   while (csite_p)
   {
      next_csite_p = csite_p->next;
      free (csite_p);
      csite_p = next_csite_p;
   }
}


void
free_call_list_hash_table ()
{
   int i;
   call_list *clist_p;

   for (i = 0; i < HASH_TABLE_SIZE; i++)
   {
      clist_p = call_list_hash_table[i];
      while (clist_p)
      {
         call_list *next_clist_p = clist_p->next;
         free_call_list (clist_p->call_site_list);
         free (clist_p);
         clist_p = next_clist_p;
      }
   }
}


call_list *
new_call_list(in_file, in_sec, out_file, out_sec)
lang_input_statement_type *in_file;
asection *in_sec;
lang_input_statement_type *out_file;
asection *out_sec;
{
   call_list  *clist_p;

   clist_p = (call_list *) ldmalloc (sizeof (call_list));
   clist_p->next             = (call_list *) 0;
   clist_p->caller_file      = in_file;
   clist_p->caller_sec       = in_sec;
   clist_p->callee_file      = out_file;
   clist_p->callee_sec       = out_sec;
   clist_p->call_site_list   = (call_site *) 0;

   /* Now create the numero uno call_site and stick it on the list. */
   return clist_p;
}


call_list *
lookup_call_site (in_file, in_sec, out_file, out_sec)
lang_input_statement_type *in_file;
asection *in_sec;
lang_input_statement_type *out_file;
asection *out_sec;
{
   int hash_val;
   call_list *clist_p;

   hash_val = hash_strings (in_file->filename, in_sec->name, 
                            out_file->filename, out_sec->name);

   clist_p = call_list_hash_table[hash_val];
   while (clist_p)
   {
      if (clist_p->caller_sec == in_sec && clist_p->callee_sec == out_sec 
         && clist_p->caller_file == in_file && clist_p->callee_file == out_file)
      {
         return clist_p;
      }
      clist_p = clist_p->next;
   }
   return (call_list *) 0;
}


call_list *
insert_or_lookup_call_site (in_file, in_sec, out_file, out_sec)
lang_input_statement_type *in_file;
asection *in_sec;
lang_input_statement_type *out_file;
asection *out_sec;
{
   int hash_val;
   call_list *clist_p;

   hash_val = hash_strings (in_file->filename, in_sec->name, 
                            out_file->filename, out_sec->name);

   clist_p = call_list_hash_table[hash_val];

   while (clist_p)
   {
      if (clist_p->caller_sec == in_sec && clist_p->callee_sec == out_sec 
         && clist_p->caller_file == in_file && clist_p->callee_file == out_file)
      {
         return clist_p;
      }
      clist_p = clist_p->next;
   }
   clist_p = new_call_list (in_file, in_sec, out_file, out_sec);
   clist_p->next = call_list_hash_table[hash_val];
   call_list_hash_table[hash_val] = clist_p;
   return (clist_p);
}

call_site *
get_next_call_site (in_file, in_sec, out_file, out_sec)
lang_input_statement_type *in_file;
asection *in_sec;
lang_input_statement_type *out_file;
asection *out_sec;
{
   call_list *clist_p;
   call_site *csite_p;

   clist_p = lookup_call_site (in_file, in_sec, out_file, out_sec);
   if (clist_p)
   {
      csite_p = clist_p->call_site_list;
      while (csite_p != (call_site *) 0) 
      {
         if (csite_p->visited == 0)
         {
            csite_p->visited == 1;
            return csite_p;
         }
         csite_p = csite_p->next;
      }
   }

   return (call_site *) 0;
}

#endif
