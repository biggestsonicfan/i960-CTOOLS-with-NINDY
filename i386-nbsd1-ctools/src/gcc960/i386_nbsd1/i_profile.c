#include "config.h"

#ifdef IMSTG

#include <stdio.h>
#include "rtl.h"
#include "assert.h"
#include "flags.h"
#include "basic-block.h"
#include "i_list.h"
#include "tree.h"
#include "i_dataflow.h"
#include "i_df_set.h"
#include "cc_info.h"
#include "i_glob_db.h"
#include "i_prof_form.h"
#include "i_profile.h"
#include "i_lutil.h"

extern double d_to_d();

int prof_n_bblocks;
int prof_n_counters;
int prof_func_block;
int prof_func_used_prof_info;

static int prof_data_offset;
static rtx prof_decl;
static int original_max_uid;

static list_block_p succ_list_blks;

#define UNKNOWN_STATE       0
#define IN_QUEUE_STATE      1
#define IN_MAX_SPAN_STATE   2
#define HAS_FORMULA_STATE   3

#define MAX_WEIGHT		1E+36
#define START_WEIGHT		100.0
#define LOOP_WEIGHT		10.0
#define MIN_WEIGHT		0.0
#define UNSET_WEIGHT		-1.0

typedef
  struct bb_node
  {
    double weight;
    double span_weight;
    int lineno;
    int state;
    prof_formula_type formula;
    int n_in_arcs;
    int n_out_arcs;
    struct bb_arc * in_arcs;
    struct bb_arc * out_arcs;
    struct bb_arc * span_arc;
    struct bb_node * q_next;
  }
bb_node;

typedef
  struct bb_arc
  {
    double weight;
    int state;
    prof_formula_type formula;
    struct bb_arc * in_arc_next;
    struct bb_arc * out_arc_next;
    int fm_bb;
    int to_bb;
    int arc_id;
  }
bb_arc;

typedef struct priority_queue
{
  bb_node * head;
} priority_queue;

static bb_node *s_nodes, *save_func_nodes;
static int      n_nodes, save_func_nnodes;
static bb_arc  *s_arcs, *save_func_arcs;
static int      n_arcs, save_func_narcs;

static int func_offset;
static rtx base_reg;

static int
doub_to_int(d)
double d;
{
  if (d > 0x7fffffff)
    return 0x7fffffff;
  else
    return (int)d;
}

static rtx
prof_gen_instrument(arc, targ_insn, first_in_function, before)
bb_arc * arc;
rtx targ_insn;
int first_in_function;
int before;
{
  rtx insn, reg, sav, seq, stop;
  rtx ret_insn;
  int off;
  rtx (*emit_func)();
  rtx mem1;
  rtx mem2;
  rtx reg1;
  rtx addr;

  insn = targ_insn;
  ret_insn = 0;

  if (before)
    emit_func = emit_insn_before;
  else
    emit_func = emit_insn_after;

  if (first_in_function)
  {
    func_offset = prof_data_offset;
    base_reg = gen_reg_rtx(Pmode);

    START_SEQUENCE (sav);

    if (prof_data_offset == 0)
      addr = prof_decl;
    else
      addr = gen_rtx (CONST, Pmode,
                      gen_rtx (PLUS, Pmode, prof_decl,
                                            GEN_INT(prof_data_offset)));

    emit_insn (gen_rtx (SET, VOIDmode, base_reg, addr));

    if (TARGET_PID && (SYMREF_ETC(prof_decl) & SYMREF_PIDBIT) != 0)
      emit_insn(gen_rtx(SET, VOIDmode,
                        base_reg,
                        gen_rtx(PLUS, Pmode, pid_reg_rtx, base_reg)));
  
    seq = gen_sequence();

    END_SEQUENCE (sav);
  }
  else
  {
    START_SEQUENCE (sav);

    reg1 = gen_reg_rtx(SImode);
    addr = base_reg;

    if ((off = prof_data_offset-func_offset) != 0)
      addr = gen_rtx(PLUS, Pmode, base_reg, GEN_INT(off));

    mem1 = gen_rtx(MEM, SImode, addr);
    mem2 = gen_rtx(MEM, SImode, addr);

    emit_move_insn(reg1, mem1);
    emit_insn(gen_rtx(SET, VOIDmode, reg1,
                      gen_rtx(PLUS, SImode, reg1, const1_rtx)));
    emit_move_insn(mem2, reg1);
    seq = gen_sequence();

    END_SEQUENCE (sav);

    SET_COEF(arc->formula, (prof_data_offset - func_offset)/4, COEF_POS);
    prof_data_offset += 4;
    prof_n_counters += 1;
  }

  stop = before ? PREV_INSN(insn) : NEXT_INSN(insn);

  emit_func (seq, insn);

  /* Mark the entire sequence as being profiling insns, and delete the
     instrumentation if we aren't really instrumenting. */

  if (before)
    for (insn=PREV_INSN(insn); insn != stop; insn=PREV_INSN(insn))
    {
      INSN_PROFILE_INSTR_P(insn) = 1;

      if (!PROF_CODE)
      { PUT_CODE (insn, NOTE);
	NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
	NOTE_SOURCE_FILE (insn) = 0;
      }

      ret_insn = insn;
    }
  else
    for (insn=NEXT_INSN(insn); insn != stop; insn=NEXT_INSN(insn))
    {
      INSN_PROFILE_INSTR_P(insn) = 1;

      if (!PROF_CODE)
      { PUT_CODE (insn, NOTE);
	NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
	NOTE_SOURCE_FILE (insn) = 0;
      }

      ret_insn = insn;
    }

  return ret_insn;
}

