/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994 Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and distribute this
 * software and its documentation.  Intel grants this permission provided
 * that the above copyright notice appears in all copies and that both the
 * copyright notice and this permission notice appear in supporting
 * documentation.  In addition, Intel grants this permission provided that
 * you prominently mark as "not part of the original" any modifications
 * made to this software or documentation, and that the name of Intel
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software or the documentation without specific,
 * written prior permission.
 *
 * Intel Corporation provides this AS IS, WITHOUT ANY WARRANTY, EXPRESS OR
 * IMPLIED, INCLUDING, WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY
 * OR FITNESS FOR A PARTICULAR PURPOSE.  Intel makes no guarantee or
 * representations regarding the use of, or the results of the use of,
 * the software and documentation in terms of correctness, accuracy,
 * reliability, currentness, or otherwise; and you rely on the software,
 * documentation and results solely at your own risk.
 *
 * IN NO EVENT SHALL INTEL BE LIABLE FOR ANY LOSS OF USE, LOSS OF BUSINESS,
 * LOSS OF PROFITS, INDIRECT, INCIDENTAL, SPECIAL OR CONSEQUENTIAL DAMAGES
 * OF ANY KIND.  IN NO EVENT SHALL INTEL'S TOTAL LIABILITY EXCEED THE SUM
 * PAID TO INTEL FOR THE PRODUCT LICENSED HEREUNDER.
 *
 ******************************************************************************/
/*)ce*/

#include <stdio.h>
#ifdef __STDC__
#include <stdlib.h>
#else /* __STDC__ */
extern char *malloc(), *realloc();
#endif /* __STDC__ */

#include <fcntl.h>

#ifdef MSDOS
extern kbhit();
#include <io.h>
#endif

#include <ctype.h>
#include <string.h>
#include <errno.h>

#define FOPEN_RDONLY "rb"

#if defined(MSDOS) && (defined(_MSC_VER) || defined(__HIGHC__))
#   define DOS_TIME
#   include <sys/types.h>
#   include <sys/timeb.h>
#endif

#ifndef MSDOS
    /* Assume we are on Unix and can use gettimeofday() */
#   define UNIX_TIME
#   include <sys/time.h>
#endif

#include "private.h"
#include "elf.h"


/*
 * -------- WARNING WARNING WARNING WARNING WARNING WARNING 
 *
 * bout.h must be included before coff.h .  Reason:  coff.h has
 * macro definitions that will rename fields within bout.h data structures!
 */
#include "bout.h"

/*
 * _TEXT is defined by (my version of) stdio.h and also by coff.h.
 * I don't care about either definition; this undef is just to prevent
 * the warning from the compiler about redefinition.
 */
#undef _TEXT

/*
 * Both coff.h and bout.h define N_ABS, but neither defn is used in 
 * this module.  undef the current value of N_ABS to keep the compiler
 * from barking.
 */
#undef N_ABS
#include "coff.h"


#define    SCN1FOFF    (FILHSZ+filhdr.f_opthdr)
#define BYTESWAP(n)     byteswap((char *)&n,sizeof(n))

#ifdef __STDC__
static void byteswap(char *, int);
static int  download_bout(ADDR *start_ip,
                          unsigned long textoff, 
                          unsigned long dataoff,
                          int zero_bss, 
                          DOWNLOAD_CONFIG *fast_config, 
                          int quiet);
static int  download_coff(ADDR *start_ip,
                          unsigned long textoff, 
                          unsigned long dataoff,
                          int zero_bss, 
                          DOWNLOAD_CONFIG *fast_config, 
                          int quiet);
static int  download_elf(ADDR *start_ip,
                         unsigned long textoff, 
                         unsigned long dataoff,
                         int zero_bss, 
                         DOWNLOAD_CONFIG *fast_config, 
                         int quiet);
static int  malloc_buffer(unsigned int *bufsize);
static int  xfer_scn(ADDR addr, unsigned long sz);
#else /* __STDC__ */
static void byteswap();
static int  download_bout();
static int  download_coff();
static int  download_elf();
static int  malloc_buffer();
static int  xfer_scn();
#endif /* __STDC__ */

