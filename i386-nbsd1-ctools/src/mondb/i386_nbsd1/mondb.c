/*(cb*/
/*******************************************************************************
 *
 * Copyright (c) 1993, 1994, 1995 Intel Corporation
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
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdlib.h>
#include <malloc.h>

#ifndef MSDOS
#   include <sys/types.h>
#   include <sys/stat.h>
#endif

#include "hdil.h"

/* ----------------------- Temporary Scaffolding ---------------------- */
extern void            pci_put_reg   HDI_PARAMS((unsigned short, long));
extern unsigned long   pci_get_reg   HDI_PARAMS((unsigned short));

static int pci_init_done = 0;
static int break_flag = FALSE;
/* -------------------------------------------------------------------- */

#define PCI_COMM_INDX   0         /* Index to comm_used[] array */
#define PARA_COMM_INDX  1         /* Index to comm_used[] array */

#define TA_FULL         0x1
#define TA_NAME_ONLY    0x2
#define TA_TERSE        0x4

#define BAUD_OPT        "b"
#define ENV_VAR         "MONDB"
#define SER_OPT         "ser"

#ifdef MSDOS
#   define IS_OPTION(x)  (x == '-' || x == '/')
#else
#   define IS_OPTION(x)  (x == '-')
#endif

#define PCI_COMM_DFLT    0x1
#define PCI_COMM_BUS     0x2
#define PCI_COMM_VND     0x4
#define PCI_COMM_MAX_VAL (PCI_COMM_VND)
#define PCI_COMM_MIN_VAL (PCI_COMM_DFLT | PCI_COMM_BUS)


/* --------------------- Function Declarations ----------------------- */

static char     *basename          HDI_PARAMS((const char *, const char *));
static void     configure_comm     HDI_PARAMS((void));
#ifdef WIN95
static void     ctrl_break         HDI_PARAMS((int));
#endif
static void     create_do_cmd_argv HDI_PARAMS((char *));
static int      do_cmd             HDI_PARAMS((char *, int));
static int      do_help            HDI_PARAMS((const char *, const char *));
static void     error              HDI_PARAMS((const char *fmt));
static void     envargs            HDI_PARAMS((void));
static void     establish_defaults HDI_PARAMS((void));
static int      format_mon_cfg_msg HDI_PARAMS((int));
static void     getargs            HDI_PARAMS((int, char **));
static void     handle_aplink_cmd  HDI_PARAMS((char *));
extern int      hdi_check_for_debug_msg 
                                   HDI_PARAMS((void));
static void     help_msg           HDI_PARAMS((void));
static void     leave              HDI_PARAMS((int error_code));
static void     out_of_mem         HDI_PARAMS((const char *));
static void     print_hdi_error    HDI_PARAMS((const char *));
static int      process_option     HDI_PARAMS((const char *, const char *));
static void     process_pci_option HDI_PARAMS((char **, int *));
static void     quit               HDI_PARAMS((int));
extern int      read               HDI_PARAMS((int, char *, int));
static int      set_debug          HDI_PARAMS((const char *, const char *));
static int      set_ef             HDI_PARAMS((const char *, const char *));
static int      set_exit           HDI_PARAMS((const char *, const char *));
static int      set_it             HDI_PARAMS((const char *, const char *));
static int      set_mon_debug      HDI_PARAMS((const char *, const char *));
static int      set_quiet          HDI_PARAMS((const char *, const char *));
static int      set_noexec         HDI_PARAMS((const char *, const char *));
static int      set_reset_time     HDI_PARAMS((const char *, const char *));
static int      set_serial_baud    HDI_PARAMS((const char *, const char *));
static int      set_serial_port    HDI_PARAMS((const char *, const char *));
static int      set_script_file    HDI_PARAMS((const char *, const char *));
static int      set_start_ip       HDI_PARAMS((const char *, const char *));
static int      set_ta             HDI_PARAMS((const char *, const char *));
static int      set_tan            HDI_PARAMS((const char *, const char *));
static int      set_tat            HDI_PARAMS((const char *, const char *));
static int      start_target       HDI_PARAMS((int, int, unsigned long, int *));
static int      target_stopped     HDI_PARAMS((const STOP_RECORD *));
static void     trap               HDI_PARAMS((int));
static void     usage              HDI_PARAMS((const char *, const char *));
extern int      write              HDI_PARAMS((int, const char *, int));

#ifdef __HIGHC__
/* With Metaware/Phar Lap, it is observed that CTRL-BREAK can't be trapped
   with the normal SIGINT handler installed with signal().  So we will 
   install it as a direct Phar Lap interrupt handler.  For simplicity and
   safety, CTRL-BREAK will do nothing; user CTRL-C to interrupt the target. */
#include <dos.h>
void    _Far _INTERRPT ctrl_brk();    /* interrupt handler for CTRL-BREAK */
void    install_ctrlbrk_handler();    /* defined in toolib */
void    de_install_ctrlbrk_handler();    /* defined in toolib */
#endif

/* --------------------- Data Declarations ----------------------- */

extern char gnu960_ver[];  /* External version string created by make */
#ifdef __HIGHC__
static const char os_ver[] = "(PHARLAP)";
#endif
#ifdef WIN95
static const char os_ver[] = "(WIN_95)";
#endif
#if defined(MSDOS) && !defined( __HIGHC__) && !defined(WIN95)
static const char os_ver[] = "(MS_DOS)";
#endif
#ifndef MSDOS 
static const char os_ver[] = "(UNIX)";
#endif

static const HDI_CMDLINE_OPT options[] = 
{
    { "at",   com_commopt_ack_timo,    TRUE  },  /* acknowledge pkt timeout */
    { BAUD_OPT, set_serial_baud,       TRUE  },  /* serial baud rate        */
    { "d",    set_debug,               FALSE },  /* debug                   */
    { "d1",   set_mon_debug,           FALSE },  /* mon debug               */
    { "ef",   set_ef,                  FALSE },  /* erase flash             */
#ifdef MSDOS
    { "freq", com_seropt_freq,         TRUE  },  /* UART freq               */
#endif
    { "h",    do_help,                 FALSE },  /* help                    */
    { "hpt",  com_commopt_host_timo,   TRUE  },  /* host packet timeout     */
    { "ip",   set_start_ip,            TRUE  },  /* start ip                */
    { "it",   set_it,                  FALSE },  /* interrupt target        */
    { "mpl",  com_commopt_max_pktlen,  TRUE  },  /* serial max packet len   */
    { "mr",   com_commopt_max_retry,   TRUE  },  /* max retries             */
    { "ne",   set_noexec,              FALSE },  /* noexec                  */
    { "q",    set_quiet,               FALSE },  /* quiet                   */
    { "rt",   set_reset_time,          TRUE  },  /* reset time              */
    { "sc",   set_script_file,         TRUE  },  /* script file             */
    { "ta",   set_ta,                  FALSE },  /* target attributes       */
    { "tan",  set_tan,                 FALSE },  /* -ta, target name only   */
    { "tat",  set_tat,                 FALSE },  /* target attributes brief */
    { "tpt",  com_commopt_target_timo, TRUE  },  /* host packet timeout     */
    { "x",    set_exit,                FALSE },  /* exit                    */
    { "?",    do_help,                 FALSE },  /* help                    */
};
static int num_tbl_opts = sizeof(options) / sizeof(options[0]);

static const char copyright[]  = "Copyright 1995, Intel Corp."; 
static HDI_CONFIG config;
static char *application = NULL;
static char **application_argv = NULL;
static int  arch;
static char comm_used[2];     /* Track number of parallel/pci comm
                               * devices specified by the user. More
                               * than one selection is a semantic error.
                               */
              
#define MAX_SCRIPT_FILE_NEST 4
static FILE * script_file[MAX_SCRIPT_FILE_NEST]; 
static int script_file_nest = -1;

static char *prog_name = NULL;
static unsigned long start_ip = 0xffffffffL;
static int run = TRUE;
static int background = FALSE;
static DOWNLOAD_CONFIG fast_config_record = 
    { 0, 0, NULL,
        { COM_PCI_IOSPACE }   /* real mode: only supports I/O space access */
    };
static DOWNLOAD_CONFIG *fast_config = NULL;
static int vflag = TRUE;                /* used by hdil code for display */
static int ef_opt = FALSE;
static int ta_opt = FALSE;
static int ta_opt_arg = TA_FULL;
static int debug_opt = FALSE;
static int mon_debug = FALSE;
static int interruptable = FALSE;
static unsigned long picb = 0L;
static unsigned long pidb = 0L;
static unsigned long pgm_ip = 0;
static int set_serial = FALSE, defaulted_serial = FALSE, baud_set = FALSE,
           defaulted_parallel = FALSE;
static char dflt_serial_port[32], dflt_parallel_port[32];

/* --------------------------- Begin Code ------------------------------- */

