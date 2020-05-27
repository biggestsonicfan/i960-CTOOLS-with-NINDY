/*
    Link-time substitution and object module dependency analysis code.

    Copyright (C) 1995 Intel Corporation.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include "assert.h"

#include "cc_info.h"
#include "gcdm960.h"
#include "subst.h"
#include "i_toolib.h"

void
do_compile (p)
st_node* p;
{
  /* Append the group of commands for compiling *p onto a command file.
     If p is 0, the file is ready and we should go exec it.

     Each group is preceeded by a list of files to remove upon failure of
     of any command in the group.

     The removal list and the commands consist of a sequence of 
     0 seperated strings preceeded by the length in bytes of the sequence.

     An empty removal list (length==0) is allowed, but an empty command
     signals the end of the group.

     A removal list length of -1 indicates eof.
  */

  static char *src, *obj, *outz, *tmpz, *sinf, *itmp;
  static named_fd f;
  static int do_x960;

  int  ibuf[CI_X960_NBUF_INT];
  char *cbuf = (char*)(ibuf+1);
  int  i;

  if (f.fd == 0)
  {
    char *fname;
    fname = get_cmd_file_name();

    if (fname == 0)
    {
      fname = db_pdb_file(0,"gcdm960.com");
      do_x960 = 1;
    }
    else
      do_x960 = 0;

    dbf_open_write (&f, fname, db_fatal);
    db_new_removal (f.name);
  }

  assert (f.name);

  if (p == 0)
  {
    char *argv[6];
    int argc = 0;

    argv[argc++] = "cc_x960";
    argv[argc++] = f.name;

    if (flag_dryrun)
      argv[argc++] = "-dryrun";

    if (flag_noisy)
      argv[argc++] = "-v";

    argv[argc] = 0;

    { /* unlink the lock file */
      char* b = cbuf;

      /* first, we need an empty deletion list for this group */
      ibuf[0] = 0;
      dbf_write (&f, ibuf, sizeof (int));

      /* the 'group' is a single unlink command for the lock file */
      sprintf (b, "%s%c%s%c", "unlink",0, db_lock(),0);
      for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
      ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
      dbf_write (&f, ibuf, ibuf[0]+sizeof(int));/* ...and write the buffer.  */

      /* Terminate the command group */
      ibuf[0] = 0;
      dbf_write (&f, ibuf, sizeof(int));
    }

    /* -1 marks eof */
    ibuf[0] = -1;
    dbf_write (&f, ibuf, sizeof (int));
    dbf_close (&f);

    if (decision_file.name[0])
      dbf_close (&decision_file);

    fflush (stdout);
    fflush (stderr);

    if (do_x960)
      exit(db_x960(argc, argv));
    else
      return;
  }

  assert (db_is_subst(p) && db_sindex(p)>='0');

  if (flag_dryrun)
    return;

  { /* Calculate and write the removal list. */
    char* b = cbuf;

    /* Put the cc1 output and asm output into the buffer ... */
    sprintf (b, "%s%c%s%c", db_subst_name (&src,p,'s'),0,
                            db_subst_name (&obj,p,'o'),0);

    for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/

    /* Get names for the listing files.  To get the space management right,
       we set up the names and then truncate them if not needed. */

    db_subst_name (&tmpz,p,'z');
    db_subst_name (&sinf,p,'l');
    db_subst_name (&itmp,p,'i');
    db_oname (&outz,p);

    if (SUBST_IS_SINFO(p))
    { sprintf (b, "%s%c", sinf, 0);
      b+= strlen(b)+1;				/* ...update b for 1 args ...*/
    }
    else
      sinf[0] = '\0';

    if (SUBST_IS_CLIST(p))
    { /* Put the tmpz file name into the buffer, and finish outz name. */
      sprintf (b, "%s%c", tmpz, 0);
      b+= strlen(b)+1;				/* ...update b for 1 args ...*/
      strcat (outz, ".L");
    }
    else
    { /* Truncate the names, we don't need them. */
      tmpz[0] = '\0';
      outz[0] = '\0';
    }

    /* Put the input_tmp file into the buffer */
    sprintf (b, "%s%c", itmp, 0);
    b+= strlen(b)+1;				/* ...update b for 1 args ...*/

    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  { /* Calculate and write the cc1 command. */
    char* b = cbuf;
    static char* pdb;

    /* Get the subst list, starting just past the IVECT key ... */
    char *t = ((char*) (p->db_list[CI_SRC_SUBST_LIST]->text+4))+IVK_OFF+IVK_LEN;

    /* Get "cc1 -o src" into the buffer ...*/
    sprintf (b, "%s%c%s%c%s%c", db_cc_name(p),0, "-o",0, src,0);

    for (i=1; i<=3; i++) b+= strlen(b)+1;	/* ...update b for 3 args ...*/

    if (outz[0])
    { sprintf (b, "%s%c%s%c", "-outz",0, outz,0);
      for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    }

    if (tmpz[0])
    { sprintf (b, "%s%c%s%c", "-tmpz",0, tmpz,0);
      for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    }

    if (sinf[0])
    { sprintf (b, "%s%c%s%c", "-sinfo",0, sinf,0);
      for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    }

    /* ... parse the '+' separated substitution list into the buffer ... */
    while (*t)
    {
      t = find_cc1_opt (t);

      if (*t)
      { char* r = strchr (t,'+');
        int n;
  
        *b++ = '-';
  
        if (r != 0)
        { memcpy (b, t, n=r-t);
          b += n;
          *b++ = 0;
          t = (r+1);	/* Another switch, or '\0' */
        }
        else
        { memcpy (b, t, n=strlen(t)+1);
          b += n;
          t += (n-1);	/* Leave t pointing at '\0'; we are done. */
        }
      }
    }

    /* ... append -seek pdb offset itmp */
    sprintf (b, "%s%c%s%c%u%c%s%c", "-seek",0, db_pdb_file(&pdb,""),0,
                p->db_list[CI_SRC_CC1_LIST]->out_tell,0, itmp,0);

    for (i=1; i<=4; i++) b+= strlen(b)+1;	/* ...update b for 4 args ...*/
    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  if (sinf[0])
  { /* unlink the sinf file */
    char* b = cbuf;
    sprintf (b, "%s%c%s%c", "unlink",0, sinf,0);
    for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  if (tmpz[0])
  { /* unlink the tmpz file */
    char* b = cbuf;
    sprintf (b, "%s%c%s%c", "unlink",0, tmpz,0);
    for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  if (SUBST_ASM_PP(p))
  { /* preprocess the assembly */
    char* b = cbuf;
    sprintf (b, "%s%c%s%c", SUBST_ASM_PP(p),0, src,0);
    for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  { /* Calculate and write the asm command. */
    char* b = cbuf;

    /* Get the asm command list */
    db_list_node *l = p->db_list[CI_SRC_ASM_LIST];

    char* t = (char*) l->text+5;	/* point t at assembler base name ... */
    int   n = strlen (t)+1;		/* t+n points at remainder of args... */

    /* Get the tool name ... */
    sprintf (b, "%s%c", db_asm_name (t),0);
    for (i=1; i<=1; i++) b+= strlen(b)+1;	/* ...update b for 1 args ... */

    t += n;					/* get past asm base name ... */

    assert (l->size > n);
    if ((n=l->size-(n+1)) > 0)
    { memcpy (b, t, n);		/* copy in the remaining asm command args ... */
      b += n;
    }

    /* add "-o obj src" into the buffer ...*/
    sprintf (b, "%s%c%s%c%s%c", "-o",0, obj,0, src,0);

    for (i=1; i<=3; i++) b+= strlen(b)+1;	/* ...update b for 3 args ...*/
    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  if (db_genv2 ("KEEPDBASM") == 0)
  { /* unlink the src file */
    char* b = cbuf;
    sprintf (b, "%s%c%s%c", "unlink",0, src,0);
    for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  if (db_genv2 ("KEEPDBCPP") == 0)
  { /* unlink the itmp file */
    char* b = cbuf;
    sprintf (b, "%s%c%s%c", "unlink",0, itmp,0);
    for (i=1; i<=2; i++) b+= strlen(b)+1;	/* ...update b for 2 args ...*/
    ibuf[0]=b-cbuf;				/* ...calculate the size  ...*/
    dbf_write (&f, ibuf, ibuf[0]+sizeof(int));	/* ...and write the buffer.  */
  }

  /* Terminate the command group */
  ibuf[0] = 0;
  dbf_write (&f, ibuf, sizeof(int));
}

issue_compiles ()
{
  /* The database has been written out (if it's going to be).  Issue the
     compile commands we need to bring the object files in the pdb up to
     date. */

  st_node** lo = prog_db.extra->st_nodes+prog_db.extra->first[CI_SRC_REC_TYP];
  st_node** hi = prog_db.extra->st_nodes+prog_db.extra->lim[CI_SRC_REC_TYP];
  st_node** cur;

  /* Walk all current CI_SRC records ... */
  for (cur=lo; cur < hi; cur++)
    if (db_is_subst(*cur))
    { /* We want to talk to the user about archive:member */

      unsigned char* archive = (*cur)->db_list[CI_SRC_LIB_LIST]->text + 4;
      unsigned char* member  = (*cur)->db_list[CI_SRC_MEM_LIST]->text + 4;

      static char* name;

      db_subst_name (&name, *cur, 'o');
      fprintf (decision_file.fd,"'%s'", name);

      if (IS_COMPILE(*cur))
        fprintf (decision_file.fd, " will be built and substituted for ");
      else
        fprintf (decision_file.fd, " will be substituted for ");

      if (*archive)
        fprintf (decision_file.fd, "'%s:%s'\n", archive, member);
      else
        fprintf (decision_file.fd, "'%s'\n", member);

      if (IS_COMPILE(*cur))
        do_compile (*cur);
    }

  do_compile (0);
}

compile_objects (bad_files)
unsigned *bad_files;
{
  st_node** lo = prog_db.extra->st_nodes+prog_db.extra->first[CI_SRC_REC_TYP];
  st_node** hi = prog_db.extra->st_nodes+prog_db.extra->lim[CI_SRC_REC_TYP];
  st_node** cur, **next, **lim;

  /* Walk all current CI_SRC records, assigning disambiguation indices to
     disambiguate object file names which are identical in the 1st
     eight characters.  The disambiguation index is used to generate
     the first 2 chars of the 8.3 file extension, and the subst index
     generates the final char of the extension.  We have to be careful
     to reuse the existing indices when we are reusing object modules. */

  for (cur=lo; cur < hi; cur=lim)
  {
    char di_map[CI_MAX_OBJ_CONFLICT];
    static char *pbuf, *qbuf;

    int di;

    memset (di_map, 0, sizeof(di_map));

    next = cur;
    while (next<hi && !strcmp (db_oname(&pbuf,*cur), db_oname(&qbuf,*next)))
    { di = db_dindex (*next);
      assert (di < sizeof (di_map));
      di_map[di]++;
      next++;
    }
    lim = next;

    /* Now that we know the indices which are in use, go thru again
       allocating the remainder as tightly as possible. */

    di = 1;
    next = cur;
    while (next < lim)
    { src_extra* extra = (src_extra*) (*next)->extra;
      int index = extra->dope_index - prog_db.extra->first[CI_SRC_REC_TYP];

      if (db_dindex(*next) == 0)
      { while (di_map[di] != 0)
        { di++;
          assert (di < sizeof (di_map));
        }
        db_set_dindex (*next, di);
      }

      if (db_is_subst(*next))
      { static char* name;
        db_subst_name (&name, *next, 'o');

        /* Unlink the files that are out of date before we write
           pass2.db; otherwise, we might inadvertantly accept
           out of date files on a future rerun if the compiles don't
           make it. */

        if (TEST_FSET(bad_files, index))
        { SET_IS_COMPILE(*next);
          if (!flag_dryrun)
            db_unlink (name, db_fatal);
        }
        else
          if (db_access_rok(name))
            CLR_IS_COMPILE(*next);

          else
          { SET_IS_COMPILE(*next);
            db_warning ("missing pdb file '%s' will be rebuilt", name);
            if (!flag_dryrun)
              db_unlink (name, db_fatal);
          }
      }
      else
        CLR_IS_COMPILE(*next);

      next++;
    }
  }
}