static void
rewrite_labelref(x, olabel, nlabel)
rtx x;
rtx olabel;
rtx nlabel;
{
  char *fmt;
  int i;
  register RTX_CODE code = GET_CODE (x);

  if (code == LABEL_REF)
  {
    if (XEXP (x, 0) == olabel)
      XEXP(x,0) = nlabel;
    return;
  }

  fmt = GET_RTX_FORMAT (code);
  for (i = GET_RTX_LENGTH (code) - 1; i >= 0; i--)
  {
    if (fmt[i] == 'e')
      rewrite_labelref (XEXP (x, i), olabel, nlabel);
    else if (fmt[i] == 'E')
    {
      register int j;
      for (j = 0; j < XVECLEN (x, i); j++)
        rewrite_labelref (XVECEXP (x, i, j), olabel, nlabel);
    }
  }
}

/*
 * This tries to pick a spot to insert that most closely matches the intent
 * of inserting an instruction or sequence after while preserving line number
 * info as much as possible.  Of course preserving line number info is a very
 * subjective thing.  Specifically, if inserting after a CODE_LABEL,  we
 * also chose to emit after any NOTES that immediately follow the insn.
 */
static
rtx best_after_insn(insn)
rtx insn;
{
  rtx ret_insn = insn;;

  if (GET_CODE(insn) == CODE_LABEL)
  {
    for (insn = NEXT_INSN(insn); insn != 0; insn = NEXT_INSN(insn))
    {
      if (GET_CODE(insn) != NOTE)
        break;

      ret_insn = insn;
    }
  }

  return ret_insn;
}

static void
prof_instrument_arc(arc_p)
bb_arc * arc_p;
{
  /*
   * instrument the arc in the easiest way possible.
   * if the arc comes from a basic block with a single outgoing arc,
   * then we can just instrument the arc by instrumenting the block.
   * if the arc goes to a basic block with a single incoming arc
   * again we can just instrument the block.
   * If the arc is a fall-thru arc then it is easy to add code after
   * the from_bb but before the to_bb to count the arc.
   * And finally if it isn't one of these then we have gack to deal with.
   */
  rtx t_insn;
  int offset;

  if (s_nodes[arc_p->fm_bb].n_out_arcs == 1)
  {
    int inst_before;
    t_insn = basic_block_head[arc_p->fm_bb];
    
    /*
     * This next hack roughly keeps line numbers more properly associated
     * with code.
     */
    inst_before = 1;
    if (GET_CODE(t_insn) == CODE_LABEL)
    {
      inst_before = 0;
      t_insn = best_after_insn(t_insn);
    }

    prof_gen_instrument(arc_p, t_insn, 0, inst_before);
  }
  else if (s_nodes[arc_p->to_bb].n_in_arcs == 1)
  {
    int inst_before;
    t_insn = basic_block_head[arc_p->to_bb];

    /*
     * This next hack roughly keeps line numbers more properly associated
     * with code.
     */
    inst_before = 1;
    if (GET_CODE(t_insn) == CODE_LABEL)
    {
      inst_before = 0;
      t_insn = best_after_insn(t_insn);
    }

    prof_gen_instrument(arc_p, t_insn, 0, inst_before);
  }
  else if (arc_p->to_bb == arc_p->fm_bb+1)
  {
    t_insn = basic_block_head[arc_p->to_bb];
    prof_gen_instrument(arc_p, t_insn, 0, 1);
  }
  else
  {
    /*
     * This is a real pain.  We have to find a place where we can put 
     * the profiling code, then change the label reference to go there.
     *
     * The end of fm_bb is either a conditional jump, or possibly some
     * sort of table jump.  The beginning of the to_bb must be a
     * CODE_LABEL.
     *
     * What we will do is if the to_bb has a fall-thru arc, then we will
     * generate an unconditional jump after the end of the conditional jump
     * that has the fall-thru.
     *
     * Immediately following this we generate a barrier, a label, and then
     * the instrumentation code, and an unconditional jump to to_bb
     * followed by a barrier.  The reason I didn't let the instrumentation
     * fall-thru is that if I try to insert another of these types of probes
     * at this same spot the fall-thru would allow a code botch to happen,
     * so to be safe I use the jump and will let the jump-optimization
     * delete it.
     *
     * Finally I must rewrite the conditional jump or table jump in the
     * fm_bb to refer to the label for the profile instrumentation.
     */
    
    int to_bb_fall_thru_bb = -1;
    rtx old_to_label = basic_block_head[arc_p->to_bb];
    rtx new_label    = gen_label_rtx();
    bb_arc *t_arc_p;

    for (t_arc_p = s_nodes[arc_p->to_bb].in_arcs; t_arc_p != 0;
         t_arc_p = t_arc_p->in_arc_next)
    {
      if (t_arc_p->fm_bb+1 == t_arc_p->to_bb)
      {
        to_bb_fall_thru_bb = t_arc_p->fm_bb;
        break;
      }
    }

    /*
     * This converts fall-thru to explicit jump.
     */
    if (to_bb_fall_thru_bb != -1)
    {
      t_insn = basic_block_end[to_bb_fall_thru_bb];
      t_insn = emit_jump_insn_after(gen_jump(old_to_label),
                                    best_after_insn(t_insn));
      JUMP_LABEL(t_insn) = old_to_label;
      LABEL_NUSES(old_to_label) += 1;
      t_insn = emit_barrier_after(t_insn);
    }

    /*
     * This generates the profile instrumentation.
     */
    t_insn = PREV_INSN(basic_block_head[arc_p->to_bb]);
    t_insn = emit_label_after(new_label, best_after_insn(t_insn));
    t_insn = prof_gen_instrument(arc_p, t_insn, 0, 0);
    t_insn = emit_jump_insn_after(gen_jump(old_to_label), t_insn);
    JUMP_LABEL(t_insn) = old_to_label;
    LABEL_NUSES(old_to_label) += 1;
    t_insn = emit_barrier_after(t_insn);

    /*
     * Now rewrite the old rtl to reference the new label.
     */
    t_insn = basic_block_end[arc_p->fm_bb];
    if (GET_CODE(t_insn) == JUMP_INSN)
    {
      rewrite_labelref(PATTERN(t_insn), old_to_label, new_label);
      JUMP_LABEL(t_insn) = new_label;
      LABEL_NUSES(new_label) += 1;
      LABEL_NUSES(old_to_label) -= 1;
    }
  }
}