static void
help_msg()
{
    fprintf(stdout, "'do/df filename' to download another program (df) erases flash first\n");
    fprintf(stdout, "'sc filename'    to start reading commands from a file\n");
    fprintf(stdout, "'quit' to end debug session\n");
}


static int
do_cmd(progname, ef_set)
char * progname;
int  ef_set;
{

    if (ef_set == TRUE)
		{
        fprintf(stdout, "Erasing flash memory, may take up to 15 seconds!\n");
        if (hdi_eeprom_erase(NO_ADDR, 0) != OK)
			{
            print_hdi_error("unable to erase target flash memory\n");
		    return ERR;
			}
		}
    if (hdi_download(progname, 
                     &pgm_ip, 
                     picb, 
                     pidb, 
                     TRUE, 
                     fast_config, 
                     ! vflag) != OK)
        {
        print_hdi_error("error during download\n");
        return ERR;
        }
    if (debug_opt)
    {
        /*
         * Clean up HDI's I/O file descriptors so that multiple downloads
         * in debug mode don't open too many host descriptors.
         */

        hdi_restart();
    }
    if (hdi_init_app_stack() != OK)
        {
        print_hdi_error("error establishing application stack\n");
        return ERR;
        }
    if (hdi_reg_put(REG_G12, (REG)pidb) != OK)
        {
        print_hdi_error("unable to set G12(offset)\n");
        return ERR;
        }

    return OK;
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: main                                 */
/*                                                                           */
/* ACTION:                                                                   */
/*    main is the entry point                                              */
/*                                                                           */
/*****************************************************************************/
main(argc, argv)
int argc;
char **argv;
{
    int exit_value = OK;

    signal(SIGINT, trap);
#ifdef SIGQUIT
    signal(SIGQUIT, quit);
#endif
#ifdef WIN95
    signal(SIGBREAK, ctrl_break);
#endif
#ifdef __HIGHC__
    install_ctrlbrk_handler(ctrl_brk, 64, NULL, 0);
#endif

    envargs();
    getargs(argc, argv);
    establish_defaults();

    configure_comm();

    if (vflag)
        fprintf(stdout, "%s %s, %s\n", gnu960_ver, os_ver, copyright);

    if (ef_opt || debug_opt)
        process_option("hpt", "20000");

    if (hdi_init(&config, &arch) != OK)
		{
        print_hdi_error("unable to initialize target interface\n");
        error((char *)NULL);
		}

    if (vflag)
        {
        char vbuf[128];
        hdi_version(vbuf, sizeof(vbuf));
        fprintf(stdout,"%s\n", vbuf);
        }

    if (ta_opt)
        {
        int rc = format_mon_cfg_msg(ta_opt_arg);

        (void) hdi_term(FALSE);
        leave(rc);

        /* NOT REACHED */
        }

    if (application)
        {
        if (do_cmd(application, ef_opt) == ERR)
            error((char *)NULL);

        if (start_ip == 0xffffffffL)
            start_ip = pgm_ip;
        }

    if (!debug_opt)
        {
        if (run && (start_ip != 0xffffffffL))
            {
            if (hdi_reg_put(REG_RIP, (REG)start_ip) != OK)
				{
                print_hdi_error("unable to set IP\n");
                error((char *)NULL);
				}
            start_target(background, GO_RUN, 0, &exit_value);
            }
        }
    else   /*debug mode */
        {
        char cmd_line[200];

        if (vflag)
            help_msg();
 
        if ((start_ip != 0xffffffffL) && (hdi_reg_put(REG_RIP, (REG)start_ip) != OK))
            print_hdi_error("unable to set IP\n");

        while(TRUE)
            {
            int go_mode=GO_RUN, cmd_index;

            do
                {
				cmd_line[0] = 0;
                fprintf(stdout, "-> ");

                if (script_file_nest < 0)
                {
                    char *cp;

                    if (! fgets(cmd_line, sizeof(cmd_line), stdin))
                    {
                        /* 
                         * User typed EOF char at terminal. Fake a 'qu'
                         * to exit cleanly and kill target connection.
                         */

                        cmd_line[0] = 'q';
                        cmd_line[1] = 'u';
                        cmd_line[2] = '\0';
                    }
                    /* Strip trailing spaces and delimiters (e.g., CR/LF). */
                    cp = cmd_line + strlen(cmd_line) - 1;
                    while ((cp >= cmd_line) && isspace(*cp))
                        *cp-- = '\0';
                }
                else
                    {
                    if (fgets(cmd_line, 
                        sizeof(cmd_line),
                        script_file[script_file_nest]) != NULL)
						{
						cmd_index = 0;
                        while (cmd_line[cmd_index] != 0x00)
                            cmd_index++;
                        fprintf(stdout, "%s",cmd_line);
						cmd_line[cmd_index-1] = 0x00;
						}
                    else
                        {
                        fprintf(stdout, "Script file [%i] closed.\n", script_file_nest+1);
                        fclose(script_file[script_file_nest--]);
                        }
                    }

                cmd_index = 0;
                while (cmd_line[cmd_index] == ' ')
                    cmd_index++;

                } while (cmd_line[cmd_index] == 0);

            if ((('q' == cmd_line[cmd_index]) && ('u' == cmd_line[cmd_index+1]))||
                (('r' == cmd_line[cmd_index]) && ('b' == cmd_line[cmd_index+1])))
				break;


            switch (cmd_line[cmd_index]*256 + cmd_line[cmd_index+1])
                {
                case 'g'*256 + 'o':  
                case 's'*256 + 't':  
                case 'p'*256 + 's':  
                    if (cmd_line[cmd_index+2] != 0)
                        {
                        if (! ((hdi_convert_number(&cmd_line[cmd_index + 2],
                                                  (long *) &start_ip,
                                                  HDI_CVT_UNSIGNED,
                                                  16,
                                                  NULL) == OK)
                                          &&
                              (hdi_reg_put(REG_RIP, (REG)start_ip) == OK)))
                            {
                            print_hdi_error("error setting IP\n");
                            break;
                            }
                        }
                    switch (cmd_line[cmd_index])
                        {
                        case 'g':
                            go_mode = GO_RUN;
                            break;
                        case 'p':
                            go_mode = GO_NEXT;
                            break;
                        case 's':
                            go_mode = GO_STEP;
                            break;
                        }
                    start_target(FALSE, go_mode, start_ip, &exit_value);
                    break;

                case 'd'*256 + 'o':  
                case 'd'*256 + 'f':  

                    create_do_cmd_argv(&cmd_line[cmd_index+2]);
                    if (do_cmd(application, (cmd_line[cmd_index+1] == 'f')) != OK)
                        break;

                    if (hdi_reg_put(REG_RIP, (REG)pgm_ip) != OK)
                        {
                        print_hdi_error("unable to set IP\n");
                        break;
                        }

                    start_ip = pgm_ip;
                    fprintf(stdout, "IP set at 0x%08lx\n", pgm_ip);
                    break;

                case 'r'*256 + 's':  
                    hdi_reset();
                    break;

                case 'a'*256 + 'p':  
                    handle_aplink_cmd(&cmd_line[cmd_index + 2]);
                    break;

                case 't'*256 + 'a':   /* Target Attributes */
                    (void) format_mon_cfg_msg(TA_FULL);
                    break;

                case 'm'*256 + 'b':  
                case 'm'*256 + 'o':  
                    fprintf(stdout, "mo/mb commands not available (use 'md').\n");
                    break;

                case 's'*256 + 'c':  
                    cmd_index++; cmd_index++;
                    while (cmd_line[cmd_index] == ' ')
                        cmd_index++;

                    if (script_file_nest >= MAX_SCRIPT_FILE_NEST-1)
						{
                        fprintf(stdout, "Unable to nest script files over 4 deep.\n");
                        while (script_file_nest >= 0)
							{
                            fprintf(stdout, "Script file [%i] closed.\n", script_file_nest+1);
                            fclose(script_file[script_file_nest--]);
							}
						}
                    else if ((script_file[++script_file_nest] = 
                              fopen((const char *)&cmd_line[cmd_index],"r")) == NULL)
                        {
                        fprintf(stdout, "Unable to open script file .\n");
						script_file_nest--;
						}
					else
                        fprintf(stdout, "Script file [%i] opened.\n", script_file_nest+1);
                    break;

#ifdef MSDOS
                case 'p' *256 + 'c':  
                    {
                    unsigned short pci_addr;
                    unsigned long pci_val, pci_new_val;    
                    COM_PCI_CFG pci_init = {COM_PCI_IOSPACE,
                                            COM_PCI_NO_BUS_ADDR,
                                            0,
                                            0,
                                            COM_PCI_DFLT_VENDOR,
                                            0,
                                            -1};
                    
                    if (pci_init_done == 0)
                        {
                        if (com_pci_init(&pci_init) != 0) 
                            {
                            fprintf(stdout, "unable to initialize PCI\n");
                            break;
                            }
                        pci_init_done = 1;
                        }
                    cmd_index++; cmd_index++;
                    if (cmd_line[cmd_index] != 0)
                        {
                        pci_addr=(unsigned short)strtoul(&cmd_line[cmd_index],NULL,16); 
                        if (pci_addr >= 0x80) 
                            {
                            fprintf(stdout, "PCI ADDR invalid\n");
                            break;
                            }
                        }
                    else
                        {
                        fprintf(stdout, "PCI/960 ADDR - PCI Register Values\n");
                        for (pci_addr=0; pci_addr < 0x80; pci_addr +=16)
                            {
                            fprintf(stdout, "  0x%02x/0x%02x:  %08lx  %08lx  %08lx  %08lx\n",
                                 pci_addr, pci_addr+0x80,
                                 pci_get_reg(pci_addr),
                                 pci_get_reg((unsigned short)(pci_addr + 4)),
                                 pci_get_reg((unsigned short)(pci_addr + 8)),
                                 pci_get_reg((unsigned short)(pci_addr + 12)));
                             }
                         break;
                         }

                     pci_val = pci_get_reg(pci_addr);
                     fprintf(stdout, "PCI (PC) reg at 0x%02x was 0x%08lx ",pci_addr, pci_val);
                     while (cmd_line[cmd_index] == ' ')
                         cmd_index++;
                     while ((cmd_line[cmd_index] != ' ') && (cmd_line[cmd_index] != 0))
                         cmd_index++;
                     while (cmd_line[cmd_index] == ' ')
                         cmd_index++;
                     if (cmd_line[cmd_index] != 0)
                         {
                         pci_new_val=strtoul(&cmd_line[cmd_index],NULL,16);
                         pci_put_reg(pci_addr, pci_new_val);
                         fprintf(stdout, "now set to 0x%08lx ",pci_new_val);
                         }
                      
                     fprintf(stdout, "\n");
                     }

                     break;
#endif

                case 'h' *256 + 'e':  
					help_msg();
                    /*falls thru to default to send help to UI*/
                default:
                    if (! (cmd_line[cmd_index] == '#' || 
                                                   cmd_line[cmd_index] == ';'))
                    {
                        if (hdi_ui_cmd(cmd_line) != OK)
                            fprintf(stdout, "%s\n", hdi_get_message());
                    }
                    break;
                }

            if (mon_debug == TRUE)
                {
                if (hdi_check_for_debug_msg() != OK)
                    print_hdi_error("error on debug message from target\n");
                }
            }
        }

        while (script_file_nest >= 0)
			{
            fprintf(stdout, "Script file [%i] closed.\n", script_file_nest+1);
            fclose(script_file[script_file_nest--]);
			}

        if (prog_name != NULL)
           free(prog_name);

        hdi_term(background);
        return exit_value;
}

static void
error(fmt)
const char *fmt;
{
    if (fmt != NULL)
        fprintf(stdout,fmt);
    hdi_term(FALSE);
    leave(ERR);
}


static void
print_hdi_error(fmt)
const char *fmt;
{
    fprintf(stdout, "%s: ", prog_name);
    fprintf(stdout, "%s\n", hdi_get_message());
    if (fmt != NULL)
        fprintf(stdout,fmt);
}


static void
quit(sig)
int sig;
{
    hdi_term(sig != 0);
    leave(sig);
}

/* leave: exit the program.  For DOS and Metaware/PharLap, you MUST de-install
   hardware interrupt handlers before exiting. */
static void
leave(error_code)
int error_code;
{
#ifdef __HIGHC__
    de_install_ctrlbrk_handler();
#endif
    exit(error_code);
}

static int
do_help(unused1, unused2)
const char *unused1, *unused2;
{
    usage(NULL, NULL);
    return (OK); /* NOTREACHED */
}

static int
set_debug(unused1, unused2)
const char *unused1, *unused2;
{
    debug_opt = TRUE;
    return (OK);
}

static int
set_ef(unused1, unused2)
const char *unused1, *unused2;
{
    ef_opt = TRUE;
    return (OK);
}

static int
set_exit(unused1, unused2)
const char *unused1, *unused2;
{
    background = TRUE;
    return (OK);
}

static int
set_it(unused1, unused2)
const char *unused1, *unused2;
{
    config.intr_trgt = TRUE;
    return (OK);
}

static int
set_mon_debug(unused1, unused2)
const char *unused1, *unused2;
{
    debug_opt = mon_debug = TRUE;
    return (OK);
}

static int
set_noexec(unused1, unused2)
const char *unused1, *unused2;
{
    run = FALSE;
    return (OK);
}

static int
set_parallel_port(arg, err_prefix)
const char *arg, *err_prefix;
{
    const char *port_p = arg;
    int        use_default_port = FALSE;

    /* 
     * We permit the user to default the parallel port arg for various hosts.
     * Check now to see if user is taking advantage of this feature.
     */
    if (arg && ! IS_OPTION(*arg))
    {
#ifdef MSDOS
        /*
         * DOS (ugh).  Can't stat "arg" because stat does not work in the
         * dos root directory.  Instead, check to see if user specified
         * one of the three supported DOS parallel ports.
         */
        if (!  ((*arg == 'L' || *arg == 'l') &&
             (arg[1] == 'P'  || arg[1] == 'p') &&
             (arg[2] == 'T'  || arg[2] == 't') &&
             (arg[3] == '1'  || arg[3] == '2' || arg[3] == '3') &&
             (arg[4] == '\0')))
        {
            use_default_port = TRUE;
        }
#else
        /*
         * If arg is a regular file, it's the beginning of a load command
         * line and we must, therefore, default the parallel port (if possible).
         */

        int         can_access_arg;
        struct stat stat_info;

        can_access_arg = (stat(arg, &stat_info) == 0);
        if (! can_access_arg || (can_access_arg && S_ISREG(stat_info.st_mode)))
        {
            /*
             * Argument exists and is a regular file or the argument does 
             * not appear to exist.  So we'll default the parallel port and
             * not consume the argument (which will be treated as a load
             * file).  This is appropriate action for the former case and
             * ought to really get the user's attention in the latter case
             * for commands like this:
             *
             *    $ mondb -par misspelled_parallel_port
             */

            use_default_port = TRUE;
        }
#endif
    }
    else
    {
        /* Either no arg or arg is a command line option. */

        use_default_port = TRUE;
    }

    if (use_default_port)
    {
        arg = NULL;
#if defined(MSDOS) || defined(WIN95)
        strcpy(dflt_parallel_port, "LPT1");
#else
#if defined(HP700)
        strcpy(dflt_parallel_port, "/dev/ptr_parallel");
#else
#if defined(RS6000)
        strcpy(dflt_parallel_port, "/dev/lp0");
#else
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)
        strcpy(dflt_parallel_port, "/dev/bpp0");
#else
#if defined(I386_NBSD1)
	strcpy(dflt_parallel_port, "/dev/lpt0");
#else
        {
            char msg[128];

            /* Have no known default.... */

            sprintf(msg, 
                "%s: default port unknown for this host, specify explicitly\n",
                    err_prefix);
            error(msg);
        }
#endif
#endif
#endif
#endif
#endif
        port_p = dflt_parallel_port;
    }
    defaulted_parallel                   = use_default_port;
    fast_config_record.download_selector = FAST_PARALLEL_DOWNLOAD;
    fast_config_record.fast_port         = (char *) port_p;
    fast_config                          = &fast_config_record;
    comm_used[PARA_COMM_INDX]            = 1;
    return (! defaulted_parallel); /* return (T) -> consumed argument */
}

