
static char rcsid[] = "$Id: elfdmp.c,v 1.18 1995/12/05 09:09:58 peters Exp $";

#include <stdio.h>
#include "bfd.h"
#include "elf.h"
#include "gdmp960.h"

/* The following externs are declared in gdmp960.c */
extern bfd 	*abfd;		/* BFD descriptor of object file */
extern char 	flagseen [];	/* Command-line options */
extern special_type	*special_sect;	/* Dump only this one section */

int
get_elf_hdrs( e, s, p )
    Elf32_Ehdr **e;	/* Put e header here */
    Elf32_Shdr **s;     /* Put section headers here */
    Elf32_Phdr **p;     /* Program header here. */
{
    int readsize;

    xseek( 0 );

    /* Read file header, put it into host byte order
     */
    *e = (Elf32_Ehdr *) xmalloc(sizeof(Elf32_Ehdr));

    xread( (char *) (*e), sizeof(**e) );
    if ( host_is_big_endian != file_is_big_endian ){
	BYTESWAP((*e)->e_type);
	BYTESWAP((*e)->e_machine);
	BYTESWAP((*e)->e_version);
	BYTESWAP((*e)->e_entry);
	BYTESWAP((*e)->e_phoff);
	BYTESWAP((*e)->e_shoff);
	BYTESWAP((*e)->e_flags);
	BYTESWAP((*e)->e_ehsize);
	BYTESWAP((*e)->e_phentsize);
	BYTESWAP((*e)->e_phnum);
	BYTESWAP((*e)->e_shentsize);
	BYTESWAP((*e)->e_shnum);
	BYTESWAP((*e)->e_shstrndx);
    }

    *s = (Elf32_Shdr *) 0;
    if ((*e)->e_shnum) {
	*s = (Elf32_Shdr *) xmalloc(readsize = (*e)->e_shnum * (*e)->e_shentsize);
	xseek((*e)->e_shoff);
	xread((char *) (*s),readsize);
	if ( host_is_big_endian != file_is_big_endian ){
	    int i;

	    for (i=0;i < (int)(*e)->e_shnum;i++) {
		BYTESWAP(((*s)+i)->sh_name);
		BYTESWAP(((*s)+i)->sh_type);
		BYTESWAP(((*s)+i)->sh_flags);
		BYTESWAP(((*s)+i)->sh_addr);
		BYTESWAP(((*s)+i)->sh_offset);
		BYTESWAP(((*s)+i)->sh_size);
		BYTESWAP(((*s)+i)->sh_link);
		BYTESWAP(((*s)+i)->sh_info);
		BYTESWAP(((*s)+i)->sh_addralign);
		BYTESWAP(((*s)+i)->sh_entsize);
	    }
	}
    }
    *p = (Elf32_Phdr *) 0;
    if ((*e)->e_phnum) {
	*p = (Elf32_Phdr *) xmalloc(readsize = (*e)->e_phnum * (*e)->e_phentsize);
	xseek((*e)->e_phoff);
	xread((char *) (*p),readsize);
	if ( host_is_big_endian != file_is_big_endian ){
	    int i;

	    for (i=0;i < (int)(*e)->e_phnum;i++) {
		BYTESWAP(((*p)+i)->p_type);
		BYTESWAP(((*p)+i)->p_offset);
		BYTESWAP(((*p)+i)->p_vaddr);
		BYTESWAP(((*p)+i)->p_paddr);
		BYTESWAP(((*p)+i)->p_filesz);
		BYTESWAP(((*p)+i)->p_memsz);
		BYTESWAP(((*p)+i)->p_flags);
		BYTESWAP(((*p)+i)->p_align);
	    }
	}
    }
}

static print_class(c)
    unsigned c;
{
    char classbuff[128];

    switch(c) {
 case ELFCLASSNONE:
	strcpy(classbuff,"ELFCLASSNONE");
	break;
 case ELFCLASS32:
	strcpy(classbuff,"ELFCLASS32");
	break;
 case ELFCLASS64:
	strcpy(classbuff,"ELFCLASS64");
	break;
 default:
	sprintf(classbuff,"UNKNOWN (Class = %d)",c);
	break;
    }
    printf("Class:                                                  %s\n",classbuff);
}

