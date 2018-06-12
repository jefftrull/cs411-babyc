/*****************************************************************\
*
*            semantics.c - semantic analysis for BabyC compiler
*
\*****************************************************************/


/*
* HISTORY
*    21-Mar-88  Jeffrey Trull (jt1j) at Carnegie-Mellon University
*        Created
*/


/*
* Include Files
*/

#include    "lexer.h"
#include    "ptree.h"
#include    "symtab.h"
#include    <stdio.h>

/*
* Procedure Predeclarations
*/

void    InitSemantics();
void    TreeSemantics();
void    ProgramSemantics();
void    DefinitionSemantics();
void    VarListSemantics();
void    FuncRestSemantics();
void    VariableDefsSemantics();
void    CheckDeclaredVars();
void    FunctionBodySemantics();
void	LocalVarsSemantics();
void    StatementsSemantics();
void	StatementSemantics();
void	ExpressionSemantics();
void	RelExprSemantics();
void	Expr2Semantics();
void	ArithExprSemantics();
void	ArithExpr2Semantics();
void	TermSemantics();
void	Term2Semantics();
void	FactorSemantics();
void	CheckParams();
void    idcopy();

/*
* Data Declarations
*/

short   	found_errors;
int     	prevailingscope;
int     	scopecount;
SymEntry 	*undeclared_functions[1000];
int		next_function;

/*
* Routines
*/

void    InitSemantics()
{
    extern void    SymInit();

    found_errors = 0;
    prevailingscope = 0;
    scopecount = 0;
    next_function = 0;
    SymInit();
}

/* do type checking, build symbol table */

void    TreeSemantics()
{
    extern ParseNode    *ParseTree;
    SymEntry		sym1, sym2, sym3, sym4;

    /* put in symbol table entries for predefined routines */
    bcopy("pr_int", &sym1.id.idname[0], 7);
    sym1.id.idlen = 6;
    sym1.stype = symFunc;
    sym1.dtype = dtypeVoid;
    sym1.linenum = 0; /* indicates predefined */
    sym1.scope = 0;
    SymPut(&sym1);
    bcopy("pr_flo", &sym2.id.idname[0], 7);
    sym2.id.idlen = 6;
    sym2.stype = symFunc;
    sym2.dtype = dtypeVoid;
    sym2.scope = 0;
    sym2.linenum = 0;
    SymPut(&sym2);
    bcopy("pr_str", &sym3.id.idname[0], 7);
    sym3.id.idlen = 6;
    sym3.stype = symFunc;
    sym3.scope = 0;
    sym3.linenum = 0;
    sym3.dtype = dtypeVoid;
    SymPut(&sym3);
    bcopy("pr_nl", &sym4.id.idname[0], 6);
    sym4.id.idlen = 5;
    sym4.stype = symFunc;
    sym4.dtype = dtypeVoid;
    sym4.scope = 0;
    sym4.dtype = dtypeVoid;
    SymPut(&sym4);

    ProgramSemantics(ParseTree);
    fflush(stdout);
}

/* build symbol table from parse tree starting at node */

void    ProgramSemantics(node)
    ParseNode    *node;
{
    SymEntry     symbol;
    ParseNode    *cur_node;
    int		 cur_function;

    DefinitionSemantics(node->u.child[0]);
    if (found_errors) {
        return;
    }
    cur_node = node->u.child[1];
    while (cur_node->prodnum != 5) {
        DefinitionSemantics(cur_node->u.child[0]);
        if (found_errors) {
            return;
        }
	cur_node = cur_node->u.child[1];
    }
    for (cur_function = 0; cur_function < next_function; cur_function++) {
	if (undeclared_functions[cur_function]->dtype == dtypeUndecl) {
	    printf("? end of program reached with undefined functions\n");
	    return;
	}
    }
}

/* Perform Semantic Analysis on Definition Production */

