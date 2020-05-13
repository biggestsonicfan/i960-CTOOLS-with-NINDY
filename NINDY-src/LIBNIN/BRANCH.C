
/******************************************************************
 *
 * 		Copyright (c) 1989, Intel Corporation
 *
 * Intel hereby grants you permission to copy, modify, and 
 * distribute this software and its documentation.  Intel grants
 * this permission provided that the above copyright notice 
 * appears in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation.  In
 * addition, Intel grants this permission provided that you
 * prominently mark as not part of the original any modifications
 * made to this software or documentation, and that the name of 
 * Intel Corporation not be used in advertising or publicity 
 * pertaining to distribution of the software or the documentation 
 * without specific, written prior permission.  
 *
 * Intel Corporation does not warrant, guarantee or make any 
 * representations regarding the use of, or the results of the use
 * of, the software and documentation in terms of correctness, 
 * accuracy, reliability, currentness, or otherwise; and you rely
 * on the software, documentation and results solely at your own 
 * risk.
 *
 ******************************************************************/


#include <stdio.h>

#define	COMPARE_MASK 0xFF000000
#define	COMPARE_INST 0x5A000000

static	unsigned int *branch_ptr = NULL;

typedef struct {
	unsigned int pre_branch;	/* pre-branch instruction counter */
	unsigned int post_branch;	/* post-branch instruction counter */
	unsigned int inst_addr;		/* branch instruction address */
	int	 pad;			/* structure pad word */
} br_data;


/*********************************************************************
 *
 * NAME
 *	__branch_store - save away a file's branch table address
 *
 * DESCRIPTION
 *	Every function in a module compiled with the -g flag will
 *	make a call to __branch_init, which in turn calls this function.
 *	The purpose of this function is to save away the address of
 *	a table, one per module, which contains information about
 *	instrumented branch instructions. The branch tables contain
 *	a header, and within each header is space to store the adddress
 *	of another block, enabling the blocks to be linked together
 *	into a list.
 *
 * PARAMETERS
 *	The address of the module's branch table.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
__branch_store(branch_addr)

unsigned int branch_addr;

{
	unsigned int *branches;	/* branch table address */

#ifdef DEBUG
	fprintf(stderr, "branch_store: branch_addr = 0x%x\n", branch_addr);
#endif

	/*
	 * If the main list pointer is NULL then this is our
	 * first branch table address. Assign the main pointer to
	 * it and return.
	 */
	if (branch_ptr == NULL) {
		branch_ptr = (unsigned int *) branch_addr;
		return;
	}

	/*
	 * There is one or more branch tables in the list. Traverse
	 * to the end of the list, then add the current address.
	 * If along the way we discover an identical table address
	 * then just return.
	 */
	branches = branch_ptr;
	if (branches == ((unsigned int *) branch_addr)) {
		return;
	}
	while (*branches != 0) {

		/*
		 * If the address of the branch table we are trying to
		 * add is already on the list then forget the whole mess
		 * and return.
		 */
		if (*branches == branch_addr) {
			return;
		}

		/*
		 * Go to next table.
		 */
		branches = (unsigned int *)(*branches);
	}
	*branches = branch_addr;
}



/*********************************************************************
 *
 * NAME
 *	__branch_dump - print an object's branch-counter data
 *
 * DESCRIPTION
 *	Each module, when compiled and assembled with the -g flag,
 *	and then run, will contain information about the branch
 *	behavior of the module. This function is called from a special
 *	crt file at a user program's conclusion and lives to print
 *	all the saved branch-instruction information.
 *
 * PARAMETERS
 *	None.
 *
 * RETURNS
 *	Void.
 *
 *********************************************************************/

void
__branch_dump()

{
	br_data	inst;			/* branch instruction data */
	unsigned int *branches;		/* function table block */
	unsigned int *inst_type;	/* instruction type */
	unsigned int *addr;		/* pointer to instruction address */
	int	*count_num;		/* number of branches in block */
	int	i;			/* loop control */
	FILE	*br_desc;		/* branch file descriptor */

	/*
	 * Cycle through the list of branch tables.
	 * For each table:
	 *	get the number of branches
	 *	for each branch:
	 *		print the instruction address
	 *		print the pre-branch hit count
	 *		print the post-branch hit count
	 */
	printf("\n");
	if (branch_ptr == NULL) {
		printf("NO BRANCH-PREDICTION DATA!\n");
		return;
	}

	/*
	 * Branch data, or something that closely resembles it
	 * exists. Open the data file and start traversing the
	 * list of branch tables.
	 */
	if ((br_desc = fopen("gbr960.data", "w")) == NULL) {
		printf("libnin: could not open gbr960 data file\n");
		exit(1);
	}
	branches = branch_ptr;
	do {

		/*
		 * Get the number of branch entries in this
		 * table.
		 */
		count_num = (int *) branches;
		count_num++;

#ifdef DEBUG
	fprintf(stderr, "branch_dump: block_address = 0x%x\n", branches);
	fprintf(stderr, "             count_num     = %d\n", *count_num);
#endif

		/*
		 * For each branch instruction...
		 */
		for (i = 0; i < *count_num; i++) {

			/*
			 * Print the branch instruction address.
			 */
			/*
			 * Assign a pointer to the branch instruction address.
			 * Then save the address and pre-branch instruction
			 * hit count for writing.
			 */
			addr = (unsigned int *) (count_num + 1 + i);
			inst.inst_addr = *addr;
			inst.pre_branch = *(unsigned int *)(*addr - 4);
			
			/*
			 * Check the instruction type at the instruction
			 * address. If it is a compare instruction the 
			 * assembler expanded a compare-and-branch into
			 * two separate instructions which means the branch
			 * not-taken counter is two words offset from the
			 * instruction address. If the instruction is not
			 * a compare instruction the counter is only one
			 * word offset.
			 */
			inst_type = (unsigned int *) (*addr);
			if ((*inst_type & COMPARE_MASK) == COMPARE_INST) {
				inst.post_branch = *(unsigned int *)(*addr + 12);
			}
			else {
				inst.post_branch = *(unsigned int *)(*addr + 8);
			}

			/*
			 * Save the post-branch hit count. Then write
			 * the entire instruction's data record out to
			 * file.
			 */
			if (fwrite(&inst, sizeof(br_data), 1, br_desc) < 1) {
				printf("libnin: gbr960 data write error\n");
				fclose(br_desc);
				exit(1);
			}
		}

		/*
		 * Go to next table.
		 */
		branches = (unsigned int *)(*branches);
	}
	while (branches != 0);
	fclose(br_desc);
}
