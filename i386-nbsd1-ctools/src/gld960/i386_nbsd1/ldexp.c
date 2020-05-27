/*
 * Copyright (C) 1991 Free Software Foundation, Inc.
 *
 * This file is part of GLD, the Gnu Linker.
 *
 * GLD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * GLD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GLD; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* $Id: ldexp.c,v 1.32 1995/08/11 16:02:35 paulr Exp $ */

/*
 * Written by Steve Chamberlain
 * steve@cygnus.com
 *
 * This module handles expression trees.
 */


#include "sysdep.h"
#include "bfd.h"
#include "ld.h"
#include "ldmain.h"
#include "ldmisc.h"
#include "ldexp.h"
#include "ldgramtb.h"
#include "ldsym.h"
#include "ldlang.h"

extern char *output_filename;
extern unsigned int undefined_global_sym_count;
extern unsigned int defined_global_sym_count;
extern bfd *output_bfd;
extern bfd_size_type largest_section;
extern lang_statement_list_type file_chain;
extern args_type command_line;
extern ld_config_type config;
extern lang_input_statement_type *script_file;
extern unsigned int defined_global_sym_count;
extern bfd_vma print_dot;

static int carry;	/* this variable remembers the carry bit between
			calls to add_cksum() in support of the CHECKSUM
			directive */

static
DEFUN(int new_val,(value,new),
      bfd_vma value AND
      etree_value_type *new)

{
	new->valid = true;
	new->value = value;
	return 1;
}

static void 
DEFUN(check, (os, name, op),
      lang_output_section_statement_type *os AND
      CONST char *name AND
      CONST char *op)
{
	if (!os) {
		info("%F%P %s uses undefined section %s\n", op, name);
	}
	if (!os->processed) {
		info("%F%P %s forward reference of section %s\n",op, name);
	}
}

static void
DEFUN(check_and_fix,(allocation_done,sy,def,value,r1,r2),
      lang_phase_type allocation_done      AND
      ldsym_type *sy                       AND  /* existing symbol in symbol table. */
      asymbol *def                         AND  /* new symbol to overwrite or change existing symbol. */
      bfd_vma value                        AND
      bfd_vma r1                           AND
      bfd_vma r2
      )
{
    if (allocation_done == lang_final_phase_enum) {
	if (value < r1 || value >= r2) {
	    /* The new value is outside the range of the section definition.
	       The section range is only known during final allocation phase.
	       Change the existing symbol table entry and the new one to correspond
	       to an absolute symbol. */
	    /* Change new one always: */
	    def->flags |= BSF_ABSOLUTE;
	    def->value += def->section->vma;
	    def->section = (sec_ptr) 0;
	    /* Now, change old one to absolute.  If there was an old definition. */
	    if (sy->sdefs_chain && (*sy->sdefs_chain))
		    (*sy->sdefs_chain)->section = (sec_ptr) 0;
	}
    }
}

bfd_vma 
DEFUN(add_cksum,(first, second),
bfd_vma first AND
bfd_vma second)
{
 	bfd_vma bottom1, bottom2, top1, top2, result1, result2;
	bfd_vma final_result;
 	bfd_vma in1,in2;
 
 	in1 = first;
 	in2 = second;
 	/* peel off least significant 31 bits */
 	bottom1 = 0x7fffffff & in1;
 	bottom2 = 0x7fffffff & in2;
 	/* peel off most significant bit */
 	top1 = (0x80000000 & in1) >> 31;
 	top2 = (0x80000000 & in2) >> 31;
 	/* add two bottoms plus carry bit to see whether they throw a bit
 	into the most significant bit */
 	result1 = bottom1 + bottom2 + carry;
 	/* add tops plus carry from bottom (if any) to see whether we get
 	a genuine carry out of the addition */
 	result2 = top1 + top2 + (result1 >> 31);
 	/* do addition while we still have the old carry bit */
 	final_result = in1 + in2 + carry;
 	/* set carry bit for next addition */
 	if (result2 < 2) { 
 		carry = 0;
 	}
 	else {
 		carry = 1;
 	}
 	return(final_result);
}
 

etree_type *
DEFUN(exp_intop,(value),
      bfd_vma value)
{
	etree_type *new;

	new = (etree_type *)ldmalloc((bfd_size_type)(sizeof(new->value)));
	new->type.node_code = INT;
	new->value.value = value;
	new->type.node_class = etree_value;
	return new;
}


