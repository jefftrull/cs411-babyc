/* sym.h: Types for BabyC Compiler symbol table.
 *
 * 15-411:  Compiler Design  --  Spring 1988
 * Marc Ringuette, 29-Feb-88
 */

#define MAXSYM 80	/* Max symbol length */

typedef enum { symVar, symFunc, symParam } SymType;
typedef enum { dtypeInt, dtypeFloat, dtypeVoid, dtypeUndecl } DataType;

typedef struct SymName {	/* Symbol name type */
	char	idname[MAXSYM];	/* Identifier name */
	int	idlen;		/* Its length */
    } SymName;

typedef struct SymEntry {	/* Symbol table entry */
	SymName	id;		/* Symbol identifier */
	SymType	stype;		/* Symbol type */
	DataType dtype;		/* Identifier data type */
	int	linenum;	/* Where symbol was defined */
	int	scope;		/* Lexical scope (0 if global) */
	int	offset;		/* Location of object (for code generation) */
      } SymEntry;