/*
 * It's very tempting to combine the following four declarations with
 * the previous #ifdef __STDC__ block.  Don't do it.  This blows the
 * HPUX compiler away on an HP700 and will cause it to _silently_ not
 * generate code or symbols.
 */
#if defined(DOS_TIME) && defined(__STDC__)
        static void print_elapsed_time(struct _timeb *, struct _timeb *);
#else                                    /* #elif is an ANSI C feature */
#   if defined(UNIX_TIME) && defined(__STDC__)
        static void print_elapsed_time(struct timeval *, struct timeval *);
#   endif
#endif

#if defined(DOS_TIME) && !defined(__STDC__)
        static void print_elapsed_time();
#else                                    /* #elif is an ANSI C feature */
#   if defined(UNIX_TIME) && !defined(__STDC__)
        static void print_elapsed_time();
#   endif
#endif

static char       msg[100], *dnload_exe;
static FILE       *fh;        /* coff file */
static char       *buf;        /* buffer */
static unsigned   bufsiz;    /* buffer size */
char host_is_big_endian;  /* TRUE or FALSE */
char file_is_big_endian;  /* TRUE or FALSE */
static short test_byte_order = 0x1234;
static unsigned long bytes_xferred;

/*
 * byteswap:    Reverse the order of a string of bytes.
 */
static void
byteswap( p, n )
char *p;    /* Pointer to source/destination string */
int n;      /* Number of bytes in string            */
{
        int i;  /* Index into string                    */
        char c; /* Temp holding place during 2-byte swap*/
 
        for ( n--, i = 0; i < n; i++, n-- ){
                c = p[i];
                p[i] = p[n];
                p[n] = c;
        }
}



static int
system_file_err()
{
    hdi_err_param1  = dnload_exe;
    hdi_cmd_stat    = E_FILE_ERR;
    _hdi_save_errno = errno;
    return(ERR);
}



static int
bad_magic_number()
{
    hdi_err_param1  = dnload_exe;
    hdi_cmd_stat    = E_BAD_MAGIC;
    return(ERR);
}


/* 
 * Allocate buffer for download -- 48k max for real mode DOS (medium model),
 * otherwise, use buffer size of MAX_DNLOAD_BUF to match the largest possible
 * COFF section header.
 */
static int
malloc_buffer(bufsize)
unsigned int * bufsize;
{
#if !defined(_MSC_VER) || defined(WIN95)
    /* FIXME -- real mode only, bogus for Windows NT */
    if ((buf = malloc((unsigned int) MAX_DNLOAD_BUF)) != NULL)
        *bufsize = MAX_DNLOAD_BUF;
#else
    /* Find the largest reasonable chunk of memory for real mode DOS */
    if ((buf = malloc((unsigned int)48*1024)) != NULL)
        {
        *bufsize = (unsigned int)32*1024;
        buf = realloc(buf, *bufsize);
        }
    else if ((buf = malloc((unsigned int)32*1024)) != NULL)
        {
        *bufsize = 24*1024;
        buf = realloc(buf, *bufsize);
        }
    else if ((buf = malloc((unsigned int)24*1024)) != NULL)
        {
        *bufsize = 16*1024;
        buf = realloc(buf, *bufsize);
        }
    else if ((buf = malloc((unsigned int)10*1024)) != NULL)
        {
        *bufsize = 4*1024;
        buf = realloc(buf, *bufsize);
        }
    else if ((buf = malloc((unsigned int)3*1024)) != NULL)
        {
        *bufsize = 1*1024;
        buf = realloc(buf, *bufsize);
        }
#endif
    else if ((buf = malloc((unsigned int)BUFSIZ)) != NULL)
        *bufsize = BUFSIZ;
    else 
        {
        hdi_cmd_stat = E_NOMEM;
        return(ERR);
        }

    return OK;
}

