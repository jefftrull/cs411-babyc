/*****************************************************************\
* 
*      gencode.c - generates 3-address code for BabyC compiler
* 
\*****************************************************************/


/*
* HISTORY
* 10-Apr-88  Jeffrey Trull (jt1j) at Carnegie-Mellon University
*	Created
*/


/*
* Include Files
*/

#include    "lexer.h"
#include    "ptree.h"
#include    "symtab.h"
#include    "gencode.h"
#include    <stdio.h>
#include    <sys/file.h>


/*
* Procedure Predeclarations
*/

void	    GenPseudoCode();
void	    GenDefCode();
void	    GenDefsCode();
int	    GenFuncRestCode();
int	    GenFuncBodyCode();
int	    CountTemps();
int	    CountLocals();
void	    DetermineLocalOffsets();
int	    GenStatementsCode();
int	    GenStatementCode();
int	    GenExprCode();
int	    GenRelExprCode();
int	    GenArithExprCode();
int	    GenArithExpr2Code();
int	    GenTermCode();
void	    VarPrint();
void	    IdPrint();
char	    *newlabel();


/*
* Data Declarations
*/

int	cur_block_number;
int	blockcount;
int	globalvarcount;
int	cur_label;
char	*ret_label;

/*
* Routines
*/

/* Create 3-address code from parse tree */

void	    GenPseudoCode()
/*
* main routine for code generation - recursively descends from
* root of ParseTree.
*/
{
    extern ParseNode	*ParseTree;

    cur_block_number = 0;
    blockcount = 0;
    globalvarcount = 0;
    printf("; MACRO-20 code generator for BabyC compiler\n");
    printf("; BabyC compiler by Jeffrey Trull\n");
    printf("\tR0=0\n\tR1=1\n\tR2=2\n\tP0=3\n\tP1=4\n\tP2=5\n\tP3=6\n\tP4=7\n");
    printf("\tP5=10\n\tP6=11\n\tP7=12\n\tP8=13\n\tP9=14\n\tFP=15\n\tAP=16\n");
    printf("\tSP=17\n\tVREG=R1\n");
    printf("\tEXTERN\tpr%%int\n");
    printf("\tEXTERN\tpr%%str\n");
    printf("\tEXTERN\tpr%%flo\n");
    printf("\tEXTERN\tpr%%nl\n");
    GenDefCode(ParseTree->u.child[0]);
    GenDefsCode(ParseTree->u.child[1]);
    printf("\tEND\n");
}

