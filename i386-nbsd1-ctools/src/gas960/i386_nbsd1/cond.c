/* cond.c - conditional assembly pseudo-ops, and .include
   Copyright (C) 1990, 1991 Free Software Foundation, Inc.
   
   This file is part of GAS, the GNU Assembler.
   
   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.
   
   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include "as.h"

#define COND_IF 1
#define COND_ELSE 0

/* The cond stack is designed to grow when an .if is scanned, and to
 * shrink when .endif is scanned.  .else toggles the top element.
 * When the global_val field on the top of the stack is nonzero, it means 
 * we should accept input.  Otherwise, we should ignore input.  
 */
struct cond_stack_elem
{
    unsigned char type;	/* 1 = .if, 0 = .else; To catch two .else's in a row */
    unsigned char val;	/* truth value of this element */
    unsigned char global_val;	/* inherited global truth val of parent */
};

struct cond_stack
{
    int top;		/* index into the stack; -1 means it's empty */
    int max_elems;	/* no limit; top >= max_elems triggers a realloc */
    struct cond_stack_elem *stack;  /* the stack */
};

static struct cond_stack cond_stack;

static struct cond_stack_elem *pop_cond();
static void push_cond();

#ifdef DEBUG_COND
static void dump_cond();
#endif

/* Globally visible functions */

void 
cond_begin()
{
    /* Initialize the stack.  Start off accepting input. 
       Stack elements will be allocated in the first call to push_cond. */
    cond_stack.max_elems = 0;
    cond_stack.top = -1;
}

int
ignore_input() 
{
    if (cond_stack.top < 0)
	/* Cond stack is empty; we are accepting input */
	return 0;
	
    /* We cannot ignore the cond stack pseudo ops.  */
    if (input_line_pointer[-1] == '.')
    {
	if (input_line_pointer[0] == 'i'
	    && (!strncmp (input_line_pointer, "if", 2)
		|| !strncmp (input_line_pointer, "ifdef", 5)
		|| !strncmp (input_line_pointer, "ifndef", 6)))
	    return 0;
	if (input_line_pointer[0] == 'e'
	    && (!strncmp (input_line_pointer, "else", 4)
		|| !strncmp (input_line_pointer, "endif", 5)))
	    return 0;
    }
    return ! cond_stack.stack[cond_stack.top].global_val;
}

void s_if(arg)
    int arg;
{
    expressionS operand;
	
    SKIP_WHITESPACE();	/* Leading whitespace is part of operand. */
    expr(0, &operand);
    
    if (operand.X_add_symbol != NULL
	|| operand.X_subtract_symbol != NULL)
	as_bad("non-constant expression in .if statement");
    
    /* If the above error is signaled, this will dispatch
       using an undefined result.  No big deal.  */
    push_cond((operand.X_add_number != 0) ^ arg);
}

void s_else(arg)
    int arg;
{
    struct cond_stack_elem *top = &cond_stack.stack[cond_stack.top];
    if (cond_stack.top < 0 || top->type == COND_ELSE)
    {
	as_bad(".else without matching .if");
    } 
    else 
    {
	/* We know the top of the stack is an if.  Change its type to else
	   and toggle its truth value.  */
	top->val = ! top->val;
	top->type = COND_ELSE;
	if (cond_stack.top)
	{
	    /* There is at least one prior element; toggle the global truth 
	       value only if the parent's global value is true. */
	    top->global_val ^= 
		cond_stack.stack[cond_stack.top - 1].global_val;
	}
	else
	{
	    /* This is the first element; toggle the global truth value */
	    top->global_val = ! top->global_val;
	}
    }
}

void s_endif(arg)
    int arg;
{
    pop_cond();
}

void 
s_ifdef(arg)
    int arg;
{
    register char 		*name;	  /* points to name of symbol */
    register struct symbol 	*symbolP; /* Points to symbol */
    int			c;
    
    SKIP_WHITESPACE();		/* Leading whitespace is part of operand. */
    name = input_line_pointer;
    if (!is_name_beginner(*name)) 
    {
	as_bad("invalid identifier for .ifdef");
	push_cond(0);
    } 
    else 
    {
	c = get_symbol_end();
	symbolP = symbol_find(name);
	*input_line_pointer = c;
	SKIP_WHITESPACE();
	
	push_cond((symbolP != 0) ^ arg);
    }
}

/* 
 * This checks for too many .if's.  Too many .endif's will be 
 * caught in s_endif().
 *
 * Returns 1 if balanced, 0 if not balanced.  Balances the stack
 * if it is found to be unbalanced.
 */
int check_cond_stack()
{
    if (cond_stack.top != -1)
    {
	cond_stack.top = -1;
	return 0;
    }
    else 
	return 1;
}

void s_end(arg)
	int arg;
{
    ;
}

/* Functions local to this module */

static void
push_cond(val)
    int	val;
{
    struct cond_stack_elem *top;
    if (++cond_stack.top >= cond_stack.max_elems)
    {
	if (cond_stack.stack)
	{
	    /* Double the size of the stack and continue without complaining */
	    cond_stack.max_elems *= 2;
	    cond_stack.stack = (struct cond_stack_elem *)
		xrealloc(cond_stack.stack, 
			 cond_stack.max_elems * sizeof(struct cond_stack_elem));
	}
	else
	{
	    /* First time this has been called.  Pick a number of elements;
	       we'll get more if we need them */
	    cond_stack.max_elems = 20;
	    cond_stack.stack = (struct cond_stack_elem *)
		xmalloc(cond_stack.max_elems * sizeof(struct cond_stack_elem));
	}
    }
    top = &cond_stack.stack[cond_stack.top];
    top->val = val;
    top->type = COND_IF;
    if (cond_stack.top)
    {
	/* There is at least one prior element; set the global truth value
	   only if the parent's global value is true. */
	top->global_val = 
	    cond_stack.stack[cond_stack.top - 1].global_val & val;
    }
    else
    {
	/* This is the first element; set global truth value */
	top->global_val = val;
    }
}

static 
struct cond_stack_elem *
pop_cond()
{
    if (cond_stack.top < 0)
    {
	as_bad(".endif without matching .if");
	return NULL;
    }
    else
	return cond_stack.stack + cond_stack.top--;
}

#ifdef DEBUG_COND
static
void
dump_cond(str)
    char *str;
{
    int i;
    if (! flagseen['P'])
	return;
    if (cond_stack.top < 0)
    {
	printf("%s: no stack\n", str);
	return;
    }
    printf("%s: %s\n%12s%12s%12s%12s\n",
	   str, cond_stack.stack[cond_stack.top].global_val ? "TRUE" : "FALSE",
	   "Index", "Type", "Val", "Global Val");
    for (i = cond_stack.top; i >= 0; --i)
    {
	struct cond_stack_elem *elem = &cond_stack.stack[i];
	printf("%12d%12s%12s%12s\n",
	       i,
	       elem->type == COND_IF ? "if" : "else",
	       elem->val ? "T" : "F",
	       elem->global_val ? "T" : "F");
    }
}
#endif