/*
 * Download an ELF file to the target board.
 * Let the caller know where the modules entry point is, in case he cares.
 * Uses hdi_mem_write and hdi_mem_fill, erase_flash.
 * Returns OK or ERR.
 */
static int
download_elf(start_ip, textoff, dataoff, zero_bss, fast_config, quiet)
ADDR *start_ip;
unsigned long textoff, dataoff;
int zero_bss;
DOWNLOAD_CONFIG *fast_config;
int quiet;
{
        int     ph_num;
        ADDR    cur_maddr; /* current section target memory address */

        Elf32_Ehdr filehdr;
        Elf32_Phdr cur_ph;

        /*
         * Read file header
         */
        if (fread((char *)&filehdr, sizeof(Elf32_Ehdr), 1, fh) != 1)
            return (system_file_err());
        
        /* check the endian-ness */
        if (filehdr.e_ident[EI_DATA] == ELFDATA2LSB)
                file_is_big_endian = 0;
        else if (filehdr.e_ident[EI_DATA] == ELFDATA2MSB)
                file_is_big_endian = 1;
        else 
        {
                hdi_cmd_stat   = E_ELF_CORRUPT;
                hdi_err_param1 = dnload_exe;
                hdi_err_param2 = "invalid endian field";
                return(ERR);
        }

        if (host_is_big_endian != file_is_big_endian )
        {
                BYTESWAP(filehdr.e_type);
                BYTESWAP(filehdr.e_machine);
                BYTESWAP(filehdr.e_version);
                BYTESWAP(filehdr.e_entry);
                BYTESWAP(filehdr.e_phoff);
                BYTESWAP(filehdr.e_shoff);
                BYTESWAP(filehdr.e_flags);
                BYTESWAP(filehdr.e_ehsize);
                BYTESWAP(filehdr.e_phentsize);
                BYTESWAP(filehdr.e_phnum);
                BYTESWAP(filehdr.e_shentsize);
                BYTESWAP(filehdr.e_shnum);
                BYTESWAP(filehdr.e_shstrndx);
        }

        /*
         * if we don't have a program header, farm out
         */
        if (!(filehdr.e_phnum)) 
        {
                hdi_cmd_stat   = E_ELF_CORRUPT;
                hdi_err_param1 = dnload_exe;
                hdi_err_param2 = "no program header";
                return(ERR);
        }

        /* check the magic number */
        if ((filehdr.e_ident[EI_MAG0] != ELFMAG0) ||
            (filehdr.e_ident[EI_MAG1] != ELFMAG1) ||
            (filehdr.e_ident[EI_MAG2] != ELFMAG2) ||
            (filehdr.e_ident[EI_MAG3] != ELFMAG3))
        {
            return (bad_magic_number());
        }

        *start_ip = filehdr.e_entry + textoff;

        /*
         * Wait 'til the very last minute to open a fast download
         * connection.  Reason: want to be able to jam some data (if at all
         * possible) down the opened port to prevent the client tool from
         * hanging on a "no such file" or other similar error.
         */
        if (fast_config != NO_DOWNLOAD_CONFIG)
        {
            if (hdi_fast_download_set_port(fast_config) != OK)
                return(ERR);
        }

        hdi_cmd_stat = OK;

        for (ph_num = 0; (unsigned int) ph_num < filehdr.e_phnum; ++ph_num)
        {
            long    seek_pos;

            if (_hdi_signalled)
            {
                hdi_cmd_stat = E_INTR;
                break;
            }

            seek_pos = filehdr.e_phoff + ph_num*filehdr.e_phentsize;

            if (fseek(fh, seek_pos, 0) == -1L
                || fread((char *)&cur_ph, sizeof(Elf32_Phdr), 1, fh) != 1)
            {
                (void) system_file_err();
                break;
            }

            if (host_is_big_endian != file_is_big_endian)
            {
                BYTESWAP(cur_ph.p_type);
                BYTESWAP(cur_ph.p_offset);
                BYTESWAP(cur_ph.p_vaddr);
                BYTESWAP(cur_ph.p_paddr);
                BYTESWAP(cur_ph.p_filesz);
                BYTESWAP(cur_ph.p_memsz);
                BYTESWAP(cur_ph.p_flags);
                BYTESWAP(cur_ph.p_align);
            }

        /* We print two lines of status information for each segment.
           Print the first one now.
         */

        if (!quiet)
            {
                (void) sprintf(msg,
"prog seg %2d  vaddr 0x%.8lx paddr 0x%.8lx flags 0x%lx size %lu\n",
                    ph_num, cur_ph.p_vaddr, cur_ph.p_paddr,
                    cur_ph.p_flags, cur_ph.p_memsz);
                hdi_put_line(msg);
            }

        if (cur_ph.p_type == PT_LOAD)
            {
            if (fseek(fh, (Elf32_Off)(cur_ph.p_offset), 0) == -1L)
                {
                (void) system_file_err();
                break;
                }

            if (cur_ph.p_flags & PF_X)
                cur_maddr = (Elf32_Addr)cur_ph.p_paddr + textoff;
            else
                cur_maddr = (Elf32_Addr)cur_ph.p_paddr + dataoff;

            if (!quiet) {
                sprintf(msg, "             writing %lu bytes at 0x%lx\n",
                        cur_ph.p_filesz, cur_maddr);
                hdi_put_line(msg);
                }
            if (xfer_scn(cur_maddr, cur_ph.p_filesz)!=OK)
                break;
            }
        else 
            if (!quiet) {
                sprintf(msg, "             noload\n");
                hdi_put_line(msg);
                }
        }

        if (fast_config != NO_DOWNLOAD_CONFIG)
            (void) hdi_fast_download_set_port(END_FAST_DOWNLOAD); 

        return(hdi_cmd_stat == OK ? OK : ERR);
}