static int
set_quiet(unused1, unused2)
const char *unused1, *unused2;
{
    vflag = FALSE;
    return (OK);
}

static int
set_serial_baud(arg, err_prefix)
const char *arg, *err_prefix;
{
    int ec;

    if ((ec = com_seropt_baud(arg, err_prefix)) != OK)
        leave(ec);
    baud_set = TRUE;
    return (OK);
}

static int
set_serial_port(arg, err_prefix)
const char *arg, *err_prefix;
{
    int        ec, use_default_port = FALSE;
    const char *port_p = arg;

    /* 
     * We permit the user to default the serial port arg for various hosts.
     * Check now to see if user is taking advantage of this feature.
     */
    if (arg && ! IS_OPTION(*arg))
    {
#ifdef MSDOS
        /*
         * DOS (ugh).  Can't stat "arg" because stat does not work in the
         * dos root directory.  Instead, check to see if user specified
         * one of the four canonical DOS serial ports.
         */
        if (! ((*arg == 'C' || *arg == 'c') &&
            (arg[1] == 'O'  || arg[1] == 'o') &&
            (arg[2] == 'M'  || arg[2] == 'm') &&
            (arg[3] == '1' || arg[3] == '2' || arg[3] == '3' || arg[3] == '4') &&
            (arg[4] == '\0' || arg[4] == '\0')))

        {
            use_default_port = TRUE;
        }
#else
        /*
         * If arg is a regular file, it's the beginning of a load command
         * line and we must, therefore, default the serial port (if possible).
         */

        int         can_access_arg;
        struct stat stat_info;

        can_access_arg = (stat(arg, &stat_info) == 0);
        if (! can_access_arg || (can_access_arg && S_ISREG(stat_info.st_mode)))
        {
            /*
             * Argument exists and is a regular file or the argument does 
             * not appear to exist.  So we'll default the serial port and
             * not consume the argument (which will be treated as a load
             * file).  This is appropriate action for the former case and
             * ought to really get the user's attention in the latter case
             * for commands like this:
             *
             *    $ mondb -ser misspelled_serial_port
             */

            use_default_port = TRUE;
        }
#endif
    }
    else
    {
        /* Either no arg or arg is a command line option. */

        use_default_port = TRUE;
    }

    if (use_default_port)
    {
        arg = NULL;
#if defined(MSDOS) || defined(WIN95)
        strcpy(dflt_serial_port, "COM1");
#else
#if defined(HP700)
        strcpy(dflt_serial_port, "/dev/tty00");
#else
#if defined(RS6000)
        strcpy(dflt_serial_port, "/dev/tty0");
#else
#if defined(SUNOS_SUN4) || defined(SOLARIS_SUN4)
        strcpy(dflt_serial_port, "/dev/ttya");
#else
#if defined(I386_NBSD1)
	strcpy(dflt_serial_port, "/dev/tty00");
#else
        {
            char msg[128];

            /* Have no known default.... */

            sprintf(msg, 
                "%s: default port unknown for this host, specify explicitly\n",
                    err_prefix);
            error(msg);
        }
#endif
#endif
#endif
#endif
#endif
        port_p = dflt_serial_port;
    }

    if ((ec = com_seropt_port(port_p, err_prefix)) != OK)
        leave(ec);

    set_serial       = TRUE;
    defaulted_serial = use_default_port;
    return (! defaulted_serial); /* return (T) -> consumed argument */
}

