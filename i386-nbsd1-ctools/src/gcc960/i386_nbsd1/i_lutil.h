
extern char* sinfo_name;
extern FILE* sinfo_file;

#if !defined(SUPPRESS_I_LUTIL_H_STATICS)

static char list_blanks[] = "                ";
static char list_arrows[] = "        >>>>>   ";
static char list_pluses[] = "        +++++   ";

static char *fancy_smess[]
={"WARNING: ",
  "ERROR: ",
  "CATASTROPHIC ERROR: ",
  "INTERNAL ERROR: "
};

static char *plain_smess[]
={"warning: ",
  "",
  "fatal: ",
  "internal error: "
};

#endif /* SUPPRESS_I_LUTIL_H_STATICS */

#define LISTING_MESSAGE_LEN 128
#define LISTING_HEADER_SIZE (sizeof(list_blanks)-1)
#define LISTING_BUF_DECL_SIZE (( 8192 ))

typedef
enum { ER_WARNING, ER_ERROR, ER_FATAL, ER_INTERNAL } er_severity;

/* Change gcc960 and ic960, too, if you change this */
#define flag_list_sw "clist"
#define list_name_sw "outz"
#define ltmp_name_sw "tmpz"