/*
 * Download a COFF file to the target board.
 * Let the caller know where the modules entry point is, in case he cares.
 * Uses hdi_mem_write and hdi_mem_fill, erase_flash.
 * Returns OK or ERR.
 */
static int
download_coff(start_ip, textoff, dataoff, zero_bss, fast_config, quiet)
ADDR *start_ip;
unsigned long textoff, dataoff;
int zero_bss;
DOWNLOAD_CONFIG *fast_config;
int quiet;
{
    int             scnnum, ispad_or_bss;
    ADDR            cur_maddr; /* current section target memory address */
    struct filehdr  filhdr;
    AOUTHDR         aouthdr;
    SCNHDR          cur_scn;

    if ((fread((char *)&filhdr, FILHSZ, 1, fh) == -1L) ||
        (fread((char *)&aouthdr, sizeof(AOUTHDR), 1, fh) == -1L))
    {
        return (system_file_err());
    }

        /* check the endian-ness */
        
        file_is_big_endian = (*((char *) &filhdr.f_magic) == 0x01);

    /* swap the filehdr & aouthdr if needed */
        if ( host_is_big_endian != file_is_big_endian ){
                BYTESWAP( filhdr.f_magic  );
                BYTESWAP( filhdr.f_nscns  );
                BYTESWAP( filhdr.f_timdat );
                BYTESWAP( filhdr.f_symptr );
                BYTESWAP( filhdr.f_nsyms  );
                BYTESWAP( filhdr.f_opthdr );
                BYTESWAP( filhdr.f_flags  );
        BYTESWAP( aouthdr.magic );
        BYTESWAP( aouthdr.vstamp );
        BYTESWAP( aouthdr.tsize );
        BYTESWAP( aouthdr.dsize );
        BYTESWAP( aouthdr.bsize );
        BYTESWAP( aouthdr.entry );
        BYTESWAP( aouthdr.text_start );
        BYTESWAP( aouthdr.data_start );
        BYTESWAP( aouthdr.tagentries );
        }

    if (filhdr.f_magic != 0x0160)
        return (bad_magic_number());

    /*
     * Wait 'til the very last minute to open a fast download connection. 
     * Reason: want to be able to jam some data (if at all possible) down
     * the opened port to prevent the client tool from hanging on a "no
     * such file" or other similar error.
     */
    if (fast_config != NO_DOWNLOAD_CONFIG)
    {
        if (hdi_fast_download_set_port(fast_config) != OK)
            return(ERR);
    }

    *start_ip = aouthdr.entry + textoff;
    
    hdi_cmd_stat = OK;
    for (scnnum = 0; (unsigned int) scnnum < filhdr.f_nscns; ++scnnum)
        {
        if (_hdi_signalled)
            {
            hdi_cmd_stat = E_INTR;
            break;
            }

        if (fseek(fh, (long)(SCN1FOFF + scnnum*SCNHSZ), 0) == -1L
            || fread((char *)&cur_scn, SCNHSZ, 1, fh) == -1L)
            {
            (void) system_file_err();
            break;
            }
            if ( host_is_big_endian != file_is_big_endian ){
                        BYTESWAP( cur_scn.s_paddr   );
                        BYTESWAP( cur_scn.s_vaddr   );
                        BYTESWAP( cur_scn.s_size    );
                        BYTESWAP( cur_scn.s_scnptr  );
                        BYTESWAP( cur_scn.s_relptr  );
                        BYTESWAP( cur_scn.s_lnnoptr );
                        BYTESWAP( cur_scn.s_nreloc  );
                        BYTESWAP( cur_scn.s_nlnno   );
                        BYTESWAP( cur_scn.s_flags   );
                        BYTESWAP( cur_scn.s_align   );
                }
        if (!quiet) 
            {
            char name[9];
            int i;

            for (i=0; i<8; i++) name[i] = cur_scn.s_name[i];
            name[8] = 0;
            sprintf(msg, "section %d, name %s, address 0x%lx, size 0x%lx, flags 0x%lx",
            scnnum, name, cur_scn.s_paddr, cur_scn.s_size, cur_scn.s_flags);
            hdi_put_line(msg);
            }

        if (cur_scn.s_size == 0)
            {
            if (!quiet)
            hdi_put_line("\n");
            continue;
            }

        switch (cur_scn.s_flags & (STYP_TEXT|STYP_DATA|STYP_PAD)) 
            {
            case STYP_TEXT:
            case STYP_DATA:
            case (STYP_DATA | STYP_TEXT):
                ispad_or_bss = FALSE;
                if (!quiet)
                    hdi_put_line("\n           ");
                break;

            case STYP_PAD:
                ispad_or_bss = TRUE;
                if (!quiet)
                    hdi_put_line(" -- pad\n           ");
                break;

            case STYP_BSS:
                if (zero_bss == TRUE)
                    {    
                    ispad_or_bss = TRUE;
                    if (!quiet)
                        hdi_put_line(" -- bss\n           ");
                    break;
                    }
                /* fall thru to default when bss is no load */

            default:    /* other types all NOLOAD including bss */
                if (!quiet)
                    hdi_put_line(" -- noload\n");
                continue;
            }
    
        if (fseek(fh, cur_scn.s_scnptr, 0) == -1L) 
            {
            (void) system_file_err();
            break;
            }

        if (ispad_or_bss) 
            {
            static const unsigned char zero[1] = { 0 };

            cur_maddr = (ADDR)cur_scn.s_paddr + dataoff;
            if (!quiet) 
                {
                sprintf(msg, "clearing memory at 0x%lx \n", cur_maddr);
                hdi_put_line(msg);
                }
            if (hdi_mem_fill(cur_maddr, zero, sizeof(zero),
                     (unsigned long)cur_scn.s_size, 0) != OK)
                break;
            }
        else   /* .text or .data */
            {
            if ((cur_scn.s_flags & STYP_TEXT) == STYP_TEXT)
                cur_maddr = (ADDR)cur_scn.s_paddr + textoff;
            else
                cur_maddr = (ADDR)cur_scn.s_paddr + dataoff;
            if (!quiet) 
                {
                sprintf(msg, "writing section at  0x%lx\n", cur_maddr);
                hdi_put_line(msg);
                }
            if (xfer_scn(cur_maddr, (unsigned long)cur_scn.s_size) != OK)
                break;
            }
        }

    if (fast_config != NO_DOWNLOAD_CONFIG)
        (void) hdi_fast_download_set_port(END_FAST_DOWNLOAD); 

    return(hdi_cmd_stat == OK ? OK : ERR);
}