static int
set_start_ip(arg, err_prefix)
const char *arg, *err_prefix;
{
    int ec;

    if ((ec = hdi_convert_number(arg,
                                 (long *) &start_ip,
                                 HDI_CVT_UNSIGNED,
                                 16,
                                 err_prefix)) != OK)
    {
        leave(ec);
    }
    return (OK);
}

static int
set_reset_time(arg, err_prefix)
const char *arg, *err_prefix;
{
    int  ec;
    long rt;

    if ((ec = hdi_convert_number(arg,
                                 &rt,
                                 HDI_CVT_UNSIGNED,
                                 16,
                                 err_prefix)) != OK)
    {
        leave(ec);
    }
    if (rt < 1 || rt > 60)
        leave(hdi_invalid_arg(err_prefix));
    config.reset_time = (int)(rt * 1000);
    return (OK);
}

static int
set_script_file(arg, err_prefix)
const char *arg, *err_prefix;
{
    if ((script_file[++script_file_nest] = fopen(arg, "r")) == NULL)
    {
        char buf[256];

        sprintf(buf,"%s: unable to open \"%s\"", err_prefix, arg);
        perror(buf);
        leave(1);
    }
    debug_opt = TRUE;
    return (OK);
}

static int
set_ta(unused1, unused2)
const char *unused1, *unused2;
{
    ta_opt = TRUE;
    return (OK);
}

static int
set_tan(unused1, unused2)
const char *unused1, *unused2;
{
    ta_opt     = TRUE;
    ta_opt_arg = (TA_NAME_ONLY | TA_TERSE);
    return (OK);
}

static int
set_tat(unused1, unused2)
const char *unused1, *unused2;
{
    ta_opt     = TRUE;
    ta_opt_arg = (TA_FULL | TA_TERSE);
    return (OK);
}



static void 
out_of_mem(msg)
const char *msg;
{
    hdi_cmd_stat = E_NOMEM;
    fprintf(stderr, "%s: %s\n", hdi_get_message(), msg);
    leave(1);
}



/* Check for args from an env var and process same if found. */
static void
envargs()
{
    char *env = getenv(ENV_VAR);

    if (env)
    {
        int  argc = 0, alloc_count = 20;
        char **argv, buf[32], *cp = strtok(env, " \t"), *tmp;

        strcpy(buf, ENV_VAR);
        strcat(buf, " (Env Var)");
        prog_name = buf;
        tmp  = malloc(strlen(buf) + 1);
        argv = (char **) calloc(alloc_count, sizeof(*argv));
        if (! (tmp && argv))
            out_of_mem("attempting to parse env var");
        strcpy(tmp, buf);
        argv[0] = tmp;
        argc    = 1;
        while (cp)
        {
            if (argc >= alloc_count - 1)  /* "- 1" to leave NULL arg at */
            {                             /* end of argv                */
                char **tmp2;

                /*
                 * Lengthen the argv vector...could use realloc, but
                 * be lazy and use calloc() to get the side effect of
                 * zeroing allocated memory.
                 */
                alloc_count += 20;
                tmp2         = (char **) calloc(alloc_count, sizeof(*argv));
                if (! tmp2)
                    out_of_mem("attempting to parse env var");
                memcpy(tmp2, argv, argc * sizeof(*argv));
                free(argv);
                argv = tmp2;
            }
            tmp = malloc(strlen(cp) + 1);
            if (! tmp)
                out_of_mem("attempting to parse env var");
            strcpy(tmp, cp);
            argv[argc++] = tmp;
            cp = strtok(NULL, " \t");
        }
        getargs(argc, argv);

        /*
         * Note the following restriction well:
         *
         * It's NOT okay to free the argv container or any of its elements
         * because these objects may be retained for [ab]use by the client
         * (cf., the last few lines of getargs()).
         */
    }
}



/*
 * Create a global var that can be tracked between successive invocations
 * of create_do_cmd_argv() to determine whether or not an old arg vector
 * should be free'd.
 */
static char **do_cmd_argv = NULL;

static void
create_do_cmd_argv(cmdline)
char *cmdline;
{

    int  argc = 0, alloc_count = 20;
    char **argv, *cp = strtok(cmdline, " \t"), *tmp;

    if (cp == NULL)
    {
        /* User specified no program or command line args, nothing to do. */

        application = '\0';
        return;
    }

    if (do_cmd_argv)
    {
        int i = 0;

        /* Currently have a do_cmd arg vector, free it. */

        while (do_cmd_argv[i])
            free(do_cmd_argv[i++]);

        free(do_cmd_argv);
    }

    argv = (char **) calloc(alloc_count, sizeof(*argv));
    if (! argv)
        out_of_mem("attempting to create cmd line args");
    while (cp)
    {
        if (argc >= alloc_count - 1)  /* "- 1" to leave NULL arg at */
        {                             /* end of argv                */
            char **tmp2;

            /*
             * Lengthen the argv vector...could use realloc, but
             * be lazy and use calloc() to get the side effect of
             * zeroing allocated memory.
             */
            alloc_count += 20;
            tmp2         = (char **) calloc(alloc_count, sizeof(*argv));
            if (! tmp2)
                out_of_mem("attempting to create cmd line args");
            memcpy(tmp2, argv, argc * sizeof(*argv));
            free(argv);
            argv = tmp2;
        }
        tmp = malloc(strlen(cp) + 1);
        if (! tmp)
            out_of_mem("attempting to create cmd line args");
        strcpy(tmp, cp);
        argv[argc++] = tmp;
        cp = strtok(NULL, " \t");
    }

    /* 
     * The variables "application" and "application_argv" will be used by
     * hdi_get_cmd_line() to build a command line argument vector that will
     * be passed to an app on a target.
     */
    application      = argv[0];
    application_argv = do_cmd_argv = argv;
}



static void
getargs(argc, argv)
int argc;
char **argv;
{
    int i;

    prog_name = basename(argv[0], (char *)NULL);

    /* Initialize configuration parameters whose proper defaults are not 0. */
    config.mon_priority = -1;
    config.tint = -1;

    /*
     * process all options
     */
    for (i = 1; i < argc && IS_OPTION(argv[i][0]); i++)
        {
#ifdef MSDOS
        char *cp = &argv[i][1];

        if ((*cp == 'p' || *cp == 'P') &&
                           (cp[1] == 'c' || cp[1] == 'C') &&
                                            (cp[2] == 'i' || cp[2] == 'I'))
            {
            process_pci_option(argv + i, &i);
            }
        else
#endif
            {
            if (process_option(&argv[i][1], argv[i+1])) i++;
            }
        }

    /* Did the user supply more comm options than can be processed? */
    if ((comm_used[PARA_COMM_INDX] && comm_used[PCI_COMM_INDX])  ||
         (comm_used[PCI_COMM_INDX] > PCI_COMM_MAX_VAL)           ||
         (comm_used[PCI_COMM_INDX] == PCI_COMM_MIN_VAL))
        {
        fprintf(stderr, 
                "-pci, -pciv, -pcib, and -par are mutually exclusive\n");
        leave(ERR);
        }

    /*
     * The remainder of the input line is presumably the download filename
     * and its arguments.
     */
    if (i < argc) 
        {
        application_argv = &argv[i];
        application      = argv[i];
        }
}



