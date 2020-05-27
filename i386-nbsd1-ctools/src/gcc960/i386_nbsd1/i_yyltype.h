typedef
  struct yyltype {
    int line;
    int lpos;
    char* file;
    int pline;
    int plpos;
    char* pfile;
  } yyltype;

#define YYLTYPE yyltype
extern YYLTYPE yylloc;
extern YYLTYPE *gyylsp;

#define PREV_LPOS (yylloc.plpos+0)
#define PREV_LINE (yylloc.pline+0)
#define PREV_FILE (yylloc.pfile+0)
#define PREV_NOTE imstg_emit_line_note (yylloc.pfile,yylloc.pline,yylloc.plpos,0);
#define LOOK_LPOS (yylloc.lpos+0)
#define LOOK_LINE (yylloc.line+0)
#define LOOK_FILE (yylloc.file+0)
#define LOOK_NOTE imstg_emit_line_note (yylloc.file,yylloc.line,yylloc.lpos,0);

#define LSTK_LPOS(N) (yylsp[(N)].lpos+0)
#define LSTK_LINE(N) (yylsp[(N)].line+0)
#define LSTK_FILE(N) (yylsp[(N)].file+0)
#define LSTK_NOTE(N) imstg_emit_line_note (yylsp[(N)].file,yylsp[(N)].line,yylsp[(N)].lpos,0);

