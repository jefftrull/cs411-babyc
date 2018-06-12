/*
 * lexer.h
 *
 * Interface to BabyC lexer for 15-411 Asst. 1
 *
 * Marc Ringuette, 25 Jan 88
 */

#define MAXLEN 80		/* Max string & lexeme length */

typedef enum toktyp {
	tokid, tokkeyword, tokintconst, tokfloatconst, tokstringconst,
	tokrelop, tokmulop, tokaddop, tokasgn, toksemi, tokcomma, 
	toklbrace, tokrbrace, toklparen, tokrparen, tokerror, tokeof
    } toktyp;

typedef enum keytyp {
	keyint, keyfloat, keyif, keyelse, keywhile, keybreak, keycontinue,
	keyreturn, keyvoid
    } keytyp;

typedef enum reltyp { opeq, opne, opgt, opge, oplt, ople } reltyp;

typedef enum multyp { optimes, opdiv, opmod } multyp;

typedef enum addtyp { opplus, opminus } addtyp;

typedef struct token {			/* Object returned by lexer */
	toktyp tktype;			/* Token type */
	int linenum;			/* Line number */
	union {				/* Value or subtype (as appropriate) */
	    int	i;			/* Integer value */
	    float f;			/* Floating value */
	    keytyp k;			/* Keyword subtype */
	    reltyp r;			/* Relop subtype */
	    multyp m;			/* Mulop subtype */
	    addtyp a;			/* Addop subtype */
	    struct {			/* Literal value */
		char toktext[MAXLEN];
		int toklen;
	    } l;
	} vs;
    } token;

extern void InitLexer();
/*
 * Initialize lexer module.
 * Will be called exactly once before any calls to GetNextToken.
 */

extern void GetNextToken(/* token *tok */);
/*
 * On each call, places the next lexeme into the variable tok.
 */