/*
 * We need some sane defaults for the communications port.
 */
static void
establish_defaults()
{
#ifdef MSDOS
    /*
     * Say, did the user DOS specify a communication medium?  If not, 
     * that's a gross error because DOS supports either serial or PCI
     * comm and we can't guess the user's intentions.
     */
    if (! (set_serial || comm_used[PCI_COMM_INDX]))
    {
        fprintf(stderr, "%s: communication channel not specified\n", prog_name);
        leave(1);
    }
#else  /* Unix */
    if (! set_serial)
    {
        /* See if a default serial port can be specified for Unix users. */
        (void) process_option(SER_OPT, NULL);
    }
#endif

    /* 
     * And now, if the user specified a serial port, but no baud rate,
     * pick something that's useful for the current host.
     */
    if (set_serial && ! baud_set)
    {
        char *baud_default;

#ifdef MSDOS
        baud_default = "115200";
#else
        baud_default = "38400";
#endif
        (void) process_option(BAUD_OPT, baud_default);
        if (vflag)
        {
            fprintf(stdout,
                    "%s: using default baud rate of %s.\n", 
                    prog_name,
                    baud_default);
        }
    }
    if (defaulted_serial && vflag)
    {
        fprintf(stdout,
                "%s: using default serial port, %s.\n", 
                prog_name,
                dflt_serial_port);
    }
    if (defaulted_parallel && vflag)
    {
        fprintf(stdout,
                "%s: using default parallel port, %s.\n", 
                prog_name,
                dflt_parallel_port);
    }
}



/*
 * Set up data structures to handle user-specified communication channel.
 */
static void
configure_comm()
{
#ifdef MSDOS
    /*
     * DOS user may select PCI or serial COMM.  The routine
     * establish_defaults() has already determined that the user
     * has specified at least one channel....  Do the rest of the
     * config work.
     */

    if (! set_serial)
    {
        /* User wants PCI comm.  Make it so. */

        com_select_pci_comm();
        com_pciopt_cfg(&fast_config->init_pci);
    }
    else
    {
        /*
         * Using serial COMM.  Check if user wants PCI download and handle
         * its initialization.
         */

        com_select_serial_comm();
        if (fast_config_record.download_selector == FAST_PCI_DOWNLOAD)
        {
            com_get_pci_controlling_port(
                        fast_config_record.init_pci.control_port
                                        );
        }
    }
#else

    /*
     * Unix users get serial COMM only, which is HDI's default channel. 
     * Nothing to do here.
     */

#endif
}



static int
process_option(token, arg)
const char *token;
const char *arg;
{
    int  ec, j;
    char err_prefix[128], unknown[80];

    sprintf(err_prefix, "%s: -%s", prog_name, token);

    if (*token == 'p' && 
            token[1] == 'i' &&
                (token [2] == 'c' || token[2] == 'd' || token[2] == 'x') &&
                                                               token[3] == '\0')
    {
        /* -pic or -pid or -pix */

        unsigned long val, *valp;

        if (! arg)
        {
            strcat(err_prefix, ": requires memory offset argument\n");
            error(err_prefix);
        }
        if (token[2] == 'c')
            valp = &picb;
        else if (token[2] == 'd')
            valp = &pidb;
        else
            valp = &val;

        if ((ec = hdi_convert_number(arg,
                         (long *) valp,
                         (*arg == '-') ? HDI_CVT_SIGNED : HDI_CVT_UNSIGNED,
                         16,
                         err_prefix)) != OK)
            {
            leave(ec);
            }
        if (token[2] == 'x')   /* -pixb */
            picb = pidb = val;
        return (TRUE);
    }

    /* Does the user want parallel dnload? */
    if (strcmp(token, "par") == 0)
        return (set_parallel_port(arg, err_prefix));

    /* Does user want serial comm? */
    if (strcmp(token, SER_OPT) == 0)
        return (set_serial_port(arg, err_prefix));

    /* Remaining options are processed via table-driven lookup. */
    for (j = 0; j < num_tbl_opts; j++)
    {
        if (strcmp(token, options[j].name) == 0)
        {
            /* option that requires argument? */

            if (options[j].needs_arg)
            {
                /* bomb out if argument missing */

                if ((ec = hdi_opt_arg_required(arg, err_prefix)) != OK)
                    leave(ec);
            }
            else
                arg = NULL;

            if ((ec = (options[j].hndlr)(arg, err_prefix)) != OK)
                leave(ec);
            
            return (arg != NULL);
        }
    }

    if (strchr(prog_name, '(') == NULL)
    {
        sprintf(unknown, 
                "-%s: unknown option, type '%s -h' for help\n", 
                token,
                prog_name);
    }
    else
    {
        /*
         * Syntax error in env var...don't tell user to type env var name 
         * for help :-) 
         */

        sprintf(unknown, "%s: unknown option -%s\n", prog_name, token);
    }
    error(unknown);

    return 0; /* NOTREACHED */
}


#ifdef MSDOS /* Currently, PCI operations only supported on DOS/WIN95 */