/*
 * Download a BOUT file to the target board.
 * Let the caller know where the modules entry point is, in case s/he cares.
 * Uses hdi_mem_write.  Returns OK or ERR.
 */
static int
download_bout(start_ip, textoff, dataoff, unused, fast_config, quiet)
ADDR *start_ip;
unsigned long textoff, dataoff;
int unused;
DOWNLOAD_CONFIG *fast_config;
int quiet;
{
#define BOUT_TEXT_SCN   0
#define BOUT_DATA_SCN   1
#define BOUT_BSS_SCN    2
#define BOUT_MAX_SCNS   3

    struct exec       filhdr;
    ADDR              scn_addr, scn_load;
    static const char *scn_names[] = { ".text", ".data", ".bss" };
    int               scn_num;
    long              scn_offset;
    unsigned long     scn_size;

    if (fread((char *)&filhdr, sizeof(filhdr), 1, fh) == -1L)
        return (system_file_err());
    
    /* check the endian-ness */
    
    file_is_big_endian = ! (*((char *) &filhdr.a_magic) == (BMAGIC & 0xff));

    /* swap relevant portions of filehdr if needed */
    if ( host_is_big_endian != file_is_big_endian )
    {
        BYTESWAP(filhdr.a_magic);
        BYTESWAP(filhdr.a_text);
        BYTESWAP(filhdr.a_data);
        BYTESWAP(filhdr.a_bss);
        BYTESWAP(filhdr.a_entry);
        BYTESWAP(filhdr.a_tload);
        BYTESWAP(filhdr.a_dload);
    }

    if (filhdr.a_magic != BMAGIC)
        return (bad_magic_number());
    
    /*
     * Wait 'til the very last minute to open a fast download connection. 
     * Reason: want to be able to jam some data (if at all possible) down
     * the opened port to prevent the client tool from hanging on a "no
     * such file" or other similar error.
     */
    if (fast_config != NO_DOWNLOAD_CONFIG)
    {
        if (hdi_fast_download_set_port(fast_config) != OK)
            return(ERR);
    }

    *start_ip    = filhdr.a_entry + textoff;
    hdi_cmd_stat = OK;

    for (scn_num = 0; scn_num < BOUT_MAX_SCNS; scn_num++)
    {
        /* Only 3 sections in b.out: text, data, bss. */

        if (_hdi_signalled)
        {
            hdi_cmd_stat = E_INTR;
            break;
        }
        if (scn_num == BOUT_TEXT_SCN)
        {
            scn_addr   = filhdr.a_tload;
            scn_load   = scn_addr + textoff;
            scn_size   = filhdr.a_text;
            scn_offset = N_TXTOFF(exec);
        }
        else if (scn_num == BOUT_DATA_SCN)
        {
            scn_addr   = filhdr.a_dload;
            scn_load   = scn_addr + dataoff;
            scn_size   = filhdr.a_data;
            scn_offset = N_DATOFF(filhdr);
        }
        else
        {
            /* 
             * BSS.  Simply print out the size of BSS and continue.  To do
             * this right we'd need to dig through the symbol table and
             * look up the BSS address and then check the "zero_bss" flag
             * and then zero it out if the client requested zeroing.  For 
             * now, just rely on the i960 prolog code to zero BSS.
             */

            if (! quiet)
            {
                sprintf(msg,
                "section %d, name %s, address __Bbss, size 0x%lx -- noload\n",
                       scn_num,
                       scn_names[scn_num],
                       filhdr.a_bss);
                hdi_put_line(msg);
            }
            continue;
        }
        if (fseek(fh, scn_offset, 0) == -1L)
        {
            (void) system_file_err();
            break;
        }
        if (!quiet) 
        {
            sprintf(msg, 
                    "section %d, name %s, address 0x%lx, size 0x%lx\n",
                    scn_num, 
                    scn_names[scn_num], 
                    scn_addr, 
                    scn_size);
            hdi_put_line(msg);
            sprintf(msg, "        writing section at  0x%lx\n", scn_load);
            hdi_put_line(msg);
        }
        if (xfer_scn(scn_load, scn_size) != OK)
            break;
    }

    if (fast_config != NO_DOWNLOAD_CONFIG)
        (void) hdi_fast_download_set_port(END_FAST_DOWNLOAD); 

    return(hdi_cmd_stat == OK ? OK : ERR);

#undef BOUT_MAX_SCNS
#undef BOUT_TEXT_SCN
#undef BOUT_DATA_SCN
#undef BOUT_BSS_SCN
}