static void
prof_block_arcs()
{
  list_p *succs;
  int i;
  int l_arcs;
  int have_arcs;
  /*
   * get basic block information necessary for running this dataflow.
   * This includes breaking into basic blocks, and getting a way to
   * find the predecessor blocks of a block.  Also need to know the
   * entry basic block number.
   */

  /* figure out how many pred nodes are needed. */
  succs = (list_p *)xmalloc(N_BLOCKS * sizeof(list_p));

  compute_preds_succs((list_p *)0, succs, &succ_list_blks);

  l_arcs = 0;
  for (i = 0; i < N_BLOCKS; i++)
  {
    list_p succ_l;
    if (i == EXIT_BLOCK || i == ENTRY_BLOCK)
      continue;

    for (succ_l = succs[i]; succ_l != 0; succ_l = succ_l->next)
      l_arcs ++;

    /*
     * If the block has no successors, then this probably means I called
     * a routine, which will never return.  For our purposes, we must make
     * an arc from such a block back to the start block.
     */
    if (succs[i] == 0)
      l_arcs++;
  }

  s_nodes = (bb_node *)xmalloc(N_BLOCKS * sizeof(bb_node));
  s_arcs  = (bb_arc *)xmalloc(l_arcs * sizeof(bb_arc));

  l_arcs = 0;
  for (i = 0; i < N_BLOCKS; i++)
  {
    s_nodes[i].lineno = -1;
    s_nodes[i].state = UNKNOWN_STATE;
    s_nodes[i].formula = 0;
    s_nodes[i].weight = UNSET_WEIGHT;
    s_nodes[i].span_weight = UNSET_WEIGHT;
    s_nodes[i].span_arc = 0;
    s_nodes[i].n_in_arcs = 0;
    s_nodes[i].n_out_arcs = 0;
    s_nodes[i].in_arcs = 0;
    s_nodes[i].out_arcs = 0;
  }

  for (i = 0; i < N_BLOCKS; i++)
  {
    list_p succ_l;

    if (i == EXIT_BLOCK || i == ENTRY_BLOCK)
      continue;

    for (succ_l = succs[i]; succ_l != 0; succ_l = succ_l->next)
    {
      int to_bb = GET_LIST_ELT(succ_l, int);

      if (to_bb == EXIT_BLOCK)
        to_bb = 0;

      s_arcs[l_arcs].weight = UNSET_WEIGHT;
      s_arcs[l_arcs].state = UNKNOWN_STATE;
      s_arcs[l_arcs].formula = 0;
      s_arcs[l_arcs].fm_bb = i;
      s_arcs[l_arcs].to_bb = to_bb;
      s_arcs[l_arcs].arc_id = 0;

      /* put arc in in and out lists */
      s_arcs[l_arcs].out_arc_next = s_nodes[i].out_arcs;
      s_nodes[i].out_arcs = &s_arcs[l_arcs];
      s_nodes[i].n_out_arcs ++;

      s_arcs[l_arcs].in_arc_next = s_nodes[to_bb].in_arcs;
      s_nodes[to_bb].in_arcs = &s_arcs[l_arcs];
      s_nodes[to_bb].n_in_arcs ++;

      if (to_bb != i+1)
      {
        rtx x = BLOCK_END(i);
        if (GET_CODE(x) == JUMP_INSN && GET_CODE(PATTERN(x)) == SET &&
            GET_CODE(SET_SRC(PATTERN(x))) == IF_THEN_ELSE)
          s_arcs[l_arcs].arc_id = GET_CODE(XEXP(SET_SRC(PATTERN(x)), 0));
      }

      l_arcs++;
    }

    /*
     * If the block has no successors, then this probably means I called
     * a routine, which will never return.  For our purposes, we must make
     * an arc from such a block back to the start block.
     */
    if (succs[i] == 0)
    {
      s_arcs[l_arcs].weight = UNSET_WEIGHT;
      s_arcs[l_arcs].state = UNKNOWN_STATE;
      s_arcs[l_arcs].formula = 0;
      s_arcs[l_arcs].fm_bb = i;
      s_arcs[l_arcs].to_bb = 0;

      /*
       * Mark all arcs back to start block with a code that we wouldn't
       * expect to have used anywhere else.  This will help our profile
       * stretching.  BARRIER seems a decent choice.
       */
      s_arcs[l_arcs].arc_id = BARRIER;

      /* put arc in in and out lists */
      s_arcs[l_arcs].out_arc_next = s_nodes[i].out_arcs;
      s_nodes[i].out_arcs = &s_arcs[l_arcs];
      s_nodes[i].n_out_arcs ++;

      s_arcs[l_arcs].in_arc_next = s_nodes[0].in_arcs;
      s_nodes[0].in_arcs = &s_arcs[l_arcs];
      s_nodes[0].n_in_arcs ++;

      l_arcs++;
    }
  }

  n_nodes = N_BLOCKS;
  n_arcs = l_arcs;

  free_lists(&succ_list_blks);
  free (succs);
}

