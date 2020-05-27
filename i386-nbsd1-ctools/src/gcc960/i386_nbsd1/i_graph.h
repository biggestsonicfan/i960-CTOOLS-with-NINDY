
#define GRAPH_INCLUDED

/**
***
***  Exported types
***
***  GRAPH_NODE, GRAPH_NODE_STRUCT
***
***
***     rtx insn	
***
***	   Pointer to the rtl that this node describes.  The only
***        rtl that are valid are INSN, CALL_INSN, JUMP_INSN 
***        and CODE_LABEL. There is one exception to this rule and that
***        is for the GRAPH_entry node.  The insn assocaited with this node
***        is the NOTE_INSN_FUNCTION_ENTRY NOTE.  This was done to ensure
***        that the GRAPH_entry node has no predecessors.
***
***     int loop_number
***
***        An int giving the number of the loop that this graph node is
***        most closely contained in. A loop number of -1 indicates that
***        this node os outside the scope of a loop.
***
***     int loop_backedge
***
***        A flag, if TRUE it indicates that this node is the backedge for
***        a loop and one of the successors will be the head of a loop.
***
***     int loop_exit
***
***        A flag, if TRUE it indicates that this node is an exit from the
***        loop it is contained in.
***
***     int loop_entrance
***
***        A flag, if TRUE it indicates that this node is an entrance for a
***        loop.  One of its successors must have a loop number that is
***        different from the loop number of the loop entrance node.
***
***     int loop_head
***
***        A flag, if TRUE it indicates that this node is the head of a loop
***        one of its predecessors must be a backedge,  all the rest must be
***        loop entrances. Currently we set the loop head to be the first label
***        that follows a NOTE_INSN_LOOP_BEG note.
***
***     int pred_count
***
***        Count of the number of predecessors.
***
***     int pred_size
***
***        Gives the current size of the pred vector.  Used only when building
***        the flow graph.
***
***     GRAPH_NODE *preds
***
***        Pointer to a vector of GRAPH_NODE's that are the predecessors for
***        this node.
***
***     int succ_count
***
***        Count of the number of successors.
***
***     GRAPH_NODE *succs
***
***        Pointer to a vector of GRAPH_NODE's that are the successors for this
***        node.
***
***     double *probs
***
**         Pointer to a vector of probabilities for each successor.  Gives the
***        probability that you will make the transition from the current node
***        to the successor.  There is a one to one correspondence with vector
***        of successors.  The probabilities should add up to 1.0.
***
***     double expect
***
***        The 'expect' gives the number of times this particular node is
***        expected to be executed. This value can take on either a local
***        or global weighting. It is a local weighting if the 'expect' of
***        the entry node is initialized to 1 or it can be a global weighting
***        if the 'expect' of the entry node is initialized to the functions
***        call count. This field is set by the routine EXPECT_Calculate.
***
***     int search_mark
***
***        Used to mark a given graph node as having been visited.  This is
***        just a convienient way to make sure you only visit each node once.
***     
***     int sblock_num
***
***        The number of the superblock that this node belongs to.
***
***     int line_num
***
***        The closest source line number.
**/


typedef struct graph_node_tag *GRAPH_NODE, GRAPH_NODE_STRUCT;

struct graph_node_tag {
    rtx insn;
    int loop_number;
    unsigned int loop_backedge:1;
    unsigned int loop_exit:1;
    unsigned int loop_entrance:1;
    unsigned int loop_head:1;
    int pred_count;
    int pred_size;
    GRAPH_NODE *preds;
    int succ_count;
    GRAPH_NODE *succs;
    double *probs;
    double expect;
    int search_mark;
    int sblock_num;
    int line_num;
};

    
/**
*** Exported globals    
***
*** extern GRAPH_NODE GRAPH_nodes
***
***    Pointer to the array of all GRAPH_NODE's in the flow graph.  The
***    space is allocated by GRAPH_BuildGraph and freed by GRAPH_FreeGraph.
***
***    To get from rtl to a graph node GRAPH_nodes may be indexed by the
***    INSN_UID of the insn.
***
*** extern int GRAPH_node_count
***
***    Count of the number of GRAPH_NODE's in the flow graph.
***
*** NOTE: To iterate over all the nodes in the graph the array GRAPH_nodes
***       may be indexed from 0 to GRAPH_node_count - 1.  Not all entries
***       in GRAPH_nodes may be valid. Entries are only valid if the 'insn'
***       field is not NULL.
***
*** extern int GRAPH_loop_count
***
***    Count of the number of loops in the flow graph.
*** 
*** extern GRAPH_NODE GRAPH_entry
***
***    Pointer to the entry node for the flow graph.  This interface only
***    supports a single entry to the flow graph (should not be a problem
***    for C). The insn associated with the GRAPH_entry will be a
***    NOTE_INSN_FUNCTION_ENTRY note.  This node does not have any predecessors.
***
*** extern int GRAPH_search_mark
***
***    Used in conjunction with the search_mark field of a GRAPH_NODE.
**/

extern GRAPH_NODE GRAPH_packed_nodes;
extern GRAPH_NODE * GRAPH_nodes;
extern GRAPH_NODE GRAPH_entry;
extern int GRAPH_node_count;
extern int GRAPH_loop_count;
extern int GRAPH_search_mark;


/**
*** Exported Functions
***
***
***     GRAPH_NODE GRAPH_BuildGraph(insn f)
***
***     Build the flow graph.  There will be a node in the flow graph
***     for each INSN, JUMP_INSN, CALL_INSN and CODE_LABEL. Sets up the
***     predecessors and successors for each node in the flow graph. Also
***     initialize the prob vector using the JUMP_THEN_PROB field from
***     a JUMP_INSN.  Nodes with more than 2 successors have their
***     probablities normalized between all successors.  This can be
***     changed if needed.
***
***     Returns the GRAPH_entry node if we were able to build a flow graph.
***
***     Returns NULL if unable to build the flow graph due to inconsistencies
***     in the rtl.  This should not happen for syntactically correct C
***     programs.
***
***     void GRAPH_FreeGraph()
***
***     Frees all storage that has been allocated for representing the
***     flow graph.
***
***     int GRAPH_Successor(rtx x)
***
***      x must have only one successor, ignoring the fall thorugh case,
***      find the successor and return it 
***      x must be the PATTERN of a JUMP_INSN.
***
*** Debugging Functions
***
***     GRAPH_PrintGraph( FILE *outfile)
***
***     Prints out the rtl for the current function followed by the
***     additional information that has been kept in the graph.
***
***     GRAPH_DebugGraph( char *filename)
***
***     Opens a the file 'filename' and prints then calls GRAPH_PrintGraph()
***     to print the flow graph to it. This routine is provided so that it may
***     be called from the debugger.
***
***     GRAPH_DebugNode(GRAPH_NODE node)
***
***     Prints 'node' to stderr. This routine is provided so that it may
***     be called from the debugger.
***
**/     


GRAPH_NODE GRAPH_BuildGraph();
void GRAPH_FreeGraph();

void GRAPH_PrintGraph();
void GRAPH_DebugGraph();
void GRAPH_DebugNode();
rtx GRAPH_Successor();