void    DefinitionSemantics(node)
    ParseNode    *node;
{
    SymEntry    *newentry;
    SymEntry	tempentry;
    extern int  SymPut();

    if (node->prodnum == 2) {
        newentry = (SymEntry *)malloc(sizeof(SymEntry));
	idcopy(&node->u.child[0]->u.child[0]->u.id, &newentry->id);
	newentry->stype = symFunc;
	newentry->dtype = dtypeVoid;
	newentry->linenum = node->linenum;
	newentry->scope = 0;
	node->u.child[0]->u.child[0]->u.id.idscope = 0;
	if (!SymPut(newentry)) {
            newentry->id.idname[newentry->id.idlen] = '\0';
            printf("? symbol redefined : %s in line %d\n", 
		   &newentry->id.idname[0],
		   node->linenum);
            found_errors = 1;
	    return;
        }
	FuncRestSemantics(node->u.child[1]);
        return;
    }
    if (node->u.child[2]->prodnum == 11) {
        /* this is a function definition */
        newentry = (SymEntry *)malloc(sizeof(SymEntry));
	idcopy(&node->u.child[1]->u.child[0]->u.id, &newentry->id);
	newentry->stype = symFunc;
	if (node->u.child[0]->prodnum == 9) {
            newentry->dtype = dtypeInt;
        }
	else {
            newentry->dtype = dtypeFloat;
        }
	newentry->linenum = node->linenum;
	newentry->scope = 0;
	node->u.child[1]->u.child[0]->u.id.idscope = 0;
	if (!SymPut(newentry)) {
	    if (newentry->dtype == dtypeInt) {
		bcopy(newentry, &tempentry, sizeof(SymEntry));
		SymGet(&tempentry);
		if ((tempentry.stype == symFunc) && (tempentry.dtype == dtypeUndecl)) {
		    SymUpdate(newentry);
		    FuncRestSemantics(node->u.child[2]->u.child[0]);
		    return;
		}
	    }
            found_errors = 1;
            newentry->id.idname[newentry->id.idlen] = '\0';
            printf("? symbol redefined : %s in line %d\n", 
		   &newentry->id.idname[0],
		   node->linenum);
	    return;
        }
	FuncRestSemantics(node->u.child[2]->u.child[0]);
        return;
    }
    /* otherwise, this is a global data declaration */
    newentry = (SymEntry *)malloc(sizeof(SymEntry));
    idcopy(&(node->u.child[1]->u.child[0]->u.id), &newentry->id);
    newentry->stype = symVar;
    if (node->u.child[0]->prodnum == 9) {
        newentry->dtype = dtypeInt;
    }
    else {
        newentry->dtype = dtypeFloat;
    }
    newentry->linenum = node->linenum;
    newentry->scope = 0;
    node->u.child[1]->u.child[0]->u.id.idscope = 0;
    if (!SymPut(newentry)) {
        newentry->id.idname[newentry->id.idlen] = '\0';
        printf("? symbol redefined : %s in line %d\n", 
	       &newentry->id.idname[0],
	       node->linenum);
        found_errors = 1;
	return;
    }
    VarListSemantics(node->u.child[2]->u.child[0], newentry->dtype);
}

/* semantic analysis of declarators of a given type */

void    VarListSemantics(node, dtype)
    ParseNode    *node;
    DataType     dtype;
{
    SymEntry     *newentry;

    if ((node->prodnum == 8) || (node->prodnum == 15)) {
        /* no more variables */
	return;
    }
    newentry = (SymEntry *)malloc(sizeof(SymEntry));
    idcopy(&node->u.child[0]->u.child[0]->u.id, &newentry->id);
    newentry->stype = symVar;
    newentry->dtype = dtype;
    newentry->linenum = node->linenum;
    newentry->scope = prevailingscope;
    node->u.child[0]->u.child[0]->u.id.idscope = prevailingscope;
    if (!SymPut(newentry)) {
        newentry->id.idname[newentry->id.idlen] = '\0';
        printf("? symbol redefined : %s in line %d\n", 
	       &newentry->id.idname[0],
	       node->linenum);
        found_errors = 1;
	return;
    }
    VarListSemantics(node->u.child[1], dtype);
}