static
DEFUN(int fold_binary,(tree, current_section, allocation_done, dot, dotp, result),
      etree_type *tree AND
      lang_output_section_statement_type *current_section AND
      lang_phase_type  allocation_done AND
      bfd_vma dot AND
      bfd_vma *dotp AND
      etree_value_type *result)
{
	exp_fold_tree(tree->binary.lhs, current_section,
						allocation_done, dot, dotp, result);
	if (result->valid) {
		etree_value_type other;
		exp_fold_tree(tree->binary.rhs, current_section,
						allocation_done, dot,dotp, &other) ;
		if (other.valid) {

			switch (tree->type.node_code) {
 			case CKSUM:
 				result->value = add_cksum(result->value,other.value);
 				break;
			case '%':
				/* Mod, both absolute */
				if (other.value == 0) {
					info("%F%S % by zero\n");
				}
				result->value %= other.value;
				break;
			case '/':
				if (other.value == 0) {
					info("%F%S / by zero\n");
				}
				result->value /= other.value;
				break;
#define BOP(x,y) case x : result->value = result->value y other.value;break;
				BOP('+',+);
				BOP('*',*);
				BOP('-',-);
				BOP(LSHIFT,<<);
				BOP(RSHIFT,>>);
				BOP(EQ,==);
				BOP(NE,!=);
				BOP('<',<);
				BOP('>',>);
				BOP(LE,<=);
				BOP(GE,>=);
				BOP('&',&);
				BOP('^',^);
				BOP('|',|);
				BOP(ANDAND,&&);
				BOP(OROR,||);
			default:
				FAIL();
			}
		} else {
			result->valid = false;
		}
	}
	return 1;
}


int
DEFUN(invalid,(new),etree_value_type *new)
{
	new->valid = false;
	return 1;
}


int
DEFUN(fold_name, (tree, current_section, allocation_done, dot, result),
      etree_type *tree AND
      lang_output_section_statement_type *current_section AND
      lang_phase_type  allocation_done AND
      bfd_vma dot AND
      etree_value_type *result)
{
	lang_output_section_statement_type *os;

	switch (tree->type.node_code) {
	case SIZEOF_HEADERS:
		if (allocation_done != lang_first_phase_enum) {
		    new_val(bfd_sizeof_headers(output_bfd,
					       config.relocateable_output),result);
		} else {
			result->valid = false;
		}
		break;
	case DEFINED:
		if (allocation_done != lang_first_phase_enum) {
		    new_val(!(ldsym_undefined(tree->name.name)),result);
		}
		else
			result->valid = false;
		break;
	case NAME:
		result->valid = false;
		if (tree->name.name[0] == '.' && tree->name.name[1] == 0) {
		    if (allocation_done != lang_first_phase_enum)
			    new_val(dot,result);
		    else
			    invalid(result);
		    break;
		}

		/* We can return the resolution of absolute references during
		   allocation. */
		if (allocation_done != lang_first_phase_enum) {
		    ldsym_type *sy = ldsym_get_soft(tree->name.name);

		    if (sy && sy->sdefs_chain) {
			asymbol *sdef = *sy->sdefs_chain;
			if (!sdef->section)
				/* absolute symbol */
				new_val(sdef->value,result);
		    }
		}
		if (allocation_done == lang_final_phase_enum) {
		    ldsym_type *sy = ldsym_get_soft(tree->name.name);
		    
		    if (sy && sy->sdefs_chain) {
			asymbol *sdef = *sy->sdefs_chain;
			if (sdef->section)
				new_val(sdef->value + sdef->section->output_offset +
					sdef->section->output_section->vma,result);
			else
				new_val(sdef->value,result);
		    }
		    if ( !result->valid)
			    info("Undefined symbol `%s' referenced in expression.\n",
				 tree->name.name);
		}
		break;

	case CKSUM:
		result->valid = false;
		if (allocation_done == lang_final_phase_enum) {
			ldsym_type *sy = ldsym_get_soft(tree->name.name);

			if (sy && sy->sdefs_chain) {
				asymbol *sdef = *sy->sdefs_chain;
				if (!sdef->section)
					/* absolute symbol */
					new_val(sdef->value,result);
				else
					new_val(sdef->value + sdef->section->output_offset +
						sdef->section->output_section->vma,
						result);
			}
			if ( !result->valid) {
				info("Undefined symbol `%s' referenced in expression.\n",
				    tree->name.name);
			}
		}
		break;

	case ADDR:
		if (allocation_done != lang_first_phase_enum) {
			os = lang_output_section_find(tree->name.name);
			check(os,tree->name.name,"ADDR");
			new_val(os->bfd_section->vma, result);
		} else {
			invalid(result);
		}
		break;

	case SIZEOF:
		if(allocation_done != lang_first_phase_enum) {
			os = lang_output_section_find(tree->name.name);
			check(os,tree->name.name,"SIZEOF");
			new_val((bfd_vma)(os->bfd_section->size),result);
		} else {
			invalid(result);
		}
		break;

	default:
		FAIL();
		break;
	}

	return 1;
}