static void
mark_insn_with_linecol(insn, lineno, colno)
rtx insn;
int lineno;
int colno;
{
  /*
   * record the line number for this insn in the INSN_PROF_DATA_INDEX
   * field of the insn.  Mark any instructions generated for profile
   * instrumentation as having no line numbers by using -1.
   */
  enum rtx_code c = GET_CODE(insn);

  if (c == INSN || c == JUMP_INSN || c == CALL_INSN || c == CODE_LABEL)
  {
    if (INSN_UID(insn) > original_max_uid || BLOCK_NUM(insn) == -1 ||
        lineno == -1)
      INSN_PROF_DATA_INDEX(insn) = -1;
    else
      INSN_PROF_DATA_INDEX(insn) = (lineno << 8) | (colno & 255);
  }
}

static void
mark_insn_lines(insns)
rtx insns;
{
  rtx t_insn;
  int last_lineno = -1;
  int last_colno = 0;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    enum rtx_code c = GET_CODE(t_insn);

    if (GET_CODE(t_insn) == NOTE && NOTE_LINE_NUMBER(t_insn) > 0 &&
        strcmp(NOTE_SOURCE_FILE(t_insn), main_input_filename) == 0)
    {
      last_lineno = NOTE_LINE_NUMBER(t_insn);
      if (NOTE_LISTING_START(t_insn) == 0)
      {
        last_lineno = -1;
        last_colno = 255;
      }
      else
        last_colno = GET_COL(NOTE_LISTING_START(t_insn));

      if (PREV_INSN(t_insn) != 0 && GET_CODE(PREV_INSN(t_insn)) == CODE_LABEL)
        mark_insn_with_linecol(PREV_INSN(t_insn), last_lineno, last_colno);
    }

    mark_insn_with_linecol(t_insn, last_lineno, last_colno);
  }
}

int prof_block_base;

static void
mark_insn_profs(insns, block_base)
rtx insns;
int block_base;
{
  rtx t_insn;

  prof_block_base = block_base;

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    enum rtx_code c = GET_CODE(t_insn);

    if (c == CALL_INSN || c == INSN || c == JUMP_INSN || c == CODE_LABEL)
    {
      if (INSN_UID(t_insn) < original_max_uid && BLOCK_NUM(t_insn) != -1)
        INSN_PROF_DATA_INDEX(t_insn) = BLOCK_NUM(t_insn);
      else
        INSN_PROF_DATA_INDEX(t_insn) = -1;
    }
  }
}

/*
 * Prim's algorithm for finding a minimal spanning tree modified to find
 * a maximal spanning tree.
 */
static void
init_queue(queue)
priority_queue * queue;
{
  queue->head = 0;
}

static void
add_to_queue(queue, bb_p)
priority_queue * queue;
bb_node * bb_p;
{
  if (bb_p->state == IN_QUEUE_STATE)
    return;

  /* now add it to the list */
  bb_p->q_next = queue->head;
  queue->head = bb_p;
  bb_p->state = IN_QUEUE_STATE;
}

static bb_node *
extract_max_from_queue(queue)
priority_queue * queue;
{
  bb_node * prev_p = 0;
  bb_node * cur_p = queue->head;
  bb_node * max_prev_p = 0;
  bb_node * max_cur_p = cur_p;
  int max = -1;

  while (cur_p != 0)
  {
    if (cur_p->span_weight > max)
    {
      max = cur_p->span_weight;
      max_prev_p = prev_p;
      max_cur_p = cur_p;
    }
    
    prev_p = cur_p;
    cur_p = cur_p->q_next;
  }

  if (max_cur_p == 0)
    return 0;

  /*
   * take cur_p out of list
   */
  if (max_prev_p == 0)
    queue->head = max_cur_p->q_next;
  else
    max_prev_p->q_next = max_cur_p->q_next;

  return max_cur_p;
}

static void
mark_maximal_span(bb_nodes, n_nodes, bb_arcs, n_arcs, start_node)
bb_node *bb_nodes;
int n_nodes;
bb_arc *bb_arcs;
int n_arcs;
bb_node *start_node;
{
  int i;
  priority_queue q;
  bb_node * u;
  bb_arc * arc_p;

  init_queue(&q);

  /* start at start node */
  for (u = start_node; u != 0; u = extract_max_from_queue(&q))
  {
    /* mark u as being part of the maximal spanning tree */
    u->state = IN_MAX_SPAN_STATE;
    if (u->span_arc != 0)
      u->span_arc->state = IN_MAX_SPAN_STATE;

    for (arc_p = u->out_arcs; arc_p != 0; arc_p = arc_p->out_arc_next)
    {
      bb_node * to_bb_p = &bb_nodes[arc_p->to_bb];
      if (to_bb_p->state != IN_MAX_SPAN_STATE &&
          arc_p->weight > to_bb_p->span_weight)
      {
        to_bb_p->span_weight = arc_p->weight;
        to_bb_p->span_arc = arc_p;
        add_to_queue(&q, to_bb_p);
      }
    }

    for (arc_p = u->in_arcs; arc_p != 0; arc_p = arc_p->in_arc_next)
    {
      bb_node * fm_bb_p = &bb_nodes[arc_p->fm_bb];
      if (fm_bb_p->state != IN_MAX_SPAN_STATE &&
          arc_p->weight > fm_bb_p->span_weight)
      {
        fm_bb_p->span_weight = arc_p->weight;
        fm_bb_p->span_arc = arc_p;
        add_to_queue(&q, fm_bb_p);
      }
    }
  }
}

