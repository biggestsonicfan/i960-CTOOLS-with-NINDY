/*****************************************************************************
 * 		Copyright (c) 1992, Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and 
 * distribute this software and its documentation.  Intel grants
 * this permission provided that the above copyright notice 
 * appears in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation.  In
 * addition, Intel grants this permission provided that you
 * prominently mark as not part of the original any modifications
 * made to this software or documentation, and that the name of 
 * Intel Corporation not be used in advertising or publicity 
 * pertaining to distribution of the software or the documentation 
 * without specific, written prior permission.  
 *
 * Intel Corporation does not warrant, guarantee or make any 
 * representations regarding the use of, or the results of the use
 * of, the software and documentation in terms of correctness, 
 * accuracy, reliability, currentness, or otherwise; and you rely
 * on the software, documentation and results solely at your own risk.
 *****************************************************************************/


/*****************************************************************************
 *
 * This file dumps the contents of a COFF symbol table to stdout.
 * It is in libbfd so it can share COFF internals, but is in a file by
 * itself because only gdmp960 actually needs to link it.
 *
 * The dumping is done on the raw symbol table entries associated with the
 * bfd, rather than on the canonical symbols, because developers (at least)
 * need to be able to see the actual detailed symbol info in the object
 * files created by the tools.
 *
 * However, even the raw symbol entries have been somewhat massaged by BFD
 * when the table was read, and so we "unmassage" (non-destructively) some
 * information before displaying it.  E.g.:
 *
 *	o All references from one symbol to another in the table have been
 *	  converted from symbol table indices to pointers.  Change them back
 *	  to indices before displaying.
 *
 *	o *ALL* symbol names have been moved out of the symbols and pointers
 *	  to them have been placed in the symbols.  We have to keep track of
 *	  (and display) what the string table offsets would be if the names
 *	  longer than 8 characters were in the string table and the shorter
 *	  ones were still in the symbol entries.
 *
 *	o The bal entry points of leaf procedures have been converted to
 *	  offsets from the call entry point.  Convert them back to absolute
 *	  values before showing them.
 *
 *****************************************************************************/

#include <stdio.h>
#include <time.h>
#include "bfd.h"
#include "libbfd.h"
#include "coff.h"
#include "libcoff.h"
#include "icoffdmp.h"

/* Forward declarations are for gcc960 compiler. */

static is_section_name();
static char **sections;		/* list of section names */
static int * aux_types();
static build_sect_list();
static dmp_type();
static dmp_sclass();
static dmp_aux();