int
DEFUN(exp_fold_tree,(tree, current_section, allocation_done,
		    dot, dotp, result),
      etree_type *tree AND
      lang_output_section_statement_type *current_section AND
      lang_phase_type  allocation_done AND
      bfd_vma dot AND
      bfd_vma *dotp AND
      etree_value_type *result)
{
	result->valid = true; /* Assume the result is valid for those traces through
			     this code that do not explicitly set the result
			     valid value. */
	if (tree == (etree_type *)NULL) {
		result->valid = false;
		return 1;
	}

	switch (tree->type.node_class) {
	case etree_value:
		new_val(tree->value.value, result);
		break;
	case etree_unary:
		exp_fold_tree(tree->unary.child, current_section,
						allocation_done, dot, dotp, result);
		if (result->valid) {
			switch(tree->type.node_code) {
			case ALIGN_K:
				if (allocation_done != lang_first_phase_enum) {
					unsigned int u = result->value,bitcount = 0;
					for (;u;u = u >> 1)
						bitcount += (u & 1);
					if (bitcount > 1)
						info("%x is not a power of two, alignment will not be as expected\n",
						     result->value);
					new_val(ALIGN(dot,result->value),result);
				} else {
					result->valid = false;
				}
				break;
			case '~':
				result->value = ~result->value;
				break;
			case '!':
				result->value = !result->value;
				break;
			case '-':
				result->value = -result->value;
				break;
			default:
				FAIL();
			}
		}
		break;

	case etree_trinary:
		exp_fold_tree(tree->trinary.cond, current_section,
						allocation_done, dot, dotp, result);
		if (result->valid) {
			exp_fold_tree(result->value ?
			    tree->trinary.lhs:tree->trinary.rhs,
			    current_section, allocation_done, dot, dotp, result);
		}
		break;

	case etree_binary:
		fold_binary(tree, current_section, allocation_done,
								dot, dotp, result);
		break;

	case etree_assign:
		if (tree->assign.dst[0] == '.' && tree->assign.dst[1] == 0) {
			/* Assignment to dot can only be done during allocation
		 	 */
			if (allocation_done == lang_allocating_phase_enum) {
				exp_fold_tree(tree->assign.src,
				    current_section, lang_allocating_phase_enum, dot, dotp,
					      result);
				if (result->valid == false) {
					info("%F%S invalid assignment to location counter\n");
				} else if (!current_section) {
					info("%F%S assignment to location counter invalid outside of SECTION\n");
				} else {
					unsigned long nextdot;

					nextdot = result->value;
					if (nextdot < dot) {
						info("%F%S cannot move location counter backwards");
					} else {
						*dotp = nextdot;
					}
				}
			}
		} else {
			ldsym_type *sy = ldsym_get(tree->assign.dst);

			/* If this symbol has just been created then we'll
			 * place it into a section of our choice
			 */
			exp_fold_tree(tree->assign.src,
			    current_section, allocation_done, dot, dotp, result);
			if (result->valid) {
				asymbol *def;
				asymbol **def_ptr;

				def_ptr = (asymbol **)ldmalloc((bfd_size_type)(sizeof(asymbol **)));
				/* Add this definition to script file */
				def = (asymbol *)bfd_make_empty_symbol(script_file->the_bfd);
				*def_ptr = def;
				def->value = result->value;
				def->flags = BSF_GLOBAL | BSF_EXPORT;
				def->section = (asection *)NULL;
				def->udata = (PTR)NULL;
				def->name = sy->name;

				if (!current_section)
					def->flags |= BSF_ABSOLUTE;
				else {

				    /* This block may make symbols associated with sections that are
				       out of range of the section definition.  They are fixed in
				       check_and_fix(). */
					  
				    def->section = current_section->bfd_section;
				    def->value -= current_section->bfd_section->vma;
				    check_and_fix(allocation_done,sy,def,result->value,
						  def->section->vma,def->section->vma + def->section->size);
				}
				Q_enter_global_ref(def_ptr,1,allocation_done);
			}
		}
		break;

	case etree_name:
		fold_name(tree, current_section, allocation_done, dot, result);
		break;

	default:
		info("%F%S Need more of these %d",tree->type.node_class );
	}
	return 1;
}