static void weight_node();

static void
weight_arc(arc_p, weight)
bb_arc *arc_p;
double weight;
{
  if (arc_p->weight == UNSET_WEIGHT)
  {
    arc_p->weight = weight;
    weight_node(&s_nodes[arc_p->to_bb]);
  }
}

static void
weight_node(node_p)
bb_node *node_p;
{
  int is_loop_head = 0;
  int can_weight = 1;
  bb_arc * arc_p;
  double weight = node_p->weight;

  if (weight == UNSET_WEIGHT)
  {
    int bb_num = node_p-s_nodes;

    weight = 0.0;
    if (df_data.maps.loop_info[bb_num].inner_loop == bb_num)
    {
      flex_set members = df_data.maps.loop_info[bb_num].members;

      /*
       * This basic block is the head of a loop.  We can weight this
       * if we have all the incoming arcs other than those from other
       * blocks within the loop.
       */
      for (arc_p = node_p->in_arcs; arc_p != 0; arc_p = arc_p->in_arc_next)
      {
        if (arc_p->weight == UNSET_WEIGHT)
        {
          if (!in_flex(members, arc_p->fm_bb))
          {
            can_weight = 0;
            break;
          }
        }
        else
        {
          weight = d_to_d(weight + arc_p->weight);
          if (weight > MAX_WEIGHT)
            weight = MAX_WEIGHT;
        }
      }

      /*
       * if we can weight this loop then now is the time to weight
       * the exit_arcs.
       */
      if (can_weight)
      {
        int n_exits = 0;
        double exit_weight;

        /* find the loop exit arcs and weight them. */
        for (bb_num = next_flex(members, -1); bb_num != -1;
             bb_num = next_flex(members, bb_num))
        {
          for (arc_p = s_nodes[bb_num].out_arcs; arc_p != 0;
               arc_p = arc_p->out_arc_next)
          {
            if (!in_flex(members, arc_p->to_bb))
              n_exits ++;
          }
        }
        
        if (n_exits != 0)
        {
          exit_weight = d_to_d(weight / n_exits);
          for (bb_num = next_flex(members, -1); bb_num != -1;
               bb_num = next_flex(members, bb_num))
          {
            for (arc_p = s_nodes[bb_num].out_arcs; arc_p != 0;
                 arc_p = arc_p->out_arc_next)
            {
              if (!in_flex(members, arc_p->to_bb))
                weight_arc(arc_p, exit_weight);
            }
          }
        }

        if (weight > (MAX_WEIGHT / LOOP_WEIGHT))
          weight = MAX_WEIGHT;
        else
          weight = d_to_d(weight * LOOP_WEIGHT);
      }
    }
    else
    {
      for (arc_p = node_p->in_arcs; arc_p != 0; arc_p = arc_p->in_arc_next)
      {
        if (arc_p->weight == UNSET_WEIGHT)
        {
          can_weight = 0;
          break;
        }
        weight = d_to_d(weight + arc_p->weight);
        if (weight > MAX_WEIGHT)
          weight = MAX_WEIGHT;
      }
    }
  }

  if (can_weight)
  {
    int n_unknown_arcs = node_p->n_out_arcs;
    node_p->weight = weight;
    for (arc_p = node_p->out_arcs; arc_p != 0; arc_p = arc_p->out_arc_next)
    {
      if (arc_p->weight != UNSET_WEIGHT)
      {
        n_unknown_arcs -= 1;
        weight = d_to_d(weight - arc_p->weight);
      }
    }

    if (n_unknown_arcs > 0)
    {
      weight = d_to_d(weight / n_unknown_arcs);
      for (arc_p = node_p->out_arcs; arc_p != 0; arc_p = arc_p->out_arc_next)
        weight_arc(arc_p, weight);
    }
  }
}

static void propagate_arc_formula();

static void propagate_block_formula(node_p, n_counters)
bb_node *node_p;
int n_counters;
{
  int n_unknown;
  bb_arc *t_arc;
  bb_arc *comp_arc = 0;

  n_unknown = 0;
  for (t_arc = node_p->out_arcs; t_arc != 0; t_arc = t_arc->out_arc_next)
    if (t_arc->state != HAS_FORMULA_STATE)
    {
      comp_arc = t_arc;
      n_unknown += 1;
    }

  /*
   * if only one outgoing arc is unknown, then it can be computed as
   * the basic block formula minus the sum of all the other outgoing
   * arc formulas.
   */
  if (n_unknown == 1)
  {
    copy_formula(comp_arc->formula, node_p->formula, n_counters);
    for (t_arc = node_p->out_arcs; t_arc != 0; t_arc = t_arc->out_arc_next)
      if (t_arc != comp_arc)
        sub_formula(comp_arc->formula, t_arc->formula, n_counters);
    comp_arc->state = HAS_FORMULA_STATE;
    propagate_arc_formula(comp_arc, n_counters);
  }

  n_unknown = 0;
  for (t_arc = node_p->in_arcs; t_arc != 0; t_arc = t_arc->in_arc_next)
    if (t_arc->state != HAS_FORMULA_STATE)
    {
      comp_arc = t_arc;
      n_unknown += 1;
    }

  /*
   * if only one incoming arc is unknown, then it can be computed as
   * the basic block formula minus the sum of all the other incoming
   * arc formulas.
   */
  if (n_unknown == 1)
  {
    copy_formula(comp_arc->formula, node_p->formula, n_counters);
    for (t_arc = node_p->in_arcs; t_arc != 0; t_arc = t_arc->in_arc_next)
      if (t_arc != comp_arc)
        sub_formula(comp_arc->formula, t_arc->formula, n_counters);
    comp_arc->state = HAS_FORMULA_STATE;
    propagate_arc_formula(comp_arc, n_counters);
  }
}