/* Assumptions:  the first three chars of argv[0] are "pci" */
static void
process_pci_option(argv, arg_count)
char **argv;
int  *arg_count;
{
    int           ec;
    char          *token = argv[0], prefix[128];
    unsigned long vendor_id, device_id, bus_no, dev_no, func_no;

    token += 4;    /* Skip past leading commandline switch char & "pci" */

    if (*token == 'f' && token[1] == '\0')
    {
        /* -pcif command (pci find) */

        vendor_id = (unsigned long) COM_PCI_DFLT_VENDOR;
        if (argv[1])
        {
            (*arg_count)++;
            sprintf(prefix, "%s: -%s", prog_name, "pcif");
            if ((ec = hdi_convert_number(argv[1],
                                         (long *) &vendor_id, 
                                         HDI_CVT_UNSIGNED,
                                         16,
                                         prefix)) != OK)
            {
                leave(ec);
            }
            if (argv[2])
            {
                (*arg_count)++;
                if ((ec = hdi_convert_number(argv[2],
                                             (long *) &device_id, 
                                             HDI_CVT_UNSIGNED,
                                             16,
                                             prefix)) != OK)
                {
                    leave(ec);
                }
            }
            else
            {
                /* specified a vendor ID without a device ID */

                usage(NULL, NULL);
            }
        }
        if ((ec = com_find_pci_devices((int) vendor_id, (int) device_id)) 
                                                           == COM_PCI_FIND_ERR)
        {
            fprintf(stderr, "%s: %s", prog_name, hdi_get_message());
        }
        else if (ec == COM_PCI_FIND_NONE)
            fprintf(stderr, "%s: no devices found", prog_name);
        leave(ec);
    }
    else if (*token == 'l' && token[1] == '\0')
    {
        /* 
         * -pcil command (pci list).  Note that an arg of "-1" implies
         * dumping devices for all possible PCI buses.
         */

        long bus_no = 0;

        if (argv[1] && 
                      ((! IS_OPTION(argv[1][0])) || strcmp(argv[1], "-1") == 0))
        {
            (*arg_count)++;
            sprintf(prefix, "%s: -%s", prog_name, "pcil");
            if ((ec = hdi_convert_number(argv[1],
                                         (long *) &bus_no, 
                                         HDI_CVT_SIGNED,
                                         10,
                                         prefix)) != OK)
            {
                leave(ec);
            }
        }
        if ((ec = com_list_pci_bus((int) bus_no)) == COM_PCI_LIST_ERR)
            fprintf(stderr, "%s: %s", prog_name, hdi_get_message());
        else if (ec == COM_PCI_LIST_NONE)
            fprintf(stderr, "%s: no devices found", prog_name);
        leave(ec);
    }
    else if (*token == '\0')
    {
        /* -pci command */

        fast_config_record.download_selector  = FAST_PCI_DOWNLOAD;

        /* Establish some default PCI configuration values. */
        fast_config_record.init_pci.bus       = COM_PCI_NO_BUS_ADDR;
        fast_config_record.init_pci.func      = COM_PCI_DFLT_FUNC;
        fast_config_record.init_pci.vendor_id = COM_PCI_DFLT_VENDOR;
        fast_config_record.fast_port          = PCI_UNUSED_FAST_PORT;
        fast_config                           = &fast_config_record;
        comm_used[PCI_COMM_INDX]             |= PCI_COMM_DFLT;
    }
    else if (*token == 'c' && token[1] == '\0')
    {
        /* -pcic command */

        if (! argv[1])
        {
            /* No configuration specified. */

            usage(NULL, NULL);
        }
        (*arg_count)++;
        if (strcmp(argv[1], "io") == 0)
            fast_config_record.init_pci.comm_mode = COM_PCI_IOSPACE;
        else if (strcmp(argv[1], "mmap") == 0)
            fast_config_record.init_pci.comm_mode = COM_PCI_MMAP;
        else
            usage(NULL, NULL);
    }
    else if (*token == 'd' && token[1] == '\0')
    {
        /* undocumented -pcid command (enable pci debug) */

        com_pci_debug(TRUE);
    }
    else if (*token == 'v' && token[1] == '\0')
    {
        /* -pciv command */

        if (! (argv[1] && argv[2]))
        {
            /* missing vendor and/or device id */

            usage(NULL, NULL);
        }

        (*arg_count)++;
        sprintf(prefix, "%s: -%s", prog_name, "pciv");
        if ((ec = hdi_convert_number(argv[1],
                                     (long *) &vendor_id, 
                                     HDI_CVT_UNSIGNED,
                                     16,
                                     prefix)) != OK)
        {
            leave(ec);
        }
        (*arg_count)++;
        if ((ec = hdi_convert_number(argv[2],
                                     (long *) &device_id, 
                                     HDI_CVT_UNSIGNED,
                                     16,
                                     prefix)) != OK)
        {
            leave(ec);
        }
        fast_config_record.download_selector  = FAST_PCI_DOWNLOAD;
        fast_config_record.init_pci.bus       = COM_PCI_NO_BUS_ADDR;
        fast_config_record.init_pci.func      = COM_PCI_DFLT_FUNC;
        fast_config_record.init_pci.vendor_id = (int) vendor_id;
        fast_config_record.init_pci.device_id = (int) device_id;
        fast_config_record.fast_port          = PCI_UNUSED_FAST_PORT;
        fast_config                           = &fast_config_record;
        comm_used[PCI_COMM_INDX]             |= PCI_COMM_VND;
    }
    else if (*token == 'b' && token[1] == '\0')
    {
        /* -pcib command */

        if (! (argv[1] && argv[2] && argv[3]))
        {
            /* missing some portion of required bus address */

            usage(NULL, NULL);
        }

        (*arg_count)++;
        sprintf(prefix, "%s: -%s", prog_name, "pcib");
        if ((ec = hdi_convert_number(argv[1],
                                     (long *) &bus_no, 
                                     HDI_CVT_UNSIGNED,
                                     16,
                                     prefix)) != OK)
        {
            leave(ec);
        }
        (*arg_count)++;
        if ((ec = hdi_convert_number(argv[2],
                                     (long *) &dev_no, 
                                     HDI_CVT_UNSIGNED,
                                     16,
                                     prefix)) != OK)
        {
            leave(ec);
        }
        (*arg_count)++;
        if ((ec = hdi_convert_number(argv[3],
                                     (long *) &func_no, 
                                     HDI_CVT_UNSIGNED,
                                     16,
                                     prefix)) != OK)
        {
            leave(ec);
        }
        fast_config_record.download_selector  = FAST_PCI_DOWNLOAD;
        fast_config_record.init_pci.bus       = (int) bus_no;
        fast_config_record.init_pci.dev       = (int) dev_no;
        fast_config_record.init_pci.func      = (int) func_no;
        fast_config_record.init_pci.vendor_id = COM_PCI_DFLT_VENDOR;
        fast_config_record.fast_port          = PCI_UNUSED_FAST_PORT;
        fast_config                           = &fast_config_record;
        comm_used[PCI_COMM_INDX]             |= PCI_COMM_BUS;
    }
    else
        usage(NULL, NULL);
}
#endif


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: usage                                 */
/*                                                                           */
/* ACTION:                                                                   */
/*    usage prints the invocation help command summary             */
/*                                                                           */
/*****************************************************************************/
static void
usage(msg, arg)
const char *msg;
const char *arg;
{
    if (msg)
    {
        printf("%s: ", prog_name);
        printf(msg, arg);
        printf("\n");
    }

    printf("%s usage: option... [program [args...]]\n", prog_name);
    printf("\nMiscellaneous Options:\n");
    printf("%-27s", "-d");
    printf("debug: enter User Interface mode after download\n");

    printf("%-27s", "-ef");
    printf("erase flash memory before download\n");

    printf("%-27s", "-it");
    printf("interrupt target (assumes target running a program)\n");

    printf("%-27s", "-ip  <start-ip>");
    printf("start execution at <start-ip> (hex arg required)\n", ' ');

    printf("%-27s", "-ne");
    printf("noexec: don't execute program after downloading\n");

    printf("%-27s", "-pic <offset>");
    printf("download text sections to program addr + <offset>\n");
    printf("%-27c   (hex arg required)\n", ' ');

    printf("%-27s", "-pid <offset>");
    printf("download data sections to program addr + <offset>\n");
    printf("%-27c   (hex arg required)\n", ' ');

    printf("%-27s", "-pix <offset>");
    printf("download to program file address plus <offset>\n");
    printf("%-27c   (hex arg required)\n", ' ');

    printf("%-27s", "-q");
    printf("quiet: suppress unnecessary messages\n");

    printf("%-27s", "-rt  <secs>");
    printf("reset time: wait <secs> for target reset (1-60 secs)\n");

    printf("%-27s", "-sc  <script>");
    printf("read commands from script file\n");

    printf("%-27s", "-ta[n]");
    printf("report target attributes and exit (-tan requests\n");
    printf("%-27c   only the target's common name)\n", ' ');

    printf("%-27s", "-tat");
    printf("report target attributes (terse fmt) and exit\n");


    printf("%-27s", "-x");
    printf("eXit without waiting for program to complete\n");

    printf("\nSerial Communication Options:\n");
    printf("%-27s", "-b   <baud>");
    printf("set baud rate\n");

#ifdef MSDOS
    printf("%-27s", "-freq <freq>");
    printf("set host port UART input frequency to <freq> (Hz)\n");
#endif

    printf("%-27s", "-ser [<port>]");
#ifdef MSDOS
    printf("select serial port (com[1-4])\n");
#else
    printf("select serial port\n");
#endif

#ifdef MSDOS
    printf("\nPCI Communication Options:\n");
    printf("%-27s", "-pci");
    printf("select default PCI target (defaults to Cyclone/PLX\n");
    printf("%-27c   baseboard).\n", ' ');

    printf("\n%-27s", "-pcib <bus_no> <dev_no> <func_no>\n");
    printf("%-27cselect PCI target via explicit bus address.\n", ' ');
    printf("%-27c   Hex args required.\n", ' ');

    printf("%-27s", "-pcic { io | mmap }");
    printf("configure pci communication\n");
    printf("%-27c   io   -> exchange data via I/O space\n", ' ');
    printf("%-27c   mmap -> exchange data via memory mapped access\n", ' ');

    printf("%-27s", "-pcif [<vndid> <dvcid>]");
    printf("find specified device, list PCI cfg info and exit\n");
    printf("%-27c   Default: Cyclone/PLX baseboard vendor & device\n", ' ');
    printf("%-27c   ID (hex args required).\n", ' ');

    printf("%-27s", "-pcil [<bus_no>]");
    printf("list PCI cfg info for all devices on <bus_no> & exit\n");
    printf("%-27c   Default: bus 0.  Specify bus -1 to dump devices\n", ' ');
    printf("%-27c   for all PCI buses.\n", ' ');

    printf("%-27s", "-pciv <vndid> <dvcid>");
    printf("select PCI target by specifying a PCI vendor and\n");
    printf("%-27c   device ID.  Hex args required.\n", ' ');
#endif

    printf("\nParallel Download Options:\n");
    printf("%-27s", "-par [<port>]");
#ifdef MSDOS
    printf("use parallel <port> to download pgm (lpt[1-3])\n");
#else
    printf("use parallel <port> to download pgm\n");
#endif

    printf("\nCommunication Protocol Options:\n");

    printf("%-27s", "-at  <timeout>");
    printf("set Acknowledge Timeout (1-65535 msecs)\n");

    printf("%-27s", "-hpt <timeout>");
    printf("set Host Packet Timeout (1-65535 msecs)\n");

    printf("%-27s", "-mpl <length>");
    printf("set Max Packet Length (2-4095)\n");

    printf("%-27s", "-mr  <retries>");
    printf("set Max Retries (1-255)\n");

    printf("%-27s", "-tpt <timeout>");
    printf("set Target Packet Timeout (1-65535 msecs)\n");

    printf(
#ifdef MSDOS
"\nNote 1:  At a minimum, a serial port or PCI address must be specified\n");
    printf("before a connection can be established.\n");
#else
"\nNote 1:  At a minimum, a serial port must be specified before a target\n");
    printf("connection can be established.\n");