void    FuncRestSemantics(node)
    ParseNode    *node;
{
    SymEntry     *newentry;

    prevailingscope = ++scopecount;
    if (node->u.child[0]->prodnum != 15) {
	newentry = (SymEntry *)malloc(sizeof(SymEntry));
	idcopy(&node->u.child[0]->u.child[0]->u.id, &newentry->id);
	newentry->stype = symVar;
	newentry->dtype = dtypeUndecl;
	newentry->linenum = node->linenum;
	newentry->scope = prevailingscope;
	node->u.child[0]->u.child[0]->u.id.idscope = prevailingscope;
	if (!SymPut(newentry)) {
	    newentry->id.idname[newentry->id.idlen] = '\0';
	    printf("? symbol redefined : %s in line %d\n", 
		   &newentry->id.idname[0],
		   node->linenum);
	    found_errors = 1;
	    return;
	}
	VarListSemantics(node->u.child[0]->u.child[1], dtypeUndecl);
	if (found_errors) {
	    return;
	}
    }
    VariableDefsSemantics(node->u.child[1]);
    if (found_errors) {
        return;
    }
    if (node->u.child[0]->prodnum != 15) {
	CheckDeclaredVars(node->u.child[0]->u.child[1]);
	if (found_errors) {
	    return;
	}
    }
    FunctionBodySemantics(node->u.child[2]);
    prevailingscope = 0;
}

void    VariableDefsSemantics(node)
    ParseNode    *node;
{
    DataType     cur_dtype;
    SymEntry     *newentry;
    ParseNode    *cur_node;

    if (node->prodnum == 17) {
        return;
    }
    if (node->u.child[0]->prodnum == 9) {
        cur_dtype = dtypeInt;
    }
    else {
        cur_dtype = dtypeFloat;
    }
    newentry = (SymEntry *)malloc(sizeof(SymEntry));
    idcopy(node->u.child[1]->u.child[0]->u.id.idname, &newentry->id.idname[0]);
    newentry->stype = symParam;
    newentry->dtype = cur_dtype;
    newentry->linenum = node->u.child[1]->linenum;
    newentry->scope = prevailingscope;
    if (!SymUpdate(newentry)) {
        newentry->id.idname[newentry->id.idlen] = '\0';
        printf("? unknown formal parameter : %s in line %d\n", 
	       &newentry->id.idname[0],
	       newentry->linenum);
        found_errors = 1;
	return;
    }
    cur_node = node->u.child[2];
    while (cur_node->prodnum != 8) {
	newentry = (SymEntry *)malloc(sizeof(SymEntry));
        idcopy(&cur_node->u.child[0]->u.child[0]->u.id.idname[0], &newentry->id.idname[0]);
        newentry->stype = symParam;
        newentry->dtype = cur_dtype;
        newentry->linenum = node->u.child[0]->linenum;
        newentry->scope = prevailingscope;
        if (!SymUpdate(newentry)) {
            newentry->id.idname[newentry->id.idlen] = '\0';
            printf("? unknown formal parameter : %s in line %d\n", 
		   &newentry->id.idname[0],
		   newentry->linenum);
            found_errors = 1;
	    return;
        }
        cur_node = cur_node->u.child[1];
    }
    VariableDefsSemantics(node->u.child[3]);
}