static print_data(d)
    unsigned d;
{
    char databuff[128];

    switch(d) {
 case ELFDATANONE:
	strcpy(databuff,"ELFDATANONE");
	 break;
 case ELFDATA2LSB:
	strcpy(databuff,"ELFDATA2LSB (little endian)");
	break;
 case ELFDATA2MSB:
	strcpy(databuff,"ELFDATA2MSB (big endian)");
	break;
 default:
	sprintf(databuff,"UNKNOWN (data = %d)",d);
	break;
     }
    printf("Data:                                                   %s\n",databuff);
}

static print_version(v1,v2)
    unsigned v1,v2;
{
    char versionbuff[128];

    sprintf(versionbuff,"e_version: %d, ",v1);
    switch(v2) {
 case EV_CURRENT:
	strcat(versionbuff,"EV_CURRENT");
	 break;
 default:
	sprintf(versionbuff,"e_version: %d, UNKNOWN (data = %d)",v1,v2);
	break;
     }
    printf("version:                                                %s\n",versionbuff);
}

static void print_e_flags(e_flags)
    unsigned long e_flags;
{
    printf("(");
    switch (e_flags & EF_960_MASKPROC) {
 case EF_960_GENERIC: printf("GENERIC"); break;
 case EF_960_KA: printf("KA"); break;
 case EF_960_KB: printf("KB"); break;
 case EF_960_SA: printf("SA"); break;
 case EF_960_SB: printf("SB"); break;
 case EF_960_CA: printf("CA"); break;
 case EF_960_CF: printf("CF"); break;
 case EF_960_JA: printf("JA"); break;
 case EF_960_JF: printf("JF"); break;
 case EF_960_JD: printf("JD"); break;
 case EF_960_HA: printf("HA"); break;
 case EF_960_HD: printf("HD"); break;
 case EF_960_HT: printf("HT"); break;
 case EF_960_RP: printf("RP"); break;
 case EF_960_JL: printf("JL"); break;
 default: printf("UNKNOWN ARCHITECTURE: %d",e_flags & EF_960_MASKPROC); break;
     }

    if (1) {
	int cnt = 0,i;

	static struct table {
	    char *name;
	    int value;
	} value_table[] = {
	{ "CORE_1", EF_960_CORE1 },
	{ "CORE_2", EF_960_CORE2 },
	{ "KX", EF_960_K_SERIES },
	{ "CX", EF_960_C_SERIES },
	{ "JX", EF_960_J_SERIES },
	{ "HX", EF_960_H_SERIES },
	{ "FP1", EF_960_FP1 },
	{ NULL, 0 } };

	for (i=0;value_table[i].name;i++)
		if (e_flags & value_table[i].value) {
		    if (++cnt == 1)
			    printf(" Instruction classes: %s",value_table[i].name);
		    else
			    printf(",%s",value_table[i].name);
		}
    }
    printf(")\n");
}

dmp_elf_hdr()
{
    Elf32_Ehdr *e;	/* Elf header				*/
    Elf32_Shdr *s;      /* Elf section headers. */
    Elf32_Phdr *p;      /* Elf program headers. */

    get_elf_hdrs( &e, &s, &p);
    if ( ! flagseen['p'] ) {
	bfd_center_header( "ELF HEADER" );
	printf("\n\n");
    }
    print_class(e->e_ident[EI_CLASS]);
    print_data(e->e_ident[EI_DATA]);
    print_version(e->e_ident[EI_VERSION],e->e_version);
    printf("Machine:                                                0x%08x\n",e->e_machine);
    printf("Type:                                                   0x%08x\n",e->e_type);
    printf("Entry point:                                            0x%08x\n",e->e_entry);
    printf("Program header offset:                                  0x%08x\n",e->e_phoff);
    printf("Section header table offset:                            0x%08x\n",e->e_shoff);
    printf("Flags:                                                  0x%08x\n",e->e_flags);
    print_e_flags(e->e_flags);
    printf("ELF header size:                                        0x%08x  (sizeof(Elf32_Ehdr) = 0x%x)\n",
	   e->e_ehsize,sizeof(*e));
    printf("Program header entry size:                              0x%08x\n",e->e_phentsize);
    printf("Program header number of entries:                       0x%08x\n",e->e_phnum);
    printf("Section header size:                                    0x%08x\n",e->e_shentsize);
    printf("Number of section headers:                              0x%08x\n",e->e_shnum);
    printf("Index of the string table for the section header names: 0x%08x\n",e->e_shstrndx);

    return 1; /* success */
} /* dmp_elf_file_hdr() */