/*
 * Read sz bytes of data from fh and write it
 * to target memory at the specified address.
 */
static int
xfer_scn(addr, sz)
ADDR addr;
unsigned long sz;
{
    int to_send;

    bytes_xferred += sz;
    while (sz > 0) {
#ifdef MSDOS
            /* Check for CTRL-C hit each time through the loop */
            kbhit();
#endif
        if (_hdi_signalled)
        {
            hdi_cmd_stat = E_INTR;
            return(ERR);
        }

        if (sz < bufsiz)
            to_send = (int) sz;
        else
            to_send = (int) bufsiz;

        if (fread(buf, 1, to_send, fh) != (unsigned int) to_send)
            return (system_file_err());

        if (hdi_mem_write(addr, (unsigned char *)buf, to_send,
                  TRUE, TRUE, 0) != OK)
            return(ERR);

        sz -= to_send;
        addr += to_send;
    }

    return(OK);
}


#if defined(DOS_TIME)

static void
print_elapsed_time(first, second)
struct _timeb *first, *second;
{
    char          buf[200];
    struct _timeb lapsed;
    unsigned long bps;     /* bytes per sec */

    if (first->millitm > second->millitm)
    {
        second->millitm += 1000;
        second->time--;
    }
    lapsed.millitm = second->millitm - first->millitm;
    lapsed.time    = second->time - first->time;
    bps            = ((double) bytes_xferred * 1000.0) / 
                                  (lapsed.time * 1000.0 + lapsed.millitm);
    sprintf(buf,
       "Download stats: %ld.%0.3d sec elapsed, %lu bytes/sec (%.2f kb/sec)\n",
           lapsed.time,
           lapsed.millitm,
           bps,
           bps / 1024.0);
    hdi_put_line(buf);
}