int
coff_dmp_symtab( abfd, suppress_headers, suppress_translation, section_number )
	bfd 	*abfd;
	int	suppress_headers;	/* 1 = Just the facts, ma'am */
	int	suppress_translation;	/* 1 = Print type, sclass in hex */
	int	section_number;		/* -1 = Print all symbols;
					 * >= 0 = Print symbols from this 
					 * section only 
					 */
{
	combined_entry_type *symp;
	int i;
	combined_entry_type *symtab_base;

	if (!coff_slurp_symbol_table(abfd)){
		return 0;
	}

	build_sect_list(abfd);
	symtab_base = symp = obj_raw_syments(abfd);
	if ( ! suppress_headers )
	{
	 /*--------------------
	 * PRINT HEADER
	 *--------------------*/
	bfd_center_header( "SYMBOL TABLE INFORMATION" );
	puts("[Index]\tm1 Name/Offset Value     Scnum   Flags  Type    Sclass  Numaux  Name");
	puts("[Index]\ta0             Word1      Short1 Short2 Short3 Short4 Short5 Short6  Tv");
	puts("[Index]\ta1             Fname");
	puts("[Index]\ta2             Tagndx      Fsize       Lnnoptr     Endndx      Tvndx");
	puts("[Index]\ta3             Scnlen      Nreloc  Nlinno");
	puts("[Index]\ta4             Tagndx Lnno Size    Dim[0]  Dim[1]  Dim[2]  Dim[3]");
	puts("[Index]\ta5             Identification String  Date/Time\n");
	}
	/*----------------------------------------
	 * EXECUTE LOOP ONCE PER MAIN SYMBOL
	 * ENTRY IN TABLE.
	 *----------------------------------------*/
	for ( i=0 ; i < coff_data(abfd)->raw_syment_count;  i++, symp++ ){
		SYMENT *p;
		int n;
		char * symname;

		p = &symp->u.syment;
		if ( (section_number >= 0) && (p->n_scnum != section_number) ) {
		    i += p->n_numaux;
		    symp += p->n_numaux;
		    continue;
		}

		printf( "[%d]\tm1  ", i );

		symname = (p->n_sclass == C_FILE) ? ".file" : p->n_ptr;
		n = strlen( symname );

		if ( n <= SYMNMLEN ){
			printf( "%-8s", symname );
		} else {
		    /* Un-pointerize the symbol name.  Turn it into its n_offset value. */
			printf( "0x%-6x", 4 + (((unsigned long)symname) -
			       ((unsigned long) coff_data(abfd)->string_table)));
		}

		printf("  0x%08x%6d  0x%04x ",
					p->n_value, p->n_scnum, p->n_flags);
		dmp_type(p->n_type, suppress_translation);
		dmp_sclass(p->n_sclass, suppress_translation);
		printf( "%d  %s\n", p->n_numaux, symname );


		/*----------------------------------------
		 * DISPLAY ALL AUX ENTRIES FOR THIS SYMBOL
		 *----------------------------------------*/
		if ( p->n_numaux ){
			int * aux_needed;
			int j;
			long save_bal_addr;
			AUXENT *balaux;

			balaux = 0;

			if ((p->n_sclass == C_LEAFEXT)
			||  (p->n_sclass == C_LEAFSTAT) ){
				/* Convert bal address of leafproc from an
				 * offset from call address back to abs value
				 */
				balaux = &(symp+2)->u.auxent;
				save_bal_addr = balaux->x_bal.x_balntry;
				balaux->x_bal.x_balntry += p->n_value;
			}

			aux_needed = aux_types( p );
			j= i+symp->u.syment.n_numaux;
			while ( i < j ){
				i++;
				symp++;
				printf("[%d]\t", i);
				dmp_aux( symp, *aux_needed, p->n_ptr, symtab_base);
				if ( *aux_needed != -1 ){
					aux_needed++;
				}
			}
			if ( balaux ) {
				/* Restore bal address of leafproc */
				balaux->x_bal.x_balntry = save_bal_addr;
			}
		}
	}
	putchar('\n');
	return 1;
}


/*
 * Given a pointer to a COFF main symbol entry, this routine determines
 * what kinds of auxiliary entry or entries can go with it.  It returns
 * a pointer to an array of codes indicating the anticipated aux entries
 * in their anticipated order.  The table is terminated with A_ERROR:
 * if it is encountered before the end of the symbol's aux entries, the
 * additional entries should be flagged as garbage.
 */