static print_section_header_type(sh_type)
    int sh_type;
{
    int i;
    static struct {int type; char *name; } sh_type_names[] =
	{
    { SHT_NULL,     "NULL"},
    { SHT_PROGBITS, "PROGBITS"},
    { SHT_SYMTAB,   "SYMTAB"},
    { SHT_STRTAB,   "STRTAB"},
    { SHT_RELA,     "RELA"},
    { SHT_HASH,     "HASH"},
    { SHT_DYNAMIC,  "DYNAMIC"},
    { SHT_NOTE,     "NOTE"},
    { SHT_NOBITS,   "NOBITS"},
    { SHT_REL,      "REL"},
    { SHT_SHLIB,    "SHLIB"},
    { SHT_DYNSYM,   "DYNSYM"},
    { SHT_960_INTEL_CCINFO, "CCINFO" },
    { 0, NULL }   };

    for (i=0;sh_type_names[i].name;i++)
	    if (sh_type == sh_type_names[i].type) {
		printf("%10s  ",sh_type_names[i].name);
		return;
	    }
    printf("%10s  ", "UNKNOWN");
}

static print_section_header_flags(sh_flags)
    int sh_flags;
{
    int cnt = 0;
    static struct flag_value { int flag_val;char *name;} flag_vals[] =
	{
        { SHF_ALLOC,               "ALLOC" },
    	{ SHF_960_READ,            "960_READ" },
	{ SHF_WRITE,               "WRITE" },
        { SHF_EXECINSTR,           "EXECINSTR" },
    	{ SHF_960_SUPER_READ,      "960_SUPER_READ" },
    	{ SHF_960_SUPER_WRITE,     "960_SUPER_WRITE" },
    	{ SHF_960_SUPER_EXECINSTR, "960_SUPER_EXECINSTR" },
    	{ SHF_960_MSB,             "960_MSB" },
    	{ SHF_960_PI,              "960_PI" },
    	{ SHF_960_LINK_PIX,        "960_LINK_PIX" },
        {0,0} };
    struct flag_value *fp;

    for (fp=flag_vals;sh_flags && fp->name;fp++)
	    if (sh_flags & fp->flag_val)
		    if (!cnt) {
			printf("%s",fp->name);
			cnt++;
		    }
		    else
			    printf(", %s",fp->name);
    printf(" ");  /* This trailing space is needed for depth tests. */
}

static print_section_header_index(sh_index)
    int sh_index;
{
    printf("%2d  ", sh_index);
}

static print_section_header_addr(sh_addr)
    int sh_addr;
{
    printf("0x%08x  ",sh_addr);
}

static print_section_header_offset(sh_offset)
    int sh_offset;
{
    printf("0x%08x  ",sh_offset);
}

static print_section_header_size(sh_size)
    int sh_size;
{
    printf("0x%08x ",sh_size);
}

static print_section_header_name(sh_name)
    char *sh_name;
{
    printf("%s", sh_name);
}

static print_section_header_link(sh_link)
    int sh_link;
{
    printf("%4d  ",sh_link);
}

static print_section_header_info(sh_info)
{
    printf("%4d  ",sh_info);
}

static print_section_header_addralign(sh_addralign)
    int sh_addralign;
{
    printf("%4d  ",sh_addralign);
}

static print_section_header_entsize(sh_entsize)
    int sh_entsize;
{
    printf("%10d  ",sh_entsize);
}