void    CheckDeclaredVars(node)
    ParseNode    *node;
{
    SymEntry     oldentry;

    if ((node->prodnum == 15) || (node->prodnum == 8)) {
        return;
    }
    oldentry.scope = prevailingscope;
    idcopy(&node->u.child[0]->u.child[0]->u.id.idname[0], &oldentry.id.idname[0]);
    SymGet(&oldentry);
    if (oldentry.dtype == dtypeUndecl) {
        oldentry.id.idname[oldentry.id.idlen] = '\0';
        printf("? formal parameter %s on line %d was not declared\n", 
		&oldentry.id.idname[0],
		oldentry.linenum);
	found_errors = 1;
	return;
    }
    CheckDeclaredVars(node->u.child[1]);
}

void    FunctionBodySemantics(node)
    ParseNode    *node;
{
    LocalVarsSemantics(node->u.child[0]);
    if (found_errors) {
        return;
    }
    StatementsSemantics(node->u.child[1]);
}

void	LocalVarsSemantics(node)
    ParseNode	*node;
{
    DataType	cur_dtype;
    SymEntry	*newentry;
    ParseNode	*cur_node;

    if (node->prodnum == 17) {
	return;
    }
    if (node->u.child[0]->prodnum == 9) {
	cur_dtype = dtypeInt;
    }
    else {
	cur_dtype = dtypeFloat;
    }
    newentry = (SymEntry *)malloc(sizeof(SymEntry));
    idcopy(node->u.child[1]->u.child[0]->u.id.idname, &newentry->id.idname[0]);
    newentry->stype = symVar;
    newentry->dtype = cur_dtype;
    newentry->linenum = node->u.child[1]->linenum;
    newentry->scope = prevailingscope;
    node->u.child[1]->u.child[0]->u.id.idscope = prevailingscope;
    if (!SymPut(newentry)) {
        newentry->id.idname[newentry->id.idlen] = '\0';
        printf("? Redefined local variable : %s line %d\n", 
		&newentry->id.idname[0],
		newentry->linenum);
        found_errors = 1;
	return;
    }
    cur_node = node->u.child[2];
    while (cur_node->prodnum != 8) {
	newentry = (SymEntry *)malloc(sizeof(SymEntry));
        idcopy(&cur_node->u.child[0]->u.child[0]->u.id.idname[0], &newentry->id.idname[0]);
        newentry->stype = symVar;
        newentry->dtype = cur_dtype;
        newentry->linenum = node->u.child[0]->linenum;
        newentry->scope = prevailingscope;
        if (!SymPut(newentry)) {
	    cur_node->u.child[0]->u.child[0]->u.id.idscope = prevailingscope;
            newentry->id.idname[newentry->id.idlen] = '\0';
            printf("? Redefined local variable : %s line %d\n", 
		   &newentry->id.idname[0],
		   newentry->linenum);
            found_errors = 1;
	    return;
        }
        cur_node = cur_node->u.child[1];
    }
    LocalVarsSemantics(node->u.child[3]);
}

    
void    StatementsSemantics(node)
    ParseNode	*node;
{
    if (node->prodnum == 28) {
	return;
    }
    StatementSemantics(node->u.child[0]);
    if (found_errors) {
	return;
    }
    StatementsSemantics(node->u.child[1]);
}

