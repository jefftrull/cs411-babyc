/***********************************************************************\
*                                                                       *
*              symtab.c - symbol table maintenance routines             *
*                                                                       *
\***********************************************************************/


/*
* HISTORY
* 5-Mar-88  Jeffrey Trull (jt1j) at Carnegie-Mellon University
*      Created
*/

/*
* Include Files
*/

#include    "symtab.h"

/*
* Procedure Predeclarations
*/

void    SymInit();
int     SymPut();
int     SymGet();
int     SymUpdate();
short   HashFn();
short   namecomp();

/*
* Data Declarations
*/

symtab        symbol_table;

/*
* Routines
*/

/* Initialize Symbol Table */

void        SymInit()
/*
* make the hash table entirely empty
*/
{
    short        index;

    for (index = 0; index < MAXSYMTAB; index++) {
        symbol_table.is_full[index] = 0;
    }
}

/* Add a Symbol to the Symbol Table */

int         SymPut(entry)
    SymEntry     *entry;
/*
* locate the first available space after the slot indicated by
* the hash function.  return 1 if there is not an identical symbol
* present in the table already, 0 otherwise (and no replacement
* is done)
*/
{
    short        cur_index;

    cur_index = HashFn(entry);
    while (symbol_table.is_full[cur_index]) {
        if ((namecomp(&symbol_table.contents[cur_index]->id, &entry->id)) &&
            (symbol_table.contents[cur_index]->scope == entry->scope)) {
                return 0;
        }
        cur_index = ((cur_index + 1) % MAXSYMTAB);
    }
    symbol_table.is_full[cur_index] = 1;
    symbol_table.contents[cur_index] = entry;

    return 1;
}

/* Retrieve a Symbol from the Symbol Table */

int         SymGet(entry)
    SymEntry     *entry;
/*
* search through the table, starting at the hash location, for 
* an entry matching the given id and scope fields
*/
{
    short        cur_index;

    cur_index = HashFn(entry);
    while (symbol_table.is_full[cur_index]) {
        if (namecomp(&symbol_table.contents[cur_index]->id, &entry->id) &&
            (symbol_table.contents[cur_index]->scope == entry->scope)) {
            bcopy(symbol_table.contents[cur_index], entry, sizeof(SymEntry));
            return 1;
        }
        cur_index = ((cur_index + 1) % MAXSYMTAB);
    }
    return 0;
}

/* Replace a Symbol in the Symbol Table */

int         SymUpdate(entry)
    SymEntry        *entry;
/* 
* search for the entry, starting at the slot returned by the hash
* function.  if not present, return 0 otherwise return 1 and 
* replace the entry.
*/
{
    short        cur_index;

    cur_index = HashFn(entry);
    while (symbol_table.is_full[cur_index]) {
        if (namecomp(&symbol_table.contents[cur_index]->id, &entry->id) &&
            (symbol_table.contents[cur_index]->scope == entry->scope)) {
            bcopy(entry, symbol_table.contents[cur_index], sizeof(SymEntry));
            return 1;
        }
        cur_index = ((cur_index + 1) % MAXSYMTAB);
    }
    return 0;
}

/* determine the hash value of a candidate entry */

short        HashFn(entry)
    SymEntry        *entry;
/*
* add up the ASCII values of the identifier string and multiply
* by a small prime
*/
{
    short    sum;
    short  strindex;

    sum = 0;
    for (strindex = 0; strindex < entry->id.idlen; strindex ++) {
        sum += entry->id.idname[strindex];
    }
    sum *= 53;
    sum *= entry->scope;
    if (sum < 0) {
        sum = -sum;
    }
    return (sum % MAXSYMTAB);
}

/* compare two identifiers for equality */

short         namecomp(name1, name2)
    SymName        *name1, *name2;
{
    short        cur_index;

    if (name1->idlen != name2->idlen) {
        return 0;
    }
    for (cur_index = 0; cur_index < name1->idlen; cur_index++) {
        if (name1->idname[cur_index] != name2->idname[cur_index]) {
            return 0;
        }
    }
    return 1;
}