static print_program_header_flags(f)
    int f;
{
    int count = 0;
    if (f & PF_X)
	printf(count++ ? ", EXEC" : "EXEC");
    if (f & PF_W)
	printf(count++ ? ", WRIT" : "WRIT");
    if (f & PF_R)
	printf(count++ ? ", READ" : "READ");
    printf(" ");  /* This trailing space is needed for depth tests. */
}

static print_program_header_type(t)
    int t;
{
    char *str;
    switch (t) 
    {
    case PT_NULL:
	str = "NULL";
	break;
    case PT_LOAD:
	str = "LOAD";
	break;
    case PT_DYNAMIC:
	str = "DYNAMIC";
	break;
    case PT_INTERP:
	str = "INTERP";
	break;
    case PT_NOTE:
	str = "NOTE";
	break;
    case PT_SHLIB:
	str = "SHLIB";
	break;
    case PT_PHDR:
	str = "PHDR";
	break;
    default:
	str = "UNKNOWN";
	break;
    }
    printf("%7s ", str);
}

static print_program_header_index(index)
    int index;
{
    printf("%2d ", index);
}

static print_program_header_offset(offset)
    int offset;
{
    printf("0x%08x ", offset);
}

static print_program_header_align(align)
    int align;
{
    printf("%2d ", align);
}

static print_one_program_header(i,p)
    int i;
    Elf32_Phdr *p;
{

#define print_program_header_virtual_address  print_program_header_offset
#define print_program_header_physical_address print_program_header_offset
#define print_program_header_filesize         print_program_header_offset
#define print_program_header_memsize          print_program_header_offset

    print_program_header_index(i);
    print_program_header_type(p->p_type);
    print_program_header_offset(p->p_offset);
    print_program_header_virtual_address(p->p_vaddr);
    print_program_header_physical_address(p->p_paddr);
    print_program_header_filesize(p->p_filesz);
    print_program_header_memsize(p->p_memsz);
    print_program_header_align(p->p_align);
    print_program_header_flags(p->p_flags);
    printf("\n");
}

static print_one_section_header(s,p,i)
    Elf32_Shdr *s;
    char *p;
    int i;
{
    print_section_header_index(i);
    print_section_header_addr(s->sh_addr);
    print_section_header_offset(s->sh_offset);
    print_section_header_size(s->sh_size);
    print_section_header_addralign(s->sh_addralign);
    print_section_header_type(s->sh_type);
    print_section_header_name(p + s->sh_name);
    printf("\n    ");
    print_section_header_link(s->sh_link);
    print_section_header_info(s->sh_info);
    print_section_header_entsize(s->sh_entsize);
    print_section_header_flags(s->sh_flags);
    printf("\n\n");
}

static char *read_string_table(offset,size)
    int offset,size;
{
    char *p = xmalloc(size);

    xseek(offset);
    xread(p,size);
    return p;
}

dmp_elf_section_hdrs()
{
    Elf32_Ehdr *e;	/* Elf header */
    Elf32_Shdr *s;      /* Elf section headers. */
    Elf32_Phdr *p;      /* Elf program headers. */

    int i;
    char *sect_head_string_table;

    get_elf_hdrs( &e, &s, &p );

    if ( ! flagseen['p'] ) 
    {
	bfd_center_header( "SECTION HEADERS" );
	printf("\n");
	printf("Num    Address  FileOffset  Size      Align        Type  Name\n");
	printf("    Link  Info       EntSz  Flags\n\n");
    }

    sect_head_string_table = 
	read_string_table((s+e->e_shstrndx)->sh_offset,
			  (s+e->e_shstrndx)->sh_size);
    
    for (i=1;i < (int)e->e_shnum;i++) 
    {
	if ( flagseen['n'] && strcmp (sect_head_string_table+(s+i)->sh_name, special_sect->name) )
		continue;
	print_one_section_header(s+i,sect_head_string_table,i);
    }
    if (p) 
    {
	if ( ! flagseen['p'] ) 
	{
	    bfd_center_header( "PROGRAM HEADERS" );
	    printf("\n");
	    printf("Num   Type     Offset      Vaddr      Paddr     FileSz      MemSz Al Flags\n\n");
	}

	for (i=0;i < (int)e->e_phnum;i++)
		print_one_program_header(i,p+i);
    }
    return 1;
}