static
int *
aux_types( symp )
    SYMENT *symp;
{
	static int auxs_needed[10]; /* This is the array of codes */
	int *p;		/* Pointer into auxs_needed	*/
	int type;	/* Symbol type			*/
	int btype;	/* Symbol basic type		*/
	char *symname;	/* Symbol name			*/

	p = auxs_needed;

	switch ( symp->n_sclass )
	{
	case C_FILE:
		/* FILE NAME */
		*p++ = A_FILENAME;
		*p++ = A_IDENT;		/* Optional */
		*p++ = A_IDENT;		/* Optional */
		break;
	case C_STRTAG:	/* TAG NAME */
	case C_UNTAG:	/* TAG NAME */
	case C_ENTAG:	/* TAG NAME */
	case C_EOS:	/* END OF STRUCTURE */
		*p++ = A_TAG;
		break;
	case C_BLOCK:	/* BLOCK BEGIN OR END */
	case C_FCN:	/* FUNCTION BEGIN OR END */
	case C_FIELD:	/* BIT FIELD */
		*p++ = A_CATCHALL;
		break;
	case C_SCALL:
	case C_LEAFEXT:
	case C_LEAFSTAT:
		/* LEAF OR SYSTEM PROCEDURE */
		*p++ = A_FUNCTION;
		*p++ = A_FUNCTION;
		break;
	default:
		symname = symp->n_ptr;
		type = symp->n_type;
		btype = BTYPE(type);

		/* ORDER OF CHECKING IS IMPORTANT HERE!
		 * Must check for function or array
		 * *before* looking at basic type.
		 * (E.g., a function returning
		 * struct should be treated as a
		 * function, not a struct.)
		 */
		if ( ISFCN(type) )
		{
			/* FUNCTION NAME */
			*p++ = A_FUNCTION;
		} 
		else if ( ISARY(type) )
		{
			/* ARRAY NAME */
			*p++ = A_ARRAY;
		} 
		else if ( (btype == T_STRUCT)
			 || (btype == T_UNION)
			 || (btype == T_ENUM) )
		{
			/* STRUCTURE, UNION, OR ENUMERATION */
			*p++ = A_TAG;
		} 
		else if ( is_section_name(symname) )
		{
			/* SECTION NAME */
			*p++ = A_SECTION;
		}
		else
		{
			/* Everything else */
			*p++ = A_CATCHALL;
		}
		break;
	}

	*p++ = A_ERROR;
	return auxs_needed;
}


static
dmp_aux( symp, auxtype, symname, symtab_base )
    combined_entry_type *symp;
    int auxtype;
    char *symname;
    combined_entry_type *symtab_base;
{
	AUXENT *auxp;
	int endndx;
	int tagndx;

	auxp = &symp->u.auxent;


	/*----------------------------------------------
	 * CONVERT POINTERS TO SYMTAB INDICES, IF NEEDED
	 *----------------------------------------------*/
	if ( symp->fix_end ){
		endndx = auxp->AUX_ENDNDX;
		auxp->AUX_ENDNDX =
		((combined_entry_type *)auxp->AUX_ENDNDX - symtab_base);
	}
	if ( symp->fix_tag ){
		tagndx = auxp->AUX_TAGNDX;
		auxp->AUX_TAGNDX = 
		((combined_entry_type *)auxp->AUX_TAGNDX - symtab_base);
	}

	switch ( auxtype ){
	case A_CATCHALL:
		printf( "a0            0x%08x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x    %d\n",
			auxp->AUX_TAGNDX,
			auxp->AUX_LNNO,
			auxp->AUX_SIZE,
			auxp->AUX_DIMEN[0],
			auxp->AUX_DIMEN[1],
			auxp->AUX_DIMEN[2],
			auxp->AUX_DIMEN[3],
			auxp->x_sym.x_tvndx
		);
		break;

	case A_FILENAME:
		printf( "a1     %14s\n", symname );
		break;

	case A_FUNCTION:
		printf( "a2   0x%08x  ---      0x%08x  0x%08x  0x%08x     %d\n",
			auxp->AUX_TAGNDX,
			auxp->x_sym.x_misc.x_fsize,
			auxp->AUX_LNNOPTR,
			endndx = auxp->AUX_ENDNDX,
			auxp->x_sym.x_tvndx
		);
		break;
	case A_TAG:
		printf("a2            0x%08x  0x%08x  0x%08x  0x%08x     %d\n",
		       auxp->AUX_TAGNDX,
		       auxp->AUX_SIZE,
		       auxp->AUX_LNNOPTR,
		       auxp->AUX_ENDNDX,
		       auxp->x_sym.x_tvndx);
		break;
	case A_SECTION:
		printf( "a3            0x%08x  0x%04x  0x%04x\n",
			auxp->x_scn.x_scnlen,
			auxp->x_scn.x_nreloc,
			auxp->x_scn.x_nlinno
		);
		break;

	case A_ARRAY:
		printf( "a4   %10d   0x%04x  0x%04x  0x%04x  0x%04x  0x%04x  0x%04x\n",
			auxp->AUX_TAGNDX,
			auxp->AUX_LNNO,
			auxp->AUX_SIZE,
			auxp->AUX_DIMEN[0],
			auxp->AUX_DIMEN[1],
			auxp->AUX_DIMEN[2],
			auxp->AUX_DIMEN[3],
			auxp->x_sym.x_tvndx
		);
		break;

	case A_IDENT:
		printf( "a5               %-20s  %s",
			auxp->x_ident.x_idstring,
			ctime(&auxp->x_ident.x_timestamp)
		);
		break;

	default:
		printf( " a?        *********   GARBAGE   *********\n" );
		break;
	}

	/*-------------------------------------------
	 * CONVERT SYMTAB INDICES BACK TO POINTERS
	 *-------------------------------------------*/
	if ( symp->fix_end ){
		auxp->AUX_ENDNDX = endndx;
	}
	if ( symp->fix_tag ){
		auxp->AUX_TAGNDX = tagndx;
	}
}


