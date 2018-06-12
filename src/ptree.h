/*
 * ptree.h
 *
 * BabyC parse tree data structures for 15-411 Asst. 4
 *
 * Marc Ringuette, 9 Mar 88
 */

/* Note:  requires lexer definitions - include lexer.h before this */

typedef enum ParseNodeType {
	prsError, prsProduction, prsID, prsRelop, prsAddop, prsMulop,
	prsIntConst, prsFloatConst, prsStringConst
    } ParseNodeType;

/* ExprType:  used for type-checking expressions. */
typedef enum {
	etypeVoid,	/* for ParseNodes which are not valid in expressions */
	/* All other ParseNodes must have one of the following types:  */
	etypeInt,
	etypeFloat,
	etypeString,
	etypeUndecl	/* Type of undeclared variable refs, and derivative
			   expressions such as Declared + Undeclared.
			   Note that calls to undeclared functions are legal,
			   but are assumed to be of type int.  Thus, no 
			   prsFnCall node will ever have etype etypeUndecl,
			   even though the SymEntry it points to may very well
			   have dtype dtypeUndecl until actually declared.
			 */
    } ExprType;

typedef struct ParseNode {
	ParseNodeType nodetype;
	int linenum;
	int prodnum;
	int seqnum;
	ExprType etype;
	union {
	    struct ParseNode *child[4];	/* prsProduction */
	    struct {			/* prsID */
		char idname[MAXLEN];
		int idlen,idscope;
	    } id;
	    reltyp r;			/* prsRelop */
	    addtyp a;			/* prsAddop */
	    multyp m;			/* prsMulop */
	    int i;			/* prsIntConst */
	    float f;			/* prsFloatConst */
	    struct {			/* prsStringConst */
		char txt[MAXLEN];
		int slen;
	    } s;
	} u;
} ParseNode;