static void
propagate_arc_formula(arc_p, n_counters)
bb_arc * arc_p;
int n_counters;
{
  bb_node *node_p;
  bb_arc *t_arc;
  int n_unknown;

  if (arc_p->state != HAS_FORMULA_STATE)
    abort();

  /* see if we can compute the "to" basic block */
  node_p = &s_nodes[arc_p->to_bb];
  if (node_p->state != HAS_FORMULA_STATE)
  {
    /* see if all incoming arcs have formulas */
    n_unknown = 0;
    for (t_arc = node_p->in_arcs; t_arc != 0; t_arc = t_arc->in_arc_next)
    {
      if (t_arc->state != HAS_FORMULA_STATE)
      {
        n_unknown += 1;
        break;
      }
    }

    if (n_unknown == 0)
    {
      for (t_arc = node_p->in_arcs; t_arc != 0; t_arc = t_arc->in_arc_next)
        add_formula(node_p->formula, t_arc->formula, n_counters);
      node_p->state = HAS_FORMULA_STATE;
    }
  }

  /*
   * if the node has its formula set, see if we need to compute
   * one of its out going arcs.
   */
  if (node_p->state == HAS_FORMULA_STATE)
    propagate_block_formula(node_p, n_counters);

  /* see if we can compute the "from" basic block */
  node_p = &s_nodes[arc_p->fm_bb];
  if (node_p->state != HAS_FORMULA_STATE)
  {
    /* see if all outgoing arcs have formulas */
    n_unknown = 0;
    for (t_arc = node_p->out_arcs; t_arc != 0; t_arc = t_arc->out_arc_next)
    {
      if (t_arc->state != HAS_FORMULA_STATE)
      {
        n_unknown += 1;
        break;
      }
    }

    if (n_unknown == 0)
    {
      for (t_arc = node_p->out_arcs; t_arc != 0; t_arc = t_arc->out_arc_next)
        add_formula(node_p->formula, t_arc->formula, n_counters);
      node_p->state = HAS_FORMULA_STATE;
    }
  }

  /*
   * if the node has its formula set, see if we need to compute
   * one of its incoming arcs.
   */
  if (node_p->state == HAS_FORMULA_STATE)
    propagate_block_formula(node_p, n_counters);
}

static void
allocate_formulas(nodes, n_nodes, arcs, n_arcs, n_counters, formula_p)
bb_node * nodes;
int n_nodes;
bb_arc * arcs;
int n_arcs;
int n_counters;
prof_formula_type *formula_p;
{
  int space_needed;
  prof_formula_type formula;
  int i;

  space_needed = n_counters * (n_nodes + n_arcs);
  *formula_p = formula = (prof_formula_type)xmalloc(space_needed);
  bzero(formula, space_needed);

  for (i = 0; i < n_nodes; i++)
  {
    nodes[i].formula = formula;
    formula += n_counters;
  }

  for (i = 0; i < n_arcs; i++)
  {
    arcs[i].formula = formula;
    formula += n_counters;
  }
}

static void
compute_prob_expect(insns)
rtx insns;
{
  rtx t_insn;
  rtx t1_insn;
  double dummy;
  /*
   * simply run through the insns, assigning into INSN_EXPECT the
   * block count for the basic block this instruction is in.  At the
   * same time compute the jump probabilities, and the loop iteration
   * counts.
   */

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    switch (GET_CODE(t_insn))
    {
      case JUMP_INSN:
        INSN_EXPECT(t_insn) =
          doub_to_int(s_nodes[BLOCK_NUM(t_insn)].weight * PROB_BASE);
        if (simplejump_p(t_insn))
        {
          JUMP_THEN_PROB(t_insn) = PROB_BASE;
          break;
        }

        if (condjump_p(t_insn))
        {
          double num_tried = s_nodes[BLOCK_NUM(t_insn)].weight;
          double num_taken;
          num_taken = d_to_d(num_tried - s_nodes[BLOCK_NUM(t_insn)+1].weight);

          /*
           * If this jump has never even been attempted, we should set
           * its probability to a very middling value.  This prevents
           * later optimizations from making strong inferences from
           * the THEN probability.  This will be most important for
           * superblock formation, although probably not very important
           * for anyone.  This code is mostly just added too make sure
           * no stupidity happens later.
           */
          if (num_tried == 0)
            JUMP_THEN_PROB(t_insn) = PROB_BASE / 2;
          else if (num_taken > num_tried)
            JUMP_THEN_PROB(t_insn) = PROB_BASE;
          else
          {
            dummy = d_to_d(num_taken * PROB_BASE);
            dummy = d_to_d(dummy / num_tried);
            JUMP_THEN_PROB(t_insn) = dummy;
          }
        }
        break;

      case CODE_LABEL:
      case INSN:
      case CALL_INSN:
        INSN_EXPECT(t_insn) =
          doub_to_int(d_to_d(s_nodes[BLOCK_NUM(t_insn)].weight * PROB_BASE));
        break;

      case NOTE:
        if (NOTE_LINE_NUMBER(t_insn) == NOTE_INSN_LOOP_BEG)
        {
          int head_bb = BLOCK_NUM(t_insn);
          int loop_top_bb = head_bb+1;
          double tmp;

          if (s_nodes[head_bb].weight != 0)
            tmp = d_to_d(s_nodes[loop_top_bb].weight / s_nodes[head_bb].weight);
          else
            tmp = 1000000;

          if (tmp > 1000000)
            tmp = 1000000;

          NOTE_TRIP_COUNT(t_insn) = doub_to_int(tmp * PROB_BASE);
          
          break;
        }

#if !defined(DEV_465)
        if (NOTE_LINE_NUMBER(t_insn) == NOTE_INSN_FUNCTION_ENTRY)
#else
        if (NOTE_LINE_NUMBER(t_insn) == NOTE_INSN_FUNCTION_BEG)
#endif
        {
          if (s_nodes[0].weight > 1000000)
            NOTE_CALL_COUNT(t_insn) = 1000000;
          else
            NOTE_CALL_COUNT(t_insn) = s_nodes[0].weight;
        }
        break;

      default:
        break;
    }
  }
}