/*
 * is_section_name()
 * Checks a string against a list of section names.  
 * Returns 1 if the string matches any of the section names,
 * else returns 0.
 *
 * The sections array must already exist or this function 
 * will always return 0.
 */
static 
is_section_name(name)
	char	*name;
{
	register char	**c = sections;
	if ( c == NULL ) return 0;
	for ( ; *c; ++c )
		if ( ! strcmp(name, *c) )
			return 1;
	return 0;
}

#define LINE_WIDTH	80	/* for centering within a line */

/*
 * bfd_center_header()
 * 
 * Print a string, centered within the output line.
 * The incoming string must not end in newline because it gets surrounded
 * by "***" diddles. 
 *
 * The 8 in the expression below is for the 2 *** diddles plus a 
 * space on either side of the main string.
 */
void
bfd_center_header( str )
	CONST char	*str;
{
	int	left_margin = (LINE_WIDTH - strlen(str) - 8) / 2;
	char	centering_fmt[20];
	sprintf (centering_fmt, "\n%%%ds %%s %%s\n", left_margin > 0 ? left_margin : 1);
	printf (centering_fmt, "***", str, "***");
}


static
dmp_type(type, suppress_translation)
	unsigned long 	type;
	int		suppress_translation;
{
	char *t1;

	if ( ISPTR(type) ){		t1 = " *";
	} else if ( ISFCN(type) ){	t1 = "()";
	} else if ( ISARY(type) ){	t1 = "[]";
	} else {			t1 = "  ";
	}

	if ( suppress_translation )
		printf ("  0x%04x", type);
	else
		printf("%s%-6s", t1, TYPE_STR( BTYPE(type ) ));
}



static
dmp_sclass(sclass, suppress_translation)
    char sclass;
{
	if ( suppress_translation )
		printf ("  0x%02x      ", sclass);
	else
		printf( "  %-10s", SCLASS_STR(sclass) );
}


int
coff_dmp_linenos(abfd, suppress_headers)
	bfd 	*abfd;
	int	suppress_headers;	/* Just the facts, ma'am */
{
	sec_ptr secp;	/* pointer to bfd section descriptor */
	alent *linep;
	unsigned int n;

	if (!coff_slurp_symbol_table(abfd)){
		return 0;
	}
	
	if ( ! suppress_headers )
	{
		bfd_center_header( "LINE NUMBER INFORMATION" );
		printf( "\tSymndx/Paddr  Lnno     Name\n\n" );
	}

	for ( secp = abfd->sections; secp; secp = secp->next){
		printf( "\n%s:\n", bfd_section_name(abfd,secp) );
		linep = secp->lineno;
		for ( n = secp->lineno_count; n != 0; n--, linep++ ){

			if ( linep->line_number == 0 ){
				/* A line number of 0 indicates the start of
				 * a function.  Instead of an address, a symbol
				 * index is stored.  Display the symbol
				 * (function) name and the index of the symbol
				 * in the symbol table.  Note that we must
				 * convert from a pointer to the symbol to
				 * an offset in the raw symbol table.
				 */
				int index;
				asymbol *symp;

				symp = linep->u.sym;
				index = fetch_native_coff_info(symp)
					- obj_raw_syments(abfd);
				printf( "\t %10d       0\t%s\n",
							index, symp->name );
			} else {
				printf( "\t 0x%08x  %6d\n",
					linep->u.offset, linep->line_number );
			}
		}
	}
	return 1;
}