void	GenDefCode(node)
    ParseNode	*node;
{
    SymEntry	sym;
    int		numglobals;
    int		old_label;
    int		numtemps;
    int		i;
    ParseNode	*cur_node;
    extern int	SymGet(), SymUpdate();

    if (node->prodnum == 2) {
	/* void function declaration */
	node->u.child[0]->u.child[0]->
	    u.id.idname[node->u.child[0]->u.child[0]->u.id.idlen] = '\0';
	old_label = cur_label;
	numtemps = CountTemps(node->u.child[1]);
	if (numtemps > (MAXTEMP + 1)) {
	    printf("? function too complex, function %s\n",
		   node->u.child[0]->u.child[0]->u.id.idname);
	    exit(0);
	}
	printf("%s::\n", &node->u.child[0]->u.child[0]->u.id.idname[0]);
	printf("\tPUSH\tSP,FP\n");
	for (i = 0; i < numtemps; i++) {
	    printf("\tPUSH\tSP,P%d\n", i);
	}
	printf("\tMOVEI\tFP,1(SP)\n");
	printf("\tADJSP\tSP,<%d>\n", 
	    numtemps + CountLocals(node->u.child[1]->u.child[2]->u.child[0]));
	blockcount--;
	cur_label = old_label;
	ret_label = newlabel();
	GenFuncRestCode(node->u.child[1], stdout);
	printf("%s:\n", ret_label);
	printf("\tADJSP\tSP,-<%d>\n", 
	    numtemps + CountLocals(node->u.child[1]->u.child[2]->u.child[0]));
	for (i = numtemps - 1; i >= 0; i--) {
	    printf("\tPOP\tSP,P%d\n", i);
	}
	printf("\tPOP\tSP,FP\n");
	printf("\tPOPJ\tSP,\n");
    }
    else {
	/* production 3 */
	if (node->u.child[2]->prodnum == 11) {
	    /* prodn 11 - int or real function declaration */
	    node->u.child[1]->u.child[0]->
		u.id.idname[node->u.child[1]->u.child[0]->u.id.idlen] = '\0';
	    old_label = cur_label;
	    numtemps = CountTemps(node->u.child[2]->u.child[0]);
	    if (numtemps > (MAXTEMP + 1)) {
		printf("? function too complex, function %s\n",
			node->u.child[1]->u.child[0]->u.id.idname);
	        exit(0);
	    }
	    printf("%s::\n", &node->u.child[1]->u.child[0]->u.id.idname[0]);
	    printf("\tPUSH\tSP,FP\n");
	    for (i = 0; i < numtemps; i++) {
		printf("\tPUSH\tSP,P%d\n", i);
	    }
	    printf("\tMOVEI\tFP,1(SP)\n");
	    printf("\tADJSP\tSP,<%d>\n", 
		numtemps + CountLocals(node->u.child[2]->u.child[0]->u.child[2]->u.child[0]));
	    blockcount--;
	    cur_label = old_label;
	    ret_label = newlabel();
	    GenFuncRestCode(node->u.child[2]->u.child[0], stdout);
	    printf("%s:\n", ret_label);
	    printf("\tADJSP\tSP,-<%d>\n", 
		numtemps + CountLocals(node->u.child[2]->u.child[0]->u.child[2]->u.child[0]));
	    for (i = numtemps - 1; i >= 0; i--) {
		printf("\tPOP\tSP,P%d\n", i);
	    }
	    printf("\tPOP\tSP,FP\n");
	    printf("\tPOPJ\tSP,\n");
	}
	else {
	    /* prodn 12 - list of int or real global variables */
	    numglobals = 1;
	    bcopy(&node->u.child[1]->u.child[0]->u.id.idname[0],
		  &sym.id.idname[0],
		  MAXSYM);
	    sym.id.idlen = node->u.child[1]->u.child[0]->u.id.idlen;
	    sym.scope = 0;
	    SymGet(&sym);
	    sym.offset = globalvarcount++;  /* decide which global it is */
	    SymUpdate(&sym);
	    printf("; space for global\n");
	    IdPrint(&sym.id, stdout);
	    printf(":\tBLOCK\t1\n");
	    cur_node = node->u.child[2]->u.child[0];
	    while (cur_node->prodnum != 8) {
		numglobals++;
		bcopy(&cur_node->u.child[0]->u.child[0]->u.id.idname[0],
		      &sym.id.idname[0],
		      MAXSYM);
		sym.id.idlen = cur_node->u.child[0]->u.child[0]->u.id.idlen;
		sym.scope = 0;
		SymGet(&sym);
		sym.offset = globalvarcount++;  /* decide which global it is */
		SymUpdate(&sym);
		printf("; space for global\n");
		IdPrint(&sym.id, stdout);
		printf(":\tBLOCK\t1\n");
		cur_node = cur_node->u.child[1];
	    }
	}
    }
}

/* determine the number of temporaries in a given function */

int	CountTemps(node)
    ParseNode	*node;
/*
* starting with node, which is a "function_rest" production,
* count the number of temporaries used in the routine
*/
{
    FILE	*out_file;

    out_file = fopen("/dev/null", "w");
    return GenFuncRestCode(node, out_file);
}
    
/* count the number of locals in a function */

int	CountLocals(node)
    ParseNode	*node;
/*
* node is a "variable_defs" parse node
*/
{
    int	    	ret_val;
    ParseNode	*cur_node;

    if (node->prodnum == 17) {
	return 0;
    }
    ret_val = 1;
    cur_node = node->u.child[2];
    while (cur_node->prodnum != 8) {
	ret_val++;
	cur_node = cur_node->u.child[1];
    }
    ret_val += CountLocals(node->u.child[3]);
    return ret_val;
}
    
/* code for "definitions" production */

void	GenDefsCode(node)
    ParseNode	*node;
{
    if (node->prodnum == 5) {
	return;
    }
    GenDefCode(node->u.child[0]);
    GenDefsCode(node->u.child[1]);
}

/* generate code for the internals of a function */

int	GenFuncRestCode(node, out_file)
    ParseNode	*node;
    FILE	*out_file;
