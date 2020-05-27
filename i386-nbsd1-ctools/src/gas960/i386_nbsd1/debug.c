/* Routines for debug use only.  Don't link into product.
 */

/* $Id: debug.c,v 1.5 1993/09/28 16:58:11 peters Exp $ */

#include "as.h"

extern segS	*segs;		/* Internal segment "array" */
extern int  	curr_seg;	/* Active segment (index into segs[]) */
extern int  	segs_size;	/* Number of segS's currently in use in segs */
extern int  	max_segs;	/* Total number of segS's available in segs */

dmp_frag( fp, indent )
    struct frag *fp;
    char *indent;
{
	for ( ; fp; fp = fp->fr_next ){
		printf("%sFRAGMENT @ 0x%x\n", indent, fp);
		switch( fp->fr_type ){
			case rs_align:
				printf("%srs_align(%d)\n",indent, fp->fr_offset);
				break;
			case rs_fill:
				printf("%srs_fill(%d)\n",indent, fp->fr_offset);
				printf("%s", indent);
				var_chars( fp, fp->fr_var + fp->fr_fix );
				printf("%s\t repeated %d times,",
							indent, fp->fr_offset);
				printf(" fixed length if # chars == 0)\n");
				break;
			case rs_org:
				printf("%srs_org(%d+sym @0x%x)\n",indent,
					fp->fr_offset, fp->fr_symbol);
				printf("%sfill with ",indent);
				var_chars( fp, 1 );
				printf("\n");
				break;
			case rs_machine_dependent:
				printf("%smachine_dep\n",indent);
				break;
			default:
				printf("%sunknown type\n",indent);
				break;
		}
		printf("%saddr=%d(0x%x)\n",indent,fp->fr_address,fp->fr_address);
		printf("%sfr_fix=%d\n",indent,fp->fr_fix);
		printf("%sfr_var=%d\n",indent,fp->fr_var);
		printf("%sfr_offset=%d\n",indent,fp->fr_offset);
		printf("%schars @ 0x%x\n",indent,fp->fr_literal);
		printf("\n");
	}
}

var_chars( fp, n )
    struct frag *fp;
    int n;
{
	unsigned char *p;

	for ( p=(unsigned char*)fp->fr_literal; n; n-- , p++ ){
		printf("%02x ", *p );
	}
}


/* Dump the segs array
 */
dmp_segs ()
{
	int	i;
	char	*fmt = "%7s%7s%7s%7s%7s%7s%8s%8s%8s%8s\n\n";
	printf (fmt, "num", "name", "flags", 
		"type", "size", "align", "fix_R", "fix_L", "frag_R", "frag_L");
	for ( i = 9; i < segs_size; ++i )
		dmp_seg (i);
}


/* Dump one segment struct; keep format string consistent with 
 * dmp_segs' column header above.  Draw a small arrow on left 
 * of the current segment as defined by "curr_seg" global var.
 */

int
dmp_seg (i)
	int i;
{
	segS	*sp = & segs [i];
	char	*fmt = "%2s%5d%7s%7d%7d%7d%7d%8x%8x%8x%8x\n";

		printf (fmt, 
		i == (int) curr_seg ? "=>" : "",
		sp->seg_no, 
		sp->seg_name[0] ? sp->seg_name : "<null>",
		sp->seg_flags,
		sp->seg_type,
#ifdef	OBJ_COFF
		sp->seg_scnhdr ? sp->seg_scnhdr->s_size : 0,
		sp->seg_scnhdr ? sp->seg_scnhdr->s_align : 0,
#else	/* b.out */
		sp->seg_size,
		sp->seg_align,
#endif
		sp->seg_fix_root,
		sp->seg_fix_last,
		sp->seg_frag_root,
		sp->seg_frag_last);
	
}