void
imstg_prof_annotate(insns)
rtx insns;
{
  char* func_info;
  rtx t_insn;
  int have_prof;

  /*
   * This must happen because find_basic_blocks dose not work properly when
   * optimize is 0, and get_predecessors uses find_basic_blocks.
   */
  if (!optimize)
    return;

  func_info = glob_inln_info (XSTR(XEXP(DECL_RTL(current_function_decl),0),0));
  have_prof = func_info && glob_have_prof_counts (func_info);

  prof_func_used_prof_info = have_prof;

  prof_func_block = prof_n_bblocks;
  /* We only need loop info if we have to estimate the arcs. */
  get_predecessors(0 /* no loop info needed at this time */);

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    enum rtx_code c = GET_CODE(t_insn);
    if (c == CALL_INSN || c == INSN || c == JUMP_INSN || c == CODE_LABEL)
    {
      if (have_prof)
      {
        unsigned t = glob_prof_counter (func_info, BLOCK_NUM(t_insn));

        if (t < (0x7fffffff / PROB_BASE))
          INSN_EXPECT(t_insn) = t * PROB_BASE;
        else
          INSN_EXPECT(t_insn) = 0x7fffffff;
      }
      else
        INSN_EXPECT(t_insn) = -1;
    }
  }

  prof_n_bblocks += (N_BLOCKS - 2);
  free_all_flow_space();
}

/*
 * annotate insns that are being inlined into
 * previously annotated insns.  insns points to a list of insns that are
 * to be inlined.
 */
void
imstg_prof_annotate_inline(insns, fname, prof_index, call_expect,func_info)
rtx insns;
char *fname;
int prof_index;
int call_expect;
char* func_info;
{
  rtx note_insn;
  rtx t_insn;
  double adjustment;
  unsigned t;

  if (!glob_have_prof_counts (func_info))
    return;

  t = glob_prof_counter (func_info, 0);

  adjustment = t * PROB_BASE;
  if (adjustment != 0)
    adjustment = d_to_d((double)call_expect / adjustment);

  for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
  {
    enum rtx_code c = GET_CODE(t_insn);
    if (c == CALL_INSN || c == INSN || c == JUMP_INSN || c == CODE_LABEL)
    {
      t = -1;

      if (INSN_PROF_DATA_INDEX(t_insn) != -1)
      {
        t = glob_prof_counter (func_info, INSN_PROF_DATA_INDEX(t_insn));
        t = d_to_d(t * adjustment);
      }

      if (t == -1)
        INSN_EXPECT(t_insn) = -1;
      else if (t < (0x7fffffff / PROB_BASE))
        INSN_EXPECT(t_insn) = t * PROB_BASE;
      else
        INSN_EXPECT(t_insn) = 0x7fffffff;
    }
  }
}

void
imstg_compute_prob_expect(insns)
rtx insns;
{
  int have_expects;
  rtx t_insn = 0;

  /*
   * This must happen because find_basic_blocks dose not work properly when
   * optimize is 0, and get_predecessors uses find_basic_blocks.
   */
  if (!optimize)
    return;

  have_expects = prof_func_used_prof_info != 0;

  get_predecessors(!have_expects /* don't need loop info if we have expects */);
  prof_block_arcs();

  if (!have_expects)
  {
    s_nodes[0].weight = START_WEIGHT;
    weight_node(&s_nodes[0]);
  }
  else
  {
    for (t_insn = insns; t_insn != 0; t_insn = NEXT_INSN(t_insn))
    {
      enum rtx_code c = GET_CODE(t_insn);
      if ((c == CALL_INSN || c == INSN || c == JUMP_INSN || c == CODE_LABEL) &&
          INSN_EXPECT(t_insn) != -1)
        s_nodes[BLOCK_NUM(t_insn)].weight = INSN_EXPECT(t_insn) / PROB_BASE;
    }
  }

  compute_prob_expect(insns);
  free (s_arcs);
  free (s_nodes);
  free_all_flow_space();
}

int 
prof_num_arcs ()
{
  /* Interface routine for i_glob_db.c */
  assert (save_func_narcs && save_func_arcs);
  return save_func_narcs;
}

int
prof_num_nodes ()
{
  /* Interface routine for i_glob_db.c */
  assert (save_func_nnodes > 2 && save_func_nodes != 0);
  /* Account for the entry and exit blocks that don't get put out */
  return save_func_nnodes-2;
}

void
prof_arc_info (n, fm, to, arc_id)
int n;
int *fm;
int *to;
int *arc_id;
{
  /* Interface routine for i_glob_db.c */
  assert (save_func_arcs && n >= 0 && n < save_func_narcs);
  *fm = save_func_arcs[n].fm_bb;
  *to = save_func_arcs[n].to_bb;
  *arc_id = save_func_arcs[n].arc_id;
}

unsigned
prof_node (n)
int n;
{
  /* Interface routine for i_glob_db.c */
  unsigned ret;
  assert (save_func_nodes && n >= 0 && n < save_func_nnodes);
  ret = doub_to_int (save_func_nodes[n].weight);
  return ret;
}