#endif
    printf(
"Note 2:  Options and/or file arguments may also be placed in an env var\n");
printf(
"called MONDB.  Command-line options & arguments override env var settings.\n");
    leave(1);
}


/* FUNCTION NAME: trap                                 */
/*    trap is the signal handler for a cntl-C.                 */
static void
trap(sig)
int sig;
{
    signal(SIGINT, SIG_IGN);

    if (interruptable)
        hdi_signal();
    else
        printf("CTRL_C ignored\n");

    break_flag = TRUE;
    signal(SIGINT, trap);
}


#ifdef WIN95
/* FUNCTION NAME: ctrl_break                                 */
/*    trap is the signal handler for a cntl-C.                 */
static void
ctrl_break(sig)
int sig;
{
    signal(SIGBREAK, SIG_IGN);

    if (interruptable)
        hdi_signal();
    else
        printf("CTRL_Break ignored\n");

    signal(SIGBREAK, ctrl_break);
}
#endif


#ifdef __HIGHC__
/* Disabled for now.  FIXME: make this equivalent to CTRL-C.  You must
   worry about locking memory pages that are referenced by the interrupt
   handler, i.e. all memory AND stack used by hdi_signal().  Non-trivial. */
_Far _INTERRPT void
ctrl_brk()
{
    ;
}
#endif

void
hdi_put_line(p)
const char *p;
{
    fputs(p, stdout);
}

void
hdi_put_char(c)
int c;
{
    putc(c, stdout);
}


void
hdi_user_put_line(cp, len)
const char *cp;
int len;
{
    write(1, cp, len);
}

int
hdi_user_get_line(buf, size)
char *buf;
int size;
{
    return read(0, buf, size);
}