#else
#   if defined(UNIX_TIME)

static void
print_elapsed_time(first, second)
struct timeval *first, *second;
{
    char           buf[200];
    struct timeval lapsed;
    unsigned long  bps;     /* bytes per sec */

    if (first->tv_usec > second->tv_usec)
    {
        second->tv_usec += 1000000;
        second->tv_sec--;
    }
    lapsed.tv_sec   = second->tv_sec - first->tv_sec;
    lapsed.tv_usec  = second->tv_usec - first->tv_usec;
    lapsed.tv_usec /= 1000;  /* usec is now msec */
    bps             = ((double) bytes_xferred * 1000.0) / 
                                   (lapsed.tv_sec * 1000.0 + lapsed.tv_usec);
    sprintf(buf,
       "Download stats: %ld.%0.3ld sec elapsed, %lu bytes/sec (%.2f kb/sec)\n",
           lapsed.tv_sec,
           lapsed.tv_usec,
           bps,
           bps / 1024.0);
    hdi_put_line(buf);
}
#   endif
#endif


int
hdi_download(fname, start_ip, textoff, dataoff, zero_bss, fast_config, quiet)
const char *fname;
ADDR *start_ip;
unsigned long textoff, dataoff;
int zero_bss, quiet;
DOWNLOAD_CONFIG *fast_config;
{
        int  ec;
        char sig[4];

#if defined(DOS_TIME)
        struct _timeb   first_tm, second_tm;
#else
#   if defined (UNIX_TIME)
        struct timeval  first_tm, second_tm;
        struct timezone tz_unused;
#   endif
#endif

    /*
     * Begin the process of collecting timing stats if supported on the
     * host.
     */
    bytes_xferred = 0;
#if defined(DOS_TIME)
    _ftime(&first_tm);
#else
#   if defined(UNIX_TIME)

    gettimeofday(&first_tm, &tz_unused);

#   endif
#endif

    if (fast_config != NO_DOWNLOAD_CONFIG)
    {
        fast_config->pci_quiet_connect += quiet;
        if (fast_config->download_selector == FAST_PARALLEL_DOWNLOAD)
        {
            if (hdi_fast_download_set_port(PARALLEL_CAPABLE) != OK)
                return (ERR);
        }
        else 
        {
            /* Must be PCI download request. */

            if (hdi_fast_download_set_port(PCI_CAPABLE) != OK)
                return (ERR);
        }
    }
    host_is_big_endian = (*((char *) &test_byte_order) == 0x12);
    _hdi_signalled     = FALSE;

    if (!fname || !*fname) 
    {
        hdi_cmd_stat = E_ARG;
        return(ERR);
    }

    switch (hdi_bp_rm_all())
    {
        case ERR:
            /* Note: this takes care of E_RUNNING, E_COMM_TIMO, etc. */
            return(ERR);
        case TRUE:
            if (!quiet)
                hdi_put_line("Warning: existing breakpoints removed\n");
            break;
        case FALSE:
            break;
    }

    dnload_exe = (char *) fname;  /* For future errors regarding this file. */
    if ((fh = fopen(fname, FOPEN_RDONLY)) == NULL)
        return (system_file_err());
    if (fread(sig, 1, 4, fh) != 4)
    {
        fclose(fh);
        return (system_file_err());
    }
    rewind(fh);

    if (malloc_buffer(&bufsiz) != OK)
    {
        fclose(fh);
        return(ERR);
    }
    if (sig[EI_MAG0] == ELFMAG0 && 
               sig[EI_MAG1] == ELFMAG1 &&
                       sig[EI_MAG2] == ELFMAG2 && 
                                   sig[EI_MAG3] == ELFMAG3)
    {
        ec = download_elf(start_ip,
                          textoff,
                          dataoff,
                          zero_bss,
                          fast_config,
                          quiet);
    }
    else if ((sig[0] == 0                        && 
              sig[1] == 0                        && 
              sig[2] == ((BMAGIC & 0xffff) >> 8) &&
              sig[3] == ((BMAGIC & 0xff)))   
                     ||
             (sig[0] == (BMAGIC & 0xff)          &&
              sig[1] == ((BMAGIC & 0xffff) >> 8) &&
              sig[2] == 0                        && 
              sig[3] == 0))
    {
        ec = download_bout(start_ip,
                           textoff,
                           dataoff,
                           0,        /* Not currently capable of zeroing BSS */
                           fast_config,
                           quiet);
    }
    else  /* Assume COFF */
    {
        ec = download_coff(start_ip,
                           textoff,
                           dataoff,
                           zero_bss,
                           fast_config,
                           quiet);
    }

    fclose(fh);
    free(buf);

    if (ec != OK)
        return (ec);

#if defined(DOS_TIME)
    _ftime(&second_tm);
    if (! quiet)
        print_elapsed_time(&first_tm, &second_tm);
#else
#   if defined(UNIX_TIME)
    gettimeofday(&second_tm, &tz_unused);
    if (! quiet)
        print_elapsed_time(&first_tm, &second_tm);
#endif
#endif

    return (OK);
}