void
prof_free_arcs ()
{
  /* Interface routine for i_glob_db.c */
  assert (save_func_arcs && save_func_narcs);
  free (save_func_arcs);
  save_func_narcs = 0;
  save_func_arcs = 0;
}

void
prof_free_nodes ()
{
  /* Interface routine for i_glob_db.c */
  assert (save_func_nodes && save_func_nnodes);
  free (save_func_nodes);
  save_func_nodes = 0;
  save_func_nnodes = 0;
}

void
imstg_prof_min(insns)
rtx insns;
{
  int i;
  int n_counters;
  prof_formula_type formulas;

  prof_func_block = prof_n_bblocks;

  /*
   * This must happen because find_basic_blocks dose not work properly when
   * optimize is 0, and get_predecessors uses find_basic_blocks.
   */
  if (!optimize)
    fatal("optimize must be at least 1 to use profile instrumentation.");

  get_predecessors(1 /* We need loop info */);

  original_max_uid = get_max_uid();
  prof_block_arcs();

  s_nodes[0].weight = START_WEIGHT;
  weight_node(&s_nodes[0]);
  compute_prob_expect(insns);

  mark_maximal_span(s_nodes, n_nodes, s_arcs, n_arcs, &s_nodes[0]);

  /* first count how many profile counters we will need */
  n_counters = 0;
  for (i = 0; i < n_arcs; i++)
    if (s_arcs[i].state != IN_MAX_SPAN_STATE)
      n_counters += 1;

  /* now allocate the space to compute the formulas */
  allocate_formulas(s_nodes, n_nodes, s_arcs, n_arcs, n_counters, &formulas);

  /*
   * set up the register we use to keep this the instrumentation
   * inexpensive.  This doesn't actually use a counter, it just sets
   * up a register to the base of the counter area for this function.
   */
  prof_gen_instrument((bb_arc *)0, insns, 1, 0);

  for (i = 0; i < n_arcs; i++)
    if (s_arcs[i].state != IN_MAX_SPAN_STATE)
    {
      prof_instrument_arc(&s_arcs[i]);
      s_arcs[i].state = HAS_FORMULA_STATE;
    }

  /*
   * now we need to compute formulas for all the basic blocks, and all the
   * unprofiled arcs.
   */
  for (i = 0; i < n_arcs; i++)
    if (s_arcs[i].state == HAS_FORMULA_STATE)
      propagate_arc_formula(&s_arcs[i], n_counters);

  /*
   * Before dumping the information we must run through the insns and
   * set the INSN_PROF_DATA_INDEX field with the line numbers associated with
   * each insn.  This is how save_next_blk_prof_info knows the line numbers
   * associated with the block.
   *
   * Its important that this happens after compute_prob_expect because
   * INSN_EXPECT and INSN_PROF_DATA_INDEX share the same space in the insn.
   * If this one doesn't happen last we get the wrong stuff in the
   * INSN_PROF_DATA_INDEX fields ater.
   */
  mark_insn_lines(insns);

  for (i = 0; i < n_nodes; i++)
  {
    if (s_nodes[i].state == HAS_FORMULA_STATE)
    {
      save_next_blk_prof_info(BLOCK_HEAD(i), BLOCK_END(i),
                              s_nodes[i].formula, n_counters, func_offset/4);
      save_var_usage_info(BLOCK_HEAD(i), BLOCK_END(i));
    }
    else
    {
      if (i != EXIT_BLOCK && i != ENTRY_BLOCK)
        abort();
    }
  }

  save_func_narcs = n_arcs;
  save_func_arcs = s_arcs;
  save_func_nnodes = n_nodes;
  save_func_nodes = s_nodes;
  n_arcs = 0;
  s_arcs = 0;
  s_nodes = 0;

  /* now we must set the INSN_PROF_DATA_INDEX fields in the insns to be
   * the block number of the profile information associated with the
   * insns. This must be done last since other operations in this routine
   * use INSN_PROF_DATA_INDEX for other things, and later routines after
   * this expect INSN_PROF_DATA_INDEX to have the block number for the
   * profile info.  In particular, at this time, start_func_info and
   * do_call_stats in the global data base source depend on this.
   */
  mark_insn_profs(insns, prof_func_block);

  prof_n_bblocks += (N_BLOCKS - 2);
  free (formulas);
  free_all_flow_space();
}

void
prof_instrument_begin(fname)
    char *fname;
{
  char *prof_id;

  prof_id = (char *) xmalloc(strlen(fname) + 28);
  /**
  *** Declare symbol '__profile_data_start.<filename>'
  **/
  sprintf(prof_id, "__profile_data_start.%s", fname);
  prof_decl = gen_symref_rtx(Pmode, prof_id);
  SYM_ADDR_TAKEN_P(prof_decl) = 0;
  if (TARGET_PID)
    SYMREF_ETC(prof_decl) |= SYMREF_PIDBIT;

  prof_data_offset = 0;
  prof_n_bblocks = 0;
  prof_n_counters = 0;
  prof_func_block = 0;
}

void
prof_instrument_end(asm_out_file)
    FILE* asm_out_file;
{
  /*
   * If we haven't emitted any profiling counters, then don't define the
   * symbols, and do this stuff.
   */
  if (PROF_CODE && prof_data_offset)
  {
    text_section();
    fprintf(asm_out_file, "\n\t.word\t___profile_init\n\n\t.comm\t_");
    fprintf(asm_out_file, "%s,%d\n", XSTR(prof_decl,0), prof_data_offset);
  }

  /* We always put out the profile record these days */
  dump_prof_info(base_file_name, main_input_filename);
}
#endif