/*
* assign offsets to parameters and locals, and recursively
* generate code for statements.  Return number of temporaries used.
*/
{
    SymEntry	sym;
    ParseNode	*cur_node;
    int		num_temps;
    int		num_params;
    extern int	SymGet(), SymUpdate();

    cur_block_number = ++blockcount;
    num_temps = 0;
    num_params = 0;
    if (node->u.child[0]->prodnum != 15) { /* there are parameters */
	/* select offsets for parameters */
	bcopy(&node->u.child[0]->u.child[0]->u.id.idname[0],
	      &sym.id.idname[0],
	      MAXSYM);
	sym.id.idlen = node->u.child[0]->u.child[0]->u.id.idlen;
	sym.scope = cur_block_number;
	SymGet(&sym);
	sym.offset = num_params++;
	SymUpdate(&sym);
	cur_node = node->u.child[0]->u.child[1];
	while (cur_node->prodnum != 8) {
	    bcopy(&cur_node->u.child[0]->u.child[0]->u.id.idname[0],
		  &sym.id.idname[0],
		  MAXSYM);
	    sym.id.idlen = cur_node->u.child[0]->u.child[0]->u.id.idlen;
	    sym.scope = cur_block_number;
	    SymGet(&sym);
	    sym.offset = num_params++;
	    SymUpdate(&sym);
	    cur_node = cur_node->u.child[1];
	}
    }
    num_temps = GenFuncBodyCode(node->u.child[2], out_file);
    cur_block_number = 0;
    return num_temps;
}

/* produce code for "function body" production */

int	GenFuncBodyCode(node, out_file)
    ParseNode	*node;
    FILE	*out_file;
/*
* assign offsets to local variables and generate
* code for statements
*/
{
    DetermineLocalOffsets(node->u.child[0], 0);
    return GenStatementsCode(node->u.child[1], out_file, 0, "", "");
}

/* enter offsets of local variables into symbol table */

void	DetermineLocalOffsets(node, start_offset)
    ParseNode	*node;
    int		start_offset;
/*
* recursive descent on "variable_defs" node
*/
{
    SymEntry	sym;
    ParseNode	*cur_node;
    extern int	SymGet(), SymUpdate();

    if (node->prodnum == 17) {
	return;
    }
    bcopy(&node->u.child[1]->u.child[0]->u.id.idname[0],
	  &sym.id.idname[0],
	  MAXSYM);
    sym.id.idlen = node->u.child[1]->u.child[0]->u.id.idlen;
    sym.scope = cur_block_number;
    SymGet(&sym);
    sym.offset = start_offset++;
    SymUpdate(&sym);
    cur_node = node->u.child[2];
    while (cur_node->prodnum != 8) {
	bcopy(&cur_node->u.child[0]->u.child[0]->u.id.idname[0],
	      &sym.id.idname[0],
	      MAXSYM);
	sym.id.idlen = cur_node->u.child[0]->u.child[0]->u.id.idlen;
	sym.scope = cur_block_number;
	SymGet(&sym);
	sym.offset = start_offset++;
	SymUpdate(&sym);
	cur_node = cur_node->u.child[1];
    }
    DetermineLocalOffsets(node->u.child[3], start_offset);
}
    
/* produce code for "statements" production */

int	GenStatementsCode(node, out_file, next_temp, brklbl, contlbl)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
    char	*brklbl, *contlbl;
{
    int	    firsttemps, othertemps;

    if (node->prodnum == 28) {
	return 0;
    }
    firsttemps = GenStatementCode(node->u.child[0], 
			   out_file, next_temp, brklbl, contlbl);
    othertemps = GenStatementsCode(node->u.child[1], 
			out_file, next_temp, brklbl, contlbl);
    return ((othertemps > firsttemps) ? othertemps : firsttemps);
}

/* produce code for a single statement */