/* Return a character corresponding to the symbol class of sym
 */
static
char
decode_symclass (sym)
    asymbol *sym;
{
        flagword flags = sym->flags;

        if (flags & BSF_FORT_COMM) return 'C';
        if (flags & BSF_UNDEFINED) return 'U';
        if (flags & BSF_ABSOLUTE)  return 'A';

        if ( (flags & BSF_GLOBAL) || (flags & BSF_LOCAL) ){
                if ( !strcmp(sym->section->name, ".text") ){
                        return 'T';
                } else if ( !strcmp(sym->section->name, ".data") ){
                        return 'D';
                } else if ( !strcmp(sym->section->name, ".bss") ){
                        return 'B';
                } else {
                        return 'O';
                }
        }
        return '?';
}

/* 
 * build section name list 
 * Make sure the LAST entry in the list is always NULL for fast
 * searching later.
 */
static
build_sect_list(abfd)
	bfd *abfd;
{
	sec_ptr secp;
	int i = 0;

	if ( sections ) bfd_release(abfd,sections);
	sections = (char **) bfd_alloc (abfd,(bfd_count_sections(abfd) + 1) * sizeof(char *));
	for (secp = abfd->sections; secp; secp = secp->next) 
	{
		sections[i++] = (char *) bfd_section_name(abfd, secp);
	}
	sections[i] = NULL;
}


/* 
 * Concat str1 and str2 and leave result in str2 
 */
static concat(str1, str2)
	char *str1;
	char *str2;  
{
	int s1 = strlen(str1);
	int s2 = strlen(str2);

	for ( ; s2 >= 0; --s2)
		str2[s1 + s2] = str2[s2];

	for ( s2 = 0; s1 > 0; --s1, ++s2)
		str2[s2] = str1[s2];
}


/* 
 * Display size and lineno of symbol in full format 
 */
static *display_size_full_fmt(symp, auxtype, num_base)
	combined_entry_type *symp;
	int auxtype;
	base_type num_base;
{
	AUXENT *auxp = &symp->u.auxent;
	SYMENT *S = &symp->u.syment;

	switch ( auxtype ) {
	case A_FUNCTION:
		if (auxp->x_sym.x_misc.x_fsize != 0L) {
			fprintf(stdout, fmt_fsize[(int) num_base], 
				auxp->x_sym.x_misc.x_fsize);
		} else {
                        fprintf(stdout, fmt_nosize[(int) num_base]);
                }

                fprintf(stdout, "|     ");
		break;

	case A_ARRAY:
	default:
		if (auxp->AUX_SIZE != 0 ) {
		        fprintf(stdout, fmt_size[(int)num_base], auxp->AUX_SIZE);
		} else {
			fprintf(stdout, fmt_nosize[(int)num_base]);
		}

		if (auxp->AUX_LNNO != 0) {
			fprintf(stdout, "|%5d", auxp->AUX_LNNO);
		} else {
			fprintf(stdout, "|     ");
		}
	}
 }


/* Returns an accurate ordered character array
   representing the C decl for the coff type t. */

static char * derived_types(t,p,hp,dim)
    int t      /* coff n_type */,
        p      /* passed a ptr. */;
    char *hp;  /* highest precedence type string. */
    int *dim;  /* array dimensions. */
{
    char *r                                   /* r is return value . */ ,
      *nhp = (char *) malloc(strlen(hp) + 15) /* nhp is new hp string. */;

    if ( ISPTR(t) ) {
	sprintf(nhp,"*%s",hp);
	r = derived_types(DECREF(t),1,nhp,dim);
	free(nhp);
	return r;
    }
    else if (ISFCN(t) ) {
	if (p)
		sprintf(nhp,"(%s)()",hp);
	else
		sprintf(nhp,"%s()",hp);
	r = derived_types(DECREF(t),0,nhp,dim);
	free(nhp);
	return r;
    }
    else if (ISARY(t) ) {
	if (dim[0]) {
	    if (p)
		    sprintf(nhp,"(%s)[%d]",hp,dim[0]);
	    else
		    sprintf(nhp,"%s[%d]",hp,dim[0]);
	    r = derived_types(DECREF(t),0,nhp,dim+1);
	}
	else {
	    if (p)
		    sprintf(nhp,"(%s)[]",hp);
	    else
		    sprintf(nhp,"%s[]",hp);
	    r = derived_types(DECREF(t),0,nhp,dim);
	}
	free(nhp);
	return r;
    }
    else {
	r = (char *) malloc(strlen(hp) + 1);

	strcpy(r,hp);
	free(nhp);
	return r;
    }
}

