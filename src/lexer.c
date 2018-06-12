/*****************************************************************\
*                                                                 *
*              lexer.c - Implementation of Baby C lexer           *
*                                                                 *
\*****************************************************************/

/*
* Jeffrey Trull
* February 7, 1988
*/

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include "lexer.h"

short        cur_state;        /* current state in state machine */
int          line_number;      /* current line in file */
int          saved_char;       /* storage for one-character lookahead */
short        saving_char;      /* whether a character has been pushed back */
extern int   atoi();           /* c library conversion routines */
extern float atof();

void         InitLexer()
/*
* set up initial FSM state
*/
{
    cur_state = 0;        /* starting state */
    line_number = 1;      /* starting line */
    saving_char = 0;      /* no saved characters */
}

void         GetNextToken(tokptr)
    token        *tokptr;
/*
* return next token from input.  reads input, building 
* up a token, then returns.
*/
{
    int      inchar;                /* input character */
    void     pushback();            /* single-character stack insert */
    short    is_keyword();          /* checks string for keyword */  
    short    iswhite();             /* whitespace checker */
    int      nextchar();            /* returns next input character */
    keytyp   classify_keyword();    /* converts a string into a key type */

    while (1) {                     /* move through states */
      switch (cur_state) {

        case 0:                     /* top-level FSM */
            inchar = nextchar();
            switch(inchar) {
                case EOF:
                    cur_state = -1;                 /* move to EOF state */
		    tokptr->tktype = tokeof;
                    tokptr->linenum = line_number;
		    return;
                case '=':             /* move to "found =" state */
		    cur_state = 1;
		    break;
                case '!':
		    /* is it the start of !=, or an error? */
                    inchar = nextchar();
		    if (inchar != '=') {
                        tokptr->tktype = tokerror;
			tokptr->linenum = line_number;
			tokptr->vs.l.toktext[0] = '!';
			tokptr->vs.l.toklen = 1;
			pushback(inchar);
                        cur_state = 0;
			return;
                    }
		    else {                  /* "not equal" */
		        tokptr->tktype = tokrelop;
			tokptr->linenum = line_number;
			tokptr->vs.r = opne;
                        cur_state = 0;
			return;
                    }
                case '<':
                    cur_state = 2;        /* to "found <" state */
		    break;
		case '>':
		    cur_state = 3;        /* to "found >" state */
		    break;
                case '*':                 /* always a multiplication */
		    tokptr->tktype = tokmulop;
                    tokptr->linenum = line_number;
		    tokptr->vs.m = optimes;
		    cur_state = 0;
		    return;
                case '/':
                    /* check for start of comment */
                    inchar = nextchar();
		    if (inchar == '*') {
                        cur_state = 4;    /* go to comment state */
			break;
                    }
                    pushback(inchar);     /* just division, move back */
                    tokptr->tktype = tokmulop;
		    tokptr->vs.m = opdiv;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case '%':                 /* always mod division */
		    tokptr->tktype = tokmulop;
		    tokptr->vs.m = opmod;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case '+':                 /* always addition */
                    tokptr->tktype = tokaddop;
		    tokptr->vs.a = opplus;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
		case '-':                /* always subtraction */
                    tokptr->tktype = tokaddop;
		    tokptr->vs.a = opminus;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case ';':                
		    tokptr->tktype = toksemi;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case ',':
		    tokptr->tktype = tokcomma;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case '{':
		    tokptr->tktype = toklbrace;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case '}':
		    tokptr->tktype = tokrbrace;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case '(':
                    tokptr->tktype = toklparen;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case ')':
		    tokptr->tktype = tokrparen;
		    tokptr->linenum = line_number;
		    cur_state = 0;
		    return;
                case '"':
                    cur_state = 5;        /* enter string const state */
		    break;
		default:
                    if (isalpha(inchar) || (inchar == '_')) {
                        /* start of ident or keyword, set up for ident */
		        tokptr->tktype = tokid;
			tokptr->vs.l.toktext[0] = inchar;
			tokptr->vs.l.toklen = 1;
                        tokptr->linenum = line_number;
                        cur_state = 6;   /* ident or keyword state */
			break;
                    }
                    else if (isdigit(inchar)) {
		        /* start of int or float constant */
                        tokptr->tktype = tokintconst;
			tokptr->vs.l.toktext[0] = inchar;
			tokptr->vs.l.toklen = 1;
			tokptr->linenum = line_number;
			cur_state = 7;  /* int or float state */
			break;
                    }
		    else if (iswhite(inchar)) {
		        /* scan off whitespace, count lines */
                        if (inchar == '\n')
                            line_number++;
                        while (iswhite(inchar = nextchar()))
                            if (inchar == '\n') 
			        line_number++;
			pushback(inchar);
                        break;
                    }
		    else {
		        /* must be an error */
		        tokptr->tktype = tokerror;
			tokptr->vs.l.toktext[0] = inchar;
			tokptr->vs.l.toklen = 1;
                        tokptr->linenum = line_number;
			return;
                    }
            }
	    break;

        case 1:  /* got '=' */
            inchar = nextchar();
	    /* look for equality operator */
	    if (inchar == '=') {
                tokptr->tktype = tokrelop;
		tokptr->linenum = line_number;
		tokptr->vs.r = opeq;
                cur_state = 0;
		return;
            }
	    else {
	        /* must be assignment, move back */
                pushback(inchar);
                tokptr->tktype = tokasgn;
	        tokptr->linenum = line_number;
		cur_state = 0;
		return;
            }

        case 3:  /* got '>' */
	    /* check for ">=" */
	    inchar = nextchar();
	    if (inchar == '=') {
                tokptr->tktype = tokrelop;
		tokptr->linenum = line_number;
		tokptr->vs.r = opge;
		cur_state = 0;
		return;
            }
	    else {
	        /* just a ">", move back */
                pushback(inchar);
		tokptr->tktype = tokrelop;
		tokptr->linenum = line_number;
		tokptr->vs.r = opgt;
		cur_state = 0;
		return;
            }

        case 2:  /* got '<' */
	    inchar = nextchar();
	    if (inchar == '=') {
	        tokptr->tktype = tokrelop;
		tokptr->linenum = line_number;
		tokptr->vs.r = ople;
		cur_state = 0;
		return;
            }
	    else {
                pushback(inchar);
	        tokptr->tktype = tokrelop;
		tokptr->linenum = line_number;
		tokptr->vs.r = oplt;
		cur_state = 0;
		return;
            }

        case 4:  /* in comment */
            while ((inchar = nextchar()) != '*') {
                if (inchar == EOF) {
		    /* unterminated comment, just signal eof */
                    tokptr->tktype = tokeof;
		    return;
                }
		/* count lines */
		if (inchar == '\n')
                    line_number++;
            }
            inchar = nextchar();
            /* found "*" - is it start of end of comment? */
            if (inchar == '/')   /* yes, move back to start */
                cur_state = 0;
            if (inchar == '\n')  /* count line */
                line_number++;                
            break;

        case 5:  /* in string constant */
            tokptr->tktype = tokstringconst;
	    tokptr->vs.l.toklen = 0;
            while ((inchar = nextchar()) != '"') {
	        /* unterminated string, signal eof */
                if (inchar == EOF) {
                    tokptr->tktype = tokeof;
		    return;
                }
		/* count lines */
                if (inchar == '\n')
                    line_number++;
		/* string too long, signal error */
                if (tokptr->vs.l.toklen == 80) {
                    tokptr->tktype = tokerror;
                    pushback(inchar);
		    cur_state = 0;
		    return;
		}
		/* store character in token text array */
                tokptr->vs.l.toktext[tokptr->vs.l.toklen++] = inchar;
            }
            /* found string end, set up line number and return to start */
            tokptr->linenum = line_number;
            cur_state = 0;
            return;

        case 6:  /* getting identifier or keyword */
            /* accumulate valid identifier characters */
            while (isalpha((inchar = nextchar())) || 
                                 isdigit(inchar)  ||
                                 (inchar == '_'))
	        tokptr->vs.l.toktext[tokptr->vs.l.toklen++] = inchar;
            /* make identifier text into a string for comparison */
            tokptr->vs.l.toktext[tokptr->vs.l.toklen] = '\0';
	    if (is_keyword(tokptr->vs.l.toktext)) {
	        /* set up keyword token */
                tokptr->tktype = tokkeyword;
		tokptr->vs.k = classify_keyword(tokptr->vs.l.toktext);
            }
	    /* put back the character that ended the token */
            pushback(inchar);
            cur_state = 0;
	    return;

        case 7:  /* getting number */
            /* accumulate the string part */
            while (isdigit((inchar = nextchar())))
	        tokptr->vs.l.toktext[tokptr->vs.l.toklen++] = inchar;
            if (inchar == '.') { /* number so far is part of a float */
                tokptr->vs.l.toktext[tokptr->vs.l.toklen++] = '.';
		cur_state = 8;  /* floating point number state */
            }
	    else {  /* found end of integer constant */
                pushback(inchar);
		cur_state = 0;
		tokptr->vs.l.toktext[tokptr->vs.l.toklen] = '\0';
		tokptr->vs.i = atoi(tokptr->vs.l.toktext);
		return;
            }
            break;

        case 8:  /* getting floating point number */
            while (isdigit((inchar = nextchar())))
                tokptr->vs.l.toktext[tokptr->vs.l.toklen++] = inchar;
            pushback(inchar);
	    tokptr->tktype = tokfloatconst;
            tokptr->vs.l.toktext[tokptr->vs.l.toklen] = '\0';
	    tokptr->vs.f = atof(tokptr->vs.l.toktext);
            cur_state = 0;
	    return;

	case -1:  /* end-of-file state */
            tokptr->tktype = tokeof;
	    return;
      }
    }
}

