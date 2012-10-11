
/***********************************************************************
 *
 * FILE : MemoCalcLexer.h
 * 
 * DESCRIPTION : Lexical analyser headers for MemoCalc
 * 
 * COPYRIGHT : GNU GENERAL PUBLIC LICENSE
 * http://www.gnu.org/licenses/gpl.txt
 *
 ***********************************************************************/

#ifndef MEMOCALCLEXER_H
#define MEMOCALCLEXER_H

// tokens

#define tInteger		1
#define tFloat			2
#define tName			3

// structures

typedef struct TokenCell {
	struct TokenCell * nextP;	// next token in the list
	union {
		struct IndexPair {
			UInt16 iStart;		// start index of token associated value in TokenList expression string
			UInt16 iEnd;		// end index of token associated value in TokenList expression string
			UInt32 padding;
		} indexPair ;
		double value;			// constant or variable value
	} data;
	UInt8 token;				// token defined above or char matched (no associated value)
} TokenCell;

typedef struct TokenList {
	TokenCell * headP;			// head of list
	TokenCell * cellP;			// current cell
	Char * exprStr;				// expression string
} TokenList;

typedef struct VarCell {
	struct VarCell * nextP;
	Char * name;
	double value;
} VarCell;

typedef struct VarList {
	VarCell * headP;			// head of list
	VarCell * cellP;			// current cell
	Char * varsStr;				// variables declaration string
} VarList;


// functions

UInt8 TokenizeExpression (TokenList * tokL);
UInt8 ParseVariables (VarList * varL);
UInt8 AssignTokenValue (TokenList * tokL, VarList * varL);

// transitions

#define isNumber(c)		(c >= '0' && c <= '9')
#define isLetter(c)		((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
#define isDot(c)		(c == '.')
#define isOpen(c)		(c == '(')
#define isClose(c)		(c == ')')
#define isOperator(c)	(c == '+' || c == '-' || c == '*' || c == '/' || c == '^')

#define isSeparator(c)	(c == ' ' || c == '\t' || c == '\n' || c == 0x0d || c == 0x0a)


#endif // MEMOCALCLEXER_H
