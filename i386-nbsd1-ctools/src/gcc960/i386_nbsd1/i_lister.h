extern char* sinfo_name;
extern FILE* sinfo_file;
extern int sinfo_top;

/* Each #line directive causes a new sinfo_map entry to be
   made, which records the information we need to associate the
   source which follows the note with it's information in the
   sinfo file. */

typedef
  struct
  { int si_line;	/* Sequence number of this note.  For top
			   entry, this is current line sequence number */

    int si_tell;	/* Position in sinfo file of this note.  For top
			   entry, this is current column position. */
  }
  sinfo_map_rec;

extern sinfo_map_rec* sinfo_map;

#define MAP_LINE(S)   sinfo_map[(S)].si_line
#define MAP_LPOS(S)   sinfo_map[(S)].si_tell

#define INC_LIST_LINE ( input_redirected() ? 0 : (sinfo_map[sinfo_top].si_line++) )
#define DEC_LIST_LINE ( input_redirected() ? 0 : (sinfo_map[sinfo_top].si_line--) )

