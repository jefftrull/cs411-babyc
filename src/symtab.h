/*****************************************************************\
*
*        symtab.h - definitions for symbol table routines
*
\*****************************************************************/


/*
* HISTORY
* 5-Mar-88  Jeffrey Trull (jt1j) at Carnegie-Mellon University
*      Created
*/

/*
* Include Files
*/

#include    "sym.h"

#ifndef SYMTAB_HDR
#define SYMTAB_HDR

/*
* Constants
*/

#define MAXSYMTAB    1000

/*
* Type Definitions
*/

typedef struct symtab {
    SymEntry    *contents[MAXSYMTAB];
    short       is_full[MAXSYMTAB];
} symtab;

/*
* Macros
*/



#endif  SYMTAB_HDR