int
DEFUN(exp_fold_tree_no_dot,(tree, current_section, allocation_done, result),
      etree_type *tree AND
      lang_output_section_statement_type *current_section AND
      lang_phase_type  allocation_done AND
      etree_value_type *result)
{
	return exp_fold_tree(tree, current_section, allocation_done, (bfd_vma)
	    0, (bfd_vma *)NULL,result);
}


etree_type *
DEFUN(exp_binop,(code, lhs, rhs),
      int code AND
      etree_type *lhs AND
      etree_type *rhs)
{
	etree_type value, *new;
	etree_value_type r;

	value.type.node_code = code;
	value.binary.lhs = lhs;
	value.binary.rhs = rhs;
	value.type.node_class = etree_binary;
	exp_fold_tree_no_dot(&value,
		(lang_output_section_statement_type *)NULL, lang_first_phase_enum, &r );
	if (r.valid) {
		return exp_intop(r.value);
	}
	new = (etree_type *)ldmalloc((bfd_size_type)(sizeof(new->binary)));
	memcpy((char *)new, (char *)&value, sizeof(new->binary));
	return new;
}

etree_type *
DEFUN(exp_trinop,(code, cond, lhs, rhs),
      int code AND
      etree_type *cond AND
      etree_type *lhs AND
      etree_type *rhs)
{
	etree_type value, *new;
	etree_value_type r;
	value.type.node_code = code;
	value.trinary.lhs = lhs;
	value.trinary.cond = cond;
	value.trinary.rhs = rhs;
	value.type.node_class = etree_trinary;
	exp_fold_tree_no_dot(&value, (lang_output_section_statement_type *)
			    NULL,lang_first_phase_enum, &r);
	if (r.valid) {
		return exp_intop(r.value);
	}
	new = (etree_type *)ldmalloc((bfd_size_type)(sizeof(new->trinary)));
	memcpy((char *)new,(char *) &value, sizeof(new->trinary));
	return new;
}


etree_type *
DEFUN(exp_unop,(code, child),
      int code AND
      etree_type *child)
{
	etree_type value, *new;

	etree_value_type r;
	value.unary.type.node_code = code;
	value.unary.child = child;
	value.unary.type.node_class = etree_unary;
	exp_fold_tree_no_dot(&value,(lang_output_section_statement_type *)NULL,
						lang_first_phase_enum, &r);
	if (r.valid) {
		return exp_intop(r.value);
	}
	new = (etree_type *)ldmalloc((bfd_size_type)(sizeof(new->unary)));
	memcpy((char *)new, (char *)&value, sizeof(new->unary));
	return new;
}


etree_type *
DEFUN(exp_nameop,(code, name),
      int code AND
      char *name)
{
	etree_type value, *new;
	etree_value_type r;

	value.name.type.node_code = code;
	value.name.name = name;
	value.name.type.node_class = etree_name;
	exp_fold_tree_no_dot(&value,
			(lang_output_section_statement_type *)NULL,
			lang_first_phase_enum, &r);
	if (r.valid) {
		return exp_intop(r.value);
	}
	else if (code == NAME) {
	    if (strcmp(".",name) && !ldsym_get_soft(name))
		    ldlang_add_undef(name);
	}
	new = (etree_type *)ldmalloc((bfd_size_type)(sizeof(new->name)));
	memcpy((char *)new, (char *)&value, sizeof(new->name));
	return new;
}


etree_type *
DEFUN(exp_assop,(code, dst, src),
      int code AND
      char *dst AND
      etree_type *src)
{
	etree_type value, *new;

	value.assign.type.node_code = code;
	value.assign.src = src;
	value.assign.dst = dst;
	value.assign.type.node_class = etree_assign;
	new = (etree_type*)ldmalloc((bfd_size_type)(sizeof(new->assign)));
	memcpy((char *)new, (char *)&value, sizeof(new->assign));
	return new;
}


bfd_vma
DEFUN(exp_get_vma,(tree, def, name, allocation_done),
      etree_type *tree AND
      bfd_vma def AND
      char *name AND
      lang_phase_type allocation_done)
{
	etree_value_type r;

	if (tree) {
		exp_fold_tree_no_dot(tree,
				(lang_output_section_statement_type *)NULL,
				allocation_done, &r);
		if (r.valid == false && name) {
			info("%F%S Nonconstant expression for %s\n",name);
		}
		return r.value;
	} else {
		return def;
	}
}

int 
DEFUN(exp_get_value_int,(tree,def,name, allocation_done),
      etree_type *tree AND
      int def AND
      char *name AND
      lang_phase_type allocation_done)
{
	return (int)exp_get_vma(tree,(bfd_vma)def,name, allocation_done);
}
