/**************************************************************************\
*
*              parser.c - routines to parse BabyC programs
*
\**************************************************************************/

/*
* HISTORY
*    19-Mar-88  Jeffrey Trull (jt1j) at Carnegie-Mellon University
*        Modified to create parse tree
*
*    24-Feb-88  Jeffrey Trull (jt1j) at Carnegie-Mellon University
*        Created
*/

/*
* Include Files
*/

#include    <stdio.h>
#include    <sys/types.h>
#include    "lexer.h"
#include    "ptree.h"

/*
* Procedure Predeclarations
*/

void        InitParser();
void        CopyToken();
void        PushBackToken();
void        GetToken();
void        ParseProgram();
ParseNode   *ParseDefinition();
ParseNode   *ParseDefinitions();
ParseNode   *ParseDeclarator();
ParseNode   *ParseDeclarators();
ParseNode   *ParseType();
ParseNode   *ParseDefinitionRest();
ParseNode   *ParseFunctionRest();
ParseNode   *ParseIDList();
ParseNode   *ParseVariableDefs();
ParseNode   *ParseFunctionBody();
ParseNode   *ParseStatement();
ParseNode   *ParseStatements();
ParseNode   *ParseOptionalElse();
ParseNode   *ParseOptionalExpr();
ParseNode   *ParseExpression();
ParseNode   *ParseExpr2();
ParseNode   *ParseRelExpr();
ParseNode   *ParseRelExpr2();
ParseNode   *ParseArithExpr();
ParseNode   *ParseArithExpr2();
ParseNode   *ParseTerm();
ParseNode   *ParseTerm2();
ParseNode   *ParseFactor();
ParseNode   *ParseOptionalParams();
ParseNode   *ParseArgumentList();
ParseNode   *ParseArgumentRest();

/*
* Data Declarations
*/

u_short        saved_token;        /* have we saved a token for later? */
token          saved_token_value;  /* value of saved token, if any */
ParseNode      *ParseTree;

/*
* Routines
*/

void        InitParser()
{
    extern ParseNode *ParseTree;
    extern void    InitLexer();

    saved_token = 0;
    ParseTree = NULL;
    InitLexer();
}

void        CopyToken(srctokptr, dsttokptr)
    token        *srctokptr, *dsttokptr;
{
    /* do fast memory move (note : this is portable) */
    bcopy(srctokptr, dsttokptr, sizeof(token));
}

void        PushBackToken(tokptr)
    token         *tokptr;
{
    extern void    PrintToken();

    CopyToken(tokptr, &saved_token_value);
    saved_token = 1;
}

void        GetToken(tokptr)
    token         *tokptr;
{
    extern void    PrintToken();
    if (saved_token) {
        /* restore from saved location */
        CopyToken(&saved_token_value, tokptr);
	saved_token = 0;
    }
    else {
        /* get it from lexer */
        GetNextToken(tokptr);
    }
}

/*
* Routines to actually do the parsing.  All of these routines have the
* same basic structure.  They each first decide which branch to follow,
* possibly using the one-token lookahead, and then they call subordinate
* routines for each nonterminal in the branch.  The terminals in each
* branch are checked to be as required in the production, and then
* are disregarded.  Each routine returns a 1 if it was successful,
* and a 0 if it was not.  The first error causes the parser to give
* up;  there is no recovery attempted.
*/

void        ParseProgram()
{
    extern ParseNode *ParseTree;

#ifdef DEBUG
    printf("(1) program = definition definitions .\n");
#endif
    ParseTree = (ParseNode *)malloc(sizeof(ParseNode));
    ParseTree->nodetype = prsProduction;
    ParseTree->u.child[0] = ParseDefinition();
    if (ParseTree->u.child[0]->nodetype == prsError) {
        ParseTree->nodetype = prsError;
	return;
    }
    ParseTree->u.child[1] = ParseDefinitions();
    if (ParseTree->u.child[1]->nodetype == prsError) {
        ParseTree->nodetype = prsError;
	return;
    }
    ParseTree->linenum = ParseTree->u.child[0]->linenum;
    ParseTree->prodnum = 1;
    ParseTree->etype = etypeVoid;
}