static void
dmp_sec_reloc(abfd, sec, sympp)
    bfd *abfd;	/* bfd descriptor of object file */
    sec_ptr sec;	/* pointer to bfd section descriptor */
    asymbol **sympp;
{
    arelent **relpp;
    int relcount;
    char *type;
    int i;
    unsigned long secaddr;
    int reloc_bytes;

    printf( "Section: %s\n", bfd_section_name(abfd,sec) );
    if ( (reloc_bytes = get_reloc_upper_bound(abfd,sec)) > 0 ) {
	/* For executable objects, print the relocation's virtual address.
	   Else print the section-relative offset */
	secaddr = (abfd->flags & EXEC_P) ? bfd_section_vma(abfd,sec) : 0;
	relpp = (arelent **) xmalloc( reloc_bytes );
	relcount = bfd_canonicalize_reloc(abfd,sec,relpp,sympp);
	if ( relcount && ! flagseen['p'] ) {
	    if (abfd->flags & EXEC_P)
		printf("Vaddr      ");
	    else
		printf("Offset     ");
	    if (flagseen['o'])
		    printf("  ");
	    printf("Type       Name\n");
	}
	for ( i=0; i < relcount; i++ ) {
	    if ( flagseen['o'] )
		    printf ("0%011o %-10s %s\n", secaddr+relpp[i]->address,
			    relpp[i]->howto->name, (*(relpp[i]->sym_ptr_ptr))->name);
	    else
		    printf ("0x%08x %-10s %s\n", secaddr+relpp[i]->address,
			    relpp[i]->howto->name, (*(relpp[i]->sym_ptr_ptr))->name);
	}
    }
    putchar ('\n');
}

dmp_elf_rel( abfd )
    bfd *abfd;
{
    asymbol **sympp;
    asection *sect;
    int symcount;

    sympp = (asymbol **) xmalloc( get_symtab_upper_bound(abfd) );
    symcount = bfd_canonicalize_symtab( abfd, sympp );

    if ( ! flagseen['p'] ) {
	bfd_center_header( "RELOCATION INFORMATION" );
	putchar ('\n');
    }

    if ( flagseen['n'] ) {
	/* User wants to dump only one section */
	for ( sect = abfd->sections; sect; sect = sect->next) {
	    if ( ! strcmp (sect->name, special_sect->name) ) {
		dmp_sec_reloc (abfd, sect, sympp);
		break;
	    }
	}
    }
    else
	    bfd_map_over_sections( abfd, dmp_sec_reloc, sympp );
}

static dump_string_table(t,len,header)
    char *t;
    int len;
    char *header;
{
    int i;

    if ( ! flagseen['p'] ) {
	bfd_center_header( header );
	putchar ('\n');
	printf("Index:  String: (size: %d)\n",len);
    }

    for (i=0;i < len;i += strlen(t+i) + 1)
	    printf("%5d  %s\n",i,t+i);
}

dmp_elf_stringtab()
{
    Elf32_Ehdr *e;	/* Elf header */
    Elf32_Shdr *s;      /* Elf section headers. */
    Elf32_Phdr *p;      /* Elf program headers. */
    char *sh_strtab,*strtab = NULL;
    int sh_strtab_length=0,strtab_length=0,i;

    get_elf_hdrs( &e, &s, &p );

    sh_strtab = read_string_table((s+e->e_shstrndx)->sh_offset,
				  sh_strtab_length=(s+e->e_shstrndx)->sh_size);
    for (i=1;i < (int)e->e_shnum;i++)
	    if ((s+i)->sh_type == SHT_SYMTAB) {
		i = (s+i)->sh_link;
		strtab = read_string_table((s+i)->sh_offset,strtab_length=(s+i)->sh_size);
		break;
	    }

    dump_string_table(sh_strtab,sh_strtab_length,"SECTION HEADER STRINGS");
    dump_string_table(strtab,strtab_length,"SYMTAB STRINGS");
    return 1;
}
