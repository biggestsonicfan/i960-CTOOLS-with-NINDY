#include "config.h"

#ifdef IMSTG
#include "rtl.h"
#include "regs.h"
#include "flags.h"
#include "expr.h"

/* This file implemets the imstg's idea of position independent code.
 * When pic is requested (-mpic), the rtl describing relocation bias computation
 * is inserted in every function having absolute addresses of the "text"
 * obects, prior to any occurence of said addresses in the control flow (in
 * the beginning of a function for now).
 * Also, every such address is rewritten to add the code relocation bias.
 */


static rtx bias;

static void
insert_bias_computation(insns)
rtx insns;
{
	static char* template = "lda 0(ip),%0; lda .,%1; subo %1,%0,%0";
	rtvec inputs = rtvec_alloc(0);
	rtvec incons = rtvec_alloc(0);

	if (TARGET_CAVE)
	{
		emit_insn_before(gen_rtx(SET, SImode, bias,
			gen_rtx(MEM, Pmode,
				gen_rtx(PLUS, Pmode, 
					gen_rtx(REG, SImode, 16),
					GEN_INT(12)))),
			next_real_insn(insns));
	}
	else
	{
		emit_insn_before(gen_rtx(PARALLEL, VOIDmode, gen_rtvec(2, 
			gen_rtx(SET, VOIDmode, bias, gen_rtx(ASM_OPERANDS, 
			VOIDmode, template, "=d", 0, inputs, incons, "", 0)),
			gen_rtx(SET, VOIDmode, gen_reg_rtx(SImode),
			gen_rtx(ASM_OPERANDS, VOIDmode, template, "=d", 1, 
				inputs, incons, "", 0)))), 
		next_real_insn(insns));
	}
	if (obey_regdecls)
	{
		emit_insn_before(gen_rtx(USE, VOIDmode, bias), 
			next_real_insn(insns));
		emit_insn_after(gen_rtx(USE, VOIDmode, bias), 
			get_last_insn());
	}
}

static rtx
first_use_insn(i)
rtx i;
{
	rtx u;
	for (u = PREV_INSN(i); u; u = PREV_INSN(u))
	{
		if (GET_RTX_CLASS(GET_CODE(u)) != 'i' || 
			GET_CODE(PATTERN(u)) != USE)
		{
			return NEXT_INSN(u);
		}
		i = u;
	}
	return i;
}

static void
rewrite(i, loc)
rtx i;
rtx *loc;
{
	rtx set;
	rtx before;
	rtx x = *loc;
	*loc = gen_reg_rtx(SImode);
	if (!bias)
	{
		bias = gen_reg_rtx(SImode);
		REG_USERVAR_P(bias) = 1;
	}
	set = gen_rtx(SET, VOIDmode, *loc, gen_rtx(PLUS, SImode, bias, x));
	before = first_use_insn(i);
	if (GET_CODE(x) == MEM)
	{
        	rtx *mem_loc = &XEXP(SET_SRC(set), 1);
		*mem_loc = gen_reg_rtx(SImode);
		emit_insn_before(gen_rtx(SET, VOIDmode, *mem_loc, x), before);
	}
	emit_insn_before(set, before);
	INSN_CODE(i) = -1;		/* insn was rewritten */
}

static rtx
gen_in_place(x)
rtx x;
{
	if (!bias)
	{
		bias = gen_reg_rtx(SImode);
		REG_USERVAR_P(bias) = 1;
	}
	return gen_rtx(PLUS, SImode, bias, x);
}

static int
needs_rewrite(x)
rtx x;
{
	int code;
	
	if (!x)
		return FALSE;
	switch(code = GET_CODE(x))
	{
	case LABEL_REF:
		return TRUE;
	case SYMBOL_REF:
		if (SYMREF_ETC(x) & SYMREF_PICBIT)
		{
			return TRUE;
		}
		return FALSE;
	default:
	{
		char* fmt = GET_RTX_FORMAT (code);
		int i     = GET_RTX_LENGTH (code);

		while (--i >= 0)
		{
			if (fmt[i] == 'e')
			{
				if (needs_rewrite(XEXP(x, i)))
					return TRUE;
			}
			else if (fmt[i] == 'E')
			{
				int j = XVECLEN(x, i);
				while (--j >= 0)
				{
					if (needs_rewrite(XVECEXP(x, i, j)))
						return TRUE;
				}
			}
		}
	}
	}
	return FALSE;
}

