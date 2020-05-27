/* 
 *   Packhex is a hex-file compaction utility.  It attempts to concatenate
 *   hex records to produce more size-efficient packaging.
 *
 *   Limitations: Input files must be correctly formatted.  This utility
 *                is not robust enough to detect hex-record formatting
 *                errors.
 *
 *   Written by Mark Gingrich 
 *	 
 *   modified for use in rom960 by Pete Baker, Intel Architecture Labs 8/94 
 *
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "err_msg.h"
#include "rom960.h"
#include "p7mem.h"

#define YES                   1
#define MAX_LINE_SIZE       600
#define EOS                 '\0'

#ifdef DOS
#define SLASH '\\'
#else
#define SLASH '/'
#endif

/* Convert ASCII hexadecimal digit to value. */
#define HEX_DIGIT( C )   ( ( ( ( C ) > '9' ) ? ( C ) + 25 : ( C ) ) & 0xF )

typedef unsigned char   Boolean;
typedef unsigned char   Uchar;
typedef unsigned int    Uint;
typedef unsigned long   Ulong;

typedef struct   /* Functions and constant returning Hex-record vital stats. */
{
    Boolean    ( *is_data_record  )();
    Ulong      ( *get_address     )();
    Uint       ( *get_data_count  )();
    int        max_data_count;
    char      *( *get_data_start  )();
    void       ( *put_data_record )();
} Rec_vitals;

Rec_vitals  * identify_first_data_record();
Ulong         get_ndigit_hex();
int  	      packhex();
char	    * make_tempname();
void	      copyfile();

/*
 *    Intel Hex data-record layout
 *
 *    :aabbbbccd...dee
 *
 *    :      - header character
 *    aa     - record data byte count, a 2-digit hex value
 *    bbbb   - record address, a 4-digit hex value
 *    cc     - record type, a 2-digit hex value:
 *              "00" is a data record
 *              "01" is an end-of-data record
 *              "02" is an extended-address record
 *              "03" is a start record
 *    d...d  - data (always an even number of chars)
 *    ee     - record checksum, a 2-digit hex value
 *             checksum = 2's complement [ (sum of bytes: aabbbbccd...d) modulo 256 ]
 */


Boolean is_intel_data_rec( rec_str )
char *rec_str;
{
    return( ( rec_str[ 0 ] == ':' )  &&  ( rec_str[ 8 ] == '0' ) );
}

Uint get_intel_rec_data_count( rec_str )
char *rec_str;
{
    return( ( Uint ) get_ndigit_hex( rec_str + 1, 2 ) );
}

Ulong get_intel_rec_address( rec_str )
char *rec_str;
{
    return( get_ndigit_hex( rec_str + 3, 4 ) );
}

char * get_intel_rec_data_start( rec_str )
char *rec_str;
{
    return( rec_str + 9 );
}

void put_intel_data_rec( count, address, data_str, outfile )
Uint count;
Ulong address;
char *data_str;
FILE *outfile;
{
    char    *ptr;
    Uint    sum = count + ( address >> 8 & 0xff ) + ( address & 0xff );

    for ( ptr = data_str ; *ptr != EOS ; ptr += 2 )
        sum += ( Uint ) get_ndigit_hex( ptr, 2 );

    fprintf(outfile,":%02X%04lX00%s%02X\n", count, address, data_str, (~sum + 1) & 0xff );
}


/* Structure initializers */

Rec_vitals  intel_hex =
{
    is_intel_data_rec,
    get_intel_rec_address,
    get_intel_rec_data_count,
    255,                        /* Maximum data bytes in a record. */
    get_intel_rec_data_start,
    put_intel_data_rec
};

/*
 *   Put address of additional Rec_vitals structures
 *   in this array, before the NULL entry. 
 */

Rec_vitals  *formats[] =
{
    &intel_hex,
    ( Rec_vitals * ) NULL
};

#ifdef DEBUG
main(argc, argv)
    int    argc;
    char **argv;
{

packhex(argv[1]);

}
#endif

/****   packhex   *****************************************************************
*
*
*       Expects: filename to pack 
*
*       Returns: Exit status (EXIT_SUCCESS or EXIT_FAILURE).
*
*       Reads hex records from the hexfile and attempts to
*       splice adjacent data fields together.  Results are copied
*	to a temporary file and then copied back to the hex file. 
*
*******************************************************************************/