void	StatementSemantics(node)
    ParseNode	*node;
{
    if (node->prodnum == 20) {
	ExpressionSemantics(node->u.child[0]);
	if (node->u.child[0]->etype == etypeString) {
	    printf("? string cannot appear as statement, line %d\n", 
		   node->linenum);
	    found_errors = 1;
	    return;
	}
	return;
    }
    if (node->prodnum == 21) {
	StatementsSemantics(node->u.child[0]);
	return;
    }
    if (node->prodnum == 22) {
	ExpressionSemantics(node->u.child[0]);
	if (node->u.child[0]->etype == etypeString) {
	    printf("? string cannot appear as conditional, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
	StatementSemantics(node->u.child[1]);
	if (node->u.child[2]->prodnum == 29) {
	    StatementSemantics(node->u.child[2]->u.child[0]);
	}
	return;
    }
    if (node->prodnum == 23) {
	ExpressionSemantics(node->u.child[0]);
	if (node->u.child[0]->etype == etypeString) {
	    printf("? string cannot appear as conditional, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
	StatementSemantics(node->u.child[1]);
    }
    if ((node->prodnum == 26) && (node->u.child[0]->prodnum == 31)) {
	ExpressionSemantics(node->u.child[0]->u.child[0]);
	if (node->u.child[0]->u.child[0]->etype == etypeString) {
	    printf("? string cannot appear as return value, line %d\n",
		   node->u.child[0]->linenum);
	    found_errors = 1;
	    return;
	}
    }
}

void	ExpressionSemantics(node)
    ParseNode	*node;
{
    if (node->etype != etypeUndecl) {
	return;
    }
    RelExprSemantics(node->u.child[0]);
    if (found_errors) {
	return;
    }
    Expr2Semantics(node->u.child[1]);
    if (found_errors) {
	return;
    }
    if (node->u.child[1]->prodnum == 34) {
	if (node->u.child[0]->u.child[0]->u.child[0]->u.child[0]->prodnum != 45) {
	    printf("? illegal left hand side of assignment, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
	if (node->u.child[0]->u.child[0]->u.child[0]->u.child[0]->u.child[1]->prodnum != 51) {
	    printf("? illegal left hand side of assignment, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
    }
    if (node->u.child[1]->prodnum == 35) {
	node->etype = node->u.child[0]->etype;
	return;
    }
    if (node->u.child[1]->etype != node->u.child[0]->etype) {
	printf("? incompatible types in expression, line %d\n",
		node->linenum);
	found_errors = 1;
	return;
    }
}

void	Expr2Semantics(node)
    ParseNode	*node;
{
    if (node->prodnum != 35) {
	ExpressionSemantics(node->u.child[0]);
	node->etype = node->u.child[0]->etype;
    }
}

void	RelExprSemantics(node)
    ParseNode	*node;
{
    if (node->etype != etypeUndecl) {
	return;
    }
    ArithExprSemantics(node->u.child[0]);
    if (found_errors) {
	return;
    }
    if (node->u.child[1]->prodnum == 37) {
	ArithExprSemantics(node->u.child[1]->u.child[1]);
	if (found_errors) {
	    return;
	}
	if (node->u.child[0]->etype != node->u.child[1]->u.child[1]->etype) {
	    printf("? incompatible types in relational expression, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
    }
    node->etype = node->u.child[0]->etype;
}
	
void	ArithExprSemantics(node)
    ParseNode	*node;
{
    if (node->etype != etypeUndecl) {
	return;
    }
    TermSemantics(node->u.child[0]);
    if (found_errors) {
	return;
    }
    ArithExpr2Semantics(node->u.child[1]);
    if (found_errors) {
	return;
    }
    if (node->u.child[1]->prodnum != 41) {
	if (node->u.child[0]->etype != node->u.child[1]->etype) {
	    printf("? incompatible types in arithmetic expression, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
    }
    node->etype = node->u.child[0]->etype;
}

void	ArithExpr2Semantics(node)
    ParseNode	*node;
{
    if (node->prodnum == 41) {
	return;
    }
    TermSemantics(node->u.child[1]);
    if (found_errors) {
	return;
    }
    ArithExpr2Semantics(node->u.child[2]);
    if (found_errors) {
	return;
    }
    if (node->u.child[2]->prodnum != 41) {
	if (node->u.child[2]->etype != node->u.child[1]->etype) {
	    printf("? incompatible types in arithmetic expression, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
    }
    node->etype = node->u.child[1]->etype;
}

void	TermSemantics(node)
    ParseNode	*node;
{
    if (node->etype != etypeUndecl) {
	return;
    }
    FactorSemantics(node->u.child[0]);
    if (found_errors) {
	return;
    }
    Term2Semantics(node->u.child[1]);
    if (found_errors) {
	return;
    }
    if (node->u.child[1]->prodnum != 44) {
	if (node->u.child[1]->etype != node->u.child[0]->etype) {
	    printf("? incompatible types in arithmetic expression, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
    }
    node->etype = node->u.child[0]->etype;
}

void	Term2Semantics(node)
    ParseNode	*node;
{
    if (node->prodnum == 44) {
	return;
    }
    FactorSemantics(node->u.child[1]);
    if (found_errors) {
	return;
    }
    Term2Semantics(node->u.child[2]);
    if (found_errors) {
	return;
    }
    if (node->u.child[2]->prodnum != 44) {
	if (node->u.child[1]->etype != node->u.child[2]->etype) {
	    printf("? incompatible types in arithmetic expression, line %d\n",
		   node->linenum);
	    found_errors = 1;
	    return;
	}
    }
    node->etype = node->u.child[1]->etype;
}

void	FactorSemantics(node)
    ParseNode	*node;
{
    SymEntry	*oldentry;

    oldentry = (SymEntry *)malloc(sizeof(SymEntry));
    if (node->prodnum == 45) {
	idcopy(&node->u.child[0]->u.id, &oldentry->id);
	oldentry->scope = prevailingscope;
	if (!SymGet(oldentry)) {
	    oldentry->scope = 0;
	    if (!SymGet(oldentry)) {
		if (node->u.child[1]->prodnum == 50) {
		    /* function call */
		    oldentry->stype = symFunc;
		    oldentry->dtype = dtypeUndecl;
		    oldentry->scope = 0;
		    SymPut(oldentry);
		    node->etype = etypeInt;
		    undeclared_functions[next_function++] = oldentry;
	    	}
		else {
		    printf("? undeclared variable, line %d \n", node->linenum);
	    	    found_errors = 1;
	    	    return;
		}
	    }
	}
	if (oldentry->stype == symFunc) {
	    if (node->u.child[1]->prodnum != 50) {
		printf("? function used without parameters in line %d\n",
		    node->linenum);
		found_errors = 1;
		return;
	    }
	    CheckParams(node->u.child[1]);
	    if (found_errors) {
		return;
	    }
	}
	else {
	    if (node->u.child[1]->prodnum == 50) {
		printf("? call made to non-function symbol in line %d\n",
		    node->linenum);
		found_errors = 1;
		return;
	    }
	}
	if (oldentry->dtype == dtypeUndecl) {
	    node->etype = etypeInt;
	    return;
	}
	if (oldentry->dtype == dtypeInt) {
	    node->etype = etypeInt;
	    return;
	}
	if (oldentry->dtype == dtypeFloat) {
	    node->etype = etypeFloat;
	    return;
	}
    }
    if (node->prodnum == 46) {
	node->etype = etypeInt;
	return;
    }
    if (node->prodnum == 47) {
	node->etype = etypeFloat;
	return;
    }
    if (node->prodnum == 48) {
	node->etype = etypeString;
	found_errors = 1;
	return;
    }
    /* must be a parenthesized expression */
    ExpressionSemantics(node->u.child[0]);
    if (found_errors) {
	return;
    }
    node->etype = node->u.child[0]->etype;
}

void	CheckParams(node)
    ParseNode	*node;
{
    if (node->prodnum == 50) {
	CheckParams(node->u.child[0]);
	return;
    }
    if (node->prodnum == 51) {
	return;
    }
    if ((node->prodnum == 53) || (node->prodnum == 55)) {
	return;
    }
    ExpressionSemantics(node->u.child[0]);
    if (found_errors) {
	return;
    }
    CheckParams(node->u.child[1]);
}

void    idcopy(nodeid, entryid)
    struct {
        char idname[MAXLEN];
	int idlen, idscope;
    } *nodeid;
    SymName    *entryid;
{
    bcopy(nodeid->idname, entryid->idname, nodeid->idlen);
    entryid->idlen = nodeid->idlen;
}