static void
rewrite_in_place(loc)
rtx *loc;
{
	int code;
	
	rtx x = *loc;
	if (!x)
		return;
	switch(code = GET_CODE(x))
	{
	case MEM:
		if (needs_rewrite(x))
		{
			*loc = x = copy_rtx(x);
		}
		rewrite_in_place(&XEXP(x, 0));
		break;
	case LABEL_REF:
		*loc = gen_in_place(x);
		break;
	case SYMBOL_REF:
		if (SYMREF_ETC(x) & SYMREF_PICBIT)
		{
			*loc = gen_in_place(x);
		}
		break;
	case CONST:
		if (i960_const_pic_pid_p(x, 1, 0))
		{
			*loc = gen_in_place(x);
		}
		break;
	default:
	{
		char* fmt = GET_RTX_FORMAT (code);
		int i     = GET_RTX_LENGTH (code);

		while (--i >= 0)
		{
			if (fmt[i] == 'e')
			{
				rewrite_in_place(&XEXP(x, i));
			}
			else if (fmt[i] == 'E')
			{
				int j = XVECLEN(x, i);
				while (--j >= 0)
				{
					rewrite_in_place(&XVECEXP(x, i, j));
				}
			}
		}
	}
	}
}

/* Returns TRUE if address validation is necessary and FALSE otherwise */

static int
try_rewrite(i, loc)
rtx i;
rtx *loc;
{
	int code;
	
	rtx x = *loc;
	switch(code = GET_CODE(x))
	{
	case USE:
	case CLOBBER:
		if (GET_CODE(XEXP(x, 0)) == MEM)
		{
			if (needs_rewrite(XEXP(x, 0)))
			{
				XEXP(x, 0) = copy_rtx(XEXP(x, 0));
			}
			x = XEXP(x, 0);
			rewrite_in_place(&XEXP(x, 0));
		}
	case ADDR_VEC:
	case ADDR_DIFF_VEC:
		return FALSE;	
	case SET:
		if (GET_CODE(SET_DEST(x)) == PC)
			return FALSE;
		if (try_rewrite(i, &SET_SRC(x)) &&
			recog_memoized(i) == -1)
		{

			rtx seq;
			rtx before = first_use_insn(i);
			start_sequence();
			SET_SRC(x) = copy_addr_to_reg(SET_SRC(x));
			seq = gen_sequence();
			end_sequence();
			emit_insn_before(seq, before);
		}
		return try_rewrite(i, &SET_DEST(x));
	case MEM:
		if (needs_rewrite(x))
		{
			*loc = x = copy_rtx(x);
		}
		if (try_rewrite(i, &XEXP(x, 0)))
		{
			rtx seq;
			rtx before = first_use_insn(i);
			start_sequence();
			*loc = validize_mem(x);
			seq = gen_sequence();
			end_sequence();
			emit_insn_before(seq, before);
		}
		if (IS_CASE_MEM(x))
		{
			rewrite(i, loc);
		}
		return FALSE;
	case CALL:
		if (GET_CODE(XEXP(x, 0)) == MEM && 
			GET_CODE(XEXP(XEXP(x, 0), 0)) == SYMBOL_REF)
		{
			if (TARGET_USE_CALLX || TARGET_CAVE)
			{
				XEXP(x, 0) = copy_rtx(XEXP(x, 0));
				rewrite(i, &XEXP(XEXP(x, 0), 0));
			}
		}
		return FALSE;
	case CONST:
		if (i960_const_pic_pid_p(x, 1, 0))
		{
			rewrite(i, loc);
			return TRUE;		/* Need validation */
		}
		return FALSE;
	case LABEL_REF:
		rewrite(i, loc);
		return TRUE;

	case SYMBOL_REF:
		if (SYMREF_ETC(x) & SYMREF_PICBIT)
		{
			rewrite(i, loc); 
			return TRUE;
		}
		return FALSE;
	default:
	{
		char* fmt = GET_RTX_FORMAT (code);
		int len   = GET_RTX_LENGTH (code);
		int need_validation = FALSE;

		while (--len >= 0)
		{
			if (fmt[len] == 'e')
			{
				need_validation |= try_rewrite(i, &XEXP(x,len));
			}
			else if (fmt[len] == 'E')
			{
				int vlen = XVECLEN(x, len);
				while (--vlen >= 0)
				{
					need_validation |=
						try_rewrite(i, &XVECEXP(x, len,
							vlen));
				}
			}
		}
		return need_validation;
	}
	}
}

static void 
rewrite_reg_notes(insns)
rtx insns;
{
	rtx i;
	rtx link;
	for (i = insns; i; i = NEXT_INSN(i))
	{
                if (GET_RTX_CLASS(GET_CODE(i)) != 'i')
		{
			continue;
		}
		for (link = REG_NOTES(i); link; link = XEXP (link, 1))
		{
			rewrite_in_place(&XEXP(link, 0));
		}
	}
}

void
imstg_pic_rewrite(insns)
rtx insns;
{
	rtx i;

	bias = 0;	/* A register containing the pic relocation bias */

	for (i = insns; i; i = NEXT_INSN(i))
	{
		if (GET_RTX_CLASS(GET_CODE(i)) == 'i')
		{
			try_rewrite(i, &PATTERN(i));
		}
	}
	if (bias)
	{
		insert_bias_computation(insns);
		rewrite_reg_notes(insns);
	}
}

#endif