int packhex( hexfile )
char *hexfile;
{
    char       inbuff[ MAX_LINE_SIZE ], outbuff[ MAX_LINE_SIZE ];
    char       *in_dptr, *out_dptr;
    int        d_total, d_count, d_excess, n;
    Ulong      in_rec_addr, out_rec_addr;
    Rec_vitals *rptr;

    FILE *infile;    
    FILE *outfile;    
    char *new_name;
    char *filename;

    filename = copy(get_file_name(&hexfile,"hex file to compress:")); 
	 
    if ((infile = fopen(filename,"r")) == NULL) {
	error_out(ROMNAME,NOT_OPEN,0,filename);
	return(1);
    }
    new_name = make_tempname("");
    
    if ((outfile = fopen(new_name,"w")) == NULL) {
	error_out(ROMNAME,NOT_OPEN,0,new_name);
	return(1);
    }

    /* Sift through file until first hex record is identified.    */
    if ( ( rptr = identify_first_data_record( inbuff, MAX_LINE_SIZE, infile, outfile ) ) == NULL )
    {
        error_out(ROMNAME, ROM_NO_HEX_RECORDS,0,filename );
        return(1);
    }
    
    /* Attempt data-record splicing until end-of-file is reached. */
    d_total = 0;
    do
    {
        if ( rptr->is_data_record( inbuff ) == YES )
        { /* Input record is a data record. */
            d_count     = rptr->get_data_count( inbuff );
            in_rec_addr = rptr->get_address( inbuff );
            in_dptr     = rptr->get_data_start( inbuff );

            if ( d_total == 0  ||  in_rec_addr != out_rec_addr + d_total )
            { /* Begin a new output record. */
                if ( d_total != 0 )
                    rptr->put_data_record( d_total, out_rec_addr, outbuff, outfile );
                out_dptr     = outbuff;
                n = d_total  = d_count;
                out_rec_addr = in_rec_addr;
            }
            else if ( ( d_excess = d_total + d_count - rptr->max_data_count ) > 0 )
            { /* Output a maximum-length record, then start a new record. */
                strncat( outbuff, in_dptr, 2 * ( d_count - d_excess ) );
                rptr->put_data_record( rptr->max_data_count, out_rec_addr, outbuff, outfile );
                in_dptr      += 2 * ( d_count - d_excess );
                out_dptr      = outbuff;
                n = d_total   = d_excess;
                out_rec_addr += rptr->max_data_count;
            }
            else
            { /* Append input record's data field with accumulated data. */
                out_dptr = outbuff + ( 2 * d_total );
                d_total += n = d_count;
            }
            strncpy( out_dptr, in_dptr, 2 * n );
            out_dptr[ 2 * n ] = EOS;
        }
        else
        { /* Not a data record; flush accumulated data then echo non-data record. */
            if ( d_total != 0 )
            {
                rptr->put_data_record( d_total, out_rec_addr, outbuff, outfile );
                d_total = 0;
            }
            fputs( inbuff, outfile );
        }
    } while ( fgets( inbuff, MAX_LINE_SIZE, infile) != NULL );

    fclose(infile);
    fclose(outfile);
    copyfile(new_name,filename);
    unlink(new_name); 

    return(0);
}

/****   identify_first_data_record   *******************************************
*
*       Expects: Pointer to hex-record line buffer.
*
*       Returns: Pointer to hex-record structure (NULL if no match found).
*
*       Reads the standard input, line by line, searching for a valid
*       record header character.  If a valid header is found, a pointer
*       to the hex-record's type structure is returned, otherwise NULL.
*
*       The input-stream pointer is left pointing to the first valid hex record.
*
*******************************************************************************/

Rec_vitals * identify_first_data_record(buff_ptr, n, infile, outfile )
char *buff_ptr;
int n;
FILE *infile;
FILE *outfile;
{
    Rec_vitals  ** ptr;

    while ( fgets( buff_ptr, n, infile ) != NULL )
    {
        for ( ptr = formats ; *ptr != ( Rec_vitals * ) NULL ; ptr++ )
            if ( ( *ptr )->is_data_record( buff_ptr ) == YES )
                return( *ptr );        /* Successful return.        */

        fputs( buff_ptr, outfile );              /* Echo non-hex-record line. */
    }

    return( ( Rec_vitals * ) NULL );   /* Unsuccessful return.      */
}


/****   get_ndigit_hex   *******************************************************
*
*       Expects: Pointer to first ASCII hexadecimal digit, number of digits.
*
*       Returns: Value of hexadecimal string as an unsigned long.
*
*******************************************************************************/

Ulong get_ndigit_hex( cptr, digits )
char *cptr;
int digits;
{
    Ulong    value;

    for ( value = 0 ; --digits >= 0 ; cptr++ )
        value = ( value * 16L ) + HEX_DIGIT( *cptr );

    return( value );
}

#include <string.h>

/* Create a temp file in the same directory as supplied */
static
char *
make_tempname(filename)
char *filename;
{
        static char template[] = "roXXXXXX";
        char *tmpname;
        char *slash = strrchr( filename, SLASH );
        if (slash != (char *)NULL){
                *slash = 0;
                tmpname = malloc(strlen(filename) + sizeof(template) + 1 );
                strcpy(tmpname, filename);
#ifdef DOS
                strcat(tmpname, "\\" );
#else
                strcat(tmpname, "/" );
#endif
                strcat(tmpname, template);
                mktemp(tmpname );
                *slash = '/';
        } else {
                tmpname = malloc(sizeof(template));
                strcpy(tmpname, template);
                mktemp(tmpname);
        }
        return tmpname;
}

static
void
copyfile(from,to)
    char *from,*to;
{
#ifdef DOS
        FILE *fin = fopen(from,"rb");
        FILE *fout = fopen(to,"wb");
#else
        FILE *fin = fopen(from,"r");
        FILE *fout = fopen(to,"w");
#endif
        char buff[512];
        int n;
#define _CHECK(x,y) if (!x) { fprintf(stderr,"Can not open %s?\n",y); perror(y); exit(1); }
        _CHECK(fin,from);
        _CHECK(fout,to);
        while ((n=fread(buff,1,512,fin)) > 0) {
            fwrite(buff,1,n,fout);
        }
        fclose(fin);
        fclose(fout);
        unlink(from);
}