short        iswhite(c)
    int    c;
{
    return ((c == ' ') || (c == '\t') || (c == '\n'));
}

int    nextchar()
{
    if (saving_char) {
        saving_char = 0;
	return saved_char;
    }
    return getchar();
}

void    pushback(c)
    int    c;
{
    saving_char = 1;
    saved_char = c;
}

keytyp    classify_keyword(str)
    char    *str;
{
    if ((strcmp(str, "int")) == 0)
        return keyint;
    if ((strcmp(str, "float")) == 0)
        return keyfloat;
    if ((strcmp(str, "if")) == 0)
        return keyif;
    if ((strcmp(str, "else")) == 0)
        return keyelse;
    if ((strcmp(str, "while")) == 0)
        return keywhile;
    if ((strcmp(str, "break")) == 0)
        return keybreak;
    if ((strcmp(str, "continue")) == 0)
        return keycontinue;
    if ((strcmp(str, "return")) == 0)
        return keyreturn;
    if ((strcmp(str, "void")) == 0)
        return keyvoid;
}

short    is_keyword(char * str)
{
    /* is keyword iff at least one comparison is zero (equal strings) */
    return (!(strcmp(str, "void") &&
              strcmp(str, "return") &&
	      strcmp(str, "int") &&
	      strcmp(str, "float") &&
              strcmp(str, "if") &&
	      strcmp(str, "else") &&
	      strcmp(str, "while") &&
	      strcmp(str, "break") &&
	      strcmp(str, "continue")));
}
