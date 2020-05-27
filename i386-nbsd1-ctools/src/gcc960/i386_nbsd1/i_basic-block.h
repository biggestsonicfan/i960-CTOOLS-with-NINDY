extern char* basic_block_drops_in;


/* Zombies are used to remember what used to be in an insn when
   we turn him into a deleted note, so we can revive him later. */

typedef 
  struct
  { rtx s_rtx;
    enum rtx_code s_code;
    int  s_line;
    char* s_file;
  }
  rtx_zombie;

extern rtx df_rtx[4];
extern rtx_zombie df_zombie[4];

#define ENTRY_LABEL  (( (df_rtx[0])  ))
#define ENTRY_NOTE   (( (df_rtx[1])  ))
#define EXIT_LABEL   (( (df_rtx[2])  ))
#define EXIT_NOTE    (( (df_rtx[3])  ))

#define ENTRY_LABEL_ZOMBIE  (( (df_zombie[0])  ))
#define ENTRY_NOTE_ZOMBIE   (( (df_zombie[1])  ))
#define EXIT_LABEL_ZOMBIE   (( (df_zombie[2])  ))
#define EXIT_NOTE_ZOMBIE    (( (df_zombie[3])  ))

#define ENTRY (( INSN_UID (ENTRY_LABEL) ))
#define EXIT  (( INSN_UID (EXIT_NOTE)   ))

#define BLOCK_HEAD(B)      (( basic_block_head[(B)] ))
#define BLOCK_END(B)       (( basic_block_end[(B)] ))
#define BLOCK_DROPS_IN(B)  (( basic_block_drops_in[(B)] ))
#define N_BLOCKS           (( (n_basic_blocks + 2) ))
#define ENTRY_BLOCK        (( (n_basic_blocks + 0) ))
#define EXIT_BLOCK         (( (n_basic_blocks + 1) ))

#define FIRST_REAL_BLOCK 0
#define LAST_REAL_BLOCK ((n_basic_blocks-1))

#define BORDER(B)   (( \
(/* (B)>=FIRST_REAL_BLOCK && */ (B)<=LAST_REAL_BLOCK) ? ((B)+1) : \
((B)==ENTRY_BLOCK?0:(N_BLOCKS-1)) ))

#define BUNORDER(B) (( \
((B)>0 && (B)<(N_BLOCKS-1)) ? ((B)-1) : \
((B)==0?ENTRY_BLOCK:EXIT_BLOCK) ))

extern int *uid_block_number;
#define BLOCK_NUM(INSN)  uid_block_number[INSN_UID (INSN)]

/* This is where the BLOCK_NUM values are really stored.
   This is set up by find_basic_blocks and used there and in life_analysis,
   and then freed.  */

extern short* basic_block_loop_depth;
extern char* uid_volatile;

/* Do a safe conversion from double to int */
#define DTOI_EXPECT(D) (( ((D) > 2147483647.0) ? 2147483647.0 : (D) ))