ParseNode    *ParseDefinition()
{
    token          newtoken;
    ParseNode      *ret_val;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    /* VOID indicates the first branch, anything else we'll try the second */
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keyvoid)) {
#ifdef DEBUG
	printf("(2) definition = VOID declarator function_rest .\n");
#endif
	ret_val->nodetype = prsProduction;
        ret_val->u.child[0] = ParseDeclarator();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	ret_val->u.child[1] = ParseFunctionRest();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
        ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->prodnum = 2;
	ret_val->etype = etypeVoid;
        return ret_val;
    }
    else {
        PushBackToken(&newtoken);
#ifdef DEBUG
        printf("(3) definition = type declarator defn_rest .\n");
#endif
	ret_val->nodetype = prsProduction;
        ret_val->u.child[0] = ParseType();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	ret_val->u.child[1] = ParseDeclarator();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
	ret_val->u.child[2] = ParseDefinitionRest();
	if (ret_val->u.child[2]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[2]->linenum;
	    return ret_val;
        }
        ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->prodnum = 3;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
}

ParseNode    *ParseDefinitions()
{
    ParseNode    *ret_val;
    token        newtoken;

    GetToken(&newtoken);
    /* only eof or another definition can follow a definition */
    if (newtoken.tktype == tokeof) {
#ifdef DEBUG
        printf("(5) definitions = .\n");
#endif
        ret_val = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->etype = etypeVoid;
	ret_val->prodnum = 5;
        return ret_val;
    }
    PushBackToken(&newtoken);
#ifdef DEBUG
    printf("(4) definitions = definition definitions .\n");
#endif
    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    ret_val->nodetype = prsProduction;
    ret_val->u.child[0] = ParseDefinition();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
        ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseDefinitions();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
        ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 4;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode*    ParseDeclarator()
{
    ParseNode    *ret_val;
    token        newtoken;

    GetToken(&newtoken);
    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    if (newtoken.tktype != tokid) {
        printf("? Identifier expected, line %d\n", newtoken.linenum);
	ret_val->nodetype = prsError;
        ret_val->linenum = newtoken.linenum;
	return ret_val;
    }
    newtoken.vs.l.toktext[newtoken.vs.l.toklen] = '\0';
#ifdef DEBUG
    printf("(6) declarator = ID(%s) .\n", newtoken.vs.l.toktext);
#endif
    ret_val->nodetype = prsProduction;
    ret_val->prodnum = 6;
    ret_val->linenum = newtoken.linenum;
    ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
    ret_val->u.child[0]->nodetype = prsID;
    ret_val->u.child[0]->linenum = newtoken.linenum;
    bcopy(&newtoken.vs.l.toktext[0], 
          &ret_val->u.child[0]->u.id.idname[0], 
          newtoken.vs.l.toklen);
    ret_val->u.child[0]->u.id.idlen = newtoken.vs.l.toklen;
    return ret_val;
}

ParseNode   *ParseDeclarators()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    /* first branch if token is a comma, second otherwise */
    if (newtoken.tktype == tokcomma) {
#ifdef DEBUG
        printf("(7) declarators = , declarator declarators .\n");
#endif
        ret_val->nodetype = prsProduction;
	ret_val->u.child[0] = ParseDeclarator();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	ret_val->u.child[1] = ParseDeclarators();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
        ret_val->prodnum = 7;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
#ifdef DEBUG
    printf("(8) declarators = .\n");
#endif
    PushBackToken(&newtoken);
    ret_val->nodetype = prsProduction;
    ret_val->linenum = newtoken.linenum;
    ret_val->prodnum = 8;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode    *ParseType()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keyfloat)) {
#ifdef DEBUG
        printf("(10) type = FLOAT .\n");
#endif
	ret_val->nodetype = prsProduction;
	ret_val->prodnum = 10;
	ret_val->linenum = newtoken.linenum;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keyint)) {
#ifdef DEBUG
        printf("(9) type = INT .\n");
#endif
        ret_val->nodetype = prsProduction;
	ret_val->prodnum = 9;
	ret_val->linenum = newtoken.linenum;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    printf("? expected INT or FLOAT, line %d\n", newtoken.linenum);
    ret_val->nodetype = prsError;
    ret_val->linenum = newtoken.linenum;
    return ret_val;
}