/*
 *  Displays type of a symbol, including types of any auxiliary entries.
 */
static
void display_type(symp, num_base)
	combined_entry_type *symp;
        base_type num_base;
{
    int i;
    char type_string[MAX_TYPELEN]; 
    char work[MAX_TYPELEN];
    unsigned long type;
    SYMENT *S = &symp->u.syment;
    AUXENT *auxp = &symp->u.auxent;

    type = S->n_type;
    strcpy(type_string,TYPE_STR( BTYPE(type) ) );

    if ((type == T_STRUCT || type == T_UNION || type == T_ENUM) && 
	!ISTAG(S->n_sclass) &&  (S->n_numaux)) {
	SYMENT *aux_sym;

	symp++;
	auxp = &symp->u.auxent;
	if (auxp->AUX_TAGNDX) {
	    aux_sym = &((combined_entry_type *)auxp->AUX_TAGNDX)->u.syment;
	    if (strlen(aux_sym->n_ptr) > 7)
		    sprintf(work, "-%.7s*", aux_sym->n_ptr);
	    else
		    sprintf(work, "-%.8s", aux_sym->n_ptr); 
	    strcat(type_string, work);
	}
    }

    if (1) {
	int i;
	int dimensions[4];
	char *dt,*r;

	memset(dimensions,0,sizeof(dimensions));
	if (S->n_numaux) {
	    symp++;
	    auxp = &symp->u.auxent;
	    for (i=0; i<4; i++)
		    if (auxp->AUX_DIMEN[i])
			    dimensions[i] = auxp->AUX_DIMEN[i];
	}
	dt = derived_types(type,0,"",dimensions);
	r = (char *) malloc(strlen(type_string) + strlen(dt) + 2);
	sprintf(r,(strcmp("",dt) == 0) ? "%s" : "%s %s",type_string,dt);
	printf(fmt_type[num_base],r);
	free(dt);
	free(r);
    }
}


/* 
 *display symbols in full format. This include class,
 * type, size, and section information.
 */