/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: start_target                             */
/*                                                                           */
/* ACTION:                                                                   */
/*    start_target provides the autostart function for a load and go.         */
/*                                                                           */
/*****************************************************************************/
static int
start_target(background, go_mode, targ_ip, exit_code)
int background;
int go_mode;
unsigned long targ_ip;
int *exit_code;
{
        const STOP_RECORD *stop_reason;
        REG ip;

    if (vflag || debug_opt) 
        {
        hdi_reg_get(REG_IP, &ip);
        fprintf(stdout,"Starting execution at 0x%08lx use CTRL-C to interrupt.\n", ip);
        }

    /* flush all output before starting program */
	fflush(stdout);
    interruptable = TRUE;
    stop_reason = hdi_targ_go(background ? go_mode|GO_BACKGROUND : go_mode);
    interruptable = FALSE;
    /* flush all output before printing any messages */
	fflush(stdout);

    target_stopped(stop_reason);

    if (stop_reason != NULL)
        {
        if (background && stop_reason->reason == STOP_RUNNING)
            {
            hdi_reg_get(REG_IP, &ip);
            fprintf(stdout,"Execution started at 0x%08lx\n", ip);
            return(OK);
            }

        if (!background && stop_reason->reason == STOP_EXIT)
            {
            if (targ_ip != 0)
                if (hdi_reg_put(REG_RIP, (REG)targ_ip) != OK)
					{
                    print_hdi_error("unable to set IP\n");
					return ERR;
					}

            hdi_restart();    /* reset file IO for next run */
            *exit_code = (int)stop_reason->info.exit_code;
            return(OK);
            }
        }

    hdi_reg_get(REG_IP, &ip);
    fprintf(stdout,"Execution stopped at 0x%08lx\n", ip);
    return(ERR);
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: target_stopped                                             */
/*                                                                           */
/* ACTION:                                                                   */
/*    cmd_init initializes global variables before a new command is parsed.*/
/*                                                                           */
/*****************************************************************************/
static int
target_stopped(stop_reason)
const STOP_RECORD *stop_reason;
{
    unsigned long unkreason;
    static const char *const fault_names[] = {
    "Parallel",        /* Type 0 */
    "Trace",        /* Type 1 */
    "Operation",        /* Type 2 */
    "Arithmetic",        /* Type 3 */
    "Floating Point",    /* Type 4 */
    "Constraint",        /* Type 5 */
    "Virtual Memory",    /* Type 6 */
    "Protection",        /* Type 7 */
    "Machine",        /* Type 8 */
    "Structural",        /* Type 9 */
    "Type",            /* Type a */
    "Reserved (0xb)",    /* Type b */
    "Process",        /* Type c */
    "Descriptor",        /* Type d */
    "Event",        /* Type e */
    "Reserved (0xf)"    /* Type f */
    };

    if (stop_reason == NULL)
        print_hdi_error(NULL);
    else
        {
        unsigned long reason = stop_reason->reason;

        if (reason & STOP_EXIT)
            fprintf(stdout,"Program exit: %ld\n", stop_reason->info.exit_code);

        if (reason & STOP_MON_ENTRY)
            fprintf(stdout,"Program called mon_entry().\n");

        if (reason & STOP_CTRLC)
            fprintf(stdout,"User interrupt\n");

        if (reason & STOP_BP_SW)
            if (reason & STOP_UNK_BP)
                fprintf(stdout,"Unknown software breakpoint at 0x%08lx\n",
                        stop_reason->info.sw_bp_addr);
            else
                fprintf(stdout,"Software breakpoint at 0x%08lx\n",
                        stop_reason->info.sw_bp_addr);

        if (reason & STOP_BP_HW)
            fprintf(stdout,"Hardware breakpoint at 0x%08lx\n",
                    stop_reason->info.hw_bp_addr);

        if (reason & STOP_BP_DATA0)
            fprintf(stdout,"Data breakpoint at 0x%08lx\n",
                    stop_reason->info.da0_bp_addr);

        if (reason & STOP_BP_DATA1)
            fprintf(stdout,"Data breakpoint at 0x%08lx\n",
                    stop_reason->info.da1_bp_addr);

        if (reason & STOP_TRACE)
            {
            int type = stop_reason->info.trace.type;

            /* Don't print a message for just a single-step */
            if (type != 0x02 || reason != STOP_TRACE)
                {
                fprintf(stdout,"Trace at 0x%08lx:", stop_reason->info.trace.ip);
                if (type & 0x02) fprintf(stdout," Single-step");
                if (type & 0x04) fprintf(stdout," Branch");
                if (type & 0x08) fprintf(stdout," Call");
                if (type & 0x10) fprintf(stdout," Return");
                if (type & 0x20) fprintf(stdout," Pre-return");
                if (type & 0x40) fprintf(stdout," Supervisor Call");
                fprintf(stdout,"\n");
                }
            }

        if (reason & STOP_FAULT)
            {
            int type = stop_reason->info.fault.type;

            if (type < sizeof(fault_names)/sizeof(fault_names[0]))
                fprintf(stdout,"%s fault at: ", fault_names[type] );
            else
                fprintf(stdout," Unknown fault of type 0x%02x at: ", type );

            fprintf(stdout,"0x%08lx, ", stop_reason->info.fault.ip);
            fprintf(stdout,"subtype 0x%02x\n", stop_reason->info.fault.subtype);
            fprintf(stdout,"Fault record is saved at 0x%08lx\n",
                    stop_reason->info.fault.record + 4);
            }

        if (reason & STOP_INTR)
            fprintf(stdout,"Unexpected interrupt %d\n", 
                    stop_reason->info.intr_vector);
        
        if (reason & STOP_UNK_SYS)
            fprintf(stdout,"Unrecognized system call\n");

        unkreason = reason & ~(STOP_EXIT|STOP_CTRLC|STOP_TRACE|STOP_FAULT
                |STOP_INTR|STOP_BP_SW|STOP_UNK_BP|STOP_BP_HW|STOP_MON_ENTRY
                |STOP_BP_DATA0|STOP_BP_DATA1|STOP_UNK_SYS);

        if (unkreason != 0)
            fprintf(stdout,"Unrecognized stop reason 0x%lx\n", unkreason);
        }

    return OK;
}


/*****************************************************************************/
/*                                                                           */
/* FUNCTION NAME: basename                             */
/*                                                                           */
/* ACTION:                                                                   */
/*    Returns a filename with the path component stripped, and optionally  */
/*      some trailing compenents removed.                                    */
/*                                                                           */
/*****************************************************************************/
static char *
basename(pp, ep)
const char *pp, *ep;
{
#define    PDELS    ":/\\"
    char *np;
    unsigned int len;

    if (pp==NULL)
        return NULL;

    while (*pp && pp[strcspn( pp, PDELS )])
        pp += strcspn(pp,PDELS) +1;

    if (*pp == '\0')
        return("");

#ifdef MSDOS
    /* Strip the stupid .exe suffix */

    if ((np = strchr(pp, '.')) != NULL)
    {
        if (strcmp(&np[1], "EXE") == 0 || strcmp(&np[1], "exe") == 0)
            *np = '\0';
    }
#endif

    len = strlen(pp);
    if (ep != NULL) {
        unsigned int newlen = len - strlen(ep);
        if (strcmp(pp + newlen, ep) == 0)
            len = newlen;
        }

    if ((np = (char *)malloc((unsigned)len+1)) != NULL)
        {
        strncpy(np, pp, len);
        np[len] = '\0';
        }

    return(np);
}



void
hdi_get_cmd_line(cmdline, len)
char *cmdline;
int len;
{
    register char **argv;
    register char *p, *end;

    if (application_argv == NULL)
        {
        cmdline[0] = '\0';
        return;
        }

    argv = application_argv;
    end = &cmdline[len-1];

    while (*argv && cmdline < end)
        {
        p = *argv++;
        while (*p && cmdline < end)
        *cmdline++ = *p++;
        *cmdline++ = ' ';
        }
    *(cmdline-1) = '\0';
}



static int
format_mon_cfg_msg(fmt)
register int fmt;
{
#define TRGT_NAME_LBL   " target common name  : "
#define YAK(x)          ((brief) ? "" : x)

    static char    *arch_str[] = { "", "KA/SA", "KB/SB", "Cx", "Jx", "Hx" };
    int            brief, i;
    HDI_MON_CONFIG mon_config_record;
    HDI_MON_CONFIG *mon_config = &mon_config_record;

    brief = (fmt & TA_TERSE);

    if (hdi_get_monitor_config(mon_config) != OK)
    {
        print_hdi_error("\n");
        return (ERR);
    }
    printf(YAK("Mon Config:\n"));

    if (fmt & TA_NAME_ONLY)
    {
        printf("%s%s\n", YAK(TRGT_NAME_LBL), mon_config->trgt_common_name);
        return (OK);
    }

    for (i = 0; i < HDI_MAX_FLSH_BNKS; i++)
    {
        unsigned long addr = mon_config->flash_addr[i];
        char          buf[64];

        sprintf(buf, " flash[%d] (addr/sz)  : ", i);
        printf((addr) ? "%s%#0.8lx : %#lx\n" : "%s%#lx : %#lx\n", 
                YAK(buf), 
                addr,
                mon_config->flash_size[i]);
    }

    printf((mon_config->ram_start_addr) ? "%s%#0.8lx\n" : "%s%#lx\n", 
           YAK(" DRAM start          : "), 
           mon_config->ram_start_addr);
    printf("%s%#lx\n", 
           YAK(" DRAM size           : "), 
           mon_config->ram_size);
    printf((mon_config->unwritable_addr) ? "%s%#0.8lx\n" : "%s%#lx\n", 
           YAK(" unwritable addr     : "), 
           mon_config->unwritable_addr);
    printf("%s%#0.8lx\n", 
           YAK(" CPU step info       : "), 
           mon_config->step_info);
    printf("%s%s\n", 
           YAK(" CPU architecture    : "), 
           (mon_config->arch >= ARCH_KA && mon_config->arch <= ARCH_HX) ?
                   arch_str[mon_config->arch] : "Unknown"
          );
    printf("%s%d\n", YAK(" bus speed (MHz)     : "), mon_config->bus_speed);
    printf("%s%d\n", YAK(" CPU speed (MHz)     : "), mon_config->cpu_speed);
    printf(YAK(" misc features       : "));
    {
        char *endian, *reloc, *timer = "no timer";

        endian = (mon_config->monitor & HDI_MONITOR_BENDIAN) ? 
                  "big" : "little";
        reloc  = (mon_config->monitor & HDI_MONITOR_RELOC) ?
                 "relocatable " : "static";
        if (mon_config->monitor & HDI_MONITOR_TRGT_HW_TMR)
            timer = "target timer";
        else if (mon_config->monitor & HDI_MONITOR_CPU_HW_TMR)
            timer = "CPU timer";

        printf("%s endian : %s : %s\n", endian, reloc, timer);
    }
    printf("%s%d\n",
           YAK(" num hw inst breaks  : "),
           mon_config->inst_brk_points);
    printf("%s%d\n",
           YAK(" num hw data breaks  : "),
           mon_config->data_brk_points);
    printf("%s%d\n", YAK(" num fp regs         : "), mon_config->fp_regs);
    printf("%s%d\n", YAK(" num sf regs         : "), mon_config->sf_regs);
    printf(YAK(" communication svcs  : "));
    {
        int comm = mon_config->comm_cfg, count = 0;

        if (comm & HDI_CFG_PARA_DNLD)
        {
            printf("para dnld");
            count++;
        }
        if (comm & HDI_CFG_PARA_COMM)
        {
            printf("%spara comm", count ? " : " : "");
            count++;
        }
        if (comm & HDI_CFG_PCI_CAPABLE)
        {
            printf("%spci comm", count ? " : " : "");
            count++;
        }
        if (comm & HDI_CFG_I2C_CAPABLE)
        {
            printf("%si2c comm", count ? " : " : "");
            count++;
        }
        if (comm & HDI_CFG_JTAG_CAPABLE)
        {
            printf("%sjtag comm", count ? " : " : "");
            count++;
        }
        if (comm & HDI_CFG_SERIAL_CAPABLE)
        {
            printf("%sserial comm", count ? " : " : "");
            count++;
        }
        fputs("\n", stdout);
    }
    printf("%s%s\n", YAK(TRGT_NAME_LBL), mon_config->trgt_common_name);
    printf("%s%d\n", 
           YAK(" config version      : "), 
           mon_config->config_version);
    if (mon_config->config_version != -1)
    {
        for (i = 0; (i < HDI_MAX_CFG_UNUSED && 
                     i < mon_config->config_version); i++)
        {
            char buf[64];

            sprintf(buf, " unused[%d]           : ", i);
            printf("%s%#0.8lx\n", YAK(buf), mon_config->unused[i]);
        }
    }

    return (OK);
#undef YAK
#undef TRGT_NAME_LBL
}



/*
 * Need to run some ApLink commands via HDI (rather than the UI) so that
 * proper comm handshake is maintained.
 */
static void
handle_aplink_cmd(cmd)
char *cmd;
{
    /* 
     * Possible commands are:
     *
     *   ap wt
     *   ap rs
     *   ap en value value
     *   ap sw value value
     *
     * The client has stripped "ap" from the cmd buffer.  Handle the rest.
     */

    while (isspace(*cmd))
        cmd++;

    if (strncmp(cmd, "wt", 2) == 0 || strncmp(cmd, "rs", 2) == 0)
    {
        char which = *cmd;

        cmd += 2;
        while (isspace(*cmd))
            cmd++;
        if (*cmd)
        {
            /* Garbage at the end of the command. */

            printf("invalid 'ap' command\n");
        }
        else if (hdi_aplink_sync(
                (which == 'r') ? HDI_APLINK_RESET : HDI_APLINK_WAIT) != OK)
        {
            print_hdi_error("\n");
        }
    }
    else if (strncmp(cmd, "en", 2) == 0 || strncmp(cmd, "sw", 2) == 0)
    {
        char          *cp;
        int           is_enable_cmd;
        unsigned long val1, val2;

        is_enable_cmd  = (*cmd == 'e');
        cmd           += 2;
        while (isspace(*cmd))
            cmd++;
        if (! (cp = strchr(cmd, ' ')))
        {
            printf("invalid 'ap' command argument\n");
            return;
        }
        *cp++ = '\0';
        if ((hdi_convert_number(cmd,
                                (long *) &val1, 
                                HDI_CVT_UNSIGNED, 
                                is_enable_cmd ? 10 : 16,
                                NULL) == OK)
                                   &&
             (hdi_convert_number(cp, 
                                 (long *) &val2, 
                                 HDI_CVT_UNSIGNED, 
                                 10,
                                 NULL) == OK))
        {
            if (is_enable_cmd)
            {
                if (hdi_aplink_enable(val1, val2) == OK)
                    return;
            }
            else
	    {
		if (hdi_aplink_switch(val1, val2) != OK)
		    {
		    print_hdi_error("\n");
		    return;
		    }
		/* Set the stack again in case a download was done first. */
		if (hdi_init_app_stack() != OK)
		    {
		    print_hdi_error("error establishing application stack\n");
		    return;
		    }
		if (hdi_reg_put(REG_G12, (REG)pidb) != OK)
		    {
		    print_hdi_error("unable to set G12(offset)\n");
		    return;
		    }
		return;
	    }
        }
        /* Fall through to HDI error condition. */

        print_hdi_error("\n");
    }
    else
        printf("invalid 'ap' command argument\n");
}
