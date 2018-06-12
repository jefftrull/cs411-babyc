/* main program for testing parsing routines for BabyC */
#include	"lexer.h"
#include	"symtab.h"
#include	"ptree.h"
main()
{
    extern void    	InitParser();
    extern void    	ParseProgram();
    extern void    	InitSemantics();
    extern void    	TreeSemantics();
    extern void		GenPseudoCode();
    extern ParseNode	*ParseTree;
    extern short	found_errors;

    InitParser();
    ParseProgram();
    if (ParseTree->nodetype == prsError) {
	return;
    }
    InitSemantics();
    TreeSemantics();
    if (found_errors) {
	return;
    }
    GenPseudoCode();
}