int	GenStatementCode(node, out_file, next_temp, brklbl, contlbl)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
    char	*brklbl, *contlbl;
{
    int	    numtemps, tempsused;
    char    *label1, *label2;

    if (node->prodnum == 19) {
	/* null statement */
	return 0;
    }
    if (node->prodnum == 20) {
	/* expression statement */
	return GenExprCode(node->u.child[0], out_file, next_temp);
    }
    if (node->prodnum == 21) {
	/* compound statement */
	return GenStatementsCode(node->u.child[0], 
			out_file, next_temp, brklbl, contlbl);
    }
    if (node->prodnum == 22) {
	/* if statement */
	numtemps = GenExprCode(node->u.child[0], out_file, next_temp);
	label1 = newlabel();
	fprintf(out_file, "\tJUMPE\tP%d, %s\t; line %d\n",
		next_temp, label1, node->linenum);
	tempsused = GenStatementCode(node->u.child[1], 
			out_file, next_temp, brklbl, contlbl);
	numtemps = (numtemps > tempsused) ? numtemps : tempsused;
	if (node->u.child[2]->prodnum == 29) {
	    label2 = newlabel();
	    fprintf(out_file, "\tJUMPA\t%s\t\t; line %d\n",
		label2, node->linenum);
	}
	fprintf(out_file, "%s:\n", label1);
	if (node->u.child[2]->prodnum == 29) {
	    tempsused = GenStatementCode(node->u.child[2]->u.child[0],
			    out_file, next_temp, brklbl, contlbl);
	    numtemps = (numtemps > tempsused) ? numtemps : tempsused;
	    fprintf(out_file, "%s:\n", label2);
	}
	return numtemps;
    }
    if (node->prodnum == 23) {
	/* while statement */
	label1 = newlabel();
	label2 = newlabel();
	fprintf(out_file, "%s:\n", label1);
	numtemps = GenExprCode(node->u.child[0], out_file, next_temp);
	fprintf(out_file, "\tJUMPE\tP%d, %s\t; line %d\n",
		next_temp, label2, node->linenum);
	tempsused = GenStatementCode(node->u.child[1], out_file, 
			    next_temp, label2, label1);
	numtemps = (numtemps > tempsused) ? numtemps : tempsused;
	fprintf(out_file, "\tJUMPA\t%s\t\t; line %d\n",
		label1, node->linenum);
	fprintf(out_file, "%s:\n", label2);
	return numtemps;
    }
    if (node->prodnum == 24) {
	/* break statement */
	fprintf(out_file, "\tJUMPA\t%s\t\t; line %d\n",
		brklbl, node->linenum);
	return 0;
    }
    if (node->prodnum == 25) {
	/* continue statement */
	fprintf(out_file, "\tJUMPA\t%s\t\t; line %d\n",
		contlbl, node->linenum);
	return 0;
    }
    /* otherwise, is a return statement */
    if (node->u.child[0]->prodnum == 32) {
	fprintf(out_file, "\tJUMPA\t%s\t\t; line %d\n", 
		ret_label, node->linenum);
	return 0;
    }
    numtemps = GenExprCode(node->u.child[0]->u.child[0], out_file, next_temp);
    fprintf(out_file, "\tMOVE\tVREG,P%d\n", next_temp);
    fprintf(out_file, "\tJUMPA\t%s\t\t; line %d\n", 
	ret_label, node->linenum);
    return numtemps;
}

/* generate a new label for the assembler */

char	*newlabel()
{
    char	*c;
    extern char *malloc();

    c = malloc(6);
    sprintf(c, "lbl%d", cur_label++);
    return c;
}

/* generate code to evaluate an expression */

