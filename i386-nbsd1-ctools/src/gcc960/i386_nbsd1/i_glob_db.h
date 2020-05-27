#ifndef I_GLOB_DB_H
#define I_GLOB_DB_H

extern unsigned char *db_encode_call_ptype();
extern void start_func_info();
extern void dump_func_info();
extern void dump_symref_info();
extern void end_out_db_info();
extern void open_in_glob_db();
extern int  have_in_glob_db();
extern void build_in_glob_db();
extern int    glob_addr_taken_p();
extern int    glob_sram_addr();
extern char * glob_inln_info();
extern int    glob_inln_deletable();
extern int    glob_inln_tot_inline();
extern unsigned char * glob_inln_vect();
extern unsigned char * glob_inln_prof_vect();
extern unsigned char * glob_inln_rtl_buf();
extern int    glob_inln_rtl_size();
extern unsigned char * glob_prof_info();

/* Map indicating that we should save the corresponding option in the db */
extern char* db_argv;

#endif