ParseNode    *ParseDefinitionRest()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    PushBackToken(&newtoken);
    if (newtoken.tktype == toklparen) {
        /* function_rest always starts with a left paren */
#ifdef DEBUG
        printf("(11) defn_rest = function_rest .\n");
#endif
	ret_val->nodetype = prsProduction;
	ret_val->u.child[0] = ParseFunctionRest();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
        ret_val->prodnum = 11;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    /* otherwise, it is the second form */
#ifdef DEBUG
    printf("(12) defn_rest = declarators ; .\n");
#endif
    ret_val->u.child[0] = ParseDeclarators();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    GetToken(&newtoken);
    if (newtoken.tktype  != toksemi) {
        printf("? expected ; in line %d\n", newtoken.linenum);
        ret_val->nodetype = prsError;
	ret_val->linenum = newtoken.linenum;
	return ret_val;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 12;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode    *ParseFunctionRest()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
#ifdef DEBUG
    printf("(13) function_rest = ( id_list ) variable_defs function_body .\n");
#endif
    if (newtoken.tktype != toklparen) {
        printf("? expected ( in line %d\n", newtoken.linenum);
        ret_val->nodetype = prsError;
        ret_val->linenum = newtoken.linenum;
        return ret_val;
    }
    ret_val->u.child[0] = ParseIDList();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
        ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    GetToken(&newtoken);
    if (newtoken.tktype != tokrparen) {
        printf("? expected ) in line %d\n", newtoken.linenum);
        ret_val->nodetype = prsError;
        ret_val->linenum = newtoken.linenum;
        return ret_val;
    }
    ret_val->u.child[1] = ParseVariableDefs();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
        ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    ret_val->u.child[2] = ParseFunctionBody();
    if (ret_val->u.child[2]->nodetype == prsError) {
        ret_val->nodetype = prsError;
        ret_val->linenum = ret_val->u.child[2]->linenum;
	return ret_val;
    }
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->nodetype = prsProduction;
    ret_val->prodnum = 13;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode    *ParseIDList()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == tokid) {
        newtoken.vs.l.toktext[newtoken.vs.l.toklen] = '\0';
#ifdef DEBUG
        printf("(14) id_list = ID(%s) declarators .\n", newtoken.vs.l.toktext);
#endif
        ret_val->u.child[1] = ParseDeclarators();
        if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
        ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsID;
	ret_val->u.child[0]->linenum = newtoken.linenum;
	bcopy(&newtoken.vs.l.toktext[0], 
            &ret_val->u.child[0]->u.id.idname[0],
	    newtoken.vs.l.toklen);
	ret_val->u.child[0]->u.id.idlen = newtoken.vs.l.toklen;
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 14;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    /* if it's not an id, we are at a zero-length id list */
    PushBackToken(&newtoken);
#ifdef DEBUG
    printf("(15) id_list = .\n");
#endif
    ret_val->nodetype = prsProduction;
    ret_val->linenum = newtoken.linenum;
    ret_val->prodnum = 15;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode    *ParseVariableDefs()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    PushBackToken(&newtoken);
    /* we're either starting a var definition or seeing the end of the list */
    if (((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keyint)) ||
        ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keyfloat))) {
#ifdef DEBUG
	printf("(16) variable_defs = type declarator declarators ; variable_defs .\n");
#endif
        ret_val->u.child[0] = ParseType();
        if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
        ret_val->u.child[1] = ParseDeclarator();
        if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
        ret_val->u.child[2] = ParseDeclarators();
        if (ret_val->u.child[2]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[2]->linenum;
	    return ret_val;
        }
	GetToken(&newtoken);
	if (newtoken.tktype != toksemi) {
            printf("? ; expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[2]->linenum;
	    return ret_val;
        }
        ret_val->u.child[3] = ParseVariableDefs();
        if (ret_val->u.child[3]->nodetype == prsError) {
            ret_val->nodetype = prsError;
            ret_val->linenum = ret_val->u.child[3]->linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->prodnum = 16;
	ret_val->etype = etypeVoid;
        return ret_val;
    }
#ifdef DEBUG
    printf("(17) variable_defs = .\n");
#endif
    ret_val->nodetype = prsProduction;
    ret_val->linenum = newtoken.linenum;
    ret_val->prodnum = 17;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode    *ParseFunctionBody()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
#ifdef DEBUG
    printf("(18) function_body = { variable_defs statements } .\n");
#endif
    if (newtoken.tktype != toklbrace) {
        printf("? { expected in line %d\n", newtoken.linenum);
	ret_val->nodetype = prsError;
	ret_val->linenum = newtoken.linenum;
	return ret_val;
    }
    ret_val->u.child[0] = ParseVariableDefs();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
        ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseStatements();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
        ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    GetToken(&newtoken);
    if (newtoken.tktype != tokrbrace) {
        printf("? } expected in line %d\n", newtoken.linenum);
	ret_val->nodetype = prsError;
	ret_val->linenum = newtoken.linenum;
	return ret_val;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 18;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode    *ParseStatement()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == toksemi) {
        /* null statement */
#ifdef DEBUG
        printf("(19) statement = ; .\n");
#endif
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 19;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    if (newtoken.tktype == toklbrace) {
        /* compound statement */
#ifdef DEBUG
        printf("(21) statement = { statements } .\n");
#endif
	ret_val->u.child[0] = ParseStatements();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	GetToken(&newtoken);
	if (newtoken.tktype != tokrbrace) {
            printf("? } expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->etype = etypeVoid;
	ret_val->prodnum = 21;
	return ret_val;
    }
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keyif)) {
        /* conditional */
#ifdef DEBUG
        printf("(22) statement = IF ( expression ) statement optional_else .\n");
#endif
	GetToken(&newtoken);
	if (newtoken.tktype != toklparen) {
            printf("? ( expected in line %d\n", newtoken.linenum);
	    ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
	ret_val->u.child[0] = ParseExpression();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
        GetToken(&newtoken);
	if (newtoken.tktype != tokrparen) {
            printf("? ) expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
	ret_val->u.child[1] = ParseStatement();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
	ret_val->u.child[2] = ParseOptionalElse();
	if (ret_val->u.child[2]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[2]->linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->prodnum = 22;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keywhile)) {
        /* while loop */
#ifdef DEBUG
        printf("(23) statement = WHILE ( expression ) statement .\n");
#endif
	GetToken(&newtoken);
	if (newtoken.tktype != toklparen) {
            printf("? ( expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
	ret_val->u.child[0] = ParseExpression();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	GetToken(&newtoken);
	if (newtoken.tktype != tokrparen) {
            printf("? ) expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
	ret_val->u.child[1] = ParseStatement();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	ret_val->prodnum = 23;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    /* non-loop control statements */
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keybreak)) {
#ifdef DEBUG
        printf("(24) statement = BREAK ; .\n");
#endif
	ret_val->linenum = newtoken.linenum;
	GetToken(&newtoken);
	if (newtoken.tktype != toksemi) {
            printf("? ; expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->prodnum = 24;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keycontinue)) {
#ifdef DEBUG
        printf("(25) statement = CONTINUE ; .\n");
#endif
	ret_val->linenum = newtoken.linenum;
	GetToken(&newtoken);
	if (newtoken.tktype != toksemi) {
            printf("? ; expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->prodnum = 25;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    if ((newtoken.tktype == tokkeyword) && (newtoken.vs.k == keyreturn)) {
#ifdef DEBUG
        printf("(26) statement = RETURN optional_expr ; .\n");
#endif
	ret_val->linenum = newtoken.linenum;
	ret_val->u.child[0] = ParseOptionalExpr();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	GetToken(&newtoken);
	if (newtoken.tktype != toksemi) {
            printf("? ; expected in line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->prodnum = 26;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    /* we didn't see any of the others, must be the start of an expression */
    PushBackToken(&newtoken);
#ifdef DEBUG
    printf("(20) statement = expression ; .\n");
#endif
    ret_val->u.child[0] = ParseExpression();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    GetToken(&newtoken);
    /* pick up the semicolon */
    if (newtoken.tktype != toksemi) {
        printf("? ; expected in line %d\n", newtoken.linenum);
        ret_val->nodetype = prsError;
        ret_val->linenum = newtoken.linenum;
	return ret_val;
    }
    ret_val->nodetype = prsProduction;
    ret_val->prodnum = 20;
    ret_val->etype = ret_val->u.child[0]->etype;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    return ret_val;
}

ParseNode   *ParseStatements()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    PushBackToken(&newtoken);
    if (newtoken.tktype == tokrbrace) {
#ifdef DEBUG
        printf("(28) statements = .\n");
#endif
        ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 28;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
#ifdef DEBUG
    printf("(27) statements = statement statements .\n");
#endif
    ret_val->u.child[0] = ParseStatement();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseStatements();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    ret_val->nodetype = prsProduction;
    ret_val->prodnum = 27;
    ret_val->etype = etypeVoid;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    return ret_val;
}

ParseNode   *ParseOptionalElse()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    /* if we don't find the else, move back - no else clause present */
    if ((newtoken.tktype != tokkeyword) || (newtoken.vs.k != keyelse)) {
        PushBackToken(&newtoken);
#ifdef DEBUG
	printf("(30) optional_else = .\n");
#endif
        ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 30;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
#ifdef DEBUG
    printf("(29) optional_else = ELSE statement .\n");
#endif
    ret_val->u.child[0] = ParseStatement();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->nodetype = prsProduction;
    ret_val->prodnum = 29;
    ret_val->etype = etypeVoid;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    return ret_val;
}

ParseNode    *ParseOptionalExpr()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    /* optional expressions can only be followed by a semicolon */
    if (newtoken.tktype == toksemi) {
#ifdef DEBUG
        printf("(32) optional_expr = .\n");
#endif
	PushBackToken(&newtoken);
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 32;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    /* we actually have an expression - parse it */
#ifdef DEBUG
    printf("(31) optional_expr = expression .\n");
#endif
    PushBackToken(&newtoken);
    ret_val->u.child[0] = ParseExpression();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 31;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode   *ParseExpression()
{
    ParseNode    *ret_val;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
#ifdef DEBUG
    printf("(33) expression = relexpr expr2 .\n");
#endif
    ret_val->u.child[0] = ParseRelExpr();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseExpr2();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    if (ret_val->u.child[1]->prodnum == 35) {
        ret_val->etype = ret_val->u.child[0]->etype;
    }
    else if ((ret_val->u.child[0]->etype == etypeUndecl) || 
             (ret_val->u.child[1]->etype == etypeUndecl)) {
	 ret_val->etype = etypeUndecl;
    }
    else if (ret_val->u.child[0]->etype != ret_val->u.child[1]->etype) {
        printf("? incompatible types in arithmetic expression, line %d\n",
               ret_val->u.child[1]->linenum);
	ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    else {
        ret_val->etype = ret_val->u.child[0]->etype;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->etype = ret_val->u.child[0]->etype;
    ret_val->prodnum = 33;
    return ret_val;
}

ParseNode    *ParseExpr2()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == tokasgn) {
#ifdef DEBUG
        printf("(34) expr2 = = expression .\n");
#endif
        ret_val->u.child[0] = ParseExpression();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 34;
	ret_val->etype = ret_val->u.child[0]->etype;
	return ret_val;
    }
#ifdef DEBUG
    printf("(35) expr2 = .\n");
#endif
    PushBackToken(&newtoken);
    ret_val->linenum = newtoken.linenum;
    ret_val->nodetype = prsProduction;
    ret_val->prodnum = 35;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode   *ParseRelExpr()
{
    ParseNode    *ret_val;

#ifdef DEBUG
    printf("(36) relexpr = arithexpr relexpr2 .\n");
#endif
    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    ret_val->u.child[0] = ParseArithExpr();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseRelExpr2();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    if (ret_val->u.child[1]->prodnum == 38) {
	ret_val->etype = ret_val->u.child[0]->etype;
    }
    else if ((ret_val->u.child[0]->etype == etypeUndecl) || 
             (ret_val->u.child[1]->etype == etypeUndecl)) {
	 ret_val->etype = etypeUndecl;
    }
    else if (ret_val->u.child[0]->etype != ret_val->u.child[1]->etype) {
        printf("? incompatible types in relational expression, line %d\n",
               ret_val->u.child[1]->linenum);
	ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    else {
        ret_val->etype = ret_val->u.child[0]->etype;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 36;
    return ret_val;
}

ParseNode    *ParseRelExpr2()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == tokrelop) {
#ifdef DEBUG
        printf("(37) relexpr2 = RELOP(");
	switch (newtoken.vs.r) {
            case opeq:    printf("==");
	                  break;
            case opne:    printf("!=");
                          break;
            case opgt:    printf(">");
	                  break;
            case opge:    printf(">=");
                          break;
            case oplt:    printf("<");
                          break;
            case ople:    printf("<=");
        }
	printf(") arithexpr .\n");
#endif
        ret_val->u.child[1] = ParseArithExpr();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 37;
	ret_val->etype = ret_val->u.child[1]->etype;
	ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsRelop;
	ret_val->u.child[0]->linenum = newtoken.linenum;
	ret_val->u.child[0]->u.r = newtoken.vs.r;
	return ret_val;
    }
#ifdef DEBUG
    printf("(38) relexpr2 = .\n");
#endif
    PushBackToken(&newtoken);
    ret_val->nodetype = prsProduction;
    ret_val->prodnum = 38;
    ret_val->linenum = newtoken.linenum;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode   *ParseArithExpr()
{
    ParseNode    *ret_val;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
#ifdef DEBUG
    printf("(39) arithexpr = term arithexpr2 .\n");
#endif
    ret_val->u.child[0] = ParseTerm();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseArithExpr2();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    if (ret_val->u.child[1]->prodnum == 41) {
        ret_val->etype = ret_val->u.child[0]->etype;
    }
    else if ((ret_val->u.child[0]->etype == etypeUndecl) || 
             (ret_val->u.child[1]->etype == etypeUndecl)) {
	 ret_val->etype = etypeUndecl;
    }
    else if (ret_val->u.child[0]->etype != ret_val->u.child[1]->etype) {
        printf("? incompatible types in arithmetic expression, line %d\n",
               ret_val->u.child[1]->linenum);
	ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    else {
        ret_val->etype = ret_val->u.child[0]->etype;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 39;
    return ret_val;
}

ParseNode   *ParseArithExpr2()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == tokaddop) {
#ifdef DEBUG
        printf("(40) arithexpr2 = ADDOP(");
	if (newtoken.vs.a == opplus) {
            printf("+");
	}
	else {
            printf("-");
        }
	printf(") term arithexpr2 .\n");
#endif
	ret_val->u.child[1] = ParseTerm();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
	ret_val->u.child[2] = ParseArithExpr2();
	if (ret_val->u.child[2]->nodetype ==  prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[2]->linenum;
	    return ret_val;
        }
	if (ret_val->u.child[2]->prodnum == 41) {
            ret_val->etype = ret_val->u.child[1]->etype;
        }
        else if ((ret_val->u.child[2]->etype == etypeUndecl) || 
                 (ret_val->u.child[1]->etype == etypeUndecl)) {
	    ret_val->etype = etypeUndecl;
        }
        else if (ret_val->u.child[2]->etype != ret_val->u.child[1]->etype) {
            printf("? incompatible types in arithmetic expression, line %d\n",
                   ret_val->u.child[1]->linenum);
	    ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
	else {
	    ret_val->etype = ret_val->u.child[1]->etype;
        }
	ret_val->nodetype = prsProduction;
	ret_val->prodnum = 40;
	ret_val->linenum = newtoken.linenum;
	ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsAddop;
	ret_val->u.child[0]->u.a = newtoken.vs.a;
	return ret_val;
    }
    PushBackToken(&newtoken);
#ifdef DEBUG
    printf("(41) arithexpr2 = .\n");
#endif
    ret_val->nodetype = prsProduction;
    ret_val->linenum = newtoken.linenum;
    ret_val->prodnum = 41;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode   *ParseTerm()
{
    ParseNode    *ret_val;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
#ifdef DEBUG
    printf("(42) term = factor term2 .\n");
#endif
    ret_val->u.child[0] = ParseFactor();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseTerm2();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    if (ret_val->u.child[1]->prodnum == 44) {
        ret_val->etype = ret_val->u.child[0]->etype;
    }
    else if ((ret_val->u.child[0]->etype == etypeUndecl) || 
             (ret_val->u.child[1]->etype == etypeUndecl)) {
	 ret_val->etype = etypeUndecl;
    }
    else if (ret_val->u.child[0]->etype != ret_val->u.child[1]->etype) {
        printf("? incompatible types in arithmetic expression, line %d\n",
               ret_val->u.child[1]->linenum);
	ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    else {
        ret_val->etype = ret_val->u.child[0]->etype;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 42;
    return ret_val;
}

ParseNode   *ParseTerm2()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == tokmulop) {
#ifdef DEBUG
        printf("(43) term2 = MULOP(");
	if (newtoken.vs.m == optimes) {
            printf("*");
        }
	if (newtoken.vs.m == opdiv) {
            printf("/");
        }
	if (newtoken.vs.m == opmod) {
            printf("%");
        }
	printf(") factor term2 .\n");
#endif
	ret_val->u.child[1] = ParseFactor();
        if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
            return ret_val;
        }
	ret_val->u.child[2] = ParseTerm2();
        if (ret_val->u.child[2]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[2]->linenum;
            return ret_val;
        }
        ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsMulop;
	ret_val->u.child[0]->linenum = newtoken.linenum;
	ret_val->u.child[0]->u.m = newtoken.vs.m;
        ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 43;
	ret_val->etype = ret_val->u.child[1]->etype;
        return ret_val;
    }
#ifdef DEBUG
    printf("(44) term2 = .\n");
#endif
    PushBackToken(&newtoken);
    ret_val->nodetype = prsProduction;
    ret_val->linenum = newtoken.linenum;
    ret_val->prodnum = 44;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode   *ParseFactor()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == tokid) {
        newtoken.vs.l.toktext[newtoken.vs.l.toklen] = '\0';
#ifdef DEBUG
	printf("(45) factor = ID(%s) optional_params .\n", newtoken.vs.l.toktext);
#endif
	ret_val->u.child[1] = ParseOptionalParams();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
	ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsID;
	ret_val->u.child[0]->u.id.idlen = newtoken.vs.l.toklen;
	bcopy(&newtoken.vs.l.toktext[0], 
              &ret_val->u.child[0]->u.id.idname[0],
	      newtoken.vs.l.toklen);
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 45;
	ret_val->etype = etypeUndecl;
	return ret_val;
    }
    if (newtoken.tktype == tokintconst) {
#ifdef DEBUG
        printf("(46) factor = INTCONST(%d) .\n", newtoken.vs.i);
#endif
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 46;
	ret_val->etype = etypeInt;
	ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsIntConst;
	ret_val->u.child[0]->u.i = newtoken.vs.i;
	return ret_val;
    }
    if (newtoken.tktype == tokfloatconst) {
#ifdef DEBUG
        printf("(47) factor = FLOATCONST(%f) .\n", newtoken.vs.f);
#endif
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 47;
	ret_val->etype = etypeFloat;
	ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsFloatConst;
	ret_val->u.child[0]->u.f = newtoken.vs.f;
	return ret_val;
    }
    if (newtoken.tktype == tokstringconst) {
        newtoken.vs.l.toktext[newtoken.vs.l.toklen] = '\0';
#ifdef DEBUG
        printf("(48) factor = STRINGCONST(%s) .\n", newtoken.vs.l.toktext);
#endif
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 48;
	ret_val->etype = etypeString;
	ret_val->u.child[0] = (ParseNode *)malloc(sizeof(ParseNode));
	ret_val->u.child[0]->nodetype = prsStringConst;
        bcopy(&newtoken.vs.l.toktext[0], 
              &ret_val->u.child[0]->u.s.txt[0],
	      newtoken.vs.l.toklen);
        ret_val->u.child[0]->u.s.slen = newtoken.vs.l.toklen;
	return ret_val;
    }
    if (newtoken.tktype == toklparen) {
#ifdef DEBUG
        printf("(49) factor = ( expression ) .\n");
#endif
	ret_val->linenum = newtoken.linenum;
        ret_val->u.child[0] = ParseExpression();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	GetToken(&newtoken);
	if (newtoken.tktype != tokrparen) {
            printf("? ) expected, line %d\n", newtoken.linenum);
            ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
	ret_val->nodetype = prsProduction;
	ret_val->prodnum = 49;
	ret_val->etype = ret_val->u.child[0]->etype;
	return ret_val;
    }
    /* if we get this far, the input matched none of the possibilities */
    printf("? factor expected, line %d\n", newtoken.linenum);
    ret_val->nodetype = prsError;
    ret_val->linenum = newtoken.linenum;
    return ret_val;
}

ParseNode   *ParseOptionalParams()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == toklparen) {
#ifdef DEBUG
        printf("(50) optional_params = ( argument_list ) .\n");
#endif
	ret_val->linenum = newtoken.linenum;
        ret_val->u.child[0] = ParseArgumentList();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
            return ret_val;
        }
	GetToken(&newtoken);
	if (newtoken.tktype != tokrparen) {
            printf("? ) expected in line %d\n", newtoken.linenum);
	    ret_val->nodetype = prsError;
	    ret_val->linenum = newtoken.linenum;
	    return ret_val;
        }
	ret_val->nodetype = prsProduction;
        ret_val->prodnum = 50;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    PushBackToken(&newtoken);
#ifdef DEBUG
    printf("(51) optional_params = .\n");
#endif
    ret_val->nodetype = prsProduction;
    ret_val->linenum = newtoken.linenum;
    ret_val->prodnum = 51;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode   *ParseArgumentList()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    PushBackToken(&newtoken);
    if (newtoken.tktype == tokrparen) {
#ifdef DEBUG
        printf("(53) argument_list = .\n");
#endif
	ret_val->nodetype = prsProduction;
	ret_val->linenum = newtoken.linenum;
	ret_val->prodnum = 53;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
#ifdef DEBUG
    printf("(52) argument_list = expression argument_rest .\n");
#endif
    ret_val->u.child[0] = ParseExpression();
    if (ret_val->u.child[0]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[0]->linenum;
	return ret_val;
    }
    ret_val->u.child[1] = ParseArgumentRest();
    if (ret_val->u.child[1]->nodetype == prsError) {
        ret_val->nodetype = prsError;
	ret_val->linenum = ret_val->u.child[1]->linenum;
	return ret_val;
    }
    ret_val->nodetype = prsProduction;
    ret_val->linenum = ret_val->u.child[0]->linenum;
    ret_val->prodnum = 52;
    ret_val->etype = etypeVoid;
    return ret_val;
}

ParseNode   *ParseArgumentRest()
{
    ParseNode    *ret_val;
    token        newtoken;

    ret_val = (ParseNode *)malloc(sizeof(ParseNode));
    GetToken(&newtoken);
    if (newtoken.tktype == tokcomma) {
#ifdef DEBUG
        printf("(54) argument_rest = , expression argument_rest .\n");
#endif
	ret_val->u.child[0] = ParseExpression();
	if (ret_val->u.child[0]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[0]->linenum;
	    return ret_val;
        }
	ret_val->u.child[1] = ParseArgumentRest();
	if (ret_val->u.child[1]->nodetype == prsError) {
            ret_val->nodetype = prsError;
	    ret_val->linenum = ret_val->u.child[1]->linenum;
	    return ret_val;
        }
        ret_val->nodetype = prsProduction;
	ret_val->prodnum = 54;
	ret_val->linenum = newtoken.linenum;
	ret_val->etype = etypeVoid;
	return ret_val;
    }
    PushBackToken(&newtoken);
#ifdef DEBUG
    printf("(55) argument_rest = .\n");
#endif
    ret_val->nodetype = prsProduction;
    ret_val->linenum = newtoken.linenum;
    ret_val->prodnum = 55;
    ret_val->etype = etypeVoid;
    return ret_val;
}