int	GenExprCode(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
/*
* recursively produces code to evaluate an expression and place the
* value in the temporary variable next_temp.  Returns the number
* of temporaries used, including next_temp.
*/
{
    SymEntry	sym;
    int		numtemps;
    extern int	SymGet();

    if (node->u.child[1]->prodnum == 34) {
	/* an assignment statement */
	bcopy(&node->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.id.idname[0],
	      &sym.id.idname[0], MAXSYM);
	sym.id.idlen = node->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.id.idlen;
	sym.scope = cur_block_number;
	if (!SymGet(&sym)) {
	    sym.scope = 0;
	    SymGet(&sym);
	}
        numtemps = GenExprCode(node->u.child[1]->u.child[0], out_file, next_temp);
	fprintf(out_file, "\tMOVEM\tP%d,", next_temp);
	if (!SymGet(&sym)) {
	    sym.scope = 0;
	    SymGet(&sym);
	}
	if (sym.scope == 0) {
	    IdPrint(&sym.id, out_file);
	}
	else {
	    VarPrint(sym.stype, sym.offset, sym.scope, out_file);
	}
	fprintf(out_file, "\t; line %d, var ", node->linenum);
	IdPrint(&sym.id, out_file);
	fprintf(out_file, "\n");
	return numtemps;
    }
    /* not assignment, evaluate relexpr */
    numtemps = GenRelExprCode(node->u.child[0], out_file, next_temp);
    return numtemps;
}

/* generate code for relational expressions */

int	GenRelExprCode(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
{
    int	    numtemps;
    int	    temp2;
    char    *label1, *label2;

    if (node->u.child[1]->prodnum == 37) {
	/* arithexpr relop arithexpr */
	numtemps = GenArithExprCode(node->u.child[0], out_file, next_temp);
	temp2 = next_temp + numtemps;
	numtemps += GenArithExprCode(node->u.child[1]->u.child[1],
				     out_file, temp2);
	fprintf(out_file, "\tMOVE\tR0,P%d\n", next_temp);
	if (node->etype == etypeFloat) {
	    fprintf(out_file, "\tFSB\t");
	}
	else {
	    fprintf(out_file, "\tSUB\t");
	}
	fprintf(out_file, "R0,P%d\n", temp2);
	fprintf(out_file, "\tJUMP");
	switch (node->u.child[1]->u.child[0]->u.r) {
	    case opeq:  fprintf(out_file, "E\t");
			break;
	    case opne:  fprintf(out_file, "N\t");
			break;
	    case opgt:  fprintf(out_file, "G\t");
			break;
	    case opge:  fprintf(out_file, "GE\t");
			break;
	    case oplt:  fprintf(out_file, "L\t");
			break;
	    default:	fprintf(out_file, "LE\t");
	}
	label1 = newlabel();
	label2 = newlabel();
	fprintf(out_file, "R0,%s\t; line %d\n",
		label1, node->linenum);
	fprintf(out_file, "\tMOVEI\tP%d,0\n", next_temp);
	fprintf(out_file, "\tJUMPA\t%s\n", label2);
	fprintf(out_file, "%s:\n", label1);
	fprintf(out_file, "\tMOVEI\tP%d,1\n", next_temp);
	fprintf(out_file, "%s:\n", label2);
	return numtemps;
    }
    /* no relexpr, just an arithexpr */
    numtemps = GenArithExprCode(node->u.child[0], out_file, next_temp);
    return numtemps;
}

/* code for an arithmetic expression */

int	GenArithExprCode(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
{
    int	    numtemps;
    int	    temp2;

    numtemps = GenArithExpr2Code(node->u.child[1], out_file, next_temp);
    if (numtemps == 0) {
	return GenTermCode(node->u.child[0], out_file, next_temp);
    }
    temp2 = next_temp + numtemps;
    numtemps += GenTermCode(node->u.child[0], out_file, temp2);
    if (node->etype == etypeFloat) {
	if (node->u.child[1]->u.child[0]->u.a == opplus) {
	    fprintf(out_file, "\tFADM\t");
	}
	else {
	    fprintf(out_file, "\tFSBM\t");
	}
    }
    else {
	if (node->u.child[1]->u.child[0]->u.a == opplus) {
	    fprintf(out_file, "\tADDM\t");
	}
	else {
	    fprintf(out_file, "\tSUBM\t");
	}
    }
    fprintf(out_file, "P%d, P%d\t\t; line %d\n",
	temp2, next_temp, node->linenum);
    return numtemps;
}

/* arithexpr2 code generation */

int	GenArithExpr2Code(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
{
    int	    numtemps;
    int	    temp2;

    if (node->prodnum == 41) {
	return 0;
    }
    numtemps = GenArithExpr2Code(node->u.child[2], out_file, next_temp);
    if (numtemps == 0) {
	return GenTermCode(node->u.child[1], out_file, next_temp);
    }
    temp2 = next_temp + numtemps;
    numtemps += GenTermCode(node->u.child[1], out_file, temp2);
    if (node->etype == etypeFloat) {
	if (node->u.child[2]->u.child[0]->u.a == opplus) {
	    fprintf(out_file, "\tFADM\t");
	}
	else {
	    fprintf(out_file, "\tFSBM\t");
	}
    }
    else {
	if (node->u.child[2]->u.child[0]->u.a == opplus) {
	    fprintf(out_file, "\tADDM\t");
	}
	else {
	    fprintf(out_file, "\tSUBM\t");
	}
    }
    fprintf(out_file, "P%d, P%d\t\t; line %d\n",
	temp2, next_temp, node->linenum);
    return numtemps;
}

void	VarPrint(stype, offset, scope, out_file)
    SymType	stype;
    int		offset;
    int		scope;
    FILE	*out_file;
/*
* print expression representing the current location of the variable
*/
{
    /* globals taken care of elsewhere */
    if (stype == symVar) {
	fprintf(out_file, "%d(FP)", offset);
    }
    else {
	fprintf(out_file, "%d(AP)", offset);
    }
}

void	IdPrint(id, out_file)
    SymName	*id;
    FILE	*out_file;
/*
* print a symbol's name
*/
{
    int i;

    for (i = 0; i < id->idlen; i++) {
	if (id->idname[i] == '_') {
	    fprintf(out_file, "%%");
	    continue;
	}
	fprintf(out_file, "%c", id->idname[i]);
    }
}

/* generate code to evaluate a term of an expression */

int	GenTermCode(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
{
    int	    numtemps;
    int	    temp2;

    numtemps = GenTerm2Code(node->u.child[1], out_file, next_temp);
    if (numtemps == 0) {
	return GenFactorCode(node->u.child[0], out_file, next_temp);
    }
    temp2 = next_temp + numtemps;
    numtemps += GenFactorCode(node->u.child[0], out_file, temp2);
    if (node->etype == etypeFloat) {
	if (node->u.child[1]->u.child[0]->u.m == optimes) {
	    fprintf(out_file, "\tFMPM\t");
	}
	else {
	    fprintf(out_file, "\tFDVM\t");
	}
    }
    else {
	if (node->u.child[1]->u.child[0]->u.m == optimes) {
	    fprintf(out_file, "\tIMULM\t");
	}
	else if (node->u.child[1]->u.child[0]->u.m == opdiv) {
	    fprintf(out_file, "\tMOVE\tR0,P%d\n", temp2);
	    fprintf(out_file, "\tIDIV\tR0,P%d\n", next_temp);
	    fprintf(out_file, "\tMOVE\tP%d,R0\n", next_temp);
	}
	else {
	    fprintf(out_file, "\tMOVE\tR0,P%d\n", temp2);
	    fprintf(out_file, "\tIDIV\tR0,P%d\n", next_temp);
	    fprintf(out_file, "\tMOVE\tP%d,R1\n", next_temp);
	}
    }
    if ((node->u.child[1]->u.child[0]->u.m == optimes) || (node->etype == etypeFloat)) {
	fprintf(out_file, "P%d, P%d\t\t; line %d\n",
	    temp2, next_temp, node->linenum);
    }
    return numtemps;
}

/* term2 code generation */

int	GenTerm2Code(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
{
    int	    numtemps;
    int	    temp2;

    if (node->prodnum == 44) {
	return 0;
    }
    numtemps = GenTerm2Code(node->u.child[2], out_file, next_temp);
    if (numtemps == 0) {
	return GenFactorCode(node->u.child[1], out_file, next_temp);
    }
    temp2 = next_temp + numtemps;
    numtemps += GenFactorCode(node->u.child[1], out_file, temp2);
    if (node->etype == etypeFloat) {
	if (node->u.child[2]->u.child[0]->u.m == optimes) {
	    fprintf(out_file, "\tFMPM\t");
	}
	else {
	    fprintf(out_file, "\tFDVM\t");
	}
    }
    else {
	if (node->u.child[2]->u.child[0]->u.m == optimes) {
	    fprintf(out_file, "\tIMULM\t");
	}
	else if (node->u.child[2]->u.child[0]->u.m == opdiv) {
	    fprintf(out_file, "\tMOVE\tR0,P%d\n", temp2);
	    fprintf(out_file, "\tIDIV\tR0,P%d\n", next_temp);
	    fprintf(out_file, "\tMOVE\tP%d,R0\n", next_temp);
	}
	else {
	    fprintf(out_file, "\tMOVE\tR0,P%d\n", temp2);
	    fprintf(out_file, "\tIDIV\tR0,P%d\n", next_temp);
	    fprintf(out_file, "\tMOVE\tP%d,R1\n", next_temp);
	}
    }

    if ((node->u.child[2]->u.child[0]->u.m == optimes) || (node->etype == etypeFloat)) {
	fprintf(out_file, "P%d, P%d\t\t; line %d\n",
	    temp2, next_temp, node->linenum);
    }
    return numtemps;
}

/* generate code for factors */

int	GenFactorCode(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
{
    int	    	numtemps;
    int	    	temp2;
    SymEntry	sym;

    if (node->prodnum == 45) {
	if (node->u.child[1]->prodnum == 50) {
	    return GenFunCallCode(node, out_file, next_temp);
	}
	fprintf(out_file, "\tMOVE\tP%d,", next_temp);
	bcopy(&node->u.child[0]->u.id.idname[0],
	      &sym.id.idname[0],
	      MAXSYM);
	sym.id.idlen = node->u.child[0]->u.id.idlen;
	sym.scope = cur_block_number;
	if (!SymGet(&sym)) {
	    sym.scope = 0;
	    SymGet(&sym);
	}
	if (sym.scope == 0) {
	    IdPrint(&sym.id, out_file);
	}
	else {
	    VarPrint(sym.stype, sym.offset, sym.scope, out_file);
	}
	fprintf(out_file, "\t; line %d var ",node->linenum);
	IdPrint(&sym.id, out_file);
	fprintf(out_file, "\n");
	return 1;
    }
    if (node->prodnum == 46) {
	fprintf(out_file, "\tMOVEI\tP%d,%d", next_temp, 
		node->u.child[0]->u.i);
	fprintf(out_file, "\t\t; line %d \n ", node->linenum);
	return 1;
    }
    if (node->prodnum == 47) {
	fprintf(out_file, "\tMOVEI\tP%d,%f", next_temp, 
		node->u.child[0]->u.f);
	fprintf(out_file, "\t\t; line %d \n ", node->linenum);
	return 1;
    }
    /* must be a parenthesized expression - strings are not handled here */
    return GenExprCode(node->u.child[0], out_file, next_temp);
}

/* code for function call */

int	GenFunCallCode(node, out_file, next_temp)
    ParseNode	*node;
    FILE	*out_file;
    int		next_temp;
{
    int		last_param_temp;
    int		numtemps, tempsused;
    int		cur_temp;
    int		ret_temp;
    int		arg_count;
    ParseNode	*cur_node;

    cur_node = node->u.child[1]->u.child[0];
    arg_count = CountParams(cur_node);
    numtemps = 0;
    fprintf(out_file, "\tPUSH\tSP,[%d]\n", arg_count);
    while ((cur_node->prodnum != 53) && (cur_node->prodnum != 55)) {
	if (cur_node->u.child[0]->etype == etypeString) {
	    /* strings are special */
	    cur_node->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.s.txt[
		cur_node->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.s.slen] = '\0';
	    fprintf(out_file, "\tMOVEI\tR0,[ASCIZ \"%s\"]\n",
		&cur_node->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.s.txt[0]);
	    fprintf(out_file, "\tPUSH\tSP,R0\n");
	    cur_node = cur_node->u.child[1];
	    continue;
	}
	tempsused = GenExprCode(cur_node->u.child[0], 
		    		out_file, next_temp);
	numtemps = (numtemps > tempsused) ? numtemps : tempsused;
	fprintf(out_file, "\tPUSH\tSP,P%d\n", next_temp);

	cur_node = cur_node->u.child[1];
    }
    fprintf(out_file, "\tPUSH\tSP,AP\n");
    fprintf(out_file, "\tMOVEI\tAP,-%d(SP)\n", arg_count);
    cur_node = node->u.child[1]->u.child[0];
    fprintf(out_file, "\tPUSHJ\tSP,");
    IdPrint(&node->u.child[0]->u.id, out_file);
    fprintf(out_file, "\n");
    fprintf(out_file, "\tPOP\tSP,AP\n");
    fprintf(out_file, "\tADJSP\tSP,-<%d>\n", arg_count + 1);
    fprintf(out_file, "\tMOVE\tP%d,VREG\n", next_temp);
    return numtemps;
}

int	CountParams(node)
    ParseNode	*node;
{
    if ((node->prodnum == 53) || (node->prodnum == 55)) {
	return 0;
    }
    return 1 + CountParams(node->u.child[1]);
}