int coff_dmp_full_fmt( abfd, syms, symcount, flags )
	bfd *abfd;
	asymbol **syms;
	unsigned long symcount;
	char flags;
{
	int i;
        base_type num_base = hexadecimal;

	AUXENT *auxp;

	char uflag = flags & F_UNDEFINED;
	char hflag = flags & F_HEADER;
	char rflag = flags & F_PREPEND;
	char Tflag = flags & F_TRUNCATE;

	char name_field[MAX_NAME];

	/* print header */
	if (!uflag && !hflag) {
		fprintf(stdout, fmt_head[(int)num_base]);
	}

	/* determine number base */
	if (flags & F_DECIMAL) num_base = decimal;
	if (flags & F_HEXADECIMAL) num_base = hexadecimal;
	if (flags & F_OCTAL) num_base = octal;
       
	build_sect_list(abfd);
		
	/* loop thru the main symbol entry in table */
	for (i=0 ; i < symcount ; i++) {
		SYMENT *s;
		int fn_len;
		coff_symbol_type *cs = coffsymbol(syms[i]);

		s = &fetch_native_coff_info(&cs->symbol)->u.syment;
		auxp = &fetch_native_coff_info(&cs->symbol)->u.auxent;

		/* externals only ? */
		if ((flags & F_EXTERNAL) && 
		    (s->n_sclass != C_STAT) && (s->n_sclass != C_EXT) &&
		    (s->n_sclass != C_SCALL) && (s->n_sclass != C_LEAFEXT))
			continue;

		/* only display .text, .data, .bss when full flag is on */
		if (!(flags & F_FULL) &&
			(s->n_sclass == C_STAT) && ((!strcmp(s->n_ptr, ".text") ||
						     !strcmp(s->n_ptr, ".data") ||
						     !strcmp(s->n_ptr, ".bss")))) {
			continue;
		}
		strncpy(name_field, bfd_get_filename(abfd), MAX_NAME);
						     
		if (rflag) {
			if (Tflag) {
				fn_len = 19 - strlen (s->n_ptr); 
				if (fn_len > 0) {
					if(strlen(name_field) > fn_len) {
						name_field[fn_len-1] = '*';
						name_field[fn_len] = '\0';
					}
				} else {
					name_field[0] = '\0';
				}
			}
			strcat(name_field, ":");
			fn_len = MAX_NAME - (strlen(name_field) + 1);
			strncat(name_field, s->n_ptr, fn_len);
		} else {
			strncpy(name_field, s->n_ptr, MAX_NAME - 1);
		}
		if (Tflag && name_field[20] != '\0') {
			name_field[19] = '*';
			name_field[20] = '\0';
		}
		if (uflag) {
			fprintf(stdout,"    %s\n", name_field);
			continue;
		}

		switch(s->n_sclass) {
		        case C_FILE:
				fprintf(stdout, fmt_file[(int)num_base], name_field);
				continue;
			
			case C_REG:
			case C_REGPARM:
			case C_AUTO:
			case C_MOS:
			case C_ARG:
			case C_MOU:
                        case C_MOE:
			case C_FIELD:
			case C_AUTOARG:
				/* print value as an offset */
				fprintf(stdout, fmt_offset[(int)num_base], name_field, s->n_value, SCLASS_STR(s->n_sclass));
				break;
			
			case C_SCALL:
			case C_EXT:
			case C_LEAFEXT:
				/* print value as an address */
				fprintf(stdout, fmt_address[(int)num_base], name_field, s->n_value,
						SCLASS_STR(s->n_sclass));
				break;

			case C_STAT:
		        case C_LEAFSTAT:
			case C_USTATIC:
			case C_LABEL:
			case C_BLOCK:
                        case C_FCN:
			case C_HIDDEN:
				/* print value as an address */
				fprintf(stdout, fmt_address[(int)num_base], name_field, s->n_value,
						SCLASS_STR(s->n_sclass));
				break;

			case C_NULL:
			case C_EXTDEF:
			case C_ULABEL:
			case C_STRTAG:
			case C_UNTAG:
			case C_TPDEF:
			case C_ENTAG:
			case C_EOS:
				/* value don't mean a thing, at this time */
				fprintf(stdout, fmt_novalue[(int)num_base], name_field,
					SCLASS_STR(s->n_sclass));
				break;
	
			default:
				fprintf(stdout, fmt_address[(int)num_base], name_field,
						s->n_value, "??????");
				break;
			} /* switch */


		/* decode and display type */
		display_type(fetch_native_coff_info(&cs->symbol), num_base);

		if (s->n_numaux) {
			int *aux_needed;
			combined_entry_type *sym_p = fetch_native_coff_info(&cs->symbol);

			aux_needed = aux_types( s );
			sym_p++;

			/* display first aux entry for this symbol */
			display_size_full_fmt(sym_p, *aux_needed, num_base);
		} else  { 
			/* no size and line info */
			fprintf(stdout, fmt_nosize[(int)num_base]);
			fprintf(stdout, "|     ");
		}
	
		if (s->n_scnum > 0 ) {
			fprintf(stdout, "|%-.8s\n", sections[s->n_scnum - 1]);
		} else if (s->n_scnum == -1) {
			fprintf(stdout, "|(ABS)\n");
		} else
		    fprintf(stdout, "|\n");
	} /* for */

	return 1; /* no error */
}
